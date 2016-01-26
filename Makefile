CC=gcc

FLAGS = -Werror -Wall -pedantic -g 
ALL: tests

tests: tests.o mymallib.a
	$(CC) $(FLAGS) tests.o mymallib.a -o tests 

mymallib.a: mymal.o 
	ar cr mymallib.a mymal.o

tests.o: tests.c
	$(CC) $(FLAGS) -c tests.c

mymal.o: mymal.c mymal.h
	$(CC) $(FLAGS) -c mymal.c

clean:
	rm -rf *.o tests


