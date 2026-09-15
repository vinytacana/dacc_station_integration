#include "config-dacc/functions.hpp"
#include "BluetoothInternal.hpp"
#include "config-dacc/ErrorCodes.hpp"

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <unistd.h>

#ifndef FAKE_BLUETOOTHCTL_PATH
#error "FAKE_BLUETOOTHCTL_PATH deve apontar para o bluetoothctl falso"
#endif

namespace {

bool g_bluetoothctl_disponivel = false;

void exigir(bool condicao, const std::string& mensagem) {
    if (!condicao) {
        std::cerr << "[FAIL] " << mensagem << std::endl;
        std::exit(1);
    }
}

void testar_info_multilinha_com_ansi() {
    std::unordered_map<std::string, device_bt> mapa;
    std::string contexto;
    std::stringstream entrada(
        "\033[0;94mDevice AA:BB:CC:DD:EE:01\033[0m Controle Pro\n"
        "    Name: Controle Pro\n"
        "    Alias: Controle Sala\n"
        "    Icon: input-gaming\n"
        "    Connected: yes\n"
        "    Paired: yes\n"
        "    Trusted: yes\n"
    );

    parsing_bluetooth_stream(entrada, mapa, contexto);

    exigir(mapa.size() == 1, "deve parsear um unico dispositivo");
    const auto& dev = mapa["AA:BB:CC:DD:EE:01"];
    exigir(dev.nome == "Controle Sala", "Alias deve sobrescrever o nome quando presente");
    exigir(dev.icon == "input-gaming", "Icon deve ser preservado");
    exigir(dev.conectado, "Connected: yes deve marcar conectado");
    exigir(dev.pareado, "Paired: yes deve marcar pareado");
    exigir(dev.confiavel, "Trusted: yes deve marcar confiavel");
}

void testar_eventos_chg_sem_repetir_mac() {
    std::unordered_map<std::string, device_bt> mapa;
    std::string contexto;
    std::stringstream entrada(
        "[NEW] Device 11:22:33:44:55:66 Fone Azul\n"
        "[CHG] Device 11:22:33:44:55:66 Connected: yes\n"
        "[CHG] Device 11:22:33:44:55:66 Paired: no\n"
        "[CHG] Device 11:22:33:44:55:66 Icon: audio-headphones\n"
    );

    parsing_bluetooth_stream(entrada, mapa, contexto);

    exigir(mapa.size() == 1, "eventos CHG do mesmo MAC nao devem duplicar dispositivo");
    const auto& dev = mapa["11:22:33:44:55:66"];
    exigir(dev.nome == "Fone Azul", "nome vindo do evento NEW deve ser mantido");
    exigir(dev.conectado, "evento Connected: yes deve ser aplicado");
    exigir(!dev.pareado, "evento Paired: no deve ser aplicado");
    exigir(dev.icon == "audio-headphones", "evento Icon deve ser aplicado");
}

void testar_delete_remove_dispositivo() {
    std::unordered_map<std::string, device_bt> mapa;
    std::string contexto;
    std::stringstream entrada(
        "[NEW] Device 22:33:44:55:66:77 Teclado\n"
        "[DEL] Device 22:33:44:55:66:77 Teclado\n"
    );

    parsing_bluetooth_stream(entrada, mapa, contexto);

    exigir(mapa.empty(), "evento DEL deve remover dispositivo do mapa");
    exigir(contexto.empty(), "evento DEL deve limpar contexto do MAC removido");
}

void testar_status_final_bluetoothctl() {
    system_result falha = bluetooth_internal::classificar_status_bluetoothctl(
        23 << 8,
        true,
        "erro de teste"
    );
    exigir(!falha.ok, "status de saida diferente de zero nao pode ser tratado como sucesso");
    exigir(
        falha.codigo == config_dacc::errors::BLUETOOTHCTL_FAILED,
        "status de saida diferente de zero deve usar bluetoothctl_failed"
    );

    system_result ausente = bluetooth_internal::classificar_status_bluetoothctl(0, false, "");
    exigir(!ausente.ok, "status nao coletado nao pode usar o valor zero inicial como sucesso");
    exigir(
        ausente.codigo == config_dacc::errors::BLUETOOTHCTL_FAILED,
        "status nao coletado deve ser reportado como falha"
    );

    system_result sucesso = bluetooth_internal::classificar_status_bluetoothctl(0, true, "ok");
    exigir(sucesso.ok, "status zero coletado deve continuar indicando sucesso");
}

std::string ler_arquivo(const std::string& caminho) {
    std::ifstream arquivo(caminho);
    std::ostringstream conteudo;
    conteudo << arquivo.rdbuf();
    return conteudo.str();
}

void limpar_arquivo(const std::string& caminho) {
    unlink(caminho.c_str());
}

system_result executar_fake(const std::string& modo, int timeout_ms = 1000) {
    setenv("DACC_BT_FAKE_MODE", modo.c_str(), 1);
    return bluetooth_internal::executar_bluetoothctl("show", timeout_ms);
}

void testar_ciclo_pty_bluetoothctl() {
    char diretorio_template[] = "/tmp/dacc-bt-test-XXXXXX";
    char* diretorio_criado = mkdtemp(diretorio_template);
    exigir(diretorio_criado != nullptr, "deve criar diretorio temporario para bluetoothctl falso");

    const std::string diretorio(diretorio_criado);
    const std::string executavel = diretorio + "/bluetoothctl";
    const std::string signal_log = diretorio + "/signal.log";
    const std::string pid_log = diretorio + "/pid.log";
    exigir(
        symlink(FAKE_BLUETOOTHCTL_PATH, executavel.c_str()) == 0,
        "deve expor bluetoothctl falso no PATH"
    );

    const char* path_anterior_env = std::getenv("PATH");
    const std::string path_anterior = path_anterior_env == nullptr ? "" : path_anterior_env;
    setenv("PATH", diretorio.c_str(), 1);
    setenv("DACC_BT_FAKE_SIGNAL_LOG", signal_log.c_str(), 1);
    setenv("DACC_BT_FAKE_PID_LOG", pid_log.c_str(), 1);
    g_bluetoothctl_disponivel = true;

    system_result sucesso = executar_fake("exit0");
    exigir(sucesso.ok, "exit 0 do bluetoothctl deve indicar sucesso");
    exigir(sucesso.codigo == config_dacc::errors::OK, "exit 0 deve usar codigo ok");
    exigir(
        sucesso.mensagem.find("saida parcial") != std::string::npos,
        "saida do bluetoothctl deve ser preservada no sucesso"
    );

    system_result falha = executar_fake("exit23");
    exigir(!falha.ok, "exit diferente de zero do bluetoothctl deve indicar falha");
    exigir(
        falha.codigo == config_dacc::errors::BLUETOOTHCTL_FAILED,
        "exit diferente de zero deve usar bluetoothctl_failed"
    );
    exigir(
        falha.mensagem.find("saida parcial") != std::string::npos,
        "saida anterior ao exit diferente de zero deve ser preservada"
    );

    system_result sinal = executar_fake("signal");
    exigir(!sinal.ok, "termino do bluetoothctl por sinal deve indicar falha");
    exigir(
        sinal.codigo == config_dacc::errors::BLUETOOTHCTL_SIGNALED,
        "termino por sinal deve usar bluetoothctl_signaled"
    );
    exigir(
        sinal.mensagem.find("saida parcial") != std::string::npos,
        "saida anterior ao sinal deve ser preservada"
    );

    limpar_arquivo(signal_log);
    system_result timeout_term = executar_fake("timeout_term", 150);
    exigir(!timeout_term.ok, "timeout deve indicar falha");
    exigir(
        timeout_term.codigo == config_dacc::errors::BLUETOOTHCTL_TIMEOUT,
        "timeout deve usar bluetoothctl_timeout"
    );
    exigir(
        timeout_term.detalhes.find("saida parcial") != std::string::npos,
        "timeout deve preservar a saida parcial"
    );
    exigir(
        ler_arquivo(signal_log).find("SIGTERM") != std::string::npos,
        "timeout deve enviar SIGTERM antes de encerrar o bluetoothctl"
    );

    limpar_arquivo(signal_log);
    limpar_arquivo(pid_log);
    const auto inicio_kill = std::chrono::steady_clock::now();
    system_result timeout_kill = executar_fake("timeout_kill", 150);
    const auto duracao_kill = std::chrono::steady_clock::now() - inicio_kill;
    exigir(
        timeout_kill.codigo == config_dacc::errors::BLUETOOTHCTL_TIMEOUT,
        "processo resistente a TERM deve continuar reportando timeout"
    );
    exigir(
        timeout_kill.detalhes.find("saida parcial") != std::string::npos,
        "fallback SIGKILL deve preservar a saida parcial"
    );
    exigir(
        ler_arquivo(signal_log).find("SIGTERM") != std::string::npos,
        "fallback SIGKILL deve ser precedido por SIGTERM"
    );
    exigir(
        duracao_kill >= std::chrono::milliseconds(250) && duracao_kill < std::chrono::seconds(2),
        "fallback SIGKILL deve respeitar a graca e finalizar em tempo limitado"
    );

    std::ifstream pid_arquivo(pid_log);
    pid_t pid = -1;
    pid_arquivo >> pid;
    exigir(pid > 0, "bluetoothctl falso deve registrar seu PID");
    errno = 0;
    exigir(
        kill(pid, 0) == -1 && errno == ESRCH,
        "fallback SIGKILL deve coletar o processo sem deixar zombie"
    );

    g_bluetoothctl_disponivel = false;
    setenv("PATH", path_anterior.c_str(), 1);
    unsetenv("DACC_BT_FAKE_MODE");
    unsetenv("DACC_BT_FAKE_SIGNAL_LOG");
    unsetenv("DACC_BT_FAKE_PID_LOG");
    unlink(executavel.c_str());
    limpar_arquivo(signal_log);
    limpar_arquivo(pid_log);
    rmdir(diretorio.c_str());
}

} // namespace

command_result exec_command_result(const std::string&) {
    return {};
}

command_result exec_command_args_result(const std::vector<std::string>&) {
    return {};
}

bool comando_existe(const std::string& comando) {
    return g_bluetoothctl_disponivel && comando == "bluetoothctl";
}

bluetooth_adapter_status obter_status_bluetooth() {
    return {};
}

int main() {
    testar_info_multilinha_com_ansi();
    testar_eventos_chg_sem_repetir_mac();
    testar_delete_remove_dispositivo();
    testar_status_final_bluetoothctl();
    testar_ciclo_pty_bluetoothctl();
    std::cout << "Bluetooth parser tests passed." << std::endl;
    return 0;
}
