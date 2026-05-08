#include "BluetoothInternal.hpp"
#include "config-dacc/ErrorCodes.hpp"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <pty.h>
#include <signal.h>
#include <sstream>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace bluetooth_internal {

namespace err = config_dacc::errors;

namespace {

std::mutex g_bluetooth_mutex;

struct BluetoothctlSession {
    int master_fd = -1;
    pid_t pid = -1;
    bool wait_blocking = false;

    ~BluetoothctlSession() {
        if (master_fd >= 0) {
            close(master_fd);
        }
        if (pid > 0) {
            int status = 0;
            waitpid(pid, &status, wait_blocking ? 0 : WNOHANG);
        }
    }

    void close_master_fd() {
        if (master_fd >= 0) {
            close(master_fd);
            master_fd = -1;
        }
    }

    void enable_blocking_wait() {
        wait_blocking = true;
    }

    void kill_if_running(int signal_number) {
        if (pid > 0) {
            kill(pid, signal_number);
        }
    }
};

bool contem_algum_marcador(const std::string& texto, const std::vector<std::string>& marcadores) {
    for (const auto& marcador : marcadores) {
        if (texto.find(marcador) != std::string::npos) {
            return true;
        }
    }
    return false;
}

system_result mapear_erro_bluetoothctl(
    const std::string& codigo_falha,
    const std::string& mensagem_falha,
    const system_result& comando_result
) {
    const std::string& saida = comando_result.mensagem.empty() ? comando_result.detalhes : comando_result.mensagem;

    if (comando_result.codigo == err::BLUETOOTHCTL_SIGNALED) {
        return make_bt_error(err::BLUETOOTHCTL_SIGNALED, "bluetoothctl foi interrompido.", comando_result.detalhes);
    }
    if (comando_result.codigo == err::BLUETOOTHCTL_TIMEOUT) {
        return make_bt_error(err::BLUETOOTHCTL_TIMEOUT, "bluetoothctl excedeu o tempo limite.", comando_result.detalhes);
    }
    if (saida.find("No default controller available") != std::string::npos) {
        return make_bt_error(err::ADAPTER_UNAVAILABLE, "Nenhum adaptador Bluetooth disponivel.", saida);
    }
    if (saida.find("rfkill") != std::string::npos || saida.find("blocked") != std::string::npos) {
        return make_bt_error(err::ADAPTER_BLOCKED, "Bluetooth bloqueado.", saida);
    }
    if (saida.find("not available") != std::string::npos) {
        return make_bt_error(err::DEVICE_UNAVAILABLE, "Dispositivo Bluetooth indisponivel.", saida);
    }
    if (saida.find("not connected") != std::string::npos) {
        return make_bt_error(err::DEVICE_NOT_CONNECTED, "Dispositivo nao esta conectado.", saida);
    }
    if (saida.find("AlreadyExists") != std::string::npos || saida.find("Already Connected") != std::string::npos) {
        return make_bt_error(err::ALREADY_DONE, "Operacao ja aplicada ao dispositivo.", saida);
    }
    if (saida.find("Authentication") != std::string::npos ||
        saida.find("Failed to pair") != std::string::npos ||
        saida.find("org.bluez.Error.Authentication") != std::string::npos) {
        return make_bt_error(err::AUTHENTICATION_FAILED, "Falha de autenticacao no dispositivo Bluetooth.", saida);
    }
    if (saida.find("timed out") != std::string::npos || saida.find("Timeout") != std::string::npos) {
        return make_bt_error(err::OPERATION_TIMEOUT, "Operacao Bluetooth expirou.", saida);
    }
    return make_bt_error(codigo_falha, mensagem_falha, saida);
}

system_result executar_bluetoothctl_locked(const std::string& comando, int timeout_ms) {
    system_result resultado;
    BluetoothctlSession session;
    session.pid = forkpty(&session.master_fd, nullptr, nullptr, nullptr);
    if (session.pid < 0) {
        return make_bt_error(err::FORKPTY_FAILED, "Falha ao iniciar bluetoothctl.", std::strerror(errno));
    }

    if (session.pid == 0) {
        execlp("bluetoothctl", "bluetoothctl", nullptr);
        _exit(1);
    }

    const std::string entrada = comando + "\nquit\n";
    ssize_t escritos = write(session.master_fd, entrada.c_str(), entrada.size());
    if (escritos < 0) {
        return make_bt_error(err::WRITE_FAILED, "Falha ao enviar comando ao bluetoothctl.", std::strerror(errno));
    }

    char buffer[1024];
    std::string saida;
    const auto inicio = std::chrono::steady_clock::now();

    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(session.master_fd, &fds);
        struct timeval tv{1, 0};

        int ret = select(session.master_fd + 1, &fds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(session.master_fd, &fds)) {
            ssize_t lidos = read(session.master_fd, buffer, sizeof(buffer) - 1);
            if (lidos > 0) {
                buffer[lidos] = '\0';
                saida += buffer;
                continue;
            }
            if (lidos == 0 || errno == EIO) {
                break;
            }
        } else if (ret == 0) {
            int status = 0;
            pid_t waited = waitpid(session.pid, &status, WNOHANG);
            if (waited == session.pid) {
                session.pid = -1;
                break;
            }
        } else if (ret < 0) {
            return make_bt_error(err::READ_FAILED, "Falha ao ler resposta do bluetoothctl.", std::strerror(errno));
        }

        const auto decorrido = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - inicio
        ).count();
        if (decorrido > timeout_ms) {
            session.kill_if_running(SIGKILL);
            session.enable_blocking_wait();
            return make_bt_error(
                err::BLUETOOTHCTL_TIMEOUT,
                "bluetoothctl excedeu o tempo limite.",
                "Comando: " + comando
            );
        }
    }

    session.close_master_fd();
    int status = 0;
    if (session.pid > 0) {
        session.enable_blocking_wait();
        waitpid(session.pid, &status, 0);
        session.pid = -1;
    }

    resultado.ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    resultado.mensagem = remover_ansi(saida);
    if (resultado.ok) {
        resultado.codigo = err::OK;
    } else if (WIFSIGNALED(status)) {
        resultado.codigo = err::BLUETOOTHCTL_SIGNALED;
    } else {
        resultado.codigo = err::BLUETOOTHCTL_FAILED;
    }
    resultado.detalhes = resultado.mensagem;
    if (!resultado.ok && resultado.mensagem.empty()) {
        if (WIFSIGNALED(status)) {
            resultado.mensagem = "bluetoothctl terminou por sinal " + std::to_string(WTERMSIG(status)) + ".";
        } else {
            resultado.mensagem = "bluetoothctl falhou sem resposta.";
        }
    }
    return resultado;
}

} // namespace

