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
const int BUFFER_SIZE = 1024;
const char *DIRETORIO = "./arquivos";

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

    // Aloca memória para a nova string
    char *mensagemStuffing = (char *)malloc(tamanho_nova_string);

    // Verifica se a alocação de memória foi feita
    if (mensagemStuffing == NULL) {
        perror("Erro ao alocar memoria em charStuffing");
        return NULL;
    }

    sprintf(mensagemStuffing, "%s%s", prefixo, mensagem);

    return mensagemStuffing;
}
    
void enviarMensagem(int socket, char *buffer, char *mensagem) {
    bzero(buffer, BUFFER_SIZE);
    strcpy(buffer, mensagem); 
    printf("Enviando mensagem: %s\n", buffer);
    send(socket, buffer, strlen(buffer), 0);
}

char *receberMensagem(int socket, char *buffer) {
    bzero(buffer, BUFFER_SIZE);
    recv(socket, buffer, BUFFER_SIZE, 0);
    printf("Mensagem recebida: %s\n", buffer);

    return buffer;
}

void configurarSocketIPv6(int *cliente, struct sockaddr_in6 *cliente_addr) {
    // 1- Criar um Socket
    *cliente = socket(AF_INET6, SOCK_STREAM, 0);
    if (*cliente < 0) {
        perror("Error ao criar o socket");
        exit(1);
    }
    printf("Cliente IPv6 TCP criado.\n");

    memset(cliente_addr, 0, sizeof(struct sockaddr_in6));
    cliente_addr->sin6_family = AF_INET6;
    cliente_addr->sin6_port = htons(PORTA); // Convert port to network byte order

    if (inet_pton(AF_INET6, IP6, &cliente_addr->sin6_addr) <= 0) {
        perror("[-] Invalid IPv6 address");
        close(*cliente);
        exit(1);
    }
}

void configurarSocketIPv4(int *cliente, struct sockaddr_in *cliente_addr) {
    // 1- Criar um Socket
    *cliente = socket(AF_INET, SOCK_STREAM, 0);
    if (*cliente < 0){
        perror("Error ao criar o socket");
        exit(1);
    }
    printf("Cliente IPv4 TCP criado.\n");

    memset(cliente_addr, '\0', sizeof(*cliente_addr));
    cliente_addr->sin_family = AF_INET;
    cliente_addr->sin_port = PORTA;
    cliente_addr->sin_addr.s_addr = inet_addr(IP);
}

int main() {
    // Declaração de variáveis
    int cliente;
    struct sockaddr_in cliente_addr;
    struct sockaddr_in6 cliente_addr6;

    // Inicializações
    char buffer[BUFFER_SIZE];

    // 1- Criar um Socket e 2- Colocar Conectar
    if(true){
        configurarSocketIPv6(&cliente, &cliente_addr6);
        connect(cliente, (struct sockaddr*)&cliente_addr6, sizeof(cliente_addr6));
    }else {
        configurarSocketIPv4(&cliente, &cliente_addr);
        connect(cliente, (struct sockaddr*)&cliente_addr, sizeof(cliente_addr));
    }

    // 3- Enviar a mensagem "READY" para o servidor
    enviarMensagem(cliente, buffer, "READY");

    // 4- Recebeu a mensagem "READY ACK" do servidor
    receberMensagem(cliente, buffer);

    // 5- Começamos a enviar os arquivos para o servidor
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

            // Verificar se precisa de byte stuffing
            if (isByteStuffing(nomeArquivo)) {
                char *nomeComStuffing = charStuffing(nomeArquivo);
                enviarMensagem(cliente, buffer, nomeComStuffing);
                free(nomeComStuffing);
            } else {
                enviarMensagem(cliente, buffer, nomeArquivo);
            }

            // Esperar ACK do servidor
            receberMensagem(cliente, buffer);
            if (strcmp(buffer, "ACK") != 0) {
                printf("Erro: esperava ACK mas recebeu: %s\n", buffer);
            }
        }
        closedir(dir);
    }

    // 6- Enviar "BYE" para encerrar a comunicação
    printf("Enviando BYE para encerrar...\n");
    enviarMensagem(cliente, buffer, "BYE");

    // 7- Fechar conexão
    close(cliente);
    printf("Conexão fechada.\n");

    return 0;
}