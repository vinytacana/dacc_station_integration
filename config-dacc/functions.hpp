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

// Sistema
std::string exec_command(const char* cmd);
void aumentar_volume();
void diminuir_volume();
int obter_volume_atual();
void definir_volume(int valor_int);

//Vídeo
std::vector<DisplayOutput> obter_info_displays();
void aumentar_brilho();
void diminuir_brilho();
void verificarSessao();
void alterarResolucao(const std::string &saida, const std::string &modo);
void listar_resolucao();
void alterarEscala(const std::string &saida, float escalaX, float escalaY = -1);

// Wi-Fi
void listar_wifi();
std::vector<wifi_network> listar_wifi_parsed();
void conectar_wifi(const std::string &ssid, const std::string &senha);
void desconectar_wifi(const std::string &id);

// Bluetooth
std::vector<device_bt> scan_dispositivos_bluetooth(int segundos = 10);
bool conectar_bluetooth(const std::string &mac);
bool desconectar_bluetooth(const std::string &mac);
void listar_dispositivos_bluetooth(const std::vector<device_bt> &dispositivos);
void gerenciar_bluetooth();
bool obter_estado_bluetooth();
void definir_estado_bt(bool ligar);

// Parsing
void parsing_bluetooth_stream(
    std::istream &input,
    std::unordered_map<std::string, device_bt> &mapa,
    std::string &ultimo_mac_context
);
int obter_bateria();
long long obter_tempo_ms();

std::vector<device_audio> listar_dispositivos_audio();
void selecionar_dispositivo_audio(int id);
void imprimir_dispositivos_audio();
#endif
