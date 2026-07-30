#include "config-dacc/functions.hpp"

#include <iostream>
#include <string>

namespace {

void imprimir_bool(const std::string& nome, bool valor) {
    std::cout << "  " << nome << ": " << (valor ? "true" : "false") << '\n';
}

} // namespace

int main() {
    station_capabilities caps = obter_capacidades_sistema();

    std::cout << "DACC Station capabilities smoke test\n";
    std::cout << "====================================\n\n";

    std::cout << "Ambiente\n";
    std::cout << "  session_type: " << (caps.session_type.empty() ? "unknown" : caps.session_type) << '\n';
    std::cout << "  desktop: " << (caps.desktop.empty() ? "unknown" : caps.desktop) << "\n\n";

    std::cout << "Capacidades\n";
    imprimir_bool("audio_list", caps.audio_list);
    imprimir_bool("audio_select", caps.audio_select);
    imprimir_bool("volume_control", caps.volume_control);
    imprimir_bool("network", caps.network);
    imprimir_bool("bluetooth", caps.bluetooth);
    imprimir_bool("display_info", caps.display_info);
    imprimir_bool("display_select", caps.display_select);
    imprimir_bool("display_resolution", caps.display_resolution);
    imprimir_bool("display_scale", caps.display_scale);
    imprimir_bool("brightness", caps.brightness);
    imprimir_bool("intro_video", caps.intro_video);
    imprimir_bool("backlight_sysfs", caps.backlight_sysfs);
    imprimir_bool("user_video_group", caps.user_video_group);
    imprimir_bool("user_audio_group", caps.user_audio_group);
    imprimir_bool("user_netdev_group", caps.user_netdev_group);

    std::cout << "\nFaltando\n";
    if (caps.missing.empty()) {
        std::cout << "  nenhum item essencial ausente\n";
    } else {
        for (const auto& item : caps.missing) {
            std::cout << "  - " << item << '\n';
        }
    }

    return 0;
}
