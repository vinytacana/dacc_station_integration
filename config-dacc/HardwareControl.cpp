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


using namespace std;

void aumentar_volume()
{
    pid_t pid = fork();
    if (pid == 0)
    {
        execlp("sh", "sh", "-c", "wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%+", nullptr);
        perror("execlp falhou");
        exit(1);
    }
    else if (pid > 0)
    {
        waitpid(pid, NULL, 0);
        cout << "Volume aumentado com sucesso\n";
    }
    else
    {
        perror("fork falhou");
    }
}

void diminuir_volume()
{
    pid_t pid = fork();
    if (pid == 0)
    {
        execlp("sh", "sh", "-c", "wpctl set-volume @DEFAULT_AUDIO_SINK@ 5%-", nullptr);
        perror("execlp falhou");
        exit(1);
    }
    else if (pid > 0)
    {
        waitpid(pid, NULL, 0);
        cout << "Volume diminuído com sucesso\n";
    }
    else
    {
        perror("fork falhou");
    }
}

void definir_volume(int valor_int)
{

    if (valor_int > 100) valor_int = 100;
    if (valor_int < 0) valor_int = 0;

    float valor_float = static_cast<float>(valor_int) / 100.0f;

    string valor_str = to_string(valor_float);
    std::replace(valor_str.begin(), valor_str.end(), ',', '.'); 
    // --------------------------------------------------

    pid_t pid = fork();
    string comando = "wpctl set-volume @DEFAULT_AUDIO_SINK@ " + valor_str;
    
    if (pid == 0)
    {
        execlp("sh", "sh", "-c", comando.c_str(), nullptr);
        perror("Execlp falhou");
        exit(1);
    }
    else if (pid > 0)
    {
        waitpid(pid, nullptr, 0);
        cout << "[Hardware] Volume definido para " << valor_int << "% (Comando: " << valor_str << ")\n";
    }
    else
    {
        perror("fork falhou");
    }
}

int obter_volume_atual() {
    const char* cmd = "wpctl get-volume @DEFAULT_AUDIO_SINK@";
    std::string saida;
    
    try {
        saida = exec_command(cmd); 
    } catch (...) {
        return 50;
    }

    size_t pos = saida.find("Volume: ");
    if (pos == std::string::npos) return 50;

    std::string volStr = saida.substr(pos + 8); 
    
    try {
        float volFloat = std::stof(volStr);
        return static_cast<int>(volFloat * 100);
    } catch (...) {
        return 50;
    }
}

void aumentar_brilho()
{
    pid_t pid = fork();
    if (pid == 0)
    {
        execlp("sh", "sh", "-c", "brightnessctl set +10%", nullptr);
        perror("execlp falhou");
        exit(1);
    }
    else if (pid > 0)
    {
        waitpid(pid, NULL, 0);
        cout << "Brilho aumentado com sucesso\n";
    }
    else
    {
        perror("fork falhou");
    }
}

void diminuir_brilho()
{
    pid_t pid = fork();
    if (pid == 0)
    {
        execlp("sh", "sh", "-c", "brightnessctl set 10%-", nullptr);
        perror("execlp falhou");
        exit(1);
    }
    else if (pid > 0)
    {
        waitpid(pid, NULL, 0);
        cout << "Brilho diminuído com sucesso\n";
    }
    else
    {
        perror("fork falhou");
    }
}

void verificarSessao()
{
    const char *sessao = getenv("XDG_SESSION_TYPE");
    const char *compositor = getenv("XDG_SESSION_DESKTOP");
    if (!sessao)
    {
        cout << "Não foi possível detectar o tipo de sessão\n";
        return;
    }
    string sessaoStr(sessao);
    cout << "Sessão atual: " << sessaoStr << "\n";
    if (sessaoStr == "wayland")
    {
        cout << " Você está em Wayland. O xrandr pode não funcionar para mudar resolução.\n";
    }
    else if (sessaoStr == "x11")
    {
        cout << "Sessão Xorg detectada. xrandr deve funcionar normalmente.\n";
    }
    else
    {
        cout << "Tipo de sessão não reconhecido\n";
    }

    string comp(compositor);
    cout << "Compositor atual: " << comp << "\n";

    if (comp == "gnome")
    {
        cout << " O compositor gnome nao e compativel com o xrand nem com wlr_randr, mude de compositor para mudar configuracoes de resolucao" << "\n";
    }
    else
    {
        cout << "Compositor nao reconhecido" << "\n";
    }
}
std::string obter_tipo_sessao() {
    const char *sessao = getenv("XDG_SESSION_TYPE");
    if (!sessao) return "unknown";
    return std::string(sessao);
}

