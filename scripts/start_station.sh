#!/bin/bash

# DACC Station - Script de Inicialização
# Localização: scripts/start_station.sh

# 1. Identifica a raiz do projeto (independente de onde o script é chamado)
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)
PROJECT_ROOT=$(dirname "$SCRIPT_DIR")

# 2. Muda para a raiz do projeto para garantir que caminhos relativos funcionem
cd "$PROJECT_ROOT"
echo ">>> DACC Station Environment"
echo ">>> Root: $PWD"

# Diretórios de Log
mkdir -p data/logs

echo "[1/3] Iniciando Servidor de Logs (LogServer)..."
if [ -f "./bin/log-server" ]; then
    ./bin/log-server &
    PID_LOG=$!
    echo "    Log Server iniciado com PID $PID_LOG"
    # Pequeno delay para garantir que o socket seja criado antes do PM tentar conectar
    sleep 1 
else
    echo "    ERRO CRÍTICO: Executável do Log Server não encontrado em ./bin/log-server"
    echo "    Execute 'make' na raiz do projeto."
    exit 1
fi

echo "[2/3] Iniciando Process Manager (PM)..."
if [ -f "./bin/process-manager" ]; then
    ./bin/process-manager &
    PID_PM=$!
    echo "    Process Manager iniciado com PID $PID_PM"
else
    echo "    ERRO CRÍTICO: Executável do Process Manager não encontrado!"
    kill $PID_LOG
    exit 1
fi

# Aguarda estabilização
sleep 2

echo "[3/3] Iniciando Interface Gráfica (UI)..."
# A UI espera ser executada do diretório 'ui' para achar assets relativos,
# ou ajustamos o código da UI. O padrão atual do projeto parece ser rodar da raiz 
# ou rodar dentro de ui. O script original fazia 'cd ui'. Vamos manter o comportamento.

if [ -f "./bin/dacc-ui" ]; then
    # A UI atualmente procura assets em "assets/...", se rodarmos do bin ou da raiz,
    # precisamos garantir que ela ache. 
    # O código antigo fazia 'cd ui' e chamava '../bin/dacc-ui'.
    
    cd ui
    ../bin/dacc-ui
    
    # Ao sair da UI...
    cd ..
else
    echo "    ERRO: Executável da UI não encontrado!"
    kill $PID_PM
    kill $PID_LOG
    exit 1
fi

# Limpeza ao fechar a UI
echo "Encerrando serviços..."
kill $PID_PM 2>/dev/null
kill $PID_LOG 2>/dev/null
echo "DACC Station encerrado."