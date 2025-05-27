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
    int servidor, cliente;  // MUDANÇA: removido o * (não é ponteiro)
    struct sockaddr_in server_addr, cliente_addr;

    // Constantes
    const int PORTA = 12012;
    const int NUM_CLIENTES = 1;

    // 1- Criar um socket
    servidor = socket(AF_INET, SOCK_STREAM, 0); // MUDANÇA: removido o *
    if (servidor < 0) {
        perror("Error ao criar o socket-servidor");
        exit(1);
    }
    printf("Socket-servidor criado com sucesso!\n");

    // 2- Colocar no Bind (associar socket a um endereço IP e porta)
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORTA);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(servidor, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) { // MUDANÇA: removido o *
        perror("Erro ao fazer o bind do socket-servidor");
        exit(1);
    }
    printf("Socket-servidor associado com sucesso!\n");

    // 3- Servidor escutando 
    if (listen(servidor, NUM_CLIENTES) < 0) { // MUDANÇA: removido o *
        perror("Erro ao escutar o socket-servidor");
        exit(1);
    }
    printf("Socket-servidor escutando com sucesso na porta %d\n", PORTA);

    // 4- Aceitar conexões de clientes
    socklen_t cliente_len; // MUDANÇA: tipo correto e removido o ; extra

    cliente_len = sizeof(cliente_addr); // MUDANÇA: deve ser cliente_addr, não server_addr
    cliente = accept(servidor, (struct sockaddr*)&cliente_addr, &cliente_len); // MUDANÇA: removido o *
    if (cliente < 0) {
        perror("Erro ao aceitar conexao do cliente");
        exit(1);
    }
    printf("Conexao aceita com sucesso!\n");

    // Fechar conexão
    close(cliente);
    close(servidor); // MUDANÇA: removido o *
    printf("Conexão fechada com sucesso!\n");
    
    return 0;
}