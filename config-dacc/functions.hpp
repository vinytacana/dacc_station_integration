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

struct device_bt {
    std::string mac;
    std::string nome;
    std::string icon;
    bool conectado = false;
};

struct device_audio{
    int id;
    std::string descricao;
    bool padrao;
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

// --- Sistema ---
std::string exec_command(const char* cmd);
long long obter_tempo_ms();
int obter_bateria();

// --- Áudio ---
void aumentar_volume();
void diminuir_volume();
int obter_volume_atual();
void definir_volume(int valor_int);
std::vector<device_audio> listar_dispositivos_audio();
void selecionar_dispositivo_audio(int id);
void imprimir_dispositivos_audio();

// --- Vídeo ---
std::string obter_tipo_sessao();
void verificarSessao();
std::vector<DisplayOutput> obter_info_displays();
void listar_resolucao();
bool alterarResolucao(const std::string &saida, int width, int height, float rate);
bool alterarEscala(const std::string &saida, float escala);
void aumentar_brilho();
void diminuir_brilho();

// --- Wi-Fi ---
void listar_wifi();
std::vector<wifi_network> listar_wifi_parsed();
void conectar_wifi(const std::string &ssid, const std::string &senha);
void desconectar_wifi(const std::string &id);

// --- Bluetooth ---
bool obter_estado_bluetooth();
void definir_estado_bt(bool ligar);
std::vector<device_bt> get_list_device();
std::vector<device_bt> scan_dispositivos_bluetooth(int segundos = 10);
bool conectar_bluetooth(const std::string &mac);
bool desconectar_bluetooth(const std::string &mac);
void listar_dispositivos_bluetooth(const std::vector<device_bt> &dispositivos);
void gerenciar_bluetooth();
void parsing_bluetooth_stream(
    std::istream &input,
    std::unordered_map<std::string, device_bt> &mapa,
    std::string &ultimo_mac_context
);

#endif