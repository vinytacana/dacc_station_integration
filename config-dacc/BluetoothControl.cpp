#include "BluetoothInternal.hpp"
#include "functions.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <iostream>
#include <limits>
#include <mutex>
#include <pty.h>
#include <sstream>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

namespace {

std::mutex g_bluetooth_ui_cache_mutex;
bluetooth_ui_snapshot g_bluetooth_ui_cache;
bool g_bluetooth_ui_cache_valido = false;

std::string normalizar_mac_bt(const std::string& mac) {
    std::string out = mac;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return out;
}

bool comparar_nome_bt(const device_bt& a, const device_bt& b) {
    std::string nome_a = a.nome.empty() ? a.mac : a.nome;
    std::string nome_b = b.nome.empty() ? b.mac : b.nome;
    return nome_a < nome_b;
}

void ordenar_snapshot_ui_bt(bluetooth_ui_snapshot& snapshot) {
    std::stable_sort(
        snapshot.desconhecidos_conectados.begin(),
        snapshot.desconhecidos_conectados.end(),
        comparar_nome_bt
    );
    std::stable_sort(
        snapshot.pareados.begin(),
        snapshot.pareados.end(),
        [](const device_bt& a, const device_bt& b) {
            if (a.conectado != b.conectado) return a.conectado > b.conectado;
            return comparar_nome_bt(a, b);
        }
    );
    std::stable_sort(
        snapshot.escaneados_filtrados.begin(),
        snapshot.escaneados_filtrados.end(),
        comparar_nome_bt
    );
}

void filtrar_escaneados_ui_bt(bluetooth_ui_snapshot& snapshot, const std::vector<device_bt>& escaneados) {
    std::unordered_set<std::string> bloqueados;
    for (const auto& d : snapshot.desconhecidos_conectados) {
        bloqueados.insert(normalizar_mac_bt(d.mac));
    }
    for (const auto& d : snapshot.pareados) {
        bloqueados.insert(normalizar_mac_bt(d.mac));
    }

    std::unordered_set<std::string> inseridos;
    snapshot.escaneados_filtrados.clear();
    for (auto d : escaneados) {
        if (d.mac.empty()) continue;
        if (d.nome.empty()) d.nome = d.mac;
        const std::string mac = normalizar_mac_bt(d.mac);
        if (bloqueados.find(mac) != bloqueados.end()) continue;
        if (inseridos.find(mac) != inseridos.end()) continue;
        inseridos.insert(mac);
        d.pareado = false;
        d.conectado = false;
        snapshot.escaneados_filtrados.push_back(d);
    }
}

void salvar_cache_ui_bt(const bluetooth_ui_snapshot& snapshot) {
    std::lock_guard<std::mutex> lock(g_bluetooth_ui_cache_mutex);
    g_bluetooth_ui_cache = snapshot;
    g_bluetooth_ui_cache.vindo_do_cache = false;
    g_bluetooth_ui_cache_valido = true;
}

bluetooth_result conectar_bluetooth_direto_result(const std::string& mac);
bluetooth_result conectar_bluetooth_result_com_retry(const std::string& mac, bool permitir_retry);
bluetooth_result renovar_pareamento_obsoleto_bluetooth(const std::string& mac, const bluetooth_result& falha_original);

std::string minusculo_bt(std::string texto) {
    std::transform(texto.begin(), texto.end(), texto.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return texto;
}

bool contem_texto_bt(const std::string& texto, const std::string& trecho) {
    return minusculo_bt(texto).find(minusculo_bt(trecho)) != std::string::npos;
}

bool erro_indica_pareamento_obsoleto(const bluetooth_result& resultado) {
    const std::string saida = resultado.detalhes.empty() ? resultado.mensagem : resultado.detalhes;
    if (resultado.codigo == "authentication_failed") return true;
    if (contem_texto_bt(saida, "AuthenticationFailed")) return true;
    if (contem_texto_bt(saida, "Authentication Failed")) return true;
    if (contem_texto_bt(saida, "Authentication Rejected")) return true;
    if (contem_texto_bt(saida, "Authentication Canceled")) return true;
    if (contem_texto_bt(saida, "AuthenticationTimeout")) return true;
    if (contem_texto_bt(saida, "br-connection-profile-unavailable")) return true;
    if (contem_texto_bt(saida, "protocol")) return true;
    if (contem_texto_bt(saida, "key missing")) return true;
    if (contem_texto_bt(saida, "pin or key missing")) return true;

    return (resultado.codigo == "connect_not_reflected" ||
            resultado.codigo == "bluetoothctl_timeout" ||
            resultado.codigo == "operation_timeout") &&
           contem_texto_bt(saida, "Attempting to connect");
}

bluetooth_result preparar_agent_pareamento_bluetooth() {
    bluetooth_result resultado = bluetooth_internal::executar_bluetoothctl_ate(
        {
            "agent off",
            "agent KeyboardDisplay",
            "default-agent",
            "pairable on"
        },
        {
            "Changing pairable on succeeded"
        },
        {
            "Failed",
            "No default controller available",
            "org.bluez.Error"
        },
        {},
        8000
    );
    if (!resultado.ok) {
        return bluetooth_internal::make_bt_error(
            resultado.codigo,
            "Falha ao preparar o pareamento Bluetooth.",
            resultado.detalhes
        );
    }
    return resultado;
}

} // namespace

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

bluetooth_ui_snapshot obter_estado_bluetooth_ui() {
    bluetooth_ui_snapshot snapshot;
    snapshot.adapter = obter_status_bluetooth();

    if (!snapshot.adapter.controller_disponivel) {
        salvar_cache_ui_bt(snapshot);
        return snapshot;
    }

    if (!snapshot.adapter.powered) {
        salvar_cache_ui_bt(snapshot);
        return snapshot;
    }

    std::vector<device_bt> conhecidos = listar_dispositivos_bluetooth_conhecidos();
    if (conhecidos.empty()) {
        salvar_cache_ui_bt(snapshot);
        return snapshot;
    }

    for (auto dispositivo : conhecidos) {
        if (dispositivo.nome.empty()) dispositivo.nome = dispositivo.mac;
        if (dispositivo.conectado && !dispositivo.pareado) {
            snapshot.desconhecidos_conectados.push_back(dispositivo);
        } else if (dispositivo.pareado) {
            snapshot.pareados.push_back(dispositivo);
        }
    }

    {
        std::lock_guard<std::mutex> lock(g_bluetooth_ui_cache_mutex);
        filtrar_escaneados_ui_bt(snapshot, g_bluetooth_ui_cache.escaneados_filtrados);
    }

    ordenar_snapshot_ui_bt(snapshot);
    salvar_cache_ui_bt(snapshot);
    return snapshot;
}

bluetooth_ui_snapshot obter_estado_bluetooth_ui_com_scan(int segundos) {
    bluetooth_ui_snapshot snapshot = obter_estado_bluetooth_ui();
    if (!snapshot.adapter.controller_disponivel || !snapshot.adapter.powered) {
        return snapshot;
    }

    std::vector<device_bt> escaneados = scan_dispositivos_bluetooth(segundos);
    filtrar_escaneados_ui_bt(snapshot, escaneados);
    ordenar_snapshot_ui_bt(snapshot);
    salvar_cache_ui_bt(snapshot);
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

    std::unordered_map<std::string, device_bt> pareados =
        bluetooth_internal::listar_dispositivos_por_comando("paired-devices");
    bluetooth_internal::marcar_campo_bool_em_lote(conhecidos, pareados, &device_bt::pareado);

    for (auto& [mac, dispositivo] : conhecidos) {
        device_bt enriquecido = bluetooth_internal::consultar_dispositivo_bluetooth(mac);
        bluetooth_internal::mesclar_dispositivo_bluetooth(dispositivo, enriquecido);
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

namespace {

bluetooth_result conectar_bluetooth_direto_result(const std::string& mac) {
    bluetooth_result validacao = bluetooth_internal::validar_adaptador_pronto();
    if (!validacao.ok) {
        return validacao;
    }

    bluetooth_result resultado = bluetooth_internal::executar_bluetoothctl_ate(
        {
            "connect " + mac
        },
        {
            "Connection successful",
            "Successful connected",
            "Connected: yes"
        },
        {
            "Failed to connect",
            "Authentication",
            "Rejected",
            "Canceled",
            "not available",
            "org.bluez.Error",
            "br-connection-profile-unavailable",
            "Protocol",
            "protocol",
            "key missing"
        },
        {},
        20000
    );
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

} // namespace

namespace {

bluetooth_result conectar_bluetooth_result_com_retry(const std::string& mac, bool permitir_retry) {
    bluetooth_result resultado = conectar_bluetooth_direto_result(mac);
    if (!resultado.ok && permitir_retry && erro_indica_pareamento_obsoleto(resultado)) {
        return renovar_pareamento_obsoleto_bluetooth(mac, resultado);
    }
    return resultado;
}

} // namespace

bluetooth_result conectar_bluetooth_result(const std::string& mac) {
    return conectar_bluetooth_result_com_retry(mac, true);
}

bool conectar_bluetooth(const std::string& mac) {
    return conectar_bluetooth_result(mac).ok;
}

bluetooth_result desconectar_bluetooth_result(const std::string& mac) {
    bluetooth_result validacao = bluetooth_internal::validar_adaptador_pronto();
    if (!validacao.ok) {
        return validacao;
    }

    bluetooth_result resultado = bluetooth_internal::executar_bluetoothctl_ate(
        {
            "disconnect " + mac
        },
        {
            "Disconnection successful",
            "Successful disconnected",
            "Connected: no",
            "not connected"
        },
        {
            "Failed to disconnect",
            "not available",
            "org.bluez.Error"
        },
        {},
        15000
    );
    bluetooth_result verificado = bluetooth_internal::verificar_saida_operacao(
        resultado,
        "disconnect_failed",
        "Falha ao desconectar o dispositivo."
    );
    if (!verificado.ok) {
        return verificado;
    }

    const bool bluetoothctl_confirmou_desconexao =
        resultado.mensagem.find("Disconnection successful") != std::string::npos ||
        resultado.mensagem.find("Connected: no") != std::string::npos ||
        resultado.mensagem.find("not connected") != std::string::npos;

    device_bt dispositivo;
    if (!bluetooth_internal::aguardar_estado_dispositivo(
            mac,
            [](const device_bt& atual) { return !atual.conectado; },
            &dispositivo
        )) {
        if (bluetoothctl_confirmou_desconexao) {
            return bluetooth_internal::make_bt_success("Dispositivo desconectado com sucesso.", resultado.mensagem);
        }

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

    bluetooth_result agent = preparar_agent_pareamento_bluetooth();
    if (!agent.ok) {
        return agent;
    }

    bluetooth_result resultado = bluetooth_internal::executar_bluetoothctl_ate(
        {
            "scan on",
            "pair " + mac
        },
        {
            "Pairing successful",
            "Paired: yes"
        },
        {
            "Failed to pair",
            "not available",
            "Authentication",
            "org.bluez.Error",
            "Timed out",
            "timed out"
        },
        {
            "scan off"
        },
        90000
    );
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

namespace {

bluetooth_result renovar_pareamento_obsoleto_bluetooth(const std::string& mac, const bluetooth_result& falha_original) {
    bluetooth_result remocao = ::remover_bluetooth(mac);
    if (!remocao.ok) {
        return bluetooth_internal::make_bt_error(
            "stale_pairing_remove_failed",
            "Nao foi possivel renovar o pareamento local.",
            falha_original.detalhes + "\n" + remocao.detalhes
        );
    }

    bluetooth_result pareamento = parear_bluetooth(mac);
    if (!pareamento.ok) {
        return bluetooth_internal::make_bt_error(
            pareamento.codigo,
            pareamento.mensagem,
            remocao.detalhes + "\n" + pareamento.detalhes
        );
    }

    bluetooth_result confianca = confiar_bluetooth(mac);
    if (!confianca.ok) {
        return bluetooth_internal::make_bt_error(
            confianca.codigo,
            confianca.mensagem,
            remocao.detalhes + "\n" + pareamento.detalhes + "\n" + confianca.detalhes
        );
    }

    bluetooth_result conexao = conectar_bluetooth_result_com_retry(mac, false);
    if (!conexao.ok) {
        return bluetooth_internal::make_bt_error(
            conexao.codigo,
            conexao.mensagem,
            remocao.detalhes + "\n" + pareamento.detalhes + "\n" + confianca.detalhes + "\n" + conexao.detalhes
        );
    }

    return bluetooth_internal::make_bt_success(
        "Pareamento renovado e dispositivo conectado.",
        remocao.detalhes + "\n" + pareamento.detalhes + "\n" + confianca.detalhes + "\n" + conexao.detalhes
    );
}

} // namespace

bluetooth_result parear_confiar_conectar_bluetooth(const std::string& mac) {
    bluetooth_result pareamento = parear_bluetooth(mac);
    if (!pareamento.ok) {
        return pareamento;
    }

    bluetooth_result confianca = confiar_bluetooth(mac);
    if (!confianca.ok) {
        return confianca;
    }

    bluetooth_result conexao = conectar_bluetooth_result(mac);
    if (!conexao.ok) {
        return conexao;
    }

    return bluetooth_internal::make_bt_success(
        "Dispositivo pareado, confiavel e conectado.",
        pareamento.detalhes + "\n" + confianca.detalhes + "\n" + conexao.detalhes
    );
}

bluetooth_result remover_bluetooth(const std::string& mac) {
    device_bt estado_inicial = bluetooth_internal::consultar_dispositivo_bluetooth(mac);
    if (estado_inicial.conectado) {
        bluetooth_result desconexao = desconectar_bluetooth_result(mac);
        if (!desconexao.ok && desconexao.codigo != "device_not_connected" &&
            desconexao.codigo != "device_unavailable") {
            return desconexao;
        }
    }

    bluetooth_result resultado = bluetooth_internal::executar_bluetoothctl("remove " + mac);
    bluetooth_result verificado = bluetooth_internal::verificar_saida_operacao(
        resultado,
        "remove_failed",
        "Falha ao remover dispositivo."
    );
    if (!verificado.ok) {
        device_bt estado = bluetooth_internal::consultar_dispositivo_bluetooth(mac);
        if (!estado.pareado && !estado.confiavel) {
            return bluetooth_internal::make_bt_success("Dispositivo removido com sucesso.", resultado.mensagem);
        }
        return verificado;
    }

    bool removido = false;
    for (int tentativa = 0; tentativa < bluetooth_internal::kBluetoothStatePollAttempts * 2; ++tentativa) {
        device_bt estado = bluetooth_internal::consultar_dispositivo_bluetooth(mac);
        if (!estado.pareado && !estado.confiavel) {
            removido = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(bluetooth_internal::kBluetoothStatePollIntervalMs));
    }

    if (!removido) {
        return bluetooth_internal::make_bt_error(
            "remove_not_reflected",
            "Comando executado, mas o dispositivo ainda aparece como pareado ou confiavel.",
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