std::string remover_ansi(std::string str) {
    std::string resultado;
    bool dentro = false;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\033') {
            dentro = true;
            continue;
        }
        if (dentro) {
            if (isalpha(static_cast<unsigned char>(str[i]))) {
                dentro = false;
            }
            continue;
        }
        resultado += str[i];
    }
    return resultado;
}

system_result make_bt_error(const std::string& codigo, const std::string& mensagem, const std::string& detalhes) {
    system_result result;
    result.ok = false;
    result.codigo = codigo;
    result.mensagem = mensagem;
    result.detalhes = detalhes;
    return result;
}

system_result make_bt_success(const std::string& mensagem, const std::string& detalhes) {
    system_result result;
    result.ok = true;
    result.codigo = err::OK;
    result.mensagem = mensagem;
    result.detalhes = detalhes;
    return result;
}

system_result executar_bluetoothctl(const std::string& comando, int timeout_ms) {
    if (!comando_existe("bluetoothctl")) {
        return make_bt_error(
            err::BLUETOOTHCTL_MISSING,
            "Bluetooth indisponivel.",
            "bluetoothctl ausente no PATH."
        );
    }
    std::lock_guard<std::mutex> lock(g_bluetooth_mutex);
    return executar_bluetoothctl_locked(comando, timeout_ms);
}

