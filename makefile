all:
	gcc ./backend/backend.c -o ./backend/backend
	gcc ./frontend/frontend.c -o ./frontend/frontend

back:
	gcc ./backend/backend.c -o ./backend/backend

front:
	gcc ./frontend/frontend.c -o ./frontend/frontend

clean:
	rm -f programa *.o
