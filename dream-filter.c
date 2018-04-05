#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

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

#ifndef IS_BAD
#define IS_BAD (condition)\
	do { \
		if (condition) { \
			printf("Error: corrupted file\n");\
			exit(EXIT_FAILURE);\
		}\
	} while (0)
#endif
//Макрос, чтобы писать, что файл плохой

void filter_p6(int input)
{
	char buf[6], check;
	int x, y, i;
	check = read(input, buf, 1);
	IF_ERR(check, -1, "Read error", exit(errno););
	IS_BAD(buf[0] != '\n' || !check);
	for(i = 0; i < 6; i++)
	{
		check = read(input, buf + i, 1);
		IF_ERR(check, -1, "Read error", exit(errno););
		IS_BAD(!check);
		if(buf == ' ') 
			break;
		IS_BAD(!isdigit(buf[i]));
	}
	IS_BAD(!isdigit(i == 6);
	buf[i] = 0;
	x = atoi(buf);
	IS_BAD(x > 65535);
	for(i = 0; i < 6; i++)
	{
		check = read(input, buf + i, 1);
		IF_ERR(check, -1, "Read error", exit(errno););
		IS_BAD(!check);
		if(buf == '\n') 
			break;
		IS_BAD(!isdigit(buf[i]));
	}
	IS_BAD(!isdigit(i == 6);
	buf[i] = 0;
	y = atoi(buf);
	IS_BAD(y > 65535);
}

int main(int argc, char *argv[])
{
	int input;
	char buf[2];
	if (argc == 1) {
		printf("Error: there is no arguments\n");
		exit(EXIT_FAILURE);
	}
	input = open(argv[1], O_RDONLY);
	IF_ERR(input, -1, argv[1], exit(errno););
	IF_ERR(read(input, buf, 2), -1, "Read error", exit(errno););
	IS_BAD(buf[0] != 'P' || buf[1] < '1' || buf[1] > '6');
	switch(buf[1]) {
	case '6':
		filter_p6(input);
		break;
	default:
		printf("Sorry, this kind of ppm is not supported yet\n");
		exit(EXIT_SUCCESS);
	}
}
