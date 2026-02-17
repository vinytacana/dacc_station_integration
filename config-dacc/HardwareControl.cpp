#include "functions.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <limits>
#include <thread>
#include <chrono>
#include <pty.h>
#include <sys/select.h>
#include <unordered_map>
#include <array>  
#include <memory>  
#include <algorithm>
#include <stdexcept> 
#include <cstdio> 
#include <iomanip>
#include <mutex>

using namespace std;

// Mutexes para proteção de concorrência por subsistema
static std::mutex g_bt_mutex;
static std::mutex g_audio_mutex;

// --- UTILS ---

std::string exec_command(const char* cmd) {
    std::array<char, 256> buffer;
    std::string result;
    
    // Abre o pipe de forma segura
    FILE* fp = popen(cmd, "r");
    if (!fp) return "";

    try {
        while (fgets(buffer.data(), buffer.size(), fp) != nullptr) {
            result += buffer.data();
        }
    } catch (...) {
        pclose(fp);
        return "";
    }

    pclose(fp);
    return result;
}

bool comando_existe(const string &cmd) {
    string check = "which " + cmd + " > /dev/null 2>&1";
    return (system(check.c_str()) == 0);
}

// --- AUDIO ---

bool executar_comando_audio(const string &cmd_wp, const string &cmd_pa, const string &cmd_alsa) {
    std::thread([cmd_wp, cmd_pa, cmd_alsa]() {
        std::lock_guard<std::mutex> lock(g_audio_mutex);
        bool sucesso = false;
        if (comando_existe("wpctl")) {
            if (system((cmd_wp + " > /dev/null 2>&1").c_str()) == 0) sucesso = true;
        }
        if (!sucesso && comando_existe("pactl")) {
            if (system((cmd_pa + " > /dev/null 2>&1").c_str()) == 0) sucesso = true;
        }
        if (!sucesso && comando_existe("amixer")) {
            system((cmd_alsa + " > /dev/null 2>&1").c_str());
        }
    }).detach();
    return true; 
}

void aumentar_volume() {
    executar_comando_audio("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%+", "pactl set-sink-volume @DEFAULT_SINK@ +5%", "amixer sset Master 5%+");
}

void diminuir_volume() {
    executar_comando_audio("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%-", "pactl set-sink-volume @DEFAULT_SINK@ -5%", "amixer sset Master 5%-");
}

void definir_volume(int valor_int) {
    if (valor_int > 100) valor_int = 100;
    if (valor_int < 0) valor_int = 0;
    float valor_float = static_cast<float>(valor_int) / 100.0f;
    string v_str = to_string(valor_float);
    std::replace(v_str.begin(), v_str.end(), ',', '.'); 
    string v_perc = to_string(valor_int) + "%";
    executar_comando_audio("wpctl set-volume @DEFAULT_AUDIO_SINK@ " + v_str, "pactl set-sink-volume @DEFAULT_SINK@ " + v_perc, "amixer sset Master " + v_perc);
}

int obter_volume_atual() {
    try {
        std::string saida = exec_command("wpctl get-volume @DEFAULT_AUDIO_SINK@");
        size_t pos = saida.find("Volume: ");
        if (pos != std::string::npos) {
            return static_cast<int>(std::stof(saida.substr(pos + 8)) * 100);
        }
    } catch (...) {}
    return 50;
}

// --- VIDEO / BRILHO ---

void aumentar_brilho() { if (comando_existe("brightnessctl")) system("brightnessctl set +10% > /dev/null 2>&1"); }
void diminuir_brilho() { if (comando_existe("brightnessctl")) system("brightnessctl set 10%- > /dev/null 2>&1"); }

std::vector<DisplayOutput> obter_info_displays() {
    std::vector<DisplayOutput> displays;
    try {
        std::string output = exec_command("xrandr --verbose");
        std::stringstream ss(output);
        std::string linha;
        DisplayOutput* currentDisplay = nullptr;
        while (std::getline(ss, linha)) {
            if (linha.find(" connected ") != std::string::npos) {
                DisplayOutput disp;
                std::stringstream lineSS(linha);
                lineSS >> disp.name;
                disp.connected = true;
                displays.push_back(disp);
                currentDisplay = &displays.back();
            } else if (currentDisplay && linha.size() > 2 && linha[0] == ' ' && linha.find("x") != std::string::npos) {
                DisplayMode mode;
                mode.width = 1920; mode.height = 1080; 
                currentDisplay->modes.push_back(mode);
            }
        }
    } catch (...) {}
    return displays;
}

bool alterarResolucao(const string &saida, int width, int height, float rate) {
    string mode = to_string(width) + "x" + std::to_string(height);
    string cmd = "xrandr --output " + saida + " --mode " + mode;
    return system(cmd.c_str()) == 0;
}

// --- WIFI ---

