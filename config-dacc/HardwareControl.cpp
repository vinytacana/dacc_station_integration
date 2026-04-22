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
#include <cerrno>
#include <cstring>

using namespace std;

// Mutex para proteger concorrência
static std::mutex g_audio_mutex;
static std::mutex g_bluetooth_mutex;

std::string remover_ansi(std::string str);

namespace {

bluetooth_result executar_bluetoothctl(const std::string &comando) {
    std::lock_guard<std::mutex> lock(g_bluetooth_mutex);
    bluetooth_result resultado;
    int master_fd;
    pid_t pid = forkpty(&master_fd, nullptr, nullptr, nullptr);
    if (pid < 0) {
        resultado.ok = false;
        resultado.mensagem = "forkpty() falhou: " + std::string(std::strerror(errno));
        return resultado;
    }

    if (pid == 0) {
        execlp("bluetoothctl", "bluetoothctl", nullptr);
        _exit(1);
    }

    const std::string entrada = comando + "\nquit\n";
    ssize_t escritos = write(master_fd, entrada.c_str(), entrada.size());
    if (escritos < 0) {
        resultado.ok = false;
        resultado.mensagem = "Falha ao escrever no bluetoothctl.";
        close(master_fd);
        waitpid(pid, nullptr, 0);
        return resultado;
    }

    char buffer[1024];
    std::string saida;

    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(master_fd, &fds);
        struct timeval tv{1, 0};

        int ret = select(master_fd + 1, &fds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(master_fd, &fds)) {
            ssize_t lidos = read(master_fd, buffer, sizeof(buffer) - 1);
            if (lidos > 0) {
                buffer[lidos] = '\0';
                saida += buffer;
                continue;
            }
            if (lidos == 0) {
                break;
            }
            if (errno == EIO) {
                break;
            }
        } else if (ret == 0) {
            int status = 0;
            pid_t waited = waitpid(pid, &status, WNOHANG);
            if (waited == pid) {
                break;
            }
        } else if (ret < 0) {
            resultado.ok = false;
            resultado.mensagem = "Falha ao ler resposta do bluetoothctl.";
            close(master_fd);
            waitpid(pid, nullptr, 0);
            return resultado;
        }
    }

    close(master_fd);
    int status = 0;
    waitpid(pid, &status, 0);
    resultado.ok = WIFEXITED(status) && WEXITSTATUS(status) == 0;
    resultado.mensagem = remover_ansi(saida);
    return resultado;
}

bool bluetoothctl_saida_indica_sucesso(const std::string &saida) {
    return saida.find("Failed") == std::string::npos &&
           saida.find("not available") == std::string::npos &&
           saida.find("No default controller available") == std::string::npos;
}

std::string resumir_bluetooth_status(const bluetooth_adapter_status &status) {
    std::ostringstream oss;
    oss << "powered=" << (status.powered ? "yes" : "no")
        << ", soft_blocked=" << (status.soft_blocked ? "yes" : "no")
        << ", hard_blocked=" << (status.hard_blocked ? "yes" : "no")
        << ", controller_disponivel=" << (status.controller_disponivel ? "yes" : "no");
    return oss.str();
}

device_bt consultar_dispositivo_bluetooth(const std::string &mac) {
    device_bt dispositivo;
    dispositivo.mac = mac;

    std::unordered_map<std::string, device_bt> mapa;
    mapa[mac] = dispositivo;

    bluetooth_result resultado = executar_bluetoothctl("info " + mac);
    if (!resultado.ok || resultado.mensagem.empty()) {
        return dispositivo;
    }

    std::stringstream ss_info(resultado.mensagem);
    std::string ctx_info = mac;
    parsing_bluetooth_stream(ss_info, mapa, ctx_info);
    return mapa[mac];
}

void mesclar_dispositivo_bluetooth(device_bt &destino, const device_bt &origem) {
    if (!origem.mac.empty()) destino.mac = origem.mac;
    if (!origem.nome.empty()) destino.nome = origem.nome;
    if (!origem.icon.empty()) destino.icon = origem.icon;
    destino.conectado = origem.conectado;
    destino.pareado = origem.pareado;
    destino.confiavel = origem.confiavel;
}

} // namespace

