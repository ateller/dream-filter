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
#define IS_BAD(condition)\
	do { \
		if (condition) { \
			printf("Error: corrupted file\n");\
			exit(EXIT_FAILURE);\
		}\
	} while (0)
#endif
//Макрос, чтобы писать, что файл плохой

//Чтобы читать число от 0 до
//65535 в буфер, размером 6 
//байт из файла
int read_until_space(int input, char* buf, char space, char i)
{
	int value;
	char check;
	
	//Начинаем с того, что дадут
	for(; i < 6; i++)
	{
		check = read(input, buf + i, 1);
		//Читаем очередной символ
		IF_ERR(check, -1, "Read error", exit(errno););
		IS_BAD(!check);
		if(buf[i] == space) 
			break;
		//Пробел - число кончилось
		IS_BAD(!isdigit(buf[i]));
	}
	IS_BAD(i == 6);
	buf[i] = 0;
	value = atoi(buf);
	//Строку в число
	IS_BAD(value > 65535);
	//Это самые большие параметры, которые
	//я буду поддерживать
	return value;
}

void filter_p6(int input)
{
	char buf[6], check, max;
	int x, y, i;
	short **rgb[3];
	char l;
	check = read(input, buf, 1);
	IF_ERR(check, -1, "Read error", exit(errno););
	IS_BAD(buf[0] != '\n' || !check);
	//После строки формата должен быть
	//перевод строки
	check = read(input, buf + i, 1);
	IF_ERR(check, -1, "Read error", exit(errno););
	//На следующей строке может быть комментарий
	if (buf[0] == '#') {
		do {
			check = read(input, buf + i, 1);
			IF_ERR(check, -1, "Read error", exit(errno););
			IS_BAD(!check);
		} while (buf[0] != '\n');
		//Пропустили комментарий
		x = read_until_space(input, buf, ' ', 0);
		//Читаем количество столбцов
	} else 
		x = read_until_space(input, buf, ' ', 1);
	//Если комментария не было, то один
	//символ из разрешения у нас уже
	//считан
	y = read_until_space(input, buf, '\n', 0);
	//Читаем количество строк
	max = read_until_space(input, buf, '\n', 0);
	x += 2;
	y += 2;
	//Чтобы с рамкой в 1 пиксель
	for (i = 0; i < 3; i++) {
			rgb[i] = malloc(2 * y));
			for (int j = 0; j < y; j--)
				rgb[i][j] = malloc(2 * x);  
	}
	//Выделим память под матрицы
	for (i = 0; i < 3; i++)
		for (int j = 0; j < y; j++)
			rgb[i][j][0] = 0;
			rgb[i][j][x] = 0;
		for (int j = 1; j < (x - 1); j++)
			rgb[i][0][j] = 0;
			rgb[i][y][j] = 0;
	//Сделаем рамку из 0
	//толщиной в 1 пиксель
	
}

int main(int argc, char *argv[])
{
	int input;
	char buf[2];
	if (argc == 1) {
		printf("Error: there is no arguments\n");
		exit(EXIT_FAILURE);
	}
	//Проверяем а есть ли аргументы
	input = open(argv[1], O_RDONLY);
	IF_ERR(input, -1, argv[1], exit(errno););
	//Открываем файл, если он есть
	IF_ERR(read(input, buf, 2), -1, "Read error", exit(errno););
	//Читаем строку формата
	IS_BAD(buf[0] != 'P' || buf[1] < '1' || buf[1] > '6');
	//Проверяем, а строка ли формата 
	//это вообще
	switch(buf[1]) {
	//Смотрим, что за формат и умеем
	//ли мы с ним работать
	case '6':
		filter_p6(input);
		break;
	default:
		printf("Sorry, this kind of ppm is not supported yet\n");
		exit(EXIT_SUCCESS);
	}
}
