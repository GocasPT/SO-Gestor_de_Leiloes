#ifndef BACKEND
#define BACKEND

#define MAX 100

#include "../frontend/frontend.h"
#include "../promotor/promotor.h"

typedef struct Backend
{
  int time, heartBeat;
  int nUsersMax, nPromosMax;
  int nUsersAtivos, nPromosAtivos;
  struct Frontend users[MAX];
  struct Promotor promos[MAX];
} server;

#endif //BACKEND