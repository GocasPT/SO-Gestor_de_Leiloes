#ifndef BACKEND
#define BACKEND

#define MAX 100

#include <stdio.h>
#include "../frontend/frontend.h"
#include "../promotor/promotor.h"

typedef struct BACKENDs
{
  int nUsersMax, nPromosMax;
  int nUsersAtivos, nPromosAtivos;
  struct Frontend users[MAX];
  struct Promotor promos[MAX];
}server, *server_prt;

#endif //BACKEND