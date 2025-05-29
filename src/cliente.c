#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/time.h>

// Constantes
const int PORTA = 8489; 
const char *IP = "127.0.0.1";
const char *IP6 = "::1";
const int BUFFER_SIZE = 66000;
const char *DIRETORIO = "./arquivos";

// Estrutura para medições de performance
typedef struct {
    struct timeval tempo_inicio;
    struct timeval tempo_fim;
    long bytes_enviados;
    double tempo_total_ms;
    double throughput_mbps;
} PerformanceMetrics;

// Funções auxiliares

bool isByteStuffing(const char *mensagem) { 
    if ((strcmp(mensagem, "bye") == 0) || (strncmp(mensagem, "~", 1) == 0)) {
        return true; 
    }
    return false; 
}

char *charStuffing(const char *mensagem) {
    const char *prefixo = "~";
    
    size_t tamanho_prefixo = strlen(prefixo);
    size_t tamanho_original_mensagem = strlen(mensagem);
    size_t tamanho_nova_string = tamanho_prefixo + tamanho_original_mensagem + 1;

    char *mensagemStuffing = (char *)malloc(tamanho_nova_string);

    if (mensagemStuffing == NULL) {
        perror("Erro ao alocar memoria em charStuffing");
        return NULL;
    }

    sprintf(mensagemStuffing, "%s%s", prefixo, mensagem);
    return mensagemStuffing;
}

void iniciarMedicao(PerformanceMetrics *metrics) {
    gettimeofday(&metrics->tempo_inicio, NULL);
    metrics->bytes_enviados = 0;
    printf("=== INICIANDO MEDIÇÃO DE PERFORMANCE ===\n");
}

void finalizarMedicao(PerformanceMetrics *metrics) {
    gettimeofday(&metrics->tempo_fim, NULL);
    
    // Calcular tempo total em milissegundos
    long segundos = metrics->tempo_fim.tv_sec - metrics->tempo_inicio.tv_sec;
    long microssegundos = metrics->tempo_fim.tv_usec - metrics->tempo_inicio.tv_usec;
    metrics->tempo_total_ms = (segundos * 1000.0) + (microssegundos / 1000.0);
    
    // Calcular throughput em Mbps
    double tempo_segundos = metrics->tempo_total_ms / 1000.0;
    double bits_enviados = metrics->bytes_enviados * 8.0; // Converter bytes para bits
    metrics->throughput_mbps = (bits_enviados / (1024.0 * 1024.0)) / tempo_segundos; // Mbps
    
    printf("=== RESULTADOS DA MEDIÇÃO ===\n");
    printf("Tempo total: %.2f ms\n", metrics->tempo_total_ms);
    printf("Bytes enviados: %ld bytes\n", metrics->bytes_enviados);
    printf("Throughput: %.2f Mbps\n", metrics->throughput_mbps);
    printf("Throughput: %.2f KB/s\n", (metrics->bytes_enviados / 1024.0) / tempo_segundos);
    printf("================================\n");
}

long enviarMensagemComTamanho(int socket, char *buffer, char *mensagem) {
    bzero(buffer, BUFFER_SIZE);
    strcpy(buffer, mensagem); 
    printf("Enviando mensagem: %.20s...\n", buffer);
    
    long bytesEnviados = send(socket, buffer, strlen(buffer), 0);
    if (bytesEnviados < 0) {
        perror("Erro ao enviar mensagem");
        return 0;
    }
    
    return bytesEnviados;
}

char *receberMensagem(int socket, char *buffer) {
    bzero(buffer, BUFFER_SIZE);
    recv(socket, buffer, BUFFER_SIZE, 0);
    printf("Mensagem recebida: %s\n", buffer);

    return buffer;
}

void configurarSocketIPv6(int *cliente, struct sockaddr_in6 *cliente_addr) {
    *cliente = socket(AF_INET6, SOCK_STREAM, 0);
    if (*cliente < 0) {
        perror("Error ao criar o socket IPv6");
        exit(1);
    }

    memset(cliente_addr, 0, sizeof(struct sockaddr_in6));
    cliente_addr->sin6_family = AF_INET6;
    cliente_addr->sin6_port = htons(PORTA);

    if (inet_pton(AF_INET6, IP6, &cliente_addr->sin6_addr) <= 0) {
        perror("Error ao converter endereço IPv6");
        close(*cliente);
        exit(1);
    }
}

void configurarSocketIPv4(int *cliente, struct sockaddr_in *cliente_addr) {
    *cliente = socket(AF_INET, SOCK_STREAM, 0);
    if (*cliente < 0){
        perror("Error ao criar o socket IPv4");
        exit(1);
    }

    memset(cliente_addr, '\0', sizeof(*cliente_addr));
    cliente_addr->sin_family = AF_INET;
    cliente_addr->sin_port = PORTA;
    cliente_addr->sin_addr.s_addr = inet_addr(IP);
}

// Função para gerar dados de teste com tamanho específico
char* gerarDadosTeste(int tamanho) {
    char *dados = malloc(tamanho + 1);
    if (dados == NULL) {
        perror("Erro ao alocar memoria para dados de teste");
        return NULL;
    }
    
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < tamanho; i++) {
        dados[i] = charset[i % (sizeof(charset) - 1)];
    }
    dados[tamanho] = '\0';
    
    return dados;
}

