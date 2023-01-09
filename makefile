BACKEND = backend
FRONTEND = frontend
PROMO = promotor
LIB = users_lib
C = gcc

all:
	@$(C) ./${BACKEND}/${BACKEND}.c -o ./${BACKEND}/${BACKEND} ./${LIB}/${LIB}.o -pthread
	@$(C) ./${FRONTEND}/${FRONTEND}.c -o ./${FRONTEND}/${FRONTEND} -pthread

back:
	@$(C) ./${BACKEND}/${BACKEND}.c -o ./${BACKEND}/${BACKEND} ./${LIB}/${LIB}.o -pthread

front:
	@$(C) ./${FRONTEND}/${FRONTEND}.c -o ./${FRONTEND}/${FRONTEND} -pthread

clean:
	rm ./${BACKEND}/${BACKEND} ./${FRONTEND}/${FRONTEND}
