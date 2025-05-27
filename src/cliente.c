#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>

// Constantes
const int PORTA = 12012; 
const char *IP = "127.0.0.1";
const int BUFFER_SIZE = 1024;

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
    strcpy(buffer, mensagem); // Adicionar o tratamento de byte stuffing
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
    server_addr.sin_addr.s_addr = inet_addr(IP); // IP do servidor (localhost)

    if (connect(cliente, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { // MUDANÇA: removido o *
        perror("Erro ao conectar ao servidor");
        exit(1);
    }
    printf("Conexão com o servidor estabelecida com sucesso!\n");

    // Enviar a mensagem "READY" para o servidor
    enviarMensagem(cliente, buffer, "~READY");

    // Recebeu a mensagem "READY ACK" do servidor
    printf(receberMensagem(cliente, buffer));

    // Fechar conexão
    close(cliente);
    printf("Conexão fechada.\n");

    return 0;
}