system_result executar_bluetoothctl_ate(
    const std::vector<std::string>& comandos,
    const std::vector<std::string>& marcadores_sucesso,
    const std::vector<std::string>& marcadores_falha,
    const std::vector<std::string>& comandos_finalizacao,
    int timeout_ms
) {
    if (!comando_existe("bluetoothctl")) {
        return make_bt_error(
            err::BLUETOOTHCTL_MISSING,
            "Bluetooth indisponivel.",
            "bluetoothctl ausente no PATH."
        );
    }

    std::lock_guard<std::mutex> lock(g_bluetooth_mutex);

    BluetoothctlSession session;
    session.pid = forkpty(&session.master_fd, nullptr, nullptr, nullptr);
    if (session.pid < 0) {
        return make_bt_error(err::FORKPTY_FAILED, "Falha ao iniciar bluetoothctl.", std::strerror(errno));
    }

    if (session.pid == 0) {
        execlp("bluetoothctl", "bluetoothctl", nullptr);
        _exit(1);
    }

    auto escrever_linha = [&session](const std::string& linha) -> bool {
        const std::string entrada = linha + "\n";
        return write(session.master_fd, entrada.c_str(), entrada.size()) >= 0;
    };

    for (const auto& comando : comandos) {
        if (!escrever_linha(comando)) {
            return make_bt_error(err::WRITE_FAILED, "Falha ao enviar comando ao bluetoothctl.", std::strerror(errno));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(180));
    }

    char buffer[1024];
    std::string saida;
    bool finalizado_por_marcador = false;
    bool sucesso = false;
    size_t ultima_confirmacao_respondida = std::string::npos;
    const auto inicio = std::chrono::steady_clock::now();

    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(session.master_fd, &fds);
        struct timeval tv{1, 0};

        int ret = select(session.master_fd + 1, &fds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(session.master_fd, &fds)) {
            ssize_t lidos = read(session.master_fd, buffer, sizeof(buffer) - 1);
            if (lidos > 0) {
                buffer[lidos] = '\0';
                saida += buffer;
                std::string saida_limpa = remover_ansi(saida);

                size_t pos_confirmacao = std::string::npos;
                const std::vector<std::string> prompts_confirmacao = {
                    "Confirm passkey",
                    "Authorize service",
                    "Request confirmation",
                    "(yes/no):"
                };
                for (const auto& prompt : prompts_confirmacao) {
                    size_t pos = saida_limpa.find(prompt);
                    if (pos != std::string::npos) {
                        pos_confirmacao = pos;
                        break;
                    }
                }
                if (pos_confirmacao != std::string::npos &&
                    pos_confirmacao != ultima_confirmacao_respondida) {
                    (void)escrever_linha("yes");
                    ultima_confirmacao_respondida = pos_confirmacao;
                }

                for (const auto& marcador : marcadores_sucesso) {
                    if (saida_limpa.find(marcador) != std::string::npos) {
                        finalizado_por_marcador = true;
                        sucesso = true;
                        break;
                    }
                }
                if (!finalizado_por_marcador) {
                    for (const auto& marcador : marcadores_falha) {
                        if (saida_limpa.find(marcador) != std::string::npos) {
                            finalizado_por_marcador = true;
                            sucesso = false;
                            break;
                        }
                    }
                }
                if (finalizado_por_marcador) {
                    break;
                }
                continue;
            }
            if (lidos == 0 || errno == EIO) {
                break;
            }
        } else if (ret == 0) {
            int status = 0;
            pid_t waited = waitpid(session.pid, &status, WNOHANG);
            if (waited == session.pid) {
                session.pid = -1;
                break;
            }
        } else if (ret < 0) {
            return make_bt_error(err::READ_FAILED, "Falha ao ler resposta do bluetoothctl.", std::strerror(errno));
        }

        const auto decorrido = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - inicio
        ).count();
        if (decorrido > timeout_ms) {
            std::string saida_limpa = remover_ansi(saida);
            if (contem_algum_marcador(saida_limpa, marcadores_sucesso)) {
                finalizado_por_marcador = true;
                sucesso = true;
                break;
            }

            for (const auto& comando : comandos_finalizacao) {
                (void)escrever_linha(comando);
            }
            (void)escrever_linha("quit");
            session.kill_if_running(SIGKILL);
            session.enable_blocking_wait();
            return make_bt_error(
                err::BLUETOOTHCTL_TIMEOUT,
                "bluetoothctl excedeu o tempo limite.",
                remover_ansi(saida)
            );
        }
    }

    for (const auto& comando : comandos_finalizacao) {
        (void)escrever_linha(comando);
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
    }
    (void)escrever_linha("quit");
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    if (finalizado_por_marcador && sucesso) {
        while (true) {
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(session.master_fd, &fds);
            struct timeval tv{0, 100000};
            int ret = select(session.master_fd + 1, &fds, nullptr, nullptr, &tv);
            if (ret > 0 && FD_ISSET(session.master_fd, &fds)) {
                ssize_t lidos = read(session.master_fd, buffer, sizeof(buffer) - 1);
                if (lidos > 0) {
                    buffer[lidos] = '\0';
                    saida += buffer;
                    continue;
                }
            }
            break;
        }

        session.kill_if_running(SIGTERM);
        session.enable_blocking_wait();
        session.close_master_fd();
        if (session.pid > 0) {
            waitpid(session.pid, nullptr, 0);
            session.pid = -1;
        }

        return make_bt_success("Comando executado.", remover_ansi(saida));
    }

    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(session.master_fd, &fds);
        struct timeval tv{0, 200000};
        int ret = select(session.master_fd + 1, &fds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(session.master_fd, &fds)) {
            ssize_t lidos = read(session.master_fd, buffer, sizeof(buffer) - 1);
            if (lidos > 0) {
                buffer[lidos] = '\0';
                saida += buffer;
                continue;
            }
        }
        break;
    }

    session.close_master_fd();
    int status = 0;
    if (session.pid > 0) {
        session.enable_blocking_wait();
        waitpid(session.pid, &status, 0);
        session.pid = -1;
    }

    system_result resultado;
    resultado.ok = sucesso || (WIFEXITED(status) && WEXITSTATUS(status) == 0 && marcadores_sucesso.empty());
    resultado.codigo = resultado.ok ? err::OK : err::BLUETOOTHCTL_FAILED;
    resultado.mensagem = remover_ansi(saida);
    resultado.detalhes = resultado.mensagem;
    if (!resultado.ok && WIFSIGNALED(status)) {
        resultado.codigo = err::BLUETOOTHCTL_SIGNALED;
    }
    return resultado;
}

