#include "config-dacc/functions.hpp"
#include "config-dacc/ErrorCodes.hpp"

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <unistd.h>

namespace {

namespace err = config_dacc::errors;

bool grupo_usuario_existe(const std::string& grupo) {
    command_result result = exec_command_args_result({"id", "-nG"});
    if (!result.ok) {
        return false;
    }

    std::stringstream ss(result.stdout_output);
    std::string item;
    while (ss >> item) {
        if (item == grupo) {
            return true;
        }
    }
    return false;
}

bool backlight_sysfs_disponivel() {
    const std::filesystem::path root{"/sys/class/backlight"};
    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) {
        return false;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (!entry.is_directory(ec)) {
            continue;
        }
        if (std::filesystem::exists(entry.path() / "brightness", ec) &&
            std::filesystem::exists(entry.path() / "max_brightness", ec)) {
            return true;
        }
    }
    return false;
}

void adicionar_missing(std::vector<std::string>& missing, bool presente, const std::string& nome) {
    if (!presente) {
        missing.push_back(nome);
    }
}

bool capacidade_presente(const system_result& result, const std::string& codigo_indisponivel) {
    return result.ok || result.codigo != codigo_indisponivel;
}

bool displays_suportam_backend(
    const std::vector<DisplayOutput>& displays,
    display_backend backend
) {
    for (const auto& display : displays) {
        if (display.backend == backend) {
            return true;
        }
    }
    return false;
}

bool dispositivos_audio_suportam_selecao(const std::vector<device_audio>& dispositivos) {
    for (const auto& dispositivo : dispositivos) {
        if (dispositivo.backend == audio_backend::wpctl ||
            dispositivo.backend == audio_backend::pactl) {
            return true;
        }
    }
    return false;
}

} // namespace

station_capabilities obter_capacidades_sistema() {
    station_capabilities caps;

    const char* session = std::getenv("XDG_SESSION_TYPE");
    const char* desktop = std::getenv("XDG_SESSION_DESKTOP");
    caps.session_type = session ? std::string(session) : "unknown";
    caps.desktop = desktop ? std::string(desktop) : "";

    std::vector<device_audio> dispositivos_audio;
    system_result audio_result = listar_dispositivos_audio_result(dispositivos_audio);
    caps.audio_list = capacidade_presente(audio_result, err::AUDIO_SUBSYSTEM_MISSING);
    caps.audio_select = caps.audio_list && dispositivos_audio_suportam_selecao(dispositivos_audio);

    int volume = 0;
    system_result volume_result = obter_volume_atual_result(volume);
    caps.volume_control = capacidade_presente(volume_result, err::AUDIO_SUBSYSTEM_MISSING);

    std::vector<wifi_network> redes;
    system_result network_result = listar_wifi_result(redes);
    caps.network = capacidade_presente(network_result, err::NETWORK_MANAGER_MISSING);

    caps.bluetooth = comando_existe("bluetoothctl");

    std::vector<DisplayOutput> displays;
    system_result display_result = listar_displays_result(displays);
    caps.display_info = capacidade_presente(display_result, err::DISPLAY_SUBSYSTEM_MISSING);
    const bool tem_display_xrandr = displays_suportam_backend(displays, display_backend::xrandr);
    const bool tem_display_wlrrandr = displays_suportam_backend(displays, display_backend::wlrrandr);
    caps.display_resolution = (caps.session_type == "x11" && tem_display_xrandr) ||
                              (caps.session_type == "wayland" && tem_display_wlrrandr);
    caps.display_scale = caps.display_resolution ||
                         (caps.session_type == "wayland" &&
                          caps.desktop.find("gnome") != std::string::npos &&
                          caps.display_info);

    caps.backlight_sysfs = backlight_sysfs_disponivel();
    int brilho = 0;
    system_result brightness_result = obter_brilho_result(brilho);
    caps.brightness = capacidade_presente(brightness_result, err::BRIGHTNESS_NOT_SUPPORTED);
    caps.intro_video = comando_existe("mpv");
    caps.user_video_group = grupo_usuario_existe("video");
    caps.user_audio_group = grupo_usuario_existe("audio");
    caps.user_netdev_group = grupo_usuario_existe("netdev");

    adicionar_missing(caps.missing, caps.audio_list, "audio_list");
    adicionar_missing(caps.missing, caps.audio_select, "audio_select");
    adicionar_missing(caps.missing, caps.volume_control, "volume_control");
    adicionar_missing(caps.missing, caps.network, "network");
    adicionar_missing(caps.missing, caps.bluetooth, "bluetooth");
    adicionar_missing(caps.missing, caps.display_info, "display_info");
    adicionar_missing(caps.missing, caps.display_resolution, "display_resolution");
    adicionar_missing(caps.missing, caps.display_scale, "display_scale");
    adicionar_missing(caps.missing, caps.brightness, "brightness");
    adicionar_missing(caps.missing, caps.intro_video, "intro_video");

    return caps;
}
