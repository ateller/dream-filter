#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

//Я не настолько хорошо знаю английский,
//чтобы писать на нем комментарии и не
//позориться, а исправлять перевод
//после гугла мне сейчас лень.

#ifndef IF_ERR
#define IF_ERR(check, error, info, handle)\
	do { \
		if (check == error) { \
			perror(info);\
			handle\
		} \
	} while (0)
#endif
//Макрос для обработки ошибок

int isfile(char *path)
{
	struct stat temp;
	
	if (stat(path, &temp) == -1)
		return 0;
	if (!S_ISREG(temp.st_mode))
		return 0;	//не файл
	return 1;
}

int main(int argc, char *argv[])
{
	if (argc == 1) {
		printf("Error: there is no arguments\n");
		exit(EXIT_FAILURE);
	}
	if (!isfile(argv[1])) {
		printf("Error, %s is not file\n", argv[1]);
		exit(EXIT_FAILURE);
	}
	
}