bool bluetoothctl_saida_indica_sucesso(const std::string& saida) {
    return saida.find("Failed") == std::string::npos &&
           saida.find("not available") == std::string::npos &&
           saida.find("No default controller available") == std::string::npos;
}

std::string resumir_bluetooth_status(const bluetooth_adapter_status& status) {
    std::ostringstream oss;
    oss << "powered=" << (status.powered ? "yes" : "no")
        << ", soft_blocked=" << (status.soft_blocked ? "yes" : "no")
        << ", hard_blocked=" << (status.hard_blocked ? "yes" : "no")
        << ", controller_disponivel=" << (status.controller_disponivel ? "yes" : "no");
    return oss.str();
}

} // namespace bluetooth_internal

void parsing_bluetooth_stream(std::istream& input, std::unordered_map<std::string, device_bt>& mapa, std::string& ultimo_mac_context) {
    std::string linha_raw;
    while (std::getline(input, linha_raw)) {
        std::string linha = bluetooth_internal::remover_ansi(linha_raw);
        if (linha.empty()) continue;

        size_t pos_device = linha.find("Device ");
        if (pos_device != std::string::npos) {
            std::string sub = linha.substr(pos_device + 7);
            std::stringstream ss(sub);
            std::string mac;
            ss >> mac;

            if (mac.length() >= 17 && mac.find(':') != std::string::npos) {
                ultimo_mac_context = mac;
                auto& dev = mapa[mac];
                dev.mac = mac;

                if (linha.find("[DEL]") != std::string::npos) {
                    mapa.erase(mac);
                    ultimo_mac_context.clear();
                    continue;
                }

                std::string resto;
                std::getline(ss, resto);
                size_t first = resto.find_first_not_of(" \t");
                if (first != std::string::npos) {
                    std::string payload = resto.substr(first);
                    size_t pos_colon = payload.find(": ");

                    if (pos_colon == std::string::npos) {
                        while (!payload.empty() && std::isspace(static_cast<unsigned char>(payload.back()))) {
                            payload.pop_back();
                        }
                        if (!payload.empty()) dev.nome = payload;
                    } else {
                        std::string prop = payload.substr(0, pos_colon);
                        std::string val = payload.substr(pos_colon + 2);
                        while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back()))) {
                            val.pop_back();
                        }

                        if (prop == "Name" || prop == "Alias") dev.nome = val;
                        else if (prop == "Icon") dev.icon = val;
                        else if (prop == "Connected") dev.conectado = (val == "yes");
                        else if (prop == "Paired") dev.pareado = (val == "yes");
                        else if (prop == "Trusted") dev.confiavel = (val == "yes");
                    }
                }
                continue;
            }
        }

        if (!ultimo_mac_context.empty()) {
            auto& dev = mapa[ultimo_mac_context];
            size_t pos_colon = linha.find(": ");
            if (pos_colon != std::string::npos) {
                std::string prop_part = linha.substr(0, pos_colon);
                size_t start = prop_part.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    std::string prop = prop_part.substr(start);
                    std::string val = linha.substr(pos_colon + 2);
                    while (!val.empty() && std::isspace(static_cast<unsigned char>(val.back()))) {
                        val.pop_back();
                    }

                    if (prop == "Name" || prop == "Alias") {
                        if (!val.empty()) dev.nome = val;
                    } else if (prop == "Icon") dev.icon = val;
                    else if (prop == "Connected") dev.conectado = (val == "yes");
                    else if (prop == "Paired") dev.pareado = (val == "yes");
                    else if (prop == "Trusted") dev.confiavel = (val == "yes");
                }
            }
        }
    }
}

