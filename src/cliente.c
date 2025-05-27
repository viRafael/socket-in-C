#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    // Declaração de variáveis
    int cliente;  // MUDANÇA: removido o * (não é ponteiro)
    struct sockaddr_in server_addr;

    // Constantes
    const int PORTA = 12012;  // MUDANÇA: mesma porta do servidor
    const char *IP = "127.0.0.1";

    //1- Criar um socket
    cliente = socket(AF_INET, SOCK_STREAM, 0); // MUDANÇA: removido o *
    if (cliente < 0) {
        perror("Erro ao criar o socket-cliente");
        exit(1);
    }
    printf("Socket-cliente criado com sucesso!\n");

    //2- Conectar ao servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORTA);  // MUDANÇA: usar a constante PORTA
    server_addr.sin_addr.s_addr = inet_addr(IP); // IP do servidor (localhost)

    if (connect(cliente, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { // MUDANÇA: removido o *
        perror("Erro ao conectar ao servidor");
        exit(1);
    }
    printf("Conexão com o servidor estabelecida com sucesso!\n");

    // Enviar a mensagem "READY" para o servidor

    // Recebeu a mensagem "READY ACK" do servidor

    // Fechar conexão
    close(cliente);  // MUDANÇA: removido o *
    printf("Conexão fechada.\n");

    return 0;
}