// ==========================================
// --- UTILITÁRIOS ---
// ==========================================

std::string remover_ansi(std::string str) {
    std::string resultado;
    bool dentro = false;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '\033') {
            dentro = true;
            continue;
        }
        if (dentro) {
            if (isalpha(str[i])) dentro = false;
            continue;
        }
        resultado += str[i];
    }
    return resultado;
}

bool comando_existe(const std::string &cmd) {
    std::string check = "which " + cmd + " > /dev/null 2>&1";
    return (system(check.c_str()) == 0);
}

std::string exec_command(const char *cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, int (*)(FILE *)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() falhou!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

long long obter_tempo_ms() {
    auto agora = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(agora.time_since_epoch()).count();
}

int obter_bateria() {
    vector<string> caminhos = {
        "/sys/class/power_supply/BAT0/capacity",
        "/sys/class/power_supply/BAT1/capacity"
    };
    for (const auto &path : caminhos) {
        ifstream arquivo(path);
        if (arquivo.is_open()) {
            int porcentagem;
            arquivo >> porcentagem;
            return porcentagem;
        }
    }
    return -1;
}

// ==========================================
// --- ÁUDIO ---
// ==========================================

std::string limpar_nome_audio(std::string raw) {
    size_t colchete = raw.find('[');
    if (colchete != std::string::npos) {
        raw = raw.substr(0, colchete);
    }
    while (!raw.empty() && isspace(raw.back())) {
        raw.pop_back();
    }
    return raw;
}

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
    executar_comando_audio("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%+",
                           "pactl set-sink-volume @DEFAULT_SINK@ +5%",
                           "amixer sset Master 5%+");
}

void diminuir_volume() {
    executar_comando_audio("wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%-",
                           "pactl set-sink-volume @DEFAULT_SINK@ -5%",
                           "amixer sset Master 5%-");
}

void definir_volume(int valor_int) {
    if (valor_int > 100) valor_int = 100;
    if (valor_int < 0) valor_int = 0;
    float valor_float = static_cast<float>(valor_int) / 100.0f;
    string v_str = to_string(valor_float);
    std::replace(v_str.begin(), v_str.end(), ',', '.');
    string v_perc = to_string(valor_int) + "%";

    executar_comando_audio("wpctl set-volume @DEFAULT_AUDIO_SINK@ " + v_str,
                           "pactl set-sink-volume @DEFAULT_SINK@ " + v_perc,
                           "amixer sset Master " + v_perc);
}

int obter_volume_atual() {
    try {
        std::string saida = exec_command("wpctl get-volume @DEFAULT_AUDIO_SINK@");
        size_t pos = saida.find("Volume: ");
        if (pos != std::string::npos) {
            std::string volStr = saida.substr(pos + 8);
            return static_cast<int>(std::stof(volStr) * 100);
        }
    } catch (...) {}
    return 50;
}

std::vector<device_audio> listar_dispositivos_audio() {
    std::vector<device_audio> lista;
    std::string saida;
    try {
        saida = exec_command("wpctl status");
    } catch (...) {
        return lista;
    }

    std::stringstream ss(saida);
    std::string linha;
    bool na_secao_sinks = false;

    while (std::getline(ss, linha)) {
        if (linha.find("Sinks:") != std::string::npos) {
            na_secao_sinks = true;
            continue;
        }
        if (na_secao_sinks && (linha.find("Sources:") != std::string::npos || linha.find("Filters:") != std::string::npos || linha.empty())) {
            break;
        }
        if (na_secao_sinks) {
            size_t pontoPos = linha.find('.');
            if (pontoPos != std::string::npos && pontoPos > 0 && isdigit(linha[pontoPos-1])) {
                device_audio dev;
                dev.padrao = (linha.find('*') != std::string::npos);
                size_t inicioNum = pontoPos - 1;
                while (inicioNum > 0 && isdigit(linha[inicioNum-1])) inicioNum--;
                
                try {
                    dev.id = std::stoi(linha.substr(inicioNum, pontoPos - inicioNum));
                    if (pontoPos + 2 < linha.size()) {
                        dev.descricao = limpar_nome_audio(linha.substr(pontoPos + 2));
                        lista.push_back(dev);
                    }
                } catch (...) { continue; }
            }
        }
    }
    return lista;
}

