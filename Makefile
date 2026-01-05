# DACC Station Mono-Repo Makefile

.PHONY: all clean ui pm logs

# Alvo padrão: compila tudo
all: logs pm ui

# Compila o Process Manager (que também cuida dos logs no Makefile atual)
pm:
	@echo "--- Compilando Process Manager e Log Server ---"
	$(MAKE) -C process-manager

# Compila a Interface Gráfica
ui:
	@echo "--- Compilando Interface Gráfica ---"
	$(MAKE) -C ui

# Limpa todos os artefatos
clean:
	@echo "--- Limpando tudo ---"
	$(MAKE) -C process-manager clean
	$(MAKE) -C ui clean

# Alias para facilitar
logs: pm