std::vector<DisplayOutput> obter_info_displays() {
    std::vector<DisplayOutput> displays;
    std::string sessao = obter_tipo_sessao();
    
    std::string cmd = "xrandr --verbose"; 
    std::string output;
    
    try {
        output = exec_command(cmd.c_str());
    } catch (...) {
        return displays;
    }

    std::stringstream ss(output);
    std::string linha;
    DisplayOutput* currentDisplay = nullptr;

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

        if (currentDisplay && !displays.empty()) {
            if (linha.size() > 2 && linha[0] == ' ' && linha.find("x") != std::string::npos) {

                
                std::stringstream modeSS(linha);
                std::string token;
                
                int w = 0, h = 0;
                bool foundRes = false;
                
                while(modeSS >> token) {
                    size_t xPos = token.find('x');
                    if (xPos != std::string::npos && isdigit(token[0])) {
                        try {
                            w = std::stoi(token.substr(0, xPos));
                            size_t iPos = xPos + 1;
                            std::string hStr;
                            while(iPos < token.size() && isdigit(token[iPos])) {
                                hStr += token[iPos++];
                            }
                            h = std::stoi(hStr);
                            foundRes = true;
                            break;
                        } catch(...) {}
                    }
                }

                if (foundRes) {
                    DisplayMode mode;
                    mode.width = w;
                    mode.height = h;
                    mode.refresh_rate = 60.0f; // Default
                    mode.is_current = (linha.find("*current") != std::string::npos || linha.find("*") != std::string::npos);
                   
                    currentDisplay->modes.push_back(mode);
                    if (mode.is_current) {
                        currentDisplay->current_mode = mode;
                    }
                }
            }
        }
    }
    
    return displays;
}
void listar_resolucao()
{
    cout << "Lista de saidas e resolucoes suportadas: \n";
    pid_t pid = fork();
    if (pid == 0)
    {
        execlp("xrandr", "xrandr", nullptr);
        perror("execlp falhou");
        exit(1);
    }
    else if (pid > 1)
    {
        waitpid(pid, NULL, 0);
    }
    else
    {
        perror("execlp falhou!!");
    }
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
    } 
    else if (sessao == "wayland") {
        const char *desktop = getenv("XDG_SESSION_DESKTOP");
        std::string de = desktop ? std::string(desktop) : "";
        
        if (de.find("gnome") != std::string::npos) {
            int scaleInt = static_cast<int>(escala + 0.5f); 
            comando = "gsettings set org.gnome.desktop.interface scaling-factor " + std::to_string(scaleInt);
        } else {
            comando = "wlr-randr --output " + saida + " --scale " + scaleStr;
        }
    } else {
        return false;
    }

    std::cout << "[Video] Escala: " << comando << std::endl;
    int ret = system(comando.c_str());
    return (ret == 0);
}

bool alterarResolucao(const string &saida, int width, int height, float rate) {
    std::string sessao = obter_tipo_sessao();
    std::string comando;
    std::string modeStr = std::to_string(width) + "x" + std::to_string(height);

    if (sessao == "wayland") {
        comando = "wlr-randr --output " + saida + " --mode " + modeStr;
    } else {
    
        comando = "xrandr --output " + saida + " --mode " + modeStr;
    }

    std::cout << "[Video] Executando: " << comando << std::endl;
    int ret = system(comando.c_str());
    return (ret == 0);
}

void listar_wifi()
{
    cout << "Executando...\n";
    pid_t pid = fork();
    if (pid == 0)
    {
        execlp("nmcli", "nmcli", "device", "wifi", "list", nullptr);
        perror("execlp falhou");
        exit(1);
    }
    else if (pid > 0)
    {
        waitpid(pid, NULL, 0);
    }
    else
    {
        perror("fork falhou");
    }
}

