selvaggi:
	gcc -Wall -Wextra -pedantic -pthread -lrt -o selvaggi selvaggi.o
selvaggi.o:
	gcc -Wall -Wextra -pedantic -pthread -lrt -c selvaggi.c
clean:
	rm selvaggi
valgrind: 
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./selvaggi 5 5 5