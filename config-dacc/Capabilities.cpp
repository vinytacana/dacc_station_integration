#include "config-dacc/functions.hpp"

#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <unistd.h>

namespace {

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

} // namespace

station_capabilities obter_capacidades_sistema() {
    station_capabilities caps;

    const char* session = std::getenv("XDG_SESSION_TYPE");
    const char* desktop = std::getenv("XDG_SESSION_DESKTOP");
    caps.session_type = session ? std::string(session) : "unknown";
    caps.desktop = desktop ? std::string(desktop) : "";

    const bool tem_wpctl = comando_existe("wpctl");
    const bool tem_pactl = comando_existe("pactl");
    const bool tem_aplay = comando_existe("aplay");
    const bool tem_amixer = comando_existe("amixer");
    const bool tem_xrandr = comando_existe("xrandr");
    const bool tem_wlr_randr = comando_existe("wlr-randr");
    const bool tem_gsettings = comando_existe("gsettings");

    caps.audio_list = tem_wpctl || tem_pactl || tem_aplay;
    caps.audio_select = tem_wpctl || tem_pactl;
    caps.volume_control = tem_wpctl || tem_pactl || tem_amixer;
    caps.network = comando_existe("nmcli");
    caps.bluetooth = comando_existe("bluetoothctl");
    caps.display_info = tem_xrandr || tem_wlr_randr;
    caps.display_resolution = (caps.session_type == "x11" && tem_xrandr) ||
                              (caps.session_type == "wayland" && tem_wlr_randr);
    caps.display_scale = (caps.session_type == "x11" && tem_xrandr) ||
                         (caps.session_type == "wayland" &&
                          ((caps.desktop.find("gnome") != std::string::npos && tem_gsettings) ||
                           (caps.desktop.find("gnome") == std::string::npos && tem_wlr_randr)));
    caps.backlight_sysfs = backlight_sysfs_disponivel();
    caps.brightness = comando_existe("brightnessctl") || caps.backlight_sysfs;
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
