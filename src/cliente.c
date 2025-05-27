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

// Constantes
const int PORTA = 8489; 
const char *IP = "127.0.0.1";
const int BUFFER_SIZE = 1024;
const char *DIRETORIO = "./arquivos";

// Funções auxiliares
int isByteStuffing(const char *mensagem) { 
    if ((strcmp(mensagem, "bye") == 0) || (strncmp(mensagem, "~", 1) == 0)) {
        return 1; 
    }

    return 0; 
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

int main() {
    // Declaração de variáveis
    int cliente;
    struct sockaddr_in server_addr;

    // Inicializações
    char buffer[BUFFER_SIZE];

    // 1- Criar um socket
    cliente = socket(AF_INET, SOCK_STREAM, 0);
    if (cliente < 0) {
        perror("Erro ao criar o socket-cliente");
        exit(1);
    }
    printf("Socket-cliente criado com sucesso!\n");

    // 2- Conectar ao servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORTA); 
    server_addr.sin_addr.s_addr = inet_addr(IP); 

    if (connect(cliente, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { 
        perror("Erro ao conectar ao servidor");
        exit(1);
    }
    printf("Conexão com o servidor estabelecida com sucesso!\n");

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