std::vector<wifi_network> listar_wifi_parsed() {
    std::vector<wifi_network> redes;
    system("nmcli device wifi rescan > /dev/null 2>&1");
    try {
        std::string saida = exec_command("nmcli -t -f IN-USE,SSID,SIGNAL,SECURITY device wifi list");
        std::stringstream ss(saida);
        std::string linha;
        while (std::getline(ss, linha)) {
            std::vector<std::string> campos;
            size_t pos = 0; std::string s = linha;
            while ((pos = s.find(':')) != std::string::npos) { campos.push_back(s.substr(0, pos)); s.erase(0, pos + 1); }
            campos.push_back(s);
            if (campos.size() >= 4) {
                wifi_network net;
                net.em_uso = (campos[0] == "*");
                net.ssid = campos[1];
                try { net.sinal = std::stoi(campos[2]); } catch (...) { net.sinal = 0; }
                net.seguranca = campos[3];
                if (!net.ssid.empty()) redes.push_back(net);
            }
        }
    } catch (...) {}
    return redes;
}

void conectar_wifi(const string &ssid, const string &senha) {
    string cmd = "nmcli device wifi connect \"" + ssid + "\" password \"" + senha + "\" > /dev/null 2>&1";
    system(cmd.c_str());
}

// --- BLUETOOTH (Com Proteção Mutex Exclusiva) ---

std::vector<device_bt> get_list_device() {
    std::lock_guard<std::mutex> lock(g_bt_mutex);
    std::vector<device_bt> lista;
    try {
        std::string todos = exec_command("bluetoothctl devices");
        std::string pareados = exec_command("bluetoothctl paired-devices");
        
        auto processar = [&](const std::string& saida, bool is_p) {
            std::stringstream ss(saida);
            std::string linha;
            while (std::getline(ss, linha)) {
                if (linha.find("Device ") == 0) {
                    size_t pos_mac = 7;
                    size_t pos_nome = linha.find(' ', pos_mac + 1);
                    if (pos_nome != std::string::npos) {
                        std::string mac = linha.substr(pos_mac, pos_nome - pos_mac);
                        std::string nome = linha.substr(pos_nome + 1);
                        if (!nome.empty() && nome.back() == '\r') nome.pop_back();

                        auto it = std::find_if(lista.begin(), lista.end(), [&](const device_bt& d) {
                            return d.mac == mac;
                        });

                        if (it != lista.end()) {
                            if (is_p) it->pareado = true;
                        } else {
                            device_bt d; d.mac = mac; d.nome = nome; d.pareado = is_p; d.conectado = false;
                            lista.push_back(d);
                        }
                    }
                }
            }
        };

        processar(todos, false);
        processar(pareados, true);

    } catch (...) {}
    return lista;
}

std::vector<device_bt> scan_dispositivos_bluetooth(int segundos) {
    {
        std::lock_guard<std::mutex> lock(g_bt_mutex);
        std::string cmd = "bluetoothctl --timeout " + std::to_string(segundos) + " scan on > /dev/null 2>&1";
        system(cmd.c_str());
    }
    return get_list_device();
}

bool conectar_bluetooth(const string &mac) {
    std::lock_guard<std::mutex> lock(g_bt_mutex);
    system(("bluetoothctl trust " + mac + " > /dev/null 2>&1").c_str());
    return system(("bluetoothctl connect " + mac + " > /dev/null 2>&1").c_str()) == 0;
}

bool desconectar_bluetooth(const string &mac) {
    std::lock_guard<std::mutex> lock(g_bt_mutex);
    return system(("bluetoothctl disconnect " + mac + " > /dev/null 2>&1").c_str()) == 0;
}

bool remover_bluetooth(const string &mac) {
    std::lock_guard<std::mutex> lock(g_bt_mutex);
    return system(("bluetoothctl remove " + mac + " > /dev/null 2>&1").c_str()) == 0;
}

void definir_estado_bt(bool ligar) {
    std::lock_guard<std::mutex> lock(g_bt_mutex);
    if (ligar) {
        system("rfkill unblock bluetooth > /dev/null 2>&1");
        system("bluetoothctl power on > /dev/null 2>&1");
        system("bluetoothctl agent on > /dev/null 2>&1");
        system("bluetoothctl default-agent > /dev/null 2>&1");
    } else {
        system("bluetoothctl power off > /dev/null 2>&1");
        system("rfkill block bluetooth > /dev/null 2>&1");
    }
}

bool obter_estado_bluetooth() {
    std::lock_guard<std::mutex> lock(g_bt_mutex);
    try {
        std::string s = exec_command("bluetoothctl show");
        return (s.find("Powered: yes") != std::string::npos);
    } catch (...) { return false; }
}

// --- OUTROS (STUBS) ---
int obter_bateria() { return 100; }
long long obter_tempo_ms() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
void verificarSessao() {}
void listar_resolucao() {}
bool alterarEscala(const string&, float) { return true; }
void listar_wifi() {}
void desconectar_wifi(const string&) {}
void listar_dispositivos_bluetooth(const std::vector<device_bt>&) {}
void gerenciar_bluetooth() {}
void parsing_bluetooth_stream(std::istream&, std::unordered_map<std::string, device_bt>&, std::string&) {}
std::string obter_tipo_sessao() { return "x11"; }
std::vector<device_audio> listar_dispositivos_audio() { return {}; }
void selecionar_dispositivo_audio(int) {}
void imprimir_dispositivos_audio() {}