void selecionar_dispositivo_audio(int id) {
    std::string comando = "wpctl set-default " + std::to_string(id);
    system((comando + " > /dev/null 2>&1").c_str());
    std::cout << "[Áudio] Dispositivo " << id << " definido como padrão.\n";
}

void imprimir_dispositivos_audio() {
    auto lista = listar_dispositivos_audio();
    if (lista.empty()) {
        std::cout << "Nenhum dispositivo de saída encontrado.\n";
        return;
    }
    std::cout << "=== Dispositivos de Saída ===\n";
    for (const auto& dev : lista) {
        std::cout << (dev.padrao ? "[*] " : "[ ] ") << "ID: " << dev.id << " | " << dev.descricao << "\n";
    }
    std::cout << "=============================\n";
}

// ==========================================
// --- VÍDEO ---
// ==========================================

std::string obter_tipo_sessao() {
    const char *sessao = getenv("XDG_SESSION_TYPE");
    return sessao ? std::string(sessao) : "unknown";
}

void verificarSessao() {
    std::string sessaoStr = obter_tipo_sessao();
    const char *compositor = getenv("XDG_SESSION_DESKTOP");
    string comp = compositor ? std::string(compositor) : "unknown";

    cout << "Sessão atual: " << sessaoStr << "\n";
    if (sessaoStr == "wayland") cout << " Você está em Wayland.\n";
    else if (sessaoStr == "x11") cout << "Sessão Xorg detectada.\n";
    
    cout << "Compositor atual: " << comp << "\n";
    if (comp == "gnome") cout << " O compositor GNOME detectado.\n";
}

std::vector<DisplayOutput> obter_info_displays() {
    std::vector<DisplayOutput> displays;
    std::string output;
    try {
        output = exec_command("xrandr --verbose");
    } catch (...) { return displays; }

    std::stringstream ss(output);
    std::string linha;
    DisplayOutput *currentDisplay = nullptr;

    while (std::getline(ss, linha)) {
        if (linha.find(" connected ") != std::string::npos) {
            DisplayOutput disp;
            std::stringstream lineSS(linha);
            lineSS >> disp.name;
            disp.connected = true;
            disp.current_scale = 1.0f;
            displays.push_back(disp);
            currentDisplay = &displays.back();
            continue;
        }

        if (currentDisplay && !displays.empty() && linha.size() > 2 && linha[0] == ' ' && linha.find("x") != std::string::npos) {
            std::stringstream modeSS(linha);
            std::string token;
            int w = 0, h = 0;
            bool foundRes = false;

            while (modeSS >> token) {
                size_t xPos = token.find('x');
                if (xPos != std::string::npos && isdigit(token[0])) {
                    try {
                        w = std::stoi(token.substr(0, xPos));
                        size_t iPos = xPos + 1;
                        std::string hStr;
                        while (iPos < token.size() && isdigit(token[iPos])) hStr += token[iPos++];
                        h = std::stoi(hStr);
                        foundRes = true;
                        break;
                    } catch (...) {}
                }
            }

            if (foundRes) {
                DisplayMode mode;
                mode.width = w;
                mode.height = h;
                mode.refresh_rate = 60.0f;
                mode.is_current = (linha.find("*current") != std::string::npos || linha.find("*") != std::string::npos);
                currentDisplay->modes.push_back(mode);
                if (mode.is_current) currentDisplay->current_mode = mode;
            }
        }
    }
    return displays;
}

void listar_resolucao() {
    cout << "Lista de saidas e resolucoes suportadas: \n";
    system("xrandr");
}

