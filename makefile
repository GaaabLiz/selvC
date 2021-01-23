selvaggi:
	gcc selvaggi.c -Wall -Wextra -pedantic -pthread -lrt -o selvaggi 
clean:
	rm selvaggi
valgrind: 
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./selvaggi 5 5 5