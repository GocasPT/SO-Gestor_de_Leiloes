#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "../user_lib/users_lib.h"
#include "backend.h"

void clean_stdin(void)
{
    int c;
    do {
        c = getchar();
    } while (c != '\n' && c != EOF);
}

char **getCommad(char *str, int *index){
  char **argv = malloc(sizeof(char *));
  char s[2] = " ";
  char *tmp;
  int i = 0;

  str[strcspn(str, "\n")] = 0;  //  remover \n no final da string
  tmp = strtok(str, s); //  divir string por tokens com char DELIM

  while( tmp != NULL ) {
    argv = realloc(argv, (i + 1) * sizeof(char *));
    argv[i] = tmp; 
    tmp = strtok(NULL, s);
    i++;
  }

  *index = i - 1;
  return argv;
}

int checkString(char *str){
  int i = 0 ;

  while(i < strlen(str)){
    if(isdigit(str[i]))
      return 1;
    i++;
  }

  return 0;
}

int checkInt(char *str){
  int i = 0 ;

  while(i < strlen(str)){
    if(!isdigit(str[i]))
      return 1;
    i++;
  }

  return 0;
}

int loadItemFile(char * pathname, char **array){
    FILE * fp;
	char **argv = malloc(sizeof(char **));
    char **str = malloc(sizeof(char *));
    char *tmp, *item;
	char s[2] = " ", n[2]="\n";
	int nBytes, index = 0;

	fp = fopen(pathname, "r");

	if(fp == NULL) {
		return -1;
	}

	fseek(fp, 0L, SEEK_END);
    nBytes = ftell(fp);
    fseek(fp, 0L, SEEK_SET);  
 
    tmp = (char*)calloc(nBytes, sizeof(char));   
    if(tmp == NULL)
        return -1;
 
    fread(tmp, sizeof(char), nBytes, fp);

	fclose(fp);

	item = strtok(tmp, n); //  divir string por tokens com char DELIM

	while( item != NULL ) {
		str = realloc(str, (index + 1) * sizeof(char *));
		str[index] = item; 
		item = strtok(NULL, n);
		index++;
	}

	argv = realloc(argv, (index + 1) * sizeof(char **));

	for(int j = 0; j  < index; j++){
		argv[j] = str[j];
	}

	array = malloc(sizeof(argv));

	array = argv;

	return index;
}

int main(int argc, char *argv[]){
	printf("\033[2J\033[1;1H");
	printf("  ___           _____ _  ________ _   _ _____ \n");
	printf(" |  _ \\   /\\   / ____| |/ /  ____| \\ | |  __ \\\n");
	printf(" | |_) | /  \\ | |    | ' /| |__  |  \\| | |  | |\n");
	printf(" |  _ < / /\\ \\| |    |  < |  __| | . ` | |  | |\n");
	printf(" | |_) / ____ \\ |____| . \\| |____| |\\  | |__| |\n");
	printf(" |____/_/    \\_\\_____|_|\\_\\______|_| \\_|_____/\n");
    printf("\nBem vindo, Administrador\n");
    fflush(stdout);

	//  Verifaicar se já existe um backend aberto
	if(0){
		printf("Ja existe um backend aberto\n");
		exit(1);
	}

	//  Receber credencias do frontend
	

	//  Leitura de comandos
	char input[MAX];
	char **cmd;
	int index;

	while(1)
	{
		int option;

		printf("Qual das funcionalidades para testar?\n\t1 - comandos\n\t2 - promotores\n\t3 - utilizadores\n\t4 - itens\n\t5 - sair\n");
		scanf("%d", &option);
		switch (option)
		{
		case 1:	//	Commandos
			clean_stdin();
			while(1)
			{
				printf("<Admin> ");
				fgets(input, 50, stdin);
				cmd = getCommad(input, &index);

				if(!strcmp(cmd[0], "users")){
					printf("users...\n");

				} else if (!strcmp(cmd[0], "list")){
					printf("list...\n");

				} else if(!strcmp(cmd[0], "kick")){
					if(index != 5 || checkString(cmd[1]))
						printf("$ Parametos incompletos\n");
					else
						printf("kick...\n");

				} else if(!strcmp(cmd[0], "prom")){
					printf("prom...\n");

				} else if(!strcmp(cmd[0], "reprom")){ 
					printf("reprom...\n");

				} else if(!strcmp(cmd[0], "cancel")){
					if(index != 5 || checkString(cmd[1]))
						printf("$ Parametos incompletos\n");
					else
						printf("cancel...\n");

				} else if(!strcmp(cmd[0], "close")){
					printf("close...\n\n");
					//exit(1);
					break;

				} else
					printf("$ Comando inextente\n");
			}
			break;
		
		case 2:	//	Promotores
			printf("$ Promotores\n\n");
			break;

		case 3:	//	Utilizadores
			clean_stdin();
			int numUsers, saldo;

			numUsers = loadUsersFile("files/FUSERS.txt");

			if(numUsers == -1){
				printf("$ Erro ao abrir ficheiro dos users\n");
				break;

			}
				
			printf("\n\tNum Users =  %d\n",numUsers);

			printf("<Admin> ");
			fgets(input, 50, stdin);
			cmd = getCommad(input, &index);

			if(index != 1){
				printf("$ Parametos incompletos\n");
				break;
			}

			saldo = getUserBalance(cmd[0]);
			printf("\tSaldo do User: %d\n", saldo);
			
			switch (isUserValid(cmd[0],cmd[1]))
			{
			case -1:
				printf("$ Erro ao verificar de user exite\n");
				break;
			
			case 0:
				printf("$ O utilizador não exite ou password invalida\n");
				break;

			case 1:
				printf("$ O utilizador exite e a password esta correta\n");
				break;
			}

			switch (updateUserBalance(cmd[0],saldo-1))
			{
			case -1:
				printf("$  Erro ao obter saldo do user\n");
				break;
			
			case 0:
				printf("$ O utilizador não encontrado\n");
				break;

			case 1:
				printf("$ Saldo atualizado\n");
				break;
			}

			saldo = getUserBalance(cmd[0]);
			printf("\tSaldo do User: %d\n", saldo);

			if(saveUsersFile("files/FUSERS.txt") == -1)
				printf("$ Erro ao salvar\n");
			
			printf("\n");

			break;

		case 4:	//	Itens
			clean_stdin();
			int numItems, index;
			char **itemsList;
			
			numItems = loadItemFile("files/FITEMS.txt", itemsList);
			
			if(numItems ==-1){
				printf("$ Erro ao abrir ficheiro dos itens\n");
				break;
			}
				

			printf("\tNumero de itens: %d\n", numItems);


			for(int i = 0; i < numItems; i++){
				printf("%s\n", itemsList[i]);
			}
			
			break;

		case 5: // Sair
			printf("Backend a fechar\n");
			exit(1);
		}
	}

	free(cmd);

	return 0;
}