bool alterarEscala(const string &saida, float escala) {
    std::string sessao = obter_tipo_sessao();
    std::string comando;

    if (escala < 0.5f) escala = 0.5f;
    if (escala > 3.0f) escala = 3.0f;

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << escala;
    std::string scaleStr = ss.str();

    if (sessao == "x11") {
        comando = "xrandr --output " + saida + " --scale " + scaleStr + "x" + scaleStr;
    } else if (sessao == "wayland") {
        const char *desktop = getenv("XDG_SESSION_DESKTOP");
        std::string de = desktop ? std::string(desktop) : "";
        if (de.find("gnome") != std::string::npos) {
            int scaleInt = static_cast<int>(escala + 0.5f);
            comando = "gsettings set org.gnome.desktop.interface scaling-factor " + std::to_string(scaleInt);
        } else {
            comando = "wlr-randr --output " + saida + " --scale " + scaleStr;
        }
    } else return false;

    std::cout << "[Video] Escala: " << comando << std::endl;
    return (system(comando.c_str()) == 0);
}

bool alterarResolucao(const string &saida, int width, int height, float rate) {
    std::string sessao = obter_tipo_sessao();
    std::string modeStr = std::to_string(width) + "x" + std::to_string(height);
    std::string comando = (sessao == "wayland") ? 
                          "wlr-randr --output " + saida + " --mode " + modeStr : 
                          "xrandr --output " + saida + " --mode " + modeStr;

    std::cout << "[Video] Executando: " << comando << std::endl;
    return (system(comando.c_str()) == 0);
}

void aumentar_brilho() {
    system("brightnessctl set +10% > /dev/null 2>&1");
    cout << "Brilho aumentado\n";
}

void diminuir_brilho() {
    system("brightnessctl set 10%- > /dev/null 2>&1");
    cout << "Brilho diminuído\n";
}

// ==========================================
// --- WI-FI ---
// ==========================================

void listar_wifi() {
    system("nmcli device wifi list");
}

std::vector<wifi_network> listar_wifi_parsed() {
    std::vector<wifi_network> redes;
    std::string saida;
    try {
        saida = exec_command("nmcli -t -f IN-USE,SSID,SIGNAL,SECURITY device wifi list");
    } catch (...) { return redes; }

    std::stringstream ss(saida);
    std::string linha;

    while (std::getline(ss, linha)) {
        std::vector<std::string> campos;
        size_t pos = 0;
        std::string s = linha;
        while ((pos = s.find(':')) != std::string::npos) {
            campos.push_back(s.substr(0, pos));
            s.erase(0, pos + 1);
        }
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
    return redes;
}

void conectar_wifi(const string &ssid, const string &senha) {
    std::cerr << "Tentando conectar em: " << ssid << "...\n";
    string comando = "nmcli device wifi connect \"" + ssid + "\" password \"" + senha + "\" > /dev/null 2>&1";
    // Executa em thread para não travar a UI
    std::thread([comando]() { system(comando.c_str()); }).detach();
}

void desconectar_wifi(const string &id) {
    string comando = "nmcli connection down id \"" + id + "\" > /dev/null 2>&1";
    system(comando.c_str());
    std::cerr << "Rede \"" << id << "\" desconectada.\n";
}

// ==========================================
// --- BLUETOOTH ---
// ==========================================

void parsing_bluetooth_stream(std::istream &input, std::unordered_map<std::string, device_bt> &mapa, std::string &ultimo_mac_context) {
    std::string linha_raw;
    while (std::getline(input, linha_raw)) {
        std::string linha = remover_ansi(linha_raw);
        if (linha.empty()) continue;
        
        // 1. Linha com identificador de dispositivo
        size_t pos_device = linha.find("Device ");
        if (pos_device != std::string::npos) {
            std::string sub = linha.substr(pos_device + 7);
            std::stringstream ss(sub);
            std::string mac;
            ss >> mac;

            if (mac.length() >= 17 && mac.find(':') != std::string::npos) {
                ultimo_mac_context = mac;
                auto &dev = mapa[mac];
                dev.mac = mac;

                if (linha.find("[DEL]") != std::string::npos) {
                    mapa.erase(mac);
                    ultimo_mac_context = "";
                    continue;
                }

                // Tenta extrair o restante da linha para ver se é um nome ou propriedade
                std::string resto;
                std::getline(ss, resto);
                size_t first = resto.find_first_not_of(" \t");
                if (first != std::string::npos) {
                    std::string payload = resto.substr(first);
                    size_t pos_colon = payload.find(": ");
                    
                    // Se não tem ": ", é o nome do dispositivo (formato [NEW] Device MAC Name)
                    if (pos_colon == std::string::npos) {
                        while(!payload.empty() && isspace(payload.back())) payload.pop_back();
                        if (!payload.empty()) dev.nome = payload;
                    } else {
                        // É uma propriedade na mesma linha (formato [CHG] Device MAC Prop: Val)
                        std::string prop = payload.substr(0, pos_colon);
                        std::string val = payload.substr(pos_colon + 2);
                        while(!val.empty() && isspace(val.back())) val.pop_back();

                        if (prop == "Name" || prop == "Alias") dev.nome = val;
                        else if (prop == "Icon") dev.icon = val;
                        else if (prop == "Connected") dev.conectado = (val == "yes");
                        else if (prop == "Paired") dev.pareado = (val == "yes");
                        else if (prop == "Trusted") dev.confiavel = (val == "yes");
                    }
                }
                continue;
            }
        }

        // 2. Linha de propriedade isolada (comum no output do 'info' ou atualizações de scan)
        if (!ultimo_mac_context.empty()) {
            auto &dev = mapa[ultimo_mac_context];
            size_t pos_colon = linha.find(": ");
            if (pos_colon != std::string::npos) {
                std::string prop_part = linha.substr(0, pos_colon);
                size_t start = prop_part.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    std::string prop = prop_part.substr(start);
                    std::string val = linha.substr(pos_colon + 2);
                    while(!val.empty() && isspace(val.back())) val.pop_back();

                    if (prop == "Name" || prop == "Alias") {
                        if (!val.empty()) dev.nome = val;
                    }
                    else if (prop == "Icon") dev.icon = val;
                    else if (prop == "Connected") dev.conectado = (val == "yes");
                    else if (prop == "Paired") dev.pareado = (val == "yes");
                    else if (prop == "Trusted") dev.confiavel = (val == "yes");
                }
            }
        }
    }
}

