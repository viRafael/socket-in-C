#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/time.h>

// Constantes
const int PORTA = 8489;
const char *IP = "127.0.0.1";
const char *IP6 = "::1";
const int NUM_MAX_CLIENTES = 5;
const int BUFFER_SIZE = 66000;

//Funções auxiliares
bool isCharDestuffing(const char *mensagem) {
    if (strncmp(mensagem, "~", 1) == 0) {
        return true;
    }
    return false;
}

char *charDestuffing(const char *mensagem) {
    char *novaMensagem = NULL;
    const char *prefixo = "~";
    size_t tamanhoPrefixo = strlen(prefixo);
    size_t tamanhoOriginal = strlen(mensagem);

    size_t tamanhoSemPrefixo = tamanhoOriginal - tamanhoPrefixo;
    novaMensagem = (char *)malloc(tamanhoSemPrefixo + 1);

    if (novaMensagem == NULL) {
        perror("Erro ao alocar memoria em charDestuffing (com prefixo)");
        return NULL;
    }

    strcpy(novaMensagem, mensagem + tamanhoPrefixo);
    novaMensagem[tamanhoSemPrefixo] = '\0';

    return novaMensagem;
}

void enviarMensagem(int socket, char *buffer, char *mensagem) {
    bzero(buffer, BUFFER_SIZE);
    strcpy(buffer, mensagem);
    printf("Enviando mensagem: %s\n", buffer);
    send(socket, buffer, strlen(buffer), 0);
}

long receberMensagemComTamanho(int socket, char *buffer) {
    bzero(buffer, BUFFER_SIZE);
    long bytesRecebidos = recv(socket, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytesRecebidos > 0) {
        buffer[bytesRecebidos] = '\0'; // Garantir terminação
        printf("Mensagem recebida (%ld bytes): ", bytesRecebidos);
        
        // Mostrar apenas os primeiros 20 caracteres
        if (bytesRecebidos > 20) {
            printf("%.20s... [CORTADO]\n", buffer);
        } else {
            printf("%s\n", buffer);
        }
    }
    
    return bytesRecebidos;
}

void configurarSocketIPv6(int *servidor, struct sockaddr_in6 *servidor_addr) {
    *servidor = socket(AF_INET6, SOCK_STREAM, 0);
    if (*servidor < 0) {
        perror("Error ao criar o socket");
        exit(1);
    }
    printf("Servidor IPv6 TCP criado.\n");

    memset(servidor_addr, 0, sizeof(struct sockaddr_in6));
    servidor_addr->sin6_family = AF_INET6;
    servidor_addr->sin6_port = htons(PORTA); 
    servidor_addr->sin6_addr = in6addr_any; 

    if (bind(*servidor, (struct sockaddr *)servidor_addr, sizeof(struct sockaddr_in6)) < 0) {
        perror("Error no bind");
        close(*servidor);
        exit(1);
    }
    printf("Bind feito na porta: %d\n", PORTA);
}