namespace bluetooth_internal {

device_bt consultar_dispositivo_bluetooth(const std::string& mac) {
    device_bt dispositivo;
    dispositivo.mac = mac;

    std::unordered_map<std::string, device_bt> mapa;
    mapa[mac] = dispositivo;

    system_result resultado = executar_bluetoothctl("info " + mac);
    if (!resultado.ok || resultado.mensagem.empty()) {
        return dispositivo;
    }

    std::stringstream ss_info(resultado.mensagem);
    std::string ctx_info = mac;
    ::parsing_bluetooth_stream(ss_info, mapa, ctx_info);
    return mapa[mac];
}

std::unordered_map<std::string, device_bt> listar_dispositivos_por_comando(const std::string& comando) {
    std::unordered_map<std::string, device_bt> mapa;
    system_result resultado = executar_bluetoothctl(comando);
    if (!resultado.ok || resultado.mensagem.empty()) {
        return mapa;
    }

    std::stringstream ss(resultado.mensagem);
    std::string ctx_dummy;
    ::parsing_bluetooth_stream(ss, mapa, ctx_dummy);
    return mapa;
}

void mesclar_dispositivo_bluetooth(device_bt& destino, const device_bt& origem) {
    if (!origem.mac.empty()) destino.mac = origem.mac;
    if (!origem.nome.empty()) destino.nome = origem.nome;
    if (!origem.icon.empty()) destino.icon = origem.icon;
    destino.conectado = origem.conectado;
    destino.pareado = origem.pareado;
    destino.confiavel = origem.confiavel;
}

void marcar_campo_bool_em_lote(
    std::unordered_map<std::string, device_bt>& destino,
    const std::unordered_map<std::string, device_bt>& origem,
    bool device_bt::*campo
) {
    for (const auto& [mac, dispositivo] : origem) {
        auto it = destino.find(mac);
        if (it == destino.end()) {
            destino.emplace(mac, dispositivo);
            it = destino.find(mac);
        } else {
            mesclar_dispositivo_bluetooth(it->second, dispositivo);
        }
        it->second.*campo = true;
    }
}

std::vector<device_bt> ordenar_dispositivos(const std::unordered_map<std::string, device_bt>& mapa) {
    std::vector<device_bt> lista;
    lista.reserve(mapa.size());
    for (const auto& [_, dispositivo] : mapa) {
        device_bt ajustado = dispositivo;
        if (ajustado.nome.empty()) {
            ajustado.nome = "Dispositivo " + ajustado.mac;
        }
        lista.push_back(ajustado);
    }

    std::sort(lista.begin(), lista.end(), [](const device_bt& a, const device_bt& b) {
        if (a.pareado != b.pareado) {
            return a.pareado > b.pareado;
        }
        if (a.conectado != b.conectado) {
            return a.conectado > b.conectado;
        }
        return a.nome < b.nome;
    });
    return lista;
}

system_result garantir_bluetooth_desbloqueado() {
    if (!comando_existe("rfkill")) {
        return make_bt_success("rfkill ausente; desbloqueio Bluetooth ignorado.");
    }

    command_result rfkill = exec_command_args_result({"rfkill", "unblock", "bluetooth"});
    if (!rfkill.ok) {
        return make_bt_error(err::RFKILL_UNBLOCK_FAILED, "Falha ao desbloquear o Bluetooth.", rfkill.mensagem);
    }
    return make_bt_success("Bluetooth desbloqueado.");
}

system_result validar_adaptador_pronto() {
    bluetooth_adapter_status status = obter_status_bluetooth();
    if (!status.controller_disponivel) {
        return make_bt_error(err::ADAPTER_UNAVAILABLE, "Nenhum adaptador Bluetooth disponivel.", status.show_output);
    }
    if (status.hard_blocked) {
        return make_bt_error(err::ADAPTER_HARD_BLOCKED, "Bluetooth bloqueado fisicamente.", status.rfkill_output);
    }
    if (status.soft_blocked) {
        return make_bt_error(err::ADAPTER_SOFT_BLOCKED, "Bluetooth bloqueado por software.", status.rfkill_output);
    }
    return make_bt_success("Adaptador Bluetooth pronto.");
}

system_result verificar_saida_operacao(
    const system_result& comando_result,
    const std::string& codigo_falha,
    const std::string& mensagem_falha
) {
    if (!comando_result.ok) {
        return mapear_erro_bluetoothctl(codigo_falha, mensagem_falha, comando_result);
    }
    if (!bluetoothctl_saida_indica_sucesso(comando_result.mensagem)) {
        return mapear_erro_bluetoothctl(codigo_falha, mensagem_falha, comando_result);
    }
    return make_bt_success("Comando executado.", comando_result.mensagem);
}

bool dispositivo_esta_conhecido(const std::string& mac) {
    auto conhecidos = listar_dispositivos_por_comando("devices");
    if (conhecidos.find(mac) != conhecidos.end()) {
        return true;
    }
    auto pareados = listar_dispositivos_por_comando("paired-devices");
    return pareados.find(mac) != pareados.end();
}

} // namespace bluetooth_internal
