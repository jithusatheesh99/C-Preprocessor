mypreprocessor: main.o comment_removal.o macro_handler.o file_inclusion.o
	gcc -o mypreprocessor main.o comment_removal.o macro_handler.o file_inclusion.o

main.o: main.c file_inclusion.h
	gcc -Wall -c main.c

comment_removal.o: comment_removal.c comment_removal.h common.h
	gcc -Wall -c comment_removal.c

macro_handler.o: macro_handler.c macro_handler.h common.h
	gcc -Wall -c macro_handler.c

file_inclusion.o: file_inclusion.c file_inclusion.h comment_removal.h macro_handler.h common.h
	gcc -Wall -c file_inclusion.c

clean:
	rm -f *.o mypreprocessor
