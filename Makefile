# DACC Station Mono-Repo Makefile

.PHONY: all clean test ipc ui pm logs config

# Alvo padrão: compila tudo
all: ipc logs config pm ui

ipc:
	@echo "--- Compilando IPC compartilhado ---"
	$(MAKE) -C ipc

# Compila o Sistema de Logs (Lib + Server)
logs: ipc
	@echo "--- Compilando Sistema de Logs ---"
	$(MAKE) -C logs

# Compila o Sistema de Configuração (Lib + CLI)
config:
	@echo "--- Compilando Sistema de Configuração ---"
	$(MAKE) -C config-dacc

# Compila o Process Manager (Depende de Logs)
pm: logs ipc
	@echo "--- Compilando Process Manager ---"
	$(MAKE) -C process-manager

# Compila a Interface Gráfica (Depende de Logs e Config)
ui: logs config ipc
	@echo "--- Compilando Interface Gráfica ---"
	$(MAKE) -C ui

# Limpa todos os artefatos
clean:
	@echo "--- Limpando tudo ---"
	$(MAKE) -C ipc clean
	$(MAKE) -C logs clean
	$(MAKE) -C config-dacc clean
	$(MAKE) -C process-manager clean
	$(MAKE) -C ui clean

test:
	$(MAKE) -C config-dacc test
	$(MAKE) -C ipc test