bluetooth_adapter_status obter_status_bluetooth() {
    bluetooth_adapter_status status;

    bluetooth_result show_result = executar_bluetoothctl("show");
    status.show_output = show_result.mensagem;
    if (!show_result.ok ||
        status.show_output.find("No default controller available") != std::string::npos) {
        status.controller_disponivel = false;
    }

    if (status.show_output.find("Powered: yes") != std::string::npos) {
        status.powered = true;
    }

    try {
        status.rfkill_output = exec_command("rfkill list bluetooth 2>/dev/null");
        status.soft_blocked = status.rfkill_output.find("Soft blocked: yes") != std::string::npos;
        status.hard_blocked = status.rfkill_output.find("Hard blocked: yes") != std::string::npos;
    } catch (...) {
    }

    return status;
}

bool obter_estado_bluetooth() {
    return obter_status_bluetooth().powered;
}

bool definir_estado_bt(bool ligar) {
    std::string acao = ligar ? "on" : "off";

    if (ligar) {
        system("rfkill unblock bluetooth > /dev/null 2>&1");
    }

    bluetooth_result resultado = executar_bluetoothctl("power " + acao);
    if (!resultado.ok || !bluetoothctl_saida_indica_sucesso(resultado.mensagem)) {
        std::cerr << "Falha ao definir Bluetooth para: " << acao
                  << " | bluetoothctl: " << resultado.mensagem << "\n";
        return false;
    }

    bluetooth_adapter_status status_final;
    bool refletiu_estado = false;
    for (int tentativa = 0; tentativa < 10; ++tentativa) {
        status_final = obter_status_bluetooth();
        if (status_final.powered == ligar) {
            refletiu_estado = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!refletiu_estado) {
        std::cerr << "Bluetooth nao refletiu o estado solicitado: " << acao
                  << " | status: " << resumir_bluetooth_status(status_final)
                  << " | bluetoothctl: " << resultado.mensagem << "\n";
        return false;
    }

    std::cout << "Bluetooth definido para: " << acao << "\n";
    return true;
}

std::vector<device_bt> listar_dispositivos_bluetooth_conhecidos() {
    std::unordered_map<std::string, device_bt> mapa;
    bluetooth_result resultado = executar_bluetoothctl("devices");
    if (!resultado.ok) {
        return {};
    }

    std::string saida = resultado.mensagem;
    std::stringstream ss(saida);
    std::string ctx_dummy = "";
    parsing_bluetooth_stream(ss, mapa, ctx_dummy);

    for (auto &pair : mapa) {
        device_bt enriquecido = consultar_dispositivo_bluetooth(pair.first);
        mesclar_dispositivo_bluetooth(pair.second, enriquecido);
        if (pair.second.nome.empty()) pair.second.nome = "Dispositivo " + pair.first;
    }

    std::vector<device_bt> lista;
    for (auto &[_, d] : mapa) {
        lista.push_back(d);
    }
    return lista;
}

std::vector<device_bt> listar_dispositivos_bluetooth_pareados() {
    std::vector<device_bt> conhecidos = listar_dispositivos_bluetooth_conhecidos();
    std::vector<device_bt> pareados;
    for (const auto &dispositivo : conhecidos) {
        if (dispositivo.pareado) {
            pareados.push_back(dispositivo);
        }
    }
    return pareados;
}

std::vector<device_bt> scan_dispositivos_bluetooth(int segundos) {
    std::lock_guard<std::mutex> lock(g_bluetooth_mutex);
    std::unordered_map<std::string, device_bt> mapa;
    int master_fd;
    pid_t pid = forkpty(&master_fd, nullptr, nullptr, nullptr);
    if (pid < 0) {
        return {};
    }

    if (pid == 0) {
        setbuf(stdout, NULL);
        execlp("bluetoothctl", "bluetoothctl", nullptr);
        _exit(1);
    }

    system("rfkill unblock bluetooth > /dev/null 2>&1");
    write(master_fd, "power on\n", 9);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    write(master_fd, "agent on\ndefault-agent\n", 24);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    write(master_fd, "scan on\n", 8);

    auto inicio = std::chrono::steady_clock::now();
    char buffer[1024];
    std::string buffer_acumulado;
    std::string ultimo_mac_ctx = "";

    while (true) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(master_fd, &fds);
        struct timeval tv{1, 0};
        
        int ret = select(master_fd + 1, &fds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(master_fd, &fds)) {
            int n = read(master_fd, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                buffer_acumulado += buffer;

                size_t pos;
                while ((pos = buffer_acumulado.find('\n')) != std::string::npos) {
                    std::string linha_completa = buffer_acumulado.substr(0, pos);
                    std::stringstream ss(linha_completa);
                    parsing_bluetooth_stream(ss, mapa, ultimo_mac_ctx);
                    buffer_acumulado.erase(0, pos + 1);
                }
            }
        }

        auto agora = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(agora - inicio).count() >= segundos) break;
    }

    write(master_fd, "scan off\nquit\n", 14);
    close(master_fd);
    waitpid(pid, nullptr, 0);

    std::vector<device_bt> lista;
    for (auto &[_, d] : mapa) lista.push_back(d);
    return lista;
}

