# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99

# Pasta para arquivos de exemplo
EXEMPLO_DIR = arquivos

# Comando principal que faz tudo
all: setup compile run

# Cria pasta e arquivos de exemplo
setup:
	@echo "Criando pasta e arquivos de exemplo..."
	mkdir -p $(EXEMPLO_DIR)
	echo "Conteúdo do arquivo underscore" > $(EXEMPLO_DIR)/_.txt
	echo "Conteúdo do arquivo teste com til" > $(EXEMPLO_DIR)/~teste.txt
	echo "Conteúdo do arquivo bye.txt" > $(EXEMPLO_DIR)/bye.txt
	echo "Conteúdo do arquivo bye sem extensão" > $(EXEMPLO_DIR)/bye
	echo "Conteúdo do arquivo test2.doc" > $(EXEMPLO_DIR)/test2.doc
	@echo "Arquivos criados na pasta $(EXEMPLO_DIR)/"

# Compila cliente e servidor
compile: $(cliente) $(servidor)

$(cliente): $(cliente).c
	@echo "Compilando cliente..."
	$(CC) $(CFLAGS) -o $(cliente) $(cliente).c

$(servidor): $(servidor).c	
	@echo "Compilando servidor..."
	$(CC) $(CFLAGS) -o $(servidor) $(servidor).c

# Executa servidor em background e depois o cliente
run:
	@echo "Iniciando servidor..."
	./$(servidor) &
	@echo "Aguardando servidor inicializar..."
	sleep 2
	@echo "Executando cliente..."
	./$(cliente)

# Limpeza
clean:
	rm -f $(cliente) $(servidor)
	rm -rf $(EXEMPLO_DIR)
	@echo "Arquivos limpos"

# Para parar processos do servidor que possam estar rodando
kill-server:
	@echo "Parando processos do servidor..."
	pkill -f "./$(SERVER)" || true

# Comandos individuais (opcionais)
setup-only: setup
compile-only: compile
run-only: run

.PHONY: all setup compile run clean kill-server setup-only compile-only run-only