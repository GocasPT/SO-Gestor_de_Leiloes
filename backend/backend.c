#include "backend.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char **getCommad(char *str, int *index){
  char **argv = malloc(sizeof(char *));
  char s[2] = " ";
  char *tmp;
  int i = 0;

  tmp = strtok(str, s);
  tmp[strcspn(tmp, "\n")] = 0;

  while( tmp != NULL ) {
    argv = realloc(argv, (i + 1) * sizeof(char *));
    argv[i] = tmp; 
    tmp = strtok(NULL, s);
    i++;
  }

  *index = i - 1;
  return argv;
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

	while(1){
		printf("<Admin> ");
		fgets(input, 50, stdin);
		cmd = getCommad(input, &index);

		if(!strcmp(cmd[0], "users")){
			printf("users...\n");

		} else if (!strcmp(cmd[0], "list")){
			printf("list...\n");

		} else if(!strcmp(cmd[0], "kick")){
			if(index != 5)
				printf("$ Parametos incompletos\n");
			else
				printf("kick...\n");

		} else if(!strcmp(cmd[0], "prom")){
			printf("prom...\n");

		} else if(!strcmp(cmd[0], "reprom")){ 
			printf("reprom...\n");

		} else if(!strcmp(cmd[0], "cancel")){
			if(index != 5)
				printf("$ Parametos incompletos\n");
			else
				printf("cancel...\n");

		} else if(!strcmp(cmd[0], "close")){
			printf("close...\n");
			exit(1);

		} else
			printf("$ Comando inextente\n");
	}

	return 0;
}
