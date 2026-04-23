#ifndef FUNCOES_H
#define FUNCOES_H

#include <string>
#include <vector>
#include <unordered_map>
#include <istream>

struct wifi_network{
    std::string ssid;
    int sinal;
    std::string seguranca;
    bool em_uso;
};

struct wifi_result {
    bool ok = false;
    std::string mensagem;
    std::string codigo;
    std::string detalhes;
};

struct wifi_adapter_status {
    bool enabled = false;
    bool disponivel = true;
    std::string output;
};

struct device_bt {
    std::string mac;
    std::string nome;
    std::string icon;
    bool conectado = false;
    bool pareado = false;
    bool confiavel = false;
};

struct bluetooth_result {
    bool ok = false;
    std::string mensagem;
    std::string codigo;
    std::string detalhes;
};

struct bluetooth_adapter_status {
    bool powered = false;
    bool soft_blocked = false;
    bool hard_blocked = false;
    bool controller_disponivel = true;
    std::string show_output;
    std::string rfkill_output;
};

struct bluetooth_state_snapshot {
    bluetooth_adapter_status adapter;
    std::vector<device_bt> conhecidos;
    std::vector<device_bt> pareados;
};

struct command_result {
    bool ok = false;
    int exit_code = -1;
    std::string stdout_output;
    std::string stderr_output;
    std::string mensagem;
};

struct device_audio{
    int id;
    std::string descricao;
    bool padrao;
};

struct audio_result {
    bool ok = false;
    std::string mensagem;
    std::string codigo;
    std::string detalhes;
};

struct DisplayMode {
    int width;
    int height;
    float refresh_rate;
    bool is_current;
};

struct DisplayOutput {
    std::string name;
    bool connected;
    std::vector<DisplayMode> modes;
    DisplayMode current_mode;
    float current_scale;
};

struct display_result {
    bool ok = false;
    std::string mensagem;
    std::string codigo;
    std::string detalhes;
};

// --- Sistema ---
bool comando_existe(const std::string& cmd);
std::string exec_command(const char* cmd);
command_result exec_command_result(const std::string& cmd);
command_result exec_command_args_result(const std::vector<std::string>& args);
long long obter_tempo_ms();
int obter_bateria();

// --- Áudio ---
void aumentar_volume();
void diminuir_volume();
int obter_volume_atual();
void definir_volume(int valor_int);
std::vector<device_audio> listar_dispositivos_audio();
void selecionar_dispositivo_audio(int id);
audio_result selecionar_dispositivo_audio_result(int id);
void imprimir_dispositivos_audio();

// --- Vídeo ---
std::string obter_tipo_sessao();
void verificarSessao();
std::vector<DisplayOutput> obter_info_displays();
void listar_resolucao();
bool alterarResolucao(const std::string &saida, int width, int height, float rate);
display_result alterarResolucao_result(const std::string &saida, int width, int height, float rate);
bool alterarEscala(const std::string &saida, float escala);
display_result alterarEscala_result(const std::string &saida, float escala);
void aumentar_brilho();
void diminuir_brilho();

// --- Wi-Fi ---
void listar_wifi();
std::vector<wifi_network> listar_wifi_parsed();
wifi_adapter_status obter_status_wifi();
wifi_result definir_estado_wifi_result(bool ligar);
void conectar_wifi(const std::string &ssid, const std::string &senha);
wifi_result conectar_wifi_result(const std::string &ssid, const std::string &senha);
void desconectar_wifi(const std::string &id);
wifi_result desconectar_wifi_result(const std::string &id);

// --- Bluetooth ---
bool obter_estado_bluetooth();
bluetooth_adapter_status obter_status_bluetooth();
bluetooth_state_snapshot obter_estado_bluetooth_completo();
bool definir_estado_bt(bool ligar);
bluetooth_result definir_estado_bt_result(bool ligar);
std::vector<device_bt> listar_dispositivos_bluetooth_conhecidos();
std::vector<device_bt> listar_dispositivos_bluetooth_pareados();
std::vector<device_bt> scan_dispositivos_bluetooth(int segundos = 10);
bluetooth_result parear_bluetooth(const std::string &mac);
bluetooth_result confiar_bluetooth(const std::string &mac);
bluetooth_result conectar_bluetooth_result(const std::string &mac);
bool conectar_bluetooth(const std::string &mac);
bluetooth_result desconectar_bluetooth_result(const std::string &mac);
bool desconectar_bluetooth(const std::string &mac);
bluetooth_result remover_bluetooth(const std::string &mac);
void listar_dispositivos_bluetooth(const std::vector<device_bt> &dispositivos);
void gerenciar_bluetooth();
void parsing_bluetooth_stream(
    std::istream &input,
    std::unordered_map<std::string, device_bt> &mapa,
    std::string &ultimo_mac_context
);

#endif
