#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>

// Constantes
const int PORTA = 8489;
const int NUM_CLIENTES = 3;
const int BUFFER_SIZE = 1024;

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
    novaMensagem[tamanhoSemPrefixo] = '\0'; // Garantir terminação

    return novaMensagem;
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

    // Inicialização
    char buffer[BUFFER_SIZE];
    FILE *out_file;

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

    while(true) {
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

        // Caso a mensagem recebida seja "READY"
        if (strcmp(buffer, "READY") == 0) {
            // Enviar a mensagem "READY ACK" para o cliente
            enviarMensagem(cliente, buffer, "READY ACK");

            // TODO: Implementar a corretamente a criação do nome do arquivo (<host><diretorio>)
            out_file = fopen("localhost:dir", "w");
            if(out_file == NULL){
                perror("Servidor falhou em criar/abrir o arquivo");
                return 1;
            }

            // Receber a mensagem do cliente
            receberMensagem(cliente, buffer);
            
            // Enquanto não receber a mensagem "BYE"
            while(strcmp(buffer, "BYE") != 0) {
                char *nomeArquivoFinal = NULL;

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
                    fprintf(out_file, "%s\n", buffer);
                }

                printf("Mensagem recebida e escrita no arquivo: %s\n", buffer);
                fflush(out_file); // Garantir que dados são escritos

                enviarMensagem(cliente, buffer, "ACK");
                receberMensagem(cliente, buffer);
            }
            fclose(out_file);
            printf("Arquivo salvo com sucesso!\n");
        } 

        // Chegou aqui a mensagem é "BYE"
        if(strcmp(buffer, "BYE")==0){
            // close connection
            close(cliente);
            printf("Cliente desconectado.\n\n");
        }  
    }

    return 0;
}