bool conectar_bluetooth(const string &mac) {
    bluetooth_result resultado = executar_bluetoothctl("connect " + mac);
    if (resultado.ok && bluetoothctl_saida_indica_sucesso(resultado.mensagem)) {
        device_bt dispositivo = consultar_dispositivo_bluetooth(mac);
        if (dispositivo.conectado) {
            cout << "Conectado com sucesso: " << mac << endl;
            return true;
        }
    }
    cerr << "Falha ao conectar: " << mac << endl;
    return false;
}

bool desconectar_bluetooth(const string &mac) {
    bluetooth_result resultado = executar_bluetoothctl("disconnect " + mac);
    if (resultado.ok && bluetoothctl_saida_indica_sucesso(resultado.mensagem)) {
        device_bt dispositivo = consultar_dispositivo_bluetooth(mac);
        if (!dispositivo.conectado) {
            cout << "Desconectado com sucesso: " << mac << endl;
            return true;
        }
    }
    cerr << "Falha ao desconectar: " << mac << endl;
    return false;
}

bluetooth_result parear_bluetooth(const std::string &mac) {
    system("rfkill unblock bluetooth > /dev/null 2>&1");
    bluetooth_result resultado = executar_bluetoothctl("pair " + mac);
    if (!resultado.ok || !bluetoothctl_saida_indica_sucesso(resultado.mensagem)) {
        resultado.ok = false;
        if (resultado.mensagem.empty()) resultado.mensagem = "Falha ao parear dispositivo.";
        return resultado;
    }

    device_bt dispositivo = consultar_dispositivo_bluetooth(mac);
    if (!dispositivo.pareado) {
        return {false, "Comando executado, mas o dispositivo nao apareceu como pareado."};
    }
    return {true, "Dispositivo pareado com sucesso."};
}

bluetooth_result confiar_bluetooth(const std::string &mac) {
    bluetooth_result resultado = executar_bluetoothctl("trust " + mac);
    if (!resultado.ok || !bluetoothctl_saida_indica_sucesso(resultado.mensagem)) {
        resultado.ok = false;
        if (resultado.mensagem.empty()) resultado.mensagem = "Falha ao confiar no dispositivo.";
        return resultado;
    }

    device_bt dispositivo = consultar_dispositivo_bluetooth(mac);
    if (!dispositivo.confiavel) {
        return {false, "Comando executado, mas o dispositivo nao apareceu como confiavel."};
    }
    return {true, "Dispositivo marcado como confiavel."};
}

bluetooth_result remover_bluetooth(const std::string &mac) {
    bluetooth_result resultado = executar_bluetoothctl("remove " + mac);
    if (!resultado.ok || !bluetoothctl_saida_indica_sucesso(resultado.mensagem)) {
        resultado.ok = false;
        if (resultado.mensagem.empty()) resultado.mensagem = "Falha ao remover dispositivo.";
        return resultado;
    }

    device_bt dispositivo = consultar_dispositivo_bluetooth(mac);
    if (dispositivo.pareado || dispositivo.confiavel || !dispositivo.nome.empty()) {
        return {false, "Comando executado, mas o dispositivo ainda aparece como conhecido."};
    }
    return {true, "Dispositivo removido com sucesso."};
}

void listar_dispositivos_bluetooth(const std::vector<device_bt> &dispositivos) {
    if (dispositivos.empty()) {
        std::cout << "Nenhum dispositivo encontrado.\n";
        return;
    }
    int i = 1;
    for (const auto &d : dispositivos) {
        std::cout << i++ << ") " << d.nome << " - " << d.mac << '\n';
    }
}

void gerenciar_bluetooth() {
    std::cout << "Carregando lista de dispositivos...\n";
    std::vector<device_bt> dispositivos = listar_dispositivos_bluetooth_pareados();

    if (dispositivos.empty()) {
        std::cout << "Nenhum dispositivo pareado encontrado.\n";
        return;
    }

    std::cout << "\n=== Dispositivos Pareados ===\n";
    for (size_t i = 0; i < dispositivos.size(); ++i) {
        std::cout << "[" << i + 1 << "] " << dispositivos[i].nome << " \t(" << dispositivos[i].mac << ")\n";
    }
    std::cout << "=============================\n";
    std::cout << "Escolha um dispositivo (0 para sair): ";
    
    int escolha;
    std::cin >> escolha;

    if (std::cin.fail() || escolha <= 0 || escolha > static_cast<int>(dispositivos.size())) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    const auto &device = dispositivos[escolha - 1];
    std::cout << "\nDispositivo selecionado: " << device.nome << "\n";
    std::cout << "[1] Conectar\n[2] Desconectar\nOpção: ";

    int acao;
    std::cin >> acao;
    if (acao == 1) conectar_bluetooth(device.mac);
    else if (acao == 2) desconectar_bluetooth(device.mac);
}
