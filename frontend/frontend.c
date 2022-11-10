#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[]){

	if(argc < 2) {
        printf("Precisa de loggin\n");
        exit(1);
    } else if (argc != 3) {
		printf("Nao foi incerido password\n");
        exit(1);
	}

	return 0;
}
