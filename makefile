BACKEND = backend
FRONTEND = frontend
PROMO = promotor
C = gcc

all:
	@$(C) ./${BACKEND}/${BACKEND}.c -o ./${BACKEND}/${BACKEND} ./user_lib/users_lib.o
	@$(C) ./${FRONTEND}/${FRONTEND}.c -o ./${FRONTEND}/${FRONTEND}

back:
	@$(C) ./${BACKEND}/${BACKEND}.c -o ./${BACKEND}/${BACKEND} ./user_lib/users_lib.o

front:
	@$(C) ./${FRONTEND}/${FRONTEND}.c -o ./${FRONTEND}/${FRONTEND}

clean:
	rm ./${BACKEND}/${BACKEND} ./${FRONTEND}/${FRONTEND}