void configurarSocketIPv4(int *servidor, struct sockaddr_in *addr) {
    *servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (*servidor < 0){
        perror("Error ao criar o socket");
        exit(1);
    }
    printf("Servidor IPv4 TCP criado.\n");

    memset(addr, '\0', sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = PORTA;
    addr->sin_addr.s_addr = inet_addr(IP);

    if (bind(*servidor, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0) {
        perror("Error no bind");
        close(*servidor);
        exit(1);
    }
    printf("Bind feito na porta: %d\n", PORTA);
}

// Função para gerar nome do arquivo baseado na conexão
char* gerarNomeArquivo(const char* host, const char* diretorio, int conexao_id) {
    char* nome_arquivo = malloc(256);
    if (nome_arquivo == NULL) {
        perror("Erro ao alocar memoria para nome do arquivo");
        return NULL;
    }
    
    snprintf(nome_arquivo, 256, "%s_%s_conexao_%d.txt", host, diretorio, conexao_id);
    
    return nome_arquivo;
}

void processarCliente(int cliente, int conexao_id) {
    FILE *out_file = NULL;
    long total_bytes_recebidos = 0;
    struct timeval inicio_conexao, fim_conexao;

    char *buffer = malloc(BUFFER_SIZE);
    if (buffer == NULL) {
        perror("Erro ao alocar buffer");
        close(cliente);
        return;
    }

    printf("\n=== PROCESSANDO CLIENTE %d ===\n", conexao_id);
    gettimeofday(&inicio_conexao, NULL);

    // Receber a mensagem "READY" do cliente
    long bytes_recebidos = receberMensagemComTamanho(cliente, buffer);
    total_bytes_recebidos += bytes_recebidos;

    // Caso a mensagem recebida seja "READY"
    if (strcmp(buffer, "READY") == 0) {
        // Enviar a mensagem "READY ACK" para o cliente
        enviarMensagem(cliente, buffer, "READY ACK");

        // Criar nome do arquivo único para esta conexão
        char* nome_arquivo = gerarNomeArquivo("localhost", "dir", conexao_id);
        if (nome_arquivo == NULL) {
            free(buffer);
            close(cliente);
            return;
        }

        out_file = fopen(nome_arquivo, "w");
        if(out_file == NULL){
            perror("Servidor falhou em criar/abrir o arquivo");
            free(nome_arquivo);
            free(buffer);
            close(cliente);
            return;
        }

        printf("Arquivo criado: %s\n", nome_arquivo);

        // Receber mensagens do cliente
        bytes_recebidos = receberMensagemComTamanho(cliente, buffer);
        total_bytes_recebidos += bytes_recebidos;
        
        int mensagens_recebidas = 0;
        
        // Enquanto não receber a mensagem "BYE"
        while(strcmp(buffer, "BYE") != 0 && bytes_recebidos > 0) {
            mensagens_recebidas++;
            char *nomeArquivoFinal = NULL;

            // Processar byte stuffing se necessário
            if (isCharDestuffing(buffer)) {
                printf("Detectado byte stuffing, removendo...\n");
                nomeArquivoFinal = charDestuffing(buffer);
                if (nomeArquivoFinal != NULL) {
                    printf("Nome original do arquivo: %s\n", nomeArquivoFinal);
                    fprintf(out_file, "%s\n", nomeArquivoFinal);
                    free(nomeArquivoFinal); 
                } else {
                    printf("Erro no destuffing, usando nome original\n");
                    fprintf(out_file, "%s\n", buffer);
                }
            } else {
                // Para mensagens de teste grandes, não imprimir no arquivo
                if (strlen(buffer) > 100) {
                    fprintf(out_file, "[DADOS_TESTE_%d_BYTES]\n", (int)strlen(buffer));
                } else {
                    fprintf(out_file, "%s\n", buffer);
                }
            }

            fflush(out_file); // Garantir que dados são escritos

            // Enviar ACK
            enviarMensagem(cliente, buffer, "ACK");
            
            // Receber próxima mensagem
            bytes_recebidos = receberMensagemComTamanho(cliente, buffer);
            total_bytes_recebidos += bytes_recebidos;
        }
        
        if (out_file) {
            fclose(out_file);
            printf("Arquivo salvo com sucesso: %s\n", nome_arquivo);
        }
        
        // Calcular estatísticas da conexão
        gettimeofday(&fim_conexao, NULL);
        long tempo_ms = ((fim_conexao.tv_sec - inicio_conexao.tv_sec) * 1000) + 
                       ((fim_conexao.tv_usec - inicio_conexao.tv_usec) / 1000);
        
        printf("=== ESTATÍSTICAS DA CONEXÃO %d ===\n", conexao_id);
        printf("Total de bytes recebidos: %ld bytes\n", total_bytes_recebidos);
        printf("Mensagens processadas: %d\n", mensagens_recebidas);
        printf("Tempo de conexão: %ld ms\n", tempo_ms);
        if (tempo_ms > 0) {
            printf("Taxa de recepção: %.2f KB/s\n", (total_bytes_recebidos / 1024.0) / (tempo_ms / 1000.0));
        }
        printf("================================\n");
        
        free(nome_arquivo);
    } 

    // Mensagem "BYE" recebida - fechar conexão
    if(strcmp(buffer, "BYE") == 0){
        printf("Cliente %d desconectado.\n", conexao_id);
    }
    
    free(buffer);
    close(cliente);
}

int main() {
    // Declaração de variáveis
    int servidor, cliente;
    struct sockaddr_in server_addr, cliente_addr;
    struct sockaddr_in6 server_addr6, cliente_addr6;
    
    // Inicializando
    static int contadorConexoes = 0;

    printf("=== SERVIDOR DE TESTE DE PERFORMANCE ===\n");

    // Configurar socket
    if(true){
        configurarSocketIPv6(&servidor,&server_addr6);
    }else {
        configurarSocketIPv4(&servidor,&server_addr);
    }

    // Servidor escutando
    listen(servidor, NUM_MAX_CLIENTES);
    printf("Socket-servidor escutando com sucesso na porta %d\n", PORTA);
    printf("Aguardando conexões de clientes...\n");

    while(true) {
        // Aceitar conexões de clientes
        socklen_t cliente_addr_len;

        if(true){
            cliente_addr_len = sizeof(cliente_addr6);
            cliente = accept(servidor, (struct sockaddr*)&cliente_addr6, &cliente_addr_len);        
        }else {
            cliente_addr_len = sizeof(cliente_addr);
            cliente = accept(servidor, (struct sockaddr*)&cliente_addr, &cliente_addr_len);
        }
        
        if (cliente < 0) {
            perror("Erro ao aceitar conexão");
            continue;
        }

        contadorConexoes++;

        // Processar cliente
        processarCliente(cliente, contadorConexoes);
    }

    close(servidor);
    return 0;
}