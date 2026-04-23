#include "BluetoothInternal.hpp"
#include "functions.hpp"

#include <chrono>
#include <iostream>
#include <limits>
#include <pty.h>
#include <sstream>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

bool obter_estado_bluetooth() {
    return obter_status_bluetooth().powered;
}

bluetooth_adapter_status obter_status_bluetooth() {
    bluetooth_adapter_status status;

    bluetooth_result show_result = bluetooth_internal::executar_bluetoothctl("show");
    status.show_output = show_result.mensagem;
    if (!show_result.ok ||
        status.show_output.find("No default controller available") != std::string::npos) {
        status.controller_disponivel = false;
    }

    if (status.show_output.find("Powered: yes") != std::string::npos) {
        status.powered = true;
    }

    command_result rfkill = exec_command_result("rfkill list bluetooth");
    if (!rfkill.stdout_output.empty()) {
        status.rfkill_output = rfkill.stdout_output;
        status.soft_blocked = status.rfkill_output.find("Soft blocked: yes") != std::string::npos;
        status.hard_blocked = status.rfkill_output.find("Hard blocked: yes") != std::string::npos;
    }

    return status;
}

bluetooth_state_snapshot obter_estado_bluetooth_completo() {
    bluetooth_state_snapshot snapshot;
    snapshot.adapter = obter_status_bluetooth();
    snapshot.conhecidos = listar_dispositivos_bluetooth_conhecidos();
    for (const auto& dispositivo : snapshot.conhecidos) {
        if (dispositivo.pareado) {
            snapshot.pareados.push_back(dispositivo);
        }
    }
    return snapshot;
}

bluetooth_result definir_estado_bt_result(bool ligar) {
    if (ligar) {
        bluetooth_result desbloqueio = bluetooth_internal::garantir_bluetooth_desbloqueado();
        if (!desbloqueio.ok) {
            return desbloqueio;
        }
    }

    bluetooth_result validacao = bluetooth_internal::validar_adaptador_pronto();
    if (!validacao.ok && ligar) {
        return validacao;
    }

    const std::string acao = ligar ? "on" : "off";
    bluetooth_result resultado = bluetooth_internal::executar_bluetoothctl("power " + acao);
    bluetooth_result verificado = bluetooth_internal::verificar_saida_operacao(
        resultado,
        "power_failed",
        ligar ? "Falha ao ativar o Bluetooth." : "Falha ao desativar o Bluetooth."
    );
    if (!verificado.ok) {
        return verificado;
    }

    bluetooth_adapter_status status_final;
    bool refletiu_estado = false;
    for (int tentativa = 0; tentativa < bluetooth_internal::kBluetoothStatePollAttempts; ++tentativa) {
        status_final = obter_status_bluetooth();
        if (status_final.powered == ligar) {
            refletiu_estado = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(bluetooth_internal::kBluetoothStatePollIntervalMs));
    }

    if (!refletiu_estado) {
        return bluetooth_internal::make_bt_error(
            "state_mismatch",
            "Bluetooth nao refletiu o estado solicitado.",
            bluetooth_internal::resumir_bluetooth_status(status_final) + " | " + resultado.mensagem
        );
    }

    return bluetooth_internal::make_bt_success(
        ligar ? "Bluetooth ativado com sucesso." : "Bluetooth desativado com sucesso.",
        resultado.mensagem
    );
}

bool definir_estado_bt(bool ligar) {
    return definir_estado_bt_result(ligar).ok;
}

std::vector<device_bt> listar_dispositivos_bluetooth_conhecidos() {
    std::unordered_map<std::string, device_bt> conhecidos =
        bluetooth_internal::listar_dispositivos_por_comando("devices");
    if (conhecidos.empty()) {
        return {};
    }

    std::unordered_map<std::string, device_bt> pareados =
        bluetooth_internal::listar_dispositivos_por_comando("paired-devices");
    bluetooth_internal::marcar_campo_bool_em_lote(conhecidos, pareados, &device_bt::pareado);

    for (auto& [mac, dispositivo] : conhecidos) {
        if (!dispositivo.pareado) {
            continue;
        }
        device_bt enriquecido = bluetooth_internal::consultar_dispositivo_bluetooth(mac);
        if (enriquecido.conectado || enriquecido.confiavel || !enriquecido.icon.empty()) {
            bluetooth_internal::mesclar_dispositivo_bluetooth(dispositivo, enriquecido);
        }
    }

    return bluetooth_internal::ordenar_dispositivos(conhecidos);
}

std::vector<device_bt> listar_dispositivos_bluetooth_pareados() {
    std::unordered_map<std::string, device_bt> pareados =
        bluetooth_internal::listar_dispositivos_por_comando("paired-devices");
    for (auto& [mac, dispositivo] : pareados) {
        dispositivo.pareado = true;
        device_bt enriquecido = bluetooth_internal::consultar_dispositivo_bluetooth(mac);
        bluetooth_internal::mesclar_dispositivo_bluetooth(dispositivo, enriquecido);
    }
    return bluetooth_internal::ordenar_dispositivos(pareados);
}

