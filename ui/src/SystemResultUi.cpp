#include "SystemResultUi.hpp"

#include "LogManager.hpp"
#include "config-dacc/ErrorCodes.hpp"

#include <string>

namespace MeuProjeto {

namespace {

bool codigoIgual(const system_result& resultado, const char* codigo) {
    return resultado.codigo == codigo;
}

} // namespace

std::string mensagemResultadoUi(const system_result& resultado, const std::string& fallback) {
    namespace err = config_dacc::errors;

    if (resultado.ok) {
        return resultado.mensagem.empty() ? fallback : resultado.mensagem;
    }

    if (codigoIgual(resultado, err::AUDIO_SUBSYSTEM_MISSING)) return "Audio indisponivel neste sistema.";
    if (codigoIgual(resultado, err::AUDIO_DEVICE_NOT_FOUND)) return "Dispositivo de audio nao encontrado.";
    if (codigoIgual(resultado, err::AUDIO_NO_DEVICES)) return "Nenhum dispositivo de audio encontrado.";
    if (codigoIgual(resultado, err::AUDIO_SELECT_UNSUPPORTED)) return "Selecao de audio indisponivel.";
    if (codigoIgual(resultado, err::AUDIO_VOLUME_FAILED)) return "Falha ao alterar volume.";
    if (codigoIgual(resultado, err::AUDIO_MUTE_FAILED)) return "Falha ao alterar mudo do audio.";

    if (codigoIgual(resultado, err::DISPLAY_SUBSYSTEM_MISSING)) return "Video indisponivel nesta sessao.";
    if (codigoIgual(resultado, err::DISPLAY_NO_OUTPUTS)) return "Nenhum monitor detectado.";
    if (codigoIgual(resultado, err::DISPLAY_OUTPUT_NOT_FOUND)) return "Monitor nao encontrado.";
    if (codigoIgual(resultado, err::DISPLAY_MODE_UNSUPPORTED)) return "Resolucao nao suportada pelo monitor.";
    if (codigoIgual(resultado, err::DISPLAY_SESSION_UNKNOWN)) return "Sessao grafica nao suportada.";
    if (codigoIgual(resultado, err::DISPLAY_SCALE_FAILED)) return "Falha ao alterar escala.";
    if (codigoIgual(resultado, err::DISPLAY_RESOLUTION_FAILED)) return "Falha ao alterar resolucao.";
    if (codigoIgual(resultado, err::BRIGHTNESS_NOT_SUPPORTED)) return "Controle de brilho indisponivel.";

    if (codigoIgual(resultado, err::NETWORK_MANAGER_MISSING)) return "NetworkManager/nmcli indisponivel.";
    if (codigoIgual(resultado, err::NETWORK_MANAGER_UNAVAILABLE)) return "NetworkManager nao esta ativo.";
    if (codigoIgual(resultado, err::WIFI_AUTH_FAILED)) return "Senha Wi-Fi incorreta ou recusada.";
    if (codigoIgual(resultado, err::WIFI_CONNECT_FAILED)) return "Falha ao conectar na rede Wi-Fi.";
    if (codigoIgual(resultado, err::WIFI_DISABLED)) return "Wi-Fi desativado.";
    if (codigoIgual(resultado, err::WIFI_NO_NETWORKS)) return "Nenhuma rede Wi-Fi detectada.";
    if (codigoIgual(resultado, err::WIFI_NOT_FOUND)) return "Rede Wi-Fi nao encontrada.";
    if (codigoIgual(resultado, err::WIFI_PASSWORD_REQUIRED)) return "Senha Wi-Fi obrigatoria.";
    if (codigoIgual(resultado, err::WIFI_SCAN_FAILED)) return "Falha ao procurar redes Wi-Fi.";
    if (codigoIgual(resultado, err::WIFI_TOGGLE_FAILED)) return "Falha ao alterar estado do Wi-Fi.";

    if (codigoIgual(resultado, err::BLUETOOTHCTL_MISSING)) return "Bluetooth indisponivel: bluetoothctl nao detectado.";
    if (codigoIgual(resultado, err::ADAPTER_UNAVAILABLE)) return "Nenhum adaptador Bluetooth disponivel.";
    if (codigoIgual(resultado, err::ADAPTER_BLOCKED) ||
        codigoIgual(resultado, err::ADAPTER_SOFT_BLOCKED) ||
        codigoIgual(resultado, err::ADAPTER_HARD_BLOCKED)) {
        return "Bluetooth bloqueado no sistema.";
    }
    if (codigoIgual(resultado, err::DEVICE_UNAVAILABLE)) return "Dispositivo Bluetooth indisponivel.";
    if (codigoIgual(resultado, err::DEVICE_NOT_CONNECTED)) return "Dispositivo nao esta conectado.";
    if (codigoIgual(resultado, err::AUTHENTICATION_FAILED)) return "Falha de autenticacao no dispositivo.";
    if (codigoIgual(resultado, err::OPERATION_TIMEOUT) ||
        codigoIgual(resultado, err::BLUETOOTHCTL_TIMEOUT)) {
        return "O dispositivo demorou para responder.";
    }
    if (codigoIgual(resultado, err::CONNECT_FAILED) ||
        codigoIgual(resultado, err::CONNECT_NOT_REFLECTED)) {
        return "Falha ao conectar dispositivo Bluetooth.";
    }
    if (codigoIgual(resultado, err::DISCONNECT_FAILED) ||
        codigoIgual(resultado, err::DISCONNECT_NOT_REFLECTED)) {
        return "Falha ao desconectar dispositivo Bluetooth.";
    }
    if (codigoIgual(resultado, err::PAIR_FAILED) ||
        codigoIgual(resultado, err::PAIR_NOT_REFLECTED)) {
        return "Falha ao parear dispositivo Bluetooth.";
    }

    return resultado.mensagem.empty() ? fallback : resultado.mensagem;
}

std::string mensagemRecursoIndisponivelUi(const std::string& recurso) {
    return recurso + " indisponivel neste sistema.";
}

void registrarResultadoErroUi(const std::string& origem, const std::string& operacao, const system_result& resultado) {
    if (resultado.ok) {
        return;
    }

    LOG_WARNING("[" + origem + "][" + operacao + "] codigo=" + resultado.codigo +
                " mensagem=" + resultado.mensagem + " detalhes=" + resultado.detalhes);
}

} // namespace MeuProjeto