void conectar_wifi(const string &ssid, const string &senha)
{
    std::cerr << "Tentando conectar em: " << ssid << "...\n";
    
    pid_t pid = fork();
    if (pid == 0)
    {
        string comando = "nmcli device wifi connect \"" + ssid + "\" password \"" + senha + "\" > /dev/null 2>&1";
        execlp("sh", "sh", "-c", comando.c_str(), nullptr);
        exit(1);
    }
    else if (pid > 0)
    {
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {

        } else {

            exit(1);
        }
    }
}

void desconectar_wifi(const string &id)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        string comando = "nmcli connection down id \"" + id + "\"";
        execlp("sh", "sh", "-c", comando.c_str(), nullptr);
        perror("execlp falhou");
        exit(1);
    }
    else if (pid > 0)
    {
        waitpid(pid, nullptr, 0);
        std::cerr << "Rede \"" << id << "\" desconectada.\n";
    }
    else
    {
        perror("fork falhou");
    }
}

std::vector<device_bt> scan_dispositivos_bluetooth(int segundos)
{
    std::unordered_map<std::string, device_bt> mapa;

    int master_fd;
    pid_t pid = forkpty(&master_fd, nullptr, nullptr, nullptr);

    if (pid == 0)
    {
        setbuf(stdout, NULL);
        execlp("bluetoothctl", "bluetoothctl", nullptr);
        _exit(1);
    }

    write(master_fd, "power on\n", 9);
    write(master_fd, "agent on\ndefault-agent\n", 24);
    write(master_fd, "scan on\n", 8);

    auto inicio = std::chrono::steady_clock::now();
    char buffer[1024]; 
    
  
    std::string buffer_acumulado; 
    std::string ultimo_mac_ctx = ""; 

    while (true)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(master_fd, &fds);

        struct timeval tv{1, 0};
        int ret = select(master_fd + 1, &fds, nullptr, nullptr, &tv);

        if (ret > 0 && FD_ISSET(master_fd, &fds))
        {
            int n = read(master_fd, buffer, sizeof(buffer) - 1);
            if (n > 0)
            {
                buffer[n] = '\0';
                buffer_acumulado += buffer; 

                size_t pos;
                while ((pos = buffer_acumulado.find('\n')) != std::string::npos)
                {
                    std::string linha_completa = buffer_acumulado.substr(0, pos);
                    std::stringstream ss(linha_completa);
                    
                    parsing_bluetooth_stream(ss, mapa, ultimo_mac_ctx);

                    buffer_acumulado.erase(0, pos + 1);
                }
            }
        }

        auto agora = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(agora - inicio).count() >= segundos)
            break;
    }

    write(master_fd, "scan off\nquit\n", 14);
    close(master_fd);
    waitpid(pid, nullptr, 0);

    std::vector<device_bt> lista;
    for (auto &[_, d] : mapa)
        lista.push_back(d);

    return lista;
}

bool conectar_bluetooth(const string &mac)
{
    string comando = "bluetoothctl connect " + mac;
    int resultado = system(comando.c_str());

    if (resultado == 0)
    {
        cout << "Conectado com sucesso ao dispositivo " << mac << endl;
        return true;
    }
    else
    {
        cerr << "Falha ao conectar ao dispositivo " << mac << endl;
        return false;
    }
}

bool desconectar_bluetooth(const string &mac)
{
    string comando = "bluetoothctl disconnect " + mac;
    int resultado = system(comando.c_str());

    if (resultado == 0)
    {
        cout << "Desconectado com sucesso do dispositivo " << mac << endl;
        return true;
    }
    else
    {
        cerr << "Falha ao desconectar do dispositivo " << mac << endl;
        return false;
    }
}

