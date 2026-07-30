#include "config-dacc/DisplayParsing.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

void exigir(bool condicao, const std::string& mensagem) {
    if (!condicao) {
        std::cerr << "[FAIL] " << mensagem << std::endl;
        std::exit(1);
    }
}

void testar_wlr_randr_basico() {
    const std::string entrada =
        "HDMI-A-1 \"Samsung Electric Company 24\\\"\"\n"
        "  Enabled: yes\n"
        "  Modes:\n"
        "    1920x1080 px, 60.000000 Hz (preferred, current)\n"
        "    1280x720 px, 60.000000 Hz\n"
        "  Position: 0,0\n"
        "  Scale: 1.000000\n";

    auto displays = config_dacc::display_parsing::parse_wlr_randr(entrada);

    exigir(displays.size() == 1, "deve parsear uma saida wlr-randr");
    exigir(displays[0].name == "HDMI-A-1", "deve preservar nome da saida");
    exigir(displays[0].backend_id == "HDMI-A-1", "deve preencher backend_id wlr-randr");
    exigir(displays[0].backend == display_backend::wlrrandr, "deve marcar backend wlr-randr");
    exigir(displays[0].connected, "Enabled: yes deve marcar conectado");
    exigir(displays[0].modes.size() == 2, "deve parsear modos");
    exigir(displays[0].current_mode.width == 1920, "deve detectar largura atual");
    exigir(displays[0].current_mode.height == 1080, "deve detectar altura atual");
    exigir(displays[0].current_mode.is_current, "deve marcar modo atual");
    exigir(displays[0].current_scale == 1.0f, "deve parsear escala atual");
}

void testar_xrandr_basico() {
    const std::string entrada =
        "HDMI-1 connected primary 1920x1080+0+0 (normal left inverted right x axis y axis)\n"
        "   1920x1080     60.00*+  59.94\n"
        "   1280x720      60.00\n";

    auto displays = config_dacc::display_parsing::parse_xrandr_verbose(entrada);

    exigir(displays.size() == 1, "deve parsear uma saida xrandr");
    exigir(displays[0].name == "HDMI-1", "deve preservar nome xrandr");
    exigir(displays[0].backend_id == "HDMI-1", "deve preencher backend_id xrandr");
    exigir(displays[0].backend == display_backend::xrandr, "deve marcar backend xrandr");
    exigir(displays[0].primary, "deve identificar a saida primaria do xrandr");
    exigir(displays[0].modes.size() == 2, "deve parsear modos xrandr");
    exigir(displays[0].current_mode.width == 1920, "deve detectar modo atual xrandr");
}

} // namespace

int main() {
    testar_wlr_randr_basico();
    testar_xrandr_basico();
    std::cout << "Display parser tests passed." << std::endl;
    return 0;
}
