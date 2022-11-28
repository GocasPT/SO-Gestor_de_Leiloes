#ifndef FRONTEND
#define FRONTEND
#define MAX 100

typedef struct Frontend{
  int pid;
  char user[MAX];
  int pipeUser;
}user, *user_ptr;

#endif //FRONTEND