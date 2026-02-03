#include <iostream>
#include <cstdlib>
#include "functions.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <iomanip>

using namespace std;

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        cout << "Uso:\n";
        cout << argv[0] << " aumentar_volume\n";
        cout << argv[0] << " diminuir_volume\n";
        cout << argv[0] << " definir_volume <valor>\n";
        cout << argv[0] << " aumentar_brilho\n";
        cout << argv[0] << " diminuir_brilho\n";
        cout << argv[0] << " verificar_sessao\n";
        cout << argv[0] << " alterar_resolucao <saida> <modo>\n";
        cout << argv[0] << " listar_resolucao\n";
        cout << argv[0] << " alterar_escala <saida> <escalaX> [escalaY]\n";
        cout << argv[0] << " listar_wifi\n";
        cout << argv[0] << " conectar_wifi <SSID> <senha>\n";
        cout << argv[0] << " desconectar_wifi <SSID>\n";
        cout << argv[0] << " scan_dispositivos_bluetooth <tempo>\n";
        cout << argv[0] << " conectar_bluetooth <MAC>\n";
        cout << argv[0] << " desconectar_bluetooth <MAC>\n";
        cout << argv[0] << " listar_dispositivos_bluetooth\n";
        cout << argv[0] << " gerenciar_bluetooth\n";
        cout << argv[0] << " obter_bateria\n";
        cout << argv[0] << " obter_tempo\n";
        cout << argv[0] << " listar_audio\n";
        cout << argv[0] << " definir_audio <ID>\n";
        cout << argv[0] << " status_bluetooth\n";
        cout << argv[0] << " definir_estado_bt <1|0>\n";

        return 1;
    }

    string_view cmd{argv[1]};

    if (cmd == "aumentar_volume")
    {
        aumentar_volume();
    }
    else if (cmd == "diminuir_volume")
    {
        diminuir_volume();
    }
    else if (cmd == "definir_volume")
    {
        if (argc < 3)
        {
            cout << "Uso: " << argv[0] << " definir_volume <valor>\n";
            return 1;
        }
        int valor = atof(argv[2]);
        definir_volume(valor);
    }
    else if (cmd == "aumentar_brilho")
    {
        aumentar_brilho();
    }
    else if (cmd == "diminuir_brilho")
    {
        diminuir_brilho();
    }
    else if (cmd == "verificar_sessao")
    {
        verificarSessao();
    }
    else if (cmd == "alterar_resolucao")
    {
        if (argc < 3)
        {
            listar_resolucao();
            return 0;
        }
        else if (argc < 4)
        {
            cout << "Uso: " << argv[0] << " alterar_resolucao <saida> <modo>\n";
            return 1;
        }
        string saida = argv[2];
        string modo = argv[3];
        alterarResolucao(saida, modo);
    }
    else if (cmd == "listar_resolucao")
    {
        listar_resolucao();
    }
    else if (cmd == "alterar_escala")
    {
        if (argc < 3)
        {
            cout << "Uso: " << argv[0] << " alterar_escala <saida> <escalaX> [escalaY]\n";
            return 1;
        }
        string saida = argv[2];
        float escalaX = atof(argv[3]);
        float escalaY = -1;
        if (argc >= 5)
        {
            escalaY = atof(argv[4]);
        }
        alterarEscala(saida, escalaX, escalaY);
    }
    else if (cmd == "listar_wifi")
    {
        auto lista = listar_wifi_parsed();

        cout << "[";
        for (size_t i = 0; i < lista.size(); ++i)
        {
            const auto &net = lista[i];
            cout << "{"
                 << "\"ssid\": \"" << net.ssid << "\","
                 << "\"sinal\": " << net.sinal << ","
                 << "\"seguranca\": \"" << net.seguranca << "\","
                 << "\"conectado\": " << (net.em_uso ? "true" : "false")
                 << "}";

            if (i < lista.size() - 1)
                cout << ",";
        }
        cout << "]" << endl;
    }
    else if (cmd == "conectar_wifi")
    {
        if (argc < 4)
        {
            cout << "Uso: " << argv[0] << " conectar_wifi <SSID> <senha>\n";
            return 1;
        }
        string ssid = argv[2];
        string senha = argv[3];
        conectar_wifi(ssid, senha);
    }
    else if (cmd == "desconectar_wifi")
    {
        if (argc < 3)
        {
            cout << "Uso: " << argv[0] << " desconectar_wifi <SSID>\n";
            return 1;
        }
        string id = argv[2];
        desconectar_wifi(id);
    }
    else if (cmd == "scan_dispositivos_bluetooth")
    {
        int tempo_scan = 10;

        if (argc >= 3)
            tempo_scan = std::stoi(argv[2]);

        cerr << "Escaneando dispositivos Bluetooth por "
             << tempo_scan << " segundos...\n";

        auto dispositivos = scan_dispositivos_bluetooth(tempo_scan);

        listar_dispositivos_bluetooth(dispositivos);
    }
    else if (cmd == "conectar_bluetooth")
    {
        if (argc < 2)
        {
            cout << "Uso: " << argv[0] << "conectar_bluetooth <MAC>\n";
        }
        string mac = argv[2];
        conectar_bluetooth(mac);
    }
    else if (cmd == "desconectar_bluetooth")
    {
        if (argc < 2)
        {
            cout << "Uso: " << argv[0] << "desconectar_bluetooth <MAC>";
        }

        string mac = argv[2];
        desconectar_bluetooth(mac);
    }
    else if (cmd == "listar_dispositivos_bluetooth")
    {
        auto dispositivos = scan_dispositivos_bluetooth();
        listar_dispositivos_bluetooth(dispositivos);
    }
    else if (cmd == "gerenciar_bluetooth")
    {
        gerenciar_bluetooth();
    }
    else if (cmd == "status_bluetooth")
    {
        bool estado = obter_estado_bluetooth();
        cout << (estado ? "1" : "0") << endl;
    }
    else if (cmd == "definir_estado_bt")
    {
        if (argc < 3)
        {
            cout << "Uso: " << argv[0] << " definir_estado_bluetooth <1|0>\n";
            cout << "1 = Ligar, 0 = Desligar\n";
            return 1;
        }
        int estado = std::stoi(argv[2]);
        definir_estado_bt(estado == 1);
    }
    else if (cmd == "obter_bateria")
    {
        int bat = obter_bateria();
        cout << bat << endl;
    }
    else if (cmd == "obter_tempo")
    {
        long long tempo = obter_tempo_ms();
        cout << tempo << endl;
    }
    else if (cmd == "listar_audio")
    {
        imprimir_dispositivos_audio();
    }
    else if (cmd == "definir_audio")
    {
        if (argc < 3)
        {
            cout << "Uso: " << argv[0] << " definir_audio <ID>\n";
            return 1;
        }
        int id = std::stoi(argv[2]);
        selecionar_dispositivo_audio(id);
    }
    else
    {
        cout << "Comando desconhecido: " << cmd << "\n";
    }

    return 0;
}
