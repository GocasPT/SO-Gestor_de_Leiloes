#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "backend.h"
#include "../frontend/frontend.h"
#include "../promotor/promotor.h"
#include "../users_lib/users_lib.h"

#define FIFO_BACKEND "../SOBay_fifo"
#define FIFO_BACKEND_FRONTED "../SOBay_cliente_fifo"
#define FIFO_FRONTEND "../cliente_fifo[%d]"

#define CODE_VALIDE "SUCCESS 200 - VALIDO"
#define CODE_LIMIT "ERROR 400 - LIMITE ATINGIDO"
#define CODE_MISSING "ERROR 404 - USER NAO EXISTE"
#define CMD_MISSING "$ Parametos incompletos\n"

typedef struct Item
{
	int id;
	char name[50];
	char category[50];
	int basePrice;
	int currentPrice;
	int buyNowPrice;
	char seller[MAX];
	char topBuyer[MAX];
	int duration;
} item;

pthread_t thread_id;
pthread_t thread_free;
server servidor;
cliente user;
discount promo;
item *itemsList;
char FIFO_FINAL[MAX], filePath[MAX], itensPath[MAX];;
int numUsers, numItens, idCounter = 0;

//	Verifica se o balcão está aberto
//	0 = sim, 1 = não
int backendAberto(){
	int fd_balcao = open(FIFO_BACKEND, O_RDONLY | O_NONBLOCK);
	close(fd_balcao);
	if(fd_balcao == -1){
		return 0;
	}
	return 1;
}

//	Carrge os items do ficheiro para a lista dos items.
//	Input: caminho e endereço da lista deos items.
//	Output: número de items.
int loadItemFile(char *pathname, item **list){
    FILE *fp;
	char name[50], category[50], seller[MAX], topBuyer[MAX];
	int id, currentPrice, buyNowPrice, duration;

	(*list) = malloc(sizeof(item));
	if ((*list) == NULL) {
		free((*list));
		return -1;
	}

	fp = fopen(pathname, "r");
	if(fp == NULL) {
		return -1;
	}

	int index = 0;
	while(fscanf(fp, "%d %s %s %d %d %d %s %s", &id, name, category, &currentPrice, &buyNowPrice, &duration, seller, topBuyer) == 8) {
		(*list) = realloc((*list), (index+1) * sizeof(item));
        if ((*list) == NULL) {
			free((*list));
            return -1;
        }

		(*list)[index].id = id;
		strcpy((*list)[index].name, name);
		strcpy((*list)[index].category, category);
		(*list)[index].currentPrice = currentPrice;
		(*list)[index].buyNowPrice = buyNowPrice;
		(*list)[index].duration = duration;
		strcpy((*list)[index].seller, seller);	
		strcpy((*list)[index].topBuyer, topBuyer);

		index++;
	}

	fclose(fp);

	return index;

}

//	Salvas os items da lista no ficheiro.
//	Input: caminho, endereço da lista deos items e número de items.
//	Output: 0 = sucesso, -1 = erro.
int saveItemFile(char *pathname, item **list, int listLen){
	FILE *fp;

	fp = fopen(pathname, "w");
	if(fp == NULL) {
		return -1;
	}

	for(int i = 0; i < listLen; i++){
		fprintf(fp, "%d %s %s %d %d %d %s %s", (*list)[i].id, (*list)[i].name, (*list)[i].category, (*list)[i].currentPrice, (*list)[i].buyNowPrice, (*list)[i].duration, (*list)[i].seller, (*list)[i].topBuyer);
	}

	fclose(fp);

	return 0;
}