void listar_dispositivos_bluetooth(const std::vector<device_bt> &dispositivos)
{
    if (dispositivos.empty())
    {
        std::cout << "Nenhum dispositivo encontrado.\n";
        return;
    }

    int i = 1;
    for (const auto &d : dispositivos)
    {
        std::cout << i++ << ") "
                  << d.nome << " - "
                  << d.mac << '\n';
    }
}
void parsing_bluetooth_stream(
    std::istream &input,
    std::unordered_map<std::string, device_bt> &mapa,
    std::string &ultimo_mac_context // Recebe o estado de fora
)
{
    std::string linha;
    while (std::getline(input, linha))
    {
    
        if (linha.find("Device ") != std::string::npos)
        {
            std::stringstream ss(linha);
            std::string device_kw, mac;
            ss >> device_kw >> mac;

            if (device_kw != "Device" || mac.empty())
                continue;

            if (linha.find("[DEL]") != std::string::npos)
            {
                mapa.erase(mac);
                ultimo_mac_context = ""; 
                continue;
            }

            auto &dev = mapa[mac];
            dev.mac = mac;
            ultimo_mac_context = mac; 

          
            std::string resto;
            std::getline(ss, resto);
            size_t first = resto.find_first_not_of(" \t");
            if (first != std::string::npos)
            {
                std::string nome = resto.substr(first);
                if (!nome.empty())
                    dev.nome = nome;
            }
            continue;
        }

       
        if (!ultimo_mac_context.empty() && linha.find("Name:") != std::string::npos)
        {
            auto &dev = mapa[ultimo_mac_context];

            std::string nome = linha.substr(linha.find("Name:") + 5);
            size_t first = nome.find_first_not_of(" \t");
            size_t last  = nome.find_last_not_of(" \t\r\n");

            if (first != std::string::npos && last != std::string::npos)
            {
                nome = nome.substr(first, last - first + 1);
                if (nome.size() >= 2)
                    dev.nome = nome;
            }
        }
    }
}
std::vector<device_bt> get_list_device()
{
    std::unordered_map<std::string, device_bt> mapa;

    int ret = system("bluetoothctl devices > /tmp/bt_list.txt");
    if (ret != 0)
    {
        std::cerr << "Aviso: Falha ao executar bluetoothctl.\n";
    }

    std::ifstream arquivo("/tmp/bt_list.txt");
    if (!arquivo.is_open())
    {
        return {};
    }

    std::string ctx_dummy = "";
    parsing_bluetooth_stream(arquivo, mapa, ctx_dummy);

    std::vector<device_bt> lista;
    for (auto &[_, d] : mapa)
        lista.push_back(d);

    return lista;
}

void gerenciar_bluetooth()
{
    std::cout << "Carregando lista de dispositivos...\n";
    std::vector<device_bt> dispositivos = get_list_device();

    if (dispositivos.empty())
    {
        std::cout << "Nenhum dispositivo pareado encontrado.\n";
        std::cout << "Dica: Use 'scan_dispositivos_bluetooth' para encontrar novos.\n";
        return;
    }

    std::cout << "\n=== Dispositivos Pareados ===\n";
    for (size_t i = 0; i < dispositivos.size(); ++i)
    {
        std::cout << "[" << i + 1 << "] " << dispositivos[i].nome
                  << " \t(" << dispositivos[i].mac << ")\n";
    }
    std::cout << "=============================\n";

    std::cout << "Escolha um dispositivo (0 para sair): ";
    int escolha;
    std::cin >> escolha;

    if (std::cin.fail() || escolha < 0 || escolha > static_cast<int>(dispositivos.size()))
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Opção inválida.\n";
        return;
    }

    if (escolha == 0)
        return;

    const auto &device = dispositivos[escolha - 1];

    std::cout << "\nDispositivo selecionado: " << device.nome << "\n";
    std::cout << "[1] Conectar\n";
    std::cout << "[2] Desconectar\n";
    std::cout << "Opção: ";

    int acao;
    std::cin >> acao;

    if (acao == 1)
    {
        conectar_bluetooth(device.mac);
    }
    else if (acao == 2)
    {
        desconectar_bluetooth(device.mac);
    }
    else
    {
        std::cout << "Ação desconhecida.\n";
    }
}

int obter_bateria()
{
    vector<string> caminhos = {
        "/sys/class/power_supply/BAT0/capacity",
        "/sys/class/power_supply/BAT1/capacity"
    };

    for (const auto &path : caminhos)
    {
        ifstream arquivo(path);
        if (arquivo.is_open())
        {
            int porcentagem;
            arquivo >> porcentagem;
            return porcentagem;
        }
    }

    return -1;
}
long long obter_tempo_ms()
{
    
    auto agora = std::chrono::system_clock::now();
    
    
    auto duracao = agora.time_since_epoch();
    
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(duracao).count();
}

