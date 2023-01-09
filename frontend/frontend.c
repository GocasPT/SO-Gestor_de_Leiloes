#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "frontend.h"
#include "../backend/backend.h"

#define FIFO_BACKEND "../SOBay_fifo"
#define FIFO_BACKEND_FRONTED "../SOBay_cliente_fifo"
#define FIFO_FRONTEND "../cliente_fifo[%d]"

#define CODE_VALIDE "SUCCESS 200 - VALIDO"
#define CODE_LIMIT "ERROR 400 - LIMITE ATINGIDO"
#define CODE_MISSING "ERROR 404 - USER NAO EXISTE"
#define CMD_MISSING "<Client: Parametos incompletos>\n"

char FIFO_FRONTEND_FINAL[MAX];
char FIFO_BACNEKND_FINAL[MAX];
int fd_recebe, fd_envio, fd_backend_w, fd_backend_r;
pthread_t thread_id;
cliente user;

//  Função para fechar o user
void fecharFrontend(){
  printf("\n<Client: O user foi desconectado>\n");
  close(fd_recebe);
  close(fd_envio);
  unlink(FIFO_FRONTEND_FINAL);
  exit(0);
}

//  Rotiona para ler as mensages vindas do backend
void *readMensagem(void *vargp){
  char buffer[MAX];
  int size;
  int fd_recebe = open(FIFO_FRONTEND_FINAL, O_RDONLY | O_NONBLOCK);
  if(fd_recebe == -1){
      printf("<Client: Ocorreu um erro ao abrir o pipe de receção>\n");
      fecharFrontend();
  }
  while(1){
      size = read(fd_recebe, buffer, MAX);
      if(size < 0){
          if(errno == EAGAIN){
              continue;
          }
          else{
              printf("<Client: Ocorreu um erro ao ler a mensagem>\n");
              fecharFrontend();
          }
      }
      else{
        fflush(stdout);
        buffer[strlen(buffer)] = '\0';
        printf("<Servidor> %s", buffer);
        fflush(stdout);
      }
  }
}

