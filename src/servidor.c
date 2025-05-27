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
const int NUM_CLIENTES = 1;
const int BUFFER_SIZE = 1024;

//Funções auxiliares
char *charDestuffing(const char *mensagem) {
    const char *prefixo = "~";
    size_t tamanhoPrefixo = strlen(prefixo);

    char *nova_mensagem = NULL;
    size_t tamanhoOriginal = strlen(mensagem);

    size_t tamanhoSemPrefixo = tamanhoOriginal - tamanhoPrefixo;
    nova_mensagem = (char *)malloc(tamanhoSemPrefixo + 1);

    if (nova_mensagem == NULL) {
        perror("Erro ao alocar memoria em charDestuffing (com prefixo)");
        return NULL;
    }

    strcpy(nova_mensagem, mensagem + tamanhoPrefixo);

    return nova_mensagem;
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
    int servidor, cliente;
    struct sockaddr_in server_addr, cliente_addr;
    char buffer[BUFFER_SIZE];

    // 1- Criar um socket
    servidor = socket(AF_INET, SOCK_STREAM, 0);
    if (servidor < 0) {
        perror("Error ao criar o socket-servidor");
        exit(1);
    }
    printf("Socket-servidor criado com sucesso!\n");

    // 2- Colocar no Bind (associar socket a um endereço IP e porta)
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORTA);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(servidor, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao fazer o bind do socket-servidor");
        exit(1);
    }
    printf("Socket-servidor associado com sucesso!\n");

    // 3- Servidor escutando
    if (listen(servidor, NUM_CLIENTES) < 0) {
        perror("Erro ao escutar o socket-servidor");
        exit(1);
    }
    printf("Socket-servidor escutando com sucesso na porta %d\n", PORTA);

    // 4- Aceitar conexões de clientes
    socklen_t cliente_len;

    cliente_len = sizeof(cliente_addr);
    cliente = accept(servidor, (struct sockaddr*)&cliente_addr, &cliente_len);
    if (cliente < 0) {
        perror("Erro ao aceitar conexao do cliente");
        exit(1);
    }
    printf("Cliente conectado com sucesso!\n");

    // Receber a mensagem "READY" do cliente
    receberMensagem(cliente, buffer);

    // Enviar a mensagem "READY ACK" para o cliente
    enviarMensagem(cliente, buffer, "READY ACK");

    // Fechar conexão
    close(cliente);
    close(servidor);
    printf("Conexão fechada com sucesso!\n");
    
    return 0;
}