//	Rotina da comunicação com o user
void *tunnelUser(){	
	//	Verifica o tunel de comunicação
	int fdR = open(FIFO_BACKEND_FRONTED, O_RDONLY | O_NONBLOCK);
	if(fdR == -1){
		printf("\n<Servidor: Ocorreu um erro ao abrir o pipe FIFO_BACKEND_FRONTED>\n");
        return NULL;
	}
	do{
		int found = 0;
		int size = read(fdR, &user, sizeof(user));
		if(size > 0){
			//	Se encontrar o user na lista de users Ativos, alerta o `found`
			for(int i = 0; i < servidor.nUsersAtivos; i++){
				if(user.pid == servidor.users[i].pid){
					found = 1;
					break;
				}
			}

			//	Verifica se esse user já está na lista
			if(found == 0 && user.pid != 0){
				switch (isUserValid(user.username,user.password))
				{
				case -1:
					printf("\n<Servidor: Erro ao verificar de user exite>\n");
					break;
				
				case 0:
					// User não aceite
					printf("\n<Servidor: [PID %d] User: %s --> Não aceite por falta de validade>\n", user.pid, user.username);
					
					sprintf(FIFO_FINAL, FIFO_FRONTEND, user.pid); //Guarda no "FIFO_FINAL" o nome do pipe para onde queremos enviar as cenas
					int fd_envio = open(FIFO_FINAL, O_WRONLY);
					if(fd_envio == -1){
						printf("\n<Servidor: Ocorreu um erro ao abrir um pipe com o user com PID %d>\n", user.pid);
					}
					int size2 = write(fd_envio, CODE_MISSING, sizeof(CODE_MISSING));
					if(size2 == -1){
						printf("\n<Servidor: Ocorreu um erro ao enviar mensagem de estado ao user com PID %d>\n", user.pid);
					}
					break;

				case 1:
					// User aceite
					if(servidor.nUsersAtivos < servidor.nUsersMax){
						servidor.users[servidor.nUsersAtivos] = user;
						servidor.nUsersAtivos++;
						printf("\n[PID %d] User: %s\n", user.pid, user.username);
						fflush(stdout);

						sprintf(FIFO_FINAL, FIFO_FRONTEND, user.pid);
						user.cash = getUserBalance(user.username);

						int fd_envio = open(FIFO_FINAL, O_WRONLY);
						if(fd_envio == -1){
							printf("\n<Servidor: Ocorreu um erro ao abrir um pipe com o user com PID %d>\n", user.pid);
						}
						int size2 = write(fd_envio, CODE_VALIDE, sizeof(CODE_VALIDE));
						if(size2 == -1){
							printf("\n<Servidor: Ocorreu um erro ao enviar mensagem de estado ao user com PID %d>\n", user.pid);
						}
					} else {
						// User aceite, mas não tem espaço
						printf("\n<Servidor: [PID %d] User: %s --> Não aceite por limite>\n", user.pid, user.username);
						
						sprintf(FIFO_FINAL, FIFO_FRONTEND, user.pid);
						int fd_envio = open(FIFO_FINAL, O_WRONLY);
						if(fd_envio == -1){
							printf("\n<Servidor: Ocorreu um erro ao abrir um pipe com o user com PID %d>\n", user.pid);
						}
						int size2 = write(fd_envio, CODE_LIMIT, sizeof(CODE_LIMIT));
						if(size2 == -1){
							printf("\n<Servidor: Ocorreu um erro ao enviar mensagem de estado ao médico com PID %d>\n", user.pid);
						}
					}
					break;
				}	
			}
			
			//	Validação dos comandos
			else if(user.pid != 0){
				char buffer[MAX], cmd[6][10];
				int index;

				strcpy(buffer, "");
				printf("%s\n", user.command);

				index = sscanf(user.command, "%s %s %s %s %s %s", cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5]);


				if(!strcmp(cmd[0], "sell")){
					printf("%s %d\n",cmd[3], atoi(cmd[3]));
					if(index != 6 || atoi(cmd[2]) != 0 || atoi(cmd[3]) == 0 || atoi(cmd[4]) == 0 || atoi(cmd[5]) == 0)
						strcpy(buffer, CMD_MISSING);
					else{
						FILE *fp = fopen(itensPath, "a");
						int id = idCounter++;
						if (fp == NULL) {
							perror("Error opening file");
							break;
						}

						fprintf(fp, "%d %s %s %d %d %d %s %s\n", id, cmd[1], cmd[2], atoi(cmd[3]), atoi(cmd[4]), atoi(cmd[5]), user.username, "***");

						fclose(fp);

						numItens = loadItemFile(itensPath, &itemsList);
						printf("\nNúmero de itens carregados: %d\n", numItens);

						strcpy(buffer, "Item adicionado\n");
					}

				}else if(!strcmp(cmd[0], "list")){
					if(index != 1)
						strcpy(buffer, CMD_MISSING);
					else{
						strcpy(buffer, "Lista de items:\n");
						for(int i = 0; i < numItens; i++){
							strcat(buffer, "\t");
							strcat(buffer, itemsList[i].name);
							strcat(buffer, "\n");
						}
					}

				}else if(!strcmp(cmd[0], "licat")){
					if(index != 2 || atoi(cmd[1]) !=0)
						strcpy(buffer, CMD_MISSING);
					else{
						sprintf(buffer, "Lista de items da categora %s\n", cmd[1]);
						for(int i = 0; i < numItens; i++){
							if((strcmp(cmd[1], itemsList[i].category) == 0)){
								strcat(buffer, "\t");
								strcat(buffer, itemsList[i].name);
								strcat(buffer, "\n");
							}
						}
					}						

				}else if(!strcmp(cmd[0], "lisel")){
					if(index != 2 || atoi(cmd[1]) != 0)
						strcpy(buffer, CMD_MISSING);
					else {
						sprintf(buffer, "Lista de items do vendedor %s\n", cmd[1]);
						for(int i = 0; i < numItens; i++){
							if((strcmp(cmd[1], itemsList[i].seller) == 0)){
								strcat(buffer, "\t");
								strcat(buffer, itemsList[i].name);
								strcat(buffer, "\n");
							}
						}
					}
						
				}else if(!strcmp(cmd[0], "lival")){
					if(index != 2 || atoi(cmd[1]) == 0)
						strcpy(buffer, CMD_MISSING);
					else {
						sprintf(buffer, "Lista de items com o preço até %s\n", cmd[1]);
						for(int i = 0; i < numItens; i++){
							if(atoi(cmd[1]) <= itemsList[i].basePrice){
								strcat(buffer, "\t");
								strcat(buffer, itemsList[i].name);
								strcat(buffer, "\n");
							}
						}
					}
						

				}else if(!strcmp(cmd[0], "litime")){
					if(index != 2 || atoi(cmd[1]) == 0)
						strcpy(buffer, CMD_MISSING);
					else {
						sprintf(buffer, "Lista de items com a hora até %s\n", cmd[1]);
						for(int i = 0; i < numItens; i++){
							if(atoi(cmd[1]) <= itemsList[i].duration){
								strcat(buffer, "\t");
								strcat(buffer, itemsList[i].name);
								strcat(buffer, "\n");
							}
						}
					}

				}else if(!strcmp(cmd[0], "time")){
					if(index != 1)
						strcpy(buffer, CMD_MISSING);
					else {
						sprintf(buffer, "%d segunos\n", servidor.time);
					}

				}else if(!strcmp(cmd[0], "buy")){
					if(index  != 3 || atoi(cmd[1]) == 0 || atoi(cmd[2]) == 0)
						strcpy(buffer, CMD_MISSING);
					else {
						for(int i = 0; i < numItens; i++){
							if(itemsList[i].id == atoi(cmd[1])){
								if(itemsList[i].currentPrice <= atoi(cmd[2])){
									itemsList[i].currentPrice = atoi(cmd[2]);
									strcpy(itemsList[i].topBuyer, user.username);

									sprintf(buffer, "O Item %s (ID: %d) está em seu nome\n", itemsList[i].name, itemsList[i].id);

								}else {
									strcpy(buffer, "O valor intrudizo é menor que o valor atual\n");
								}
							}else {
								strcpy(buffer, "Não foi encontrado nenhum item com esse id\n");
							}
						}
					}
						

				}else if(!strcmp(cmd[0], "cash")){
					if(index  != 2)
						strcpy(buffer, CMD_MISSING);
					else {
						for(int i = 0; i < servidor.nUsersAtivos; i++){
							if(servidor.users[i].pid == user.pid){
								sprintf(buffer, "%d\n", user.cash);
							}
						}
					}
						
				}else if(!strcmp(cmd[0], "add")){
					if(index != 2 || atoi(cmd[1]) == 0)
						strcpy(buffer, CMD_MISSING);
					else {
						for(int i = 0; i < servidor.nUsersAtivos; i++){
							if(servidor.users[i].pid == user.pid){
								user.cash += atoi(cmd[1]);
								switch (updateUserBalance(servidor.users[i].username, atoi(cmd[1])))
								{
									case -1:
										sprintf(buffer, "Erro ao obter saldo do user\n");
										break;
									
									case 0:
										sprintf(buffer, "utilizador não encontrado\n");
										break;

									case 1:
										sprintf(buffer, "Saldo atualizado\n");
										break;
								}
							}
						}
					}	

				}else if(strcmp(user.command, "exit") == 0){
					for(int i = 0; i < servidor.nUsersAtivos; i++){
						if(user.pid == servidor.users[i].pid){
							for (int l = i; l < servidor.nUsersAtivos - 1; l++) {
								servidor.users[l] = servidor.users[l + 1];
							}
							servidor.nUsersAtivos--;
						}
					}
					sprintf(buffer, "Saldo atualizado\n");

				}else {
					strcpy(buffer, "Comando inextente\n");
				}

				int fd_envio = open(FIFO_FINAL, O_WRONLY);
				if(fd_envio == -1){
					printf("\n<Servidor: Ocorreu um erro ao abrir um pipe com o user com PID %d>\n", user.pid);
				}
				int size2 = write(fd_envio, buffer, strlen(buffer));
				if(size2 == -1){
					printf("\n<Servidor: Ocorreu um erro ao enviar mensagem de estado ao médico com PID %d>\n", user.pid);
				}
			}
		}

		if(servidor.time % servidor.heartBeat == 0){
			for(int i = 0; i < servidor.nUsersAtivos; i++){
				sprintf(FIFO_FINAL, FIFO_FRONTEND, servidor.users[i].pid);

				int fd_envio = open(FIFO_FINAL, O_WRONLY);
				if(fd_envio == -1){
					for (int l = i; l < servidor.nUsersAtivos - 1; l++) {
						servidor.users[l] = servidor.users[l + 1];
					}
					servidor.nUsersAtivos--;
				}
			}
		}
	} while(1);

	//	Quando é para acabar com a rotina, salvas as informações nas suas pastas e limpa a lista dinâmica
	if(saveUsersFile(filePath) == -1)
		printf("<Servido: Erro ao salvar ficheiro dos users>\n");

	if(saveItemFile(itensPath, &itemsList, numItens) == -1)
		printf("<Servidor: Erro ao salvar ficheiro dos items>\n");

	free(itemsList);
}