//  Rotiona para mandar comandos para o backend
void *writeMensagem(void *vargp){
  char buffer[MAX];
  int size;
  int fd_envio = open(FIFO_BACKEND_FRONTED, O_WRONLY);
  if(fd_envio == -1){
    printf("<Client: Ocorreu um erro ao abrir o pipe de envio>\n");
    fecharFrontend();
  }
  while(1){
    sleep(1);
    strcpy(buffer, "");
    printf("<User> ");
    fflush(stdout);
    fflush(stdin);
    fgets(buffer, MAX, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
    strcpy(user.command, buffer);
    size = write(fd_envio, &user, sizeof(user));
    if(size == -1){
      if(errno == EAGAIN){
          continue;
      }
      else{
          printf("<Client: Ocorreu um erro ao escrever a mensagem>\n");
          fecharFrontend();
      }
    } 
    
    else if(strcmp(buffer, "exit") == 0){
      fecharFrontend();
    }

    sleep(1);
  }
}

int backendAberto(){
  int fd_backend = open(FIFO_BACKEND, O_RDONLY | O_NONBLOCK);

  close(fd_backend);

  if(fd_backend == -1){
      return 0;
  }

  return 1;
}

int main(int argc, char *argv[]){
   //  Verifaicar de backend está aberto
  if(!backendAberto()){
    printf("<Client: O Backend nao esta aberto>\n");
    return 0;
  }

  //  Validação se foi introduzido credênciais
	if(argv[1] == NULL || argc < 2) {
    printf("<Client: Precisa de loggin>\n");
    return 0;
  } 
  else if (argc != 3) {
    printf("<Client:Nao foi incerido password>\n");
    return 0;
	}

  server server;

  //  Sinal
  struct sigaction sa;
  sa.sa_handler = fecharFrontend;
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sigaction(SIGINT, &sa, NULL);

  //  Tenta estabelecer uma conexão com o backend
  sprintf(FIFO_FRONTEND_FINAL, FIFO_FRONTEND, getpid());
  if(mkfifo(FIFO_FRONTEND_FINAL, 0666) == -1){
      printf("\n<Client: Ocorreu um erro ao criar um túnel de comunicação>\n");
      unlink(FIFO_FRONTEND_FINAL);
      return 1;
  }

  printf("\033[2J\033[1;1H");
  printf("  ______ _____   ____  _   _ _______ ______ _   _ _____  \n");  
  printf(" |  ____|  __ \\ / __ \\| \\ | |__   __|  ____| \\ | |  __ \\ \n");
  printf(" | |__  | |__) | |  | |  \\| |  | |  | |__  |  \\| | |  | |\n");
  printf(" |  __| |  _  /| |  | | . ` |  | |  |  __| | . ` | |  | |\n");
  printf(" | |    | | \\ \\| |__| | |\\  |  | |  | |____| |\\  | |__| |\n");
  printf(" |_|    |_|  \\_\\\\____/|_| \\_|  |_|  |______|_| \\_|_____/ \n"); 
  fflush(stdout);                                            

  //  Configuração do usuário na estrutura
  user.pid = getpid();
  strcpy(user.pipeUser, FIFO_FRONTEND_FINAL);
  strcpy(user.username, argv[1]);
  strcpy(user.password, argv[2]);

  //  Testa o túnel de comunicação com o backend
  fd_envio = open(FIFO_BACKEND_FRONTED, O_WRONLY);
  if(fd_envio == -1){
      printf("\n<Client: Ocorreu um erro ao abrir o túnel de comunicação WRITE>\n");
      close(fd_envio);
      unlink(FIFO_FRONTEND_FINAL);
      return 1;
  }

  //  Envias informações do user para testar e validar as credências
  int size_s = write(fd_envio, &user, sizeof(user));
  if(size_s == -1){
      printf("\n<Client: Ocorreu um erro ao autenticar-se>\n");
      close(fd_envio);
      unlink(FIFO_FRONTEND_FINAL);
      return 1;
  }

  //  Teste de resposta vinda do backend
  char resposta[MAX];
  fd_recebe = open(FIFO_FRONTEND_FINAL, O_RDONLY);
  if(fd_recebe == -1){
      printf("\n<Client: Ocorreu um erro ao abrir o túnel de comunicação READ>\n");
      close(fd_envio);
      close(fd_recebe);
      unlink(FIFO_FRONTEND_FINAL);
      return 1;
  }

  //  Leitura do resultado da validação das credências
  int size = read(fd_recebe, resposta, sizeof(resposta));
    if(size > 0){
      //  Não existe esse usuário (credências)
      if(!strcmp(CODE_MISSING, resposta)){
        printf("\n<Client: Não foi possível conectar ao servidor\n\tO usuário não existe na base de dados>\n");
        fecharFrontend();
        return 1;
      }
      //  Já tá no número máximo de usuráios no backend
      else if(!strcmp(CODE_LIMIT, resposta)){
        printf("\n<Servido: Não foi possível conectar ao balcão\n\tLimite de usuários atingido>\n");
        fecharFrontend();
        return 1;
      }
      //  Valildo para entrar e inicialização das threads de leitura/escrita
      else if(!strcmp(CODE_VALIDE, resposta)){
        printf("\n<Client> Bem vindo ao SOBay, %s\n", user.username);
        pthread_create(&thread_id, NULL, readMensagem, &server);
        pthread_create(&thread_id, NULL, writeMensagem, &server);
        pthread_join(thread_id, NULL);
      }
    } else {
        printf("\n<Client: Ocorreu um problema ao receber uma resposta do servidor>\n");
    }

  // Fechar os tubos de comunbicação e apagar o seu ficheiro fifo
  close(fd_backend_r);
  close(fd_backend_w);
  close(fd_recebe);
  close(fd_envio);
  unlink(FIFO_FRONTEND_FINAL);

	return 0;
}