// Função para testar performance com diferentes tamanhos
void testarPerformanceTamanhos(int cliente, char *buffer) {
    printf("\n=== TESTANDO PERFORMANCE COM DIFERENTES TAMANHOS ===\n");
    
    // Testar tamanhos 2^i onde 0 ≤ i < 16
    for (int i = 0; i < 16; i++) {
        int tamanho = 1 << i; // 2^i
        printf("\n--- Testando tamanho: %d bytes (2^%d) ---\n", tamanho, i);
        
        PerformanceMetrics metrics;
        iniciarMedicao(&metrics);
        
        // Gerar dados de teste
        char *dados_teste = gerarDadosTeste(tamanho);
        if (dados_teste == NULL) continue;
        
        // Enviar dados e contar bytes
        long bytes_enviados = enviarMensagemComTamanho(cliente, buffer, dados_teste);
        metrics.bytes_enviados += bytes_enviados;
        
        // Receber ACK
        receberMensagem(cliente, buffer);
        
        finalizarMedicao(&metrics);
        free(dados_teste);
    }
}

int main() {
    // Declaração de variáveis
    int cliente;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in cliente_addr;
    struct sockaddr_in6 cliente_addr6;
    PerformanceMetrics metricasPrincipais;

    // Configurar socket
    if(true){
        configurarSocketIPv6(&cliente, &cliente_addr6);
        connect(cliente, (struct sockaddr*)&cliente_addr6, sizeof(cliente_addr6));
    }else {
        configurarSocketIPv4(&cliente, &cliente_addr);
        connect(cliente, (struct sockaddr*)&cliente_addr, sizeof(cliente_addr));
    }

    // INICIAR MEDIÇÃO PRINCIPAL
    iniciarMedicao(&metricasPrincipais);

    // Enviar "READY"
    long bytes_ready = enviarMensagemComTamanho(cliente, buffer, "READY");
    metricasPrincipais.bytes_enviados += bytes_ready;

    // Receber "READY ACK"
    receberMensagem(cliente, buffer);

    // Enviar arquivos do diretório
    if (strcmp(buffer, "READY ACK") == 0) {
        printf("Abrindo diretório: %s\n", DIRETORIO);

        DIR *dir = opendir(DIRETORIO);
        if (dir == NULL) {
            perror("Erro ao abrir o diretório");
            printf("Certifique-se de que a pasta '%s' existe!\n", DIRETORIO);
            close(cliente);            
            exit(1);
        }

        struct dirent *en;
        while((en = readdir(dir)) != NULL){
            char *nomeArquivo = en->d_name;

            // Pular diretórios "." e ".."
            if (strcmp(nomeArquivo, ".") == 0 || strcmp(nomeArquivo, "..") == 0) {
                continue;
            }   

            printf("Enviando arquivo: %s\n", nomeArquivo);

            long bytes_enviados = 0;
            if (isByteStuffing(nomeArquivo)) {
                char *nomeComStuffing = charStuffing(nomeArquivo);
                bytes_enviados = enviarMensagemComTamanho(cliente, buffer, nomeComStuffing);
                free(nomeComStuffing);
            } else {
                bytes_enviados = enviarMensagemComTamanho(cliente, buffer, nomeArquivo);
            }
            
            metricasPrincipais.bytes_enviados += bytes_enviados;

            // Receber ACK
            receberMensagem(cliente, buffer);
            if (strcmp(buffer, "ACK") != 0) {
                printf("Erro: esperava ACK mas recebeu: %s\n", buffer);
            }
        }
        closedir(dir);
    }

    printf("Enviando BYE para encerrar...\n");
    long bytes_bye = enviarMensagemComTamanho(cliente, buffer, "BYE");
    metricasPrincipais.bytes_enviados += bytes_bye;

    // FINALIZAR MEDIÇÃO PRINCIPAL
    finalizarMedicao(&metricasPrincipais);
    close(cliente);
    
    printf("\n=== EXECUTANDO TESTES ADICIONAIS DE PERFORMANCE ===\n");
    
    // Fazer vários testes com diferentes tamanhos
    for (int teste = 0; teste < 5; teste++) { // 5 testes diferentes
        printf("\n--- TESTE %d ---\n", teste + 1);
        
        if(true){
            configurarSocketIPv6(&cliente, &cliente_addr6);
            connect(cliente, (struct sockaddr*)&cliente_addr6, sizeof(cliente_addr6));
        }else {
            configurarSocketIPv4(&cliente, &cliente_addr);
            connect(cliente, (struct sockaddr*)&cliente_addr, sizeof(cliente_addr));
        }
        
        enviarMensagemComTamanho(cliente, buffer, "READY");
        receberMensagem(cliente, buffer);
        
        if (strcmp(buffer, "READY ACK") == 0) {
            testarPerformanceTamanhos(cliente, buffer);
        }
        
        // Finalizar
        enviarMensagemComTamanho(cliente, buffer, "BYE");
        close(cliente);
    }

    printf("Testes de performance concluídos!\n");
    return 0;
}