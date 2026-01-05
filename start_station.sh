#!/bin/bash

# DACC Station - Script de Inicialização
# Inicia o Servidor de Log, o Process Manager e a Interface Gráfica

echo "[1/3] Iniciando Servidor de Logs (gameman-log)..."
if [ -f "./process-manager/bin/log-server" ]; then
    ./process-manager/bin/log-server &
    PID_LOG=$!
    echo "    Log Server iniciado com PID $PID_LOG"
else
    echo "    AVISO: Executável do Log Server não encontrado (rodando sem log centralizado)."
fi

echo "[2/3] Iniciando Process Manager (gameman-pm)..."
# Certifique-se de que o diretório bin existe e o executável está lá
if [ -f "./process-manager/bin/process-manager" ]; then
    ./process-manager/bin/process-manager &
    PID_PM=$!
    echo "    Process Manager iniciado com PID $PID_PM"
else
    echo "    ERRO: Executável do Process Manager não encontrado!"
    exit 1
fi

# Aguarda um momento para o socket ser criado
sleep 1

echo "[3/3] Iniciando Interface Gráfica (UI)..."
cd ui
if [ -f "./gerenciador" ]; then
    ./gerenciador
else
    echo "    ERRO: Executável da UI não encontrado!"
    # Mata o PM antes de sair
    kill $PID_PM
    exit 1
fi

# Limpeza ao fechar a UI
echo "Encerrando serviços..."
kill $PID_PM
if [ ! -z "$PID_LOG" ]; then
    kill $PID_LOG
fi
echo "DACC Station encerrado."
