#include "config-dacc/ErrorCodes.hpp"
#include "config-dacc/functions.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace {

namespace err = config_dacc::errors;

void exigir(bool condicao, const std::string& mensagem) {
    if (!condicao) {
        std::cerr << "[FAIL] " << mensagem << std::endl;
        std::exit(1);
    }
}

std::filesystem::path criar_backend_xrandr_falso(
    const std::filesystem::path& diretorio
) {
    const std::filesystem::path script = diretorio / "xrandr";
    std::ofstream arquivo(script);
    arquivo
        << "#!/bin/sh\n"
        << "if [ \"$1\" = \"--query\" ]; then\n"
        << "  printf '%s\\n' "
           "'HDMI-1 connected primary 1920x1080+0+0' "
           "'   1920x1080     60.00*+ 59.94' "
           "'DP-1 connected 1280x720+1920+0' "
           "'   1280x720      60.00*+'\n"
        << "  exit 0\n"
        << "fi\n"
        << "printf '%s\\n' \"$*\" > \"$DACC_DISPLAY_TEST_LOG\"\n";
    arquivo.close();

    exigir(arquivo.good(), "deve criar o backend xrandr falso");
    exigir(chmod(script.c_str(), 0700) == 0, "deve tornar o backend falso executavel");
    return script;
}

void testar_selecao_xrandr() {
    char modeloDiretorio[] = "/tmp/dacc-display-test-XXXXXX";
    char* diretorioCriado = mkdtemp(modeloDiretorio);
    exigir(diretorioCriado != nullptr, "deve criar diretorio temporario");

    const std::filesystem::path diretorio(diretorioCriado);
    const std::filesystem::path log = diretorio / "comando.log";
    (void)criar_backend_xrandr_falso(diretorio);

    exigir(setenv("PATH", diretorio.c_str(), 1) == 0, "deve configurar PATH de teste");
    exigir(setenv("XDG_SESSION_TYPE", "x11", 1) == 0, "deve configurar sessao X11");
    exigir(
        setenv("DACC_DISPLAY_TEST_LOG", log.c_str(), 1) == 0,
        "deve configurar log do backend falso"
    );

    std::vector<DisplayOutput> displays;
    system_result listagem = listar_displays_result(displays);
    exigir(listagem.ok, "deve listar monitores pelo xrandr falso");
    exigir(displays.size() == 2, "deve listar as duas saidas conectadas");
    exigir(displays[0].primary, "deve preservar a saida primaria");

    system_result selecao = selecionar_display_result(displays[1]);
    exigir(selecao.ok, "deve selecionar a segunda saida");
    exigir(selecao.codigo == err::OK, "selecao bem-sucedida deve retornar codigo ok");

    std::ifstream entradaLog(log);
    std::string comandoExecutado;
    std::getline(entradaLog, comandoExecutado);
    exigir(
        comandoExecutado == "--output DP-1 --primary",
        "deve executar xrandr com argumentos separados e o backend_id correto"
    );

    system_result resolucao = alterarResolucao_result("DP-1", 1280, 720, 59.94f);
    exigir(resolucao.ok, "deve aplicar resolucao e taxa pelo xrandr falso");
    entradaLog.close();
    entradaLog.open(log);
    std::getline(entradaLog, comandoExecutado);
    exigir(
        comandoExecutado == "--output DP-1 --mode 1280x720 --rate 59.94",
        "deve encaminhar a taxa real ao xrandr"
    );

    DisplayOutput inexistente;
    inexistente.backend_id = "VIRTUAL-9";
    system_result ausente = selecionar_display_result(inexistente);
    exigir(!ausente.ok, "deve rejeitar monitor inexistente");
    exigir(
        ausente.codigo == err::DISPLAY_OUTPUT_NOT_FOUND,
        "monitor inexistente deve retornar display_output_not_found"
    );

    std::error_code ec;
    std::filesystem::remove_all(diretorio, ec);
}

void testar_resolucao_wlr_randr() {
    char modeloDiretorio[] = "/tmp/dacc-wlr-display-test-XXXXXX";
    char* diretorioCriado = mkdtemp(modeloDiretorio);
    exigir(diretorioCriado != nullptr, "deve criar diretorio temporario Wayland");

    const std::filesystem::path diretorio(diretorioCriado);
    const std::filesystem::path script = diretorio / "wlr-randr";
    const std::filesystem::path log = diretorio / "comando.log";
    std::ofstream arquivo(script);
    arquivo
        << "#!/bin/sh\n"
        << "printf '%s\\n' \"$*\" > \"$DACC_DISPLAY_TEST_LOG\"\n";
    arquivo.close();
    exigir(arquivo.good(), "deve criar o backend wlr-randr falso");
    exigir(chmod(script.c_str(), 0700) == 0, "deve tornar wlr-randr falso executavel");

    exigir(setenv("PATH", diretorio.c_str(), 1) == 0, "deve configurar PATH Wayland");
    exigir(setenv("XDG_SESSION_TYPE", "wayland", 1) == 0, "deve configurar sessao Wayland");
    exigir(setenv("DACC_DISPLAY_TEST_LOG", log.c_str(), 1) == 0, "deve configurar log Wayland");

    system_result resultado = alterarResolucao_result("HDMI-A-1", 1920, 1080, 59.94f);
    exigir(resultado.ok, "deve aplicar resolucao pelo wlr-randr falso");

    std::ifstream entrada(log);
    std::string comandoExecutado;
    std::getline(entrada, comandoExecutado);
    exigir(
        comandoExecutado == "--output HDMI-A-1 --mode 1920x1080@59.94Hz",
        "deve encaminhar a taxa real no formato aceito pelo wlr-randr"
    );

    std::error_code ec;
    std::filesystem::remove_all(diretorio, ec);
}

} // namespace

int main() {
    testar_selecao_xrandr();
    testar_resolucao_wlr_randr();
    std::cout << "Display control tests passed." << std::endl;
    return 0;
}
