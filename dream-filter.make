CC=gcc
CFLAGS=-fsanitize=address -g -lm

dream-archiver: dream-filter.c
	$(CC) -o dream-filter dream-filter.c $(CFLAGS)
clean: 
	-rm dream-filter