std::string limpar_nome_audio(std::string raw) {
    size_t colchete = raw.find('[');
    if (colchete != std::string::npos) {
        raw = raw.substr(0, colchete);
    }
  
    while (!raw.empty() && isspace(raw.back())) raw.pop_back();
    return raw;
}

std::vector<device_audio> listar_dispositivos_audio()
{
    std::vector<device_audio> lista;
    
    int ret = system("wpctl status > /tmp/audio_list.txt");
    if (ret != 0) return lista;

    std::ifstream arquivo("/tmp/audio_list.txt");
    if (!arquivo.is_open()) return lista;

    std::string linha;
    bool na_secao_sinks = false;

    while (std::getline(arquivo, linha))
    {

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
                while (inicioNum > 0 && isdigit(linha[inicioNum-1])) {
                    inicioNum--;
                }
                
                try {
                    dev.id = std::stoi(linha.substr(inicioNum, pontoPos - inicioNum));

                    if (pontoPos + 2 < linha.size()) {
                        dev.descricao = limpar_nome_audio(linha.substr(pontoPos + 2));
                        lista.push_back(dev);
                    }
                } catch (...) {
                    continue; 
                }
            }
        }
    }
    return lista;
}

void selecionar_dispositivo_audio(int id)
{
    std::string comando = "wpctl set-default " + std::to_string(id);
    
    pid_t pid = fork();
    if (pid == 0)
    {
        execlp("sh", "sh", "-c", comando.c_str(), nullptr);
        perror("execlp falhou");
        exit(1);
    }
    else if (pid > 0)
    {
        waitpid(pid, nullptr, 0);
        std::cout << "Dispositivo de áudio " << id << " definido como padrão.\n";
    }
}

void imprimir_dispositivos_audio()
{
    auto lista = listar_dispositivos_audio();
    if (lista.empty()) {
        std::cout << "Nenhum dispositivo de saída encontrado.\n";
        return;
    }

    std::cout << "=== Dispositivos de Saída ===\n";
    for (const auto& dev : lista) {
        std::cout << (dev.padrao ? "[*] " : "[ ] ") 
                  << "ID: " << dev.id 
                  << " | " << dev.descricao << "\n";
    }
    std::cout << "=============================\n";
}

bool obter_estado_bluetooth()
{
    
    int ret = system("bluetoothctl show > /tmp/bt_state.txt");
    if (ret != 0) return false; 

    std::ifstream arquivo("/tmp/bt_state.txt");
    if (!arquivo.is_open()) return false;

    std::string linha;
    while (std::getline(arquivo, linha))
    {
        
        if (linha.find("Powered: yes") != std::string::npos)
        {
            return true;
        }
    }
    

    return false;
}
void definir_estado_bt(bool ligar)
{
    std::string acao = ligar ? "on" : "off";
    std::string comando = "bluetoothctl power " + acao;

    pid_t pid = fork();
    if (pid == 0)
    {
      
        execlp("sh", "sh", "-c", comando.c_str(), nullptr);
        perror("execlp falhou");
        exit(1);
    }
    else if (pid > 0)
    {
        waitpid(pid, nullptr, 0);
        std::cout << "Bluetooth definido para: " << acao << "\n";
    }
    else
    {
        perror("fork falhou");
    }
}

std::string exec_command(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, int(*)(FILE*)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() falhou!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::vector<wifi_network> listar_wifi_parsed()
{
    std::vector<wifi_network> redes;
    
    const char* cmd = "nmcli -t -f IN-USE,SSID,SIGNAL,SECURITY device wifi list";
    
    std::string saida;
    try {
        saida = exec_command(cmd);
    } catch (...) {
        return redes;
    }

    std::stringstream ss(saida);
    std::string linha;

    while (std::getline(ss, linha))
    {

        std::stringstream ss_linha(linha);
        std::string segmento;
        std::vector<std::string> campos;


        size_t pos = 0;
        std::string token;
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
            try {
                net.sinal = std::stoi(campos[2]);
            } catch (...) {
                net.sinal = 0;
            }
            net.seguranca = campos[3];

            if (!net.ssid.empty()) {
                redes.push_back(net);
            }
        }
    }
    return redes;
}

