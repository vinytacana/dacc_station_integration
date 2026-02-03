# DACC Station Mono-Repo Makefile

.PHONY: all clean ui pm logs

# Alvo padrão: compila tudo
all: logs config pm ui

# Compila o Sistema de Logs (Lib + Server)
logs:
	@echo "--- Compilando Sistema de Logs ---"
	$(MAKE) -C logs

# Compila o Sistema de Configuração (Lib + CLI)
config:
	@echo "--- Compilando Sistema de Configuração ---"
	$(MAKE) -C config-dacc

# Compila o Process Manager (Depende de Logs)
pm: logs
	@echo "--- Compilando Process Manager ---"
	$(MAKE) -C process-manager

# Compila a Interface Gráfica (Depende de Logs e Config)
ui: logs config
	@echo "--- Compilando Interface Gráfica ---"
	$(MAKE) -C ui

# Limpa todos os artefatos
clean:
	@echo "--- Limpando tudo ---"
	$(MAKE) -C logs clean
	$(MAKE) -C config-dacc clean
	$(MAKE) -C process-manager clean
	$(MAKE) -C ui clean