std::vector<device_bt> scan_dispositivos_bluetooth(int segundos) {
    int master_fd;
    pid_t pid = forkpty(&master_fd, nullptr, nullptr, nullptr);
    if (pid < 0) {
        return {};
    }

    if (pid == 0) {
        setbuf(stdout, nullptr);
        execlp("bluetoothctl", "bluetoothctl", nullptr);
        _exit(1);
    }

    command_result rfkill = exec_command_result("rfkill unblock bluetooth");
    (void)rfkill;

    write(master_fd, "power on\n", 9);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    write(master_fd, "agent on\ndefault-agent\n", 24);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    write(master_fd, "scan on\n", 8);

    auto inicio = std::chrono::steady_clock::now();
    char buffer[1024];

    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(master_fd, &fds);
        struct timeval tv{1, 0};

        int ret = select(master_fd + 1, &fds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(master_fd, &fds)) {
            int n = read(master_fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                continue;
            }
            if (n == 0 || errno == EIO) {
                break;
            }
        }

        auto agora = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(agora - inicio).count() >= segundos) {
            break;
        }
    }

    write(master_fd, "scan off\n", 9);
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    write(master_fd, "devices\nquit\n", 13);

    std::string saida_final;
    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(master_fd, &fds);
        struct timeval tv{1, 0};

        int ret = select(master_fd + 1, &fds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(master_fd, &fds)) {
            int n = read(master_fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                saida_final += buffer;
                continue;
            }
            if (n == 0 || errno == EIO) {
                break;
            }
        } else if (ret == 0) {
            int status = 0;
            pid_t waited = waitpid(pid, &status, WNOHANG);
            if (waited == pid) {
                break;
            }
        } else {
            break;
        }
    }

    close(master_fd);
    waitpid(pid, nullptr, 0);

    std::unordered_map<std::string, device_bt> mapa;
    std::stringstream ss(saida_final);
    std::string ctx_dummy;
    parsing_bluetooth_stream(ss, mapa, ctx_dummy);

    std::vector<device_bt> lista;
    for (auto& [_, d] : mapa) {
        if (d.nome.empty()) d.nome = d.mac;
        lista.push_back(d);
    }
    return lista;
}

bluetooth_result conectar_bluetooth_result(const std::string& mac) {
    bluetooth_result validacao = bluetooth_internal::validar_adaptador_pronto();
    if (!validacao.ok) {
        return validacao;
    }

    bluetooth_result resultado = bluetooth_internal::executar_bluetoothctl("connect " + mac);
    bluetooth_result verificado = bluetooth_internal::verificar_saida_operacao(
        resultado,
        "connect_failed",
        "Falha ao conectar o dispositivo."
    );
    if (!verificado.ok) {
        return verificado;
    }

    device_bt dispositivo;
    if (!bluetooth_internal::aguardar_estado_dispositivo(
            mac,
            [](const device_bt& atual) { return atual.conectado; },
            &dispositivo
        )) {
        return bluetooth_internal::make_bt_error(
            "connect_not_reflected",
            "Comando enviado, mas o dispositivo nao apareceu como conectado.",
            resultado.mensagem
        );
    }

    return bluetooth_internal::make_bt_success("Dispositivo conectado com sucesso.", resultado.mensagem);
}

bool conectar_bluetooth(const std::string& mac) {
    return conectar_bluetooth_result(mac).ok;
}

bluetooth_result desconectar_bluetooth_result(const std::string& mac) {
    bluetooth_result validacao = bluetooth_internal::validar_adaptador_pronto();
    if (!validacao.ok) {
        return validacao;
    }

    bluetooth_result resultado = bluetooth_internal::executar_bluetoothctl("disconnect " + mac);
    bluetooth_result verificado = bluetooth_internal::verificar_saida_operacao(
        resultado,
        "disconnect_failed",
        "Falha ao desconectar o dispositivo."
    );
    if (!verificado.ok) {
        return verificado;
    }

    device_bt dispositivo;
    if (!bluetooth_internal::aguardar_estado_dispositivo(
            mac,
            [](const device_bt& atual) { return !atual.conectado; },
            &dispositivo
        )) {
        return bluetooth_internal::make_bt_error(
            "disconnect_not_reflected",
            "Comando enviado, mas o dispositivo ainda aparece como conectado.",
            resultado.mensagem
        );
    }

    return bluetooth_internal::make_bt_success("Dispositivo desconectado com sucesso.", resultado.mensagem);
}

bool desconectar_bluetooth(const std::string& mac) {
    return desconectar_bluetooth_result(mac).ok;
}

