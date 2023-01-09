#ifndef FRONTEND
#define FRONTEND

#define MAX 100

typedef struct Frontend
{
  int pid;
  int cash;
  char username[MAX];
  char password[MAX];
  char pipeUser[MAX];
  char command[MAX];
} cliente;

#endif //FRONTEND