//TODO: ver se é preciso promos ou se é o promo a fazer esta parte de modificar
void *tunnelPromo(){

}

void *timerItem(){
	int itemTimer = 0;
	char buffer[MAX];

	while(1){
		sleep(1);
		servidor.time++;
		if(itemTimer == itemsList[0].duration){
			itemTimer = 0;
			for (int l = 0; l < numItens; l++) {
				itemsList[l] = itemsList[l + 1];
			}
			sprintf(buffer, "Acabou o leilão do item\nNovo item: %s com valor %d", itemsList[0].name, itemsList[0].basePrice);
			int fd_envio = open(FIFO_FINAL, O_WRONLY);
			if(fd_envio == -1){
				printf("\n<Servidor: Ocorreu um erro ao abrir um pipe com o user com PID %d>\n", user.pid);
			}
			int size2 = write(fd_envio, buffer, strlen(buffer));
			if(size2 == -1){
				printf("\n<Servidor: Ocorreu um erro ao enviar mensagem de estado ao médico com PID %d>\n", user.pid);
			}
			numItens--;
		} else {
			itemTimer++;
		}
	}
}

//	Rotina da consola do admin
void *consoleAdmin(){
	char *fitens_env = getenv("FITEMS");
	char filePath[MAX], itensPath[MAX], input[MAX], cmd[2][20];
	int index;

	//	Lê o ficheiro dos itens
	sprintf(itensPath, "../files/%s", fitens_env);
	numItens = loadItemFile(itensPath, &itemsList);

	sleep(1);
	while(1)
	{
		printf("<Admin> ");
		fgets(input, MAX, stdin);
		index = sscanf(input, "%s %s", cmd[0], cmd[1]);

		if(!strcmp(cmd[0], "users")){
			if(index != 1)
				printf(CMD_MISSING);
			else{
				if(servidor.nUsersAtivos == 0){
					printf("\n<Servidor> Nenhum user ligado ao servidor\n");
				}	
					
				else {
					printf("Users ativos: %d\n", servidor.nUsersAtivos);
					for(int i = 0; i < servidor.nUsersAtivos; i++){
						printf("\t→ %s\n", servidor.users[i].username);
					}
				}
			}	

		} else if (!strcmp(cmd[0], "list")){
			if(index != 1)
				printf(CMD_MISSING);
			else
				for(int i = 0; i < numItens; i++){
					printf("\t→ %s\n", itemsList[i].name);
				}
				
		} else if(!strcmp(cmd[0], "kick")){
			if(index != 2)
				printf(CMD_MISSING);
			else {
				for(int i = 0; i < servidor.nUsersAtivos; i++){
					if(strcmp(servidor.users[i].username, cmd[1]) == 0){
						kill(servidor.users[i].pid, SIGINT);
						
					}
					for (int l = i; l < servidor.nUsersAtivos - 1; l++) {
						servidor.users[l] = servidor.users[l + 1];
					}
					servidor.nUsersAtivos--;
					break;
				}
			}

		} else if(!strcmp(cmd[0], "prom")){
			if(index != 1)
				printf(CMD_MISSING);
			else
				printf("prom...\n");

		} else if(!strcmp(cmd[0], "reprom")){ 
			if(index != 1)
				printf(CMD_MISSING);			
			else
				printf("reprom...\n");

		} else if(!strcmp(cmd[0], "cancel")){
			if(index != 1)
				printf(CMD_MISSING);
			else
				printf("cancel...\n");

		} else if(!strcmp(cmd[0], "close")){
			if(index != 1)
				printf(CMD_MISSING);
			else
				printf("<Servidor> A fechar o servidor. A fechar ligação com os users e a guardas as informações...\n");
				break;

		} else
			printf("<Servidor: Comando inextente>\n");
	}

	for(int i = 0; i < servidor.nUsersAtivos; i++){
		kill(servidor.users[i].pid, SIGINT);
	}

	return NULL;
}