bluetooth_result parear_bluetooth(const std::string& mac) {
    bluetooth_result desbloqueio = bluetooth_internal::garantir_bluetooth_desbloqueado();
    if (!desbloqueio.ok) {
        return desbloqueio;
    }

    bluetooth_result validacao = bluetooth_internal::validar_adaptador_pronto();
    if (!validacao.ok) {
        return validacao;
    }

    bluetooth_result resultado = bluetooth_internal::executar_bluetoothctl("pair " + mac);
    bluetooth_result verificado = bluetooth_internal::verificar_saida_operacao(
        resultado,
        "pair_failed",
        "Falha ao parear dispositivo."
    );
    if (!verificado.ok) {
        return verificado;
    }

    device_bt dispositivo;
    if (!bluetooth_internal::aguardar_estado_dispositivo(
            mac,
            [](const device_bt& atual) { return atual.pareado; },
            &dispositivo
        )) {
        return bluetooth_internal::make_bt_error(
            "pair_not_reflected",
            "Comando executado, mas o dispositivo nao apareceu como pareado.",
            resultado.mensagem
        );
    }

    return bluetooth_internal::make_bt_success("Dispositivo pareado com sucesso.", resultado.mensagem);
}

bluetooth_result confiar_bluetooth(const std::string& mac) {
    bluetooth_result resultado = bluetooth_internal::executar_bluetoothctl("trust " + mac);
    bluetooth_result verificado = bluetooth_internal::verificar_saida_operacao(
        resultado,
        "trust_failed",
        "Falha ao confiar no dispositivo."
    );
    if (!verificado.ok) {
        return verificado;
    }

    device_bt dispositivo;
    if (!bluetooth_internal::aguardar_estado_dispositivo(
            mac,
            [](const device_bt& atual) { return atual.confiavel; },
            &dispositivo
        )) {
        return bluetooth_internal::make_bt_error(
            "trust_not_reflected",
            "Comando executado, mas o dispositivo nao apareceu como confiavel.",
            resultado.mensagem
        );
    }

    return bluetooth_internal::make_bt_success("Dispositivo marcado como confiavel.", resultado.mensagem);
}

bluetooth_result remover_bluetooth(const std::string& mac) {
    bluetooth_result resultado = bluetooth_internal::executar_bluetoothctl("remove " + mac);
    bluetooth_result verificado = bluetooth_internal::verificar_saida_operacao(
        resultado,
        "remove_failed",
        "Falha ao remover dispositivo."
    );
    if (!verificado.ok) {
        return verificado;
    }

    bool removido = false;
    for (int tentativa = 0; tentativa < bluetooth_internal::kBluetoothStatePollAttempts; ++tentativa) {
        if (!bluetooth_internal::dispositivo_esta_conhecido(mac)) {
            removido = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(bluetooth_internal::kBluetoothStatePollIntervalMs));
    }

    if (!removido) {
        return bluetooth_internal::make_bt_error(
            "remove_not_reflected",
            "Comando executado, mas o dispositivo ainda aparece como conhecido.",
            resultado.mensagem
        );
    }

    return bluetooth_internal::make_bt_success("Dispositivo removido com sucesso.", resultado.mensagem);
}

void listar_dispositivos_bluetooth(const std::vector<device_bt>& dispositivos) {
    if (dispositivos.empty()) {
        std::cout << "Nenhum dispositivo encontrado.\n";
        return;
    }
    int i = 1;
    for (const auto& d : dispositivos) {
        std::cout << i++ << ") " << d.nome << " - " << d.mac << '\n';
    }
}

void gerenciar_bluetooth() {
    std::cout << "Carregando lista de dispositivos...\n";
    std::vector<device_bt> dispositivos = listar_dispositivos_bluetooth_pareados();

    if (dispositivos.empty()) {
        std::cout << "Nenhum dispositivo pareado encontrado.\n";
        return;
    }

    std::cout << "\n=== Dispositivos Pareados ===\n";
    for (size_t i = 0; i < dispositivos.size(); ++i) {
        std::cout << "[" << i + 1 << "] " << dispositivos[i].nome << " \t(" << dispositivos[i].mac << ")\n";
    }
    std::cout << "=============================\n";
    std::cout << "Escolha um dispositivo (0 para sair): ";

    int escolha;
    std::cin >> escolha;

    if (std::cin.fail() || escolha <= 0 || escolha > static_cast<int>(dispositivos.size())) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    const auto& device = dispositivos[escolha - 1];
    std::cout << "\nDispositivo selecionado: " << device.nome << "\n";
    std::cout << "[1] Conectar\n[2] Desconectar\nOpcao: ";

    int acao;
    std::cin >> acao;
    if (acao == 1) conectar_bluetooth(device.mac);
    else if (acao == 2) desconectar_bluetooth(device.mac);
}
