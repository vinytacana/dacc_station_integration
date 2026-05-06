#include "config-dacc/AudioParsing.hpp"

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

void testar_wpctl_sinks() {
    const std::string entrada =
        "Audio\n"
        " ├─ Sinks:\n"
        " │  *   53. Ryzen HD Audio Controller Estereo analogico [vol: 0.99]\n"
        " │      89. HDMI Audio Controller Estereo digital [vol: 1.00]\n"
        " ├─ Sources:\n";

    auto devices = config_dacc::audio_parsing::parse_wpctl_sinks(entrada);

    exigir(devices.size() == 2, "deve parsear sinks wpctl");
    exigir(devices[0].id == 53, "deve parsear id wpctl");
    exigir(devices[0].backend_id == "53", "deve preencher backend_id wpctl");
    exigir(devices[0].backend == audio_backend::wpctl, "deve marcar backend wpctl");
    exigir(devices[0].padrao, "deve marcar sink padrao wpctl");
    exigir(devices[0].descricao == "Ryzen HD Audio Controller Estereo analogico", "deve limpar descricao wpctl");
}

void testar_pactl_sinks() {
    const std::string entrada =
        "1\talsa_output.pci-0000_00_1f.3.analog-stereo\tPipeWire\ts16le 2ch 48000Hz\tRUNNING\n"
        "2\tbluez_output.AA_BB_CC.a2dp-sink\tPipeWire\ts16le 2ch 48000Hz\tIDLE\n";

    auto devices = config_dacc::audio_parsing::parse_pactl_sinks_short(entrada, "bluez_output.AA_BB_CC.a2dp-sink\n");

    exigir(devices.size() == 2, "deve parsear sinks pactl");
    exigir(devices[1].id == 2, "deve parsear id pactl");
    exigir(devices[1].backend_id == "bluez_output.AA_BB_CC.a2dp-sink", "deve usar nome do sink como backend_id pactl");
    exigir(devices[1].backend == audio_backend::pactl, "deve marcar backend pactl");
    exigir(devices[1].padrao, "deve marcar default sink pactl");
    exigir(devices[1].descricao == "bluez_output.AA_BB_CC.a2dp-sink", "deve parsear nome pactl");
}

void testar_aplay_devices() {
    const std::string entrada =
        "card 0: Headphones [bcm2835 Headphones], device 0: bcm2835 Headphones [bcm2835 Headphones]\n"
        "card 1: HDMI [bcm2835 HDMI], device 0: bcm2835 HDMI [bcm2835 HDMI]\n";

    auto devices = config_dacc::audio_parsing::parse_aplay_devices(entrada);

    exigir(devices.size() == 2, "deve parsear dispositivos aplay");
    exigir(devices[0].backend == audio_backend::alsa, "deve marcar backend alsa");
    exigir(devices[0].backend_id == "alsa:0", "deve gerar backend_id alsa");
    exigir(devices[0].padrao, "primeiro dispositivo ALSA deve ser padrao");
}

void testar_volumes() {
    int volume = 0;
    bool mudo = false;
    exigir(config_dacc::audio_parsing::parse_wpctl_volume("Volume: 0.52\n", volume), "deve parsear volume wpctl");
    exigir(volume == 52, "volume wpctl deve virar percentual");
    exigir(config_dacc::audio_parsing::parse_wpctl_muted("Volume: 0.52 [MUTED]\n", mudo), "deve parsear mudo wpctl");
    exigir(mudo, "wpctl [MUTED] deve marcar mudo");
    exigir(config_dacc::audio_parsing::parse_pactl_volume("Volume: front-left: 65536 / 74% / 0.00 dB", volume), "deve parsear volume pactl");
    exigir(volume == 74, "volume pactl deve preservar percentual");
    exigir(config_dacc::audio_parsing::parse_pactl_muted("Mute: no\n", mudo), "deve parsear mudo pactl");
    exigir(!mudo, "pactl Mute: no deve marcar nao mudo");
    exigir(config_dacc::audio_parsing::parse_amixer_volume("Front Left: Playback 74 [74%] [-16.50dB]", volume), "deve parsear volume amixer");
    exigir(volume == 74, "volume amixer deve preservar percentual");
    exigir(config_dacc::audio_parsing::parse_amixer_muted("Front Left: Playback 74 [74%] [-16.50dB] [off]", mudo), "deve parsear mudo amixer");
    exigir(mudo, "amixer [off] deve marcar mudo");
}

} // namespace

int main() {
    testar_wpctl_sinks();
    testar_pactl_sinks();
    testar_aplay_devices();
    testar_volumes();
    std::cout << "Audio parser tests passed." << std::endl;
    return 0;
}