int main(int argc, char *argv[]){
	printf("\033[2J\033[1;1H");
	printf("  ___           _____ _  ________ _   _ _____ \n");
	printf(" |  _ \\   /\\   / ____| |/ /  ____| \\ | |  __ \\\n");
	printf(" | |_) | /  \\ | |    | ' /| |__  |  \\| | |  | |\n");
	printf(" |  _ < / /\\ \\| |    |  < |  __| | . ` | |  | |\n");
	printf(" | |_) / ____ \\ |____| . \\| |____| |\\  | |__| |\n");
	printf(" |____/_/    \\_\\_____|_|\\_\\______|_| \\_|_____/\n");
    printf("\nBem vindom ao SOBay, Administrador\n");
    fflush(stdout);

	// Bucas e armazena as variáveis ambiente
	char *user_env = getenv("MAXUSERS");
    char *promo_env = getenv("MAXPROMOS");
	char *itens_venda_env = getenv("MAXITENSVENDA");
	char *fpromo_env = getenv("FPROMOTERS");
	char *fusers_env = getenv("FUSERS");
	char *fitems_env = getenv("FITEMS");
	char *heartbeat_env = getenv("HEARTBEAT");

	//  Verifaicar se já existe um backend aberto
	if(backendAberto()){
		printf("Ja existe um backend aberto\n");
		return 0;
	}

	//	Valida as variáveis ambiente
	if(user_env == NULL || promo_env == NULL || itens_venda_env == NULL || fpromo_env == NULL || fusers_env == NULL || fitems_env == NULL || heartbeat_env == NULL){
        printf("\nAs variáveis de ambiente não estão definidas.\n");
        return 0;
	}

	int maxUseres = atoi(user_env);
    int maxPromos= atoi(promo_env);
	int maxItens = atoi(itens_venda_env);
	int	heartBeat = atoi(heartbeat_env);

	//	Verifica os valores das variaveis ambientes numéricas
    if(maxUseres < 1 || maxPromos < 1 || maxItens < 1 || heartBeat < 1){
        printf("\nAs variáveis de ambiente estão mal definidas.\n");
        return 0;
    }

	//	Configura o limite de users e promotores no backend
	servidor.nUsersMax = maxUseres;
	servidor.nPromosMax = maxPromos;
	servidor.heartBeat = heartBeat;
	servidor.time = 0;
	fflush(stdin);
    fflush(stdout);

	//	Lê o ficheiro dos users
	sprintf(filePath, "../files/%s", fusers_env);
	numUsers = loadUsersFile(filePath);

	//	Lê o ficheiro dos itens
	sprintf(itensPath, "../files/%s", fitems_env);
	numItens = loadItemFile(itensPath, &itemsList);
	printf("\nNúmero de itens carregados: %d\n", numItens);

	//	Criação dos ficheiros fifos
	if (mkfifo(FIFO_BACKEND, 0666) == -1) {
		printf("\n<Servidor: Ocorreu um erro ao criar o pipe FIFO_BACKEND>\n");
		return 0;
	}
	if (mkfifo(FIFO_BACKEND_FRONTED, 0666) == -1) {
		printf("\n<Servidor: Ocorreu um erro ao criar o pipe FIFO_BACKEND_FRONTED>\n");
		return 0;
	}

	// Criação das threads
	if(pthread_create(&thread_id, NULL, tunnelUser, NULL)){
		printf("\n<Servidor: Ocorreu um erro ao criar a thread aceitasUsers>\n");
        unlink(FIFO_BACKEND);
		unlink(FIFO_BACKEND_FRONTED);
        return 0;
    }

	/*if(pthread_create(&thread_id, NULL, tunnelPromo, NULL)){
		printf("\n<Servidor: Ocorreu um erro ao criar a thread tunnelPromo>\n");
        unlink(FIFO_BACKEND);
		unlink(FIFO_BACKEND_FRONTED);
        return 0;
    }*/

	if(pthread_create(&thread_id, NULL, timerItem, NULL)){
		printf("\n<Servidor: Ocorreu um erro ao criar a thread timerItem>\n");
        unlink(FIFO_BACKEND);
		unlink(FIFO_BACKEND_FRONTED);
        return 0;
    }

	if(pthread_create(&thread_id, NULL, consoleAdmin, NULL)){
		printf("\n<Servidor: Ocorreu um erro ao criar a thread consolaAdministrador>\n");
        unlink(FIFO_BACKEND);
		unlink(FIFO_BACKEND_FRONTED);
        return 0;
    }
	pthread_join(thread_id, NULL);

	//	Apagar os fifos
	unlink(FIFO_BACKEND);
	unlink(FIFO_BACKEND_FRONTED);

	printf("<Servidor> Servidor fechado com sucesso\n");

	return 0;
}
