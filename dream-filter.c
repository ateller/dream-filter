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
#include <math.h>

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

#define convolution * m[i + gi - 1][j + gj - 1]

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

void sobel(short** m, int x, int y, int max)
{
	short new_max = 0, op[3][3] = {{-1, -2, -1},{0, 0, 0},{1, 2, 1}};
	
	for (int i = 1; i < y; i++)
		for (int j = 1; j < x; j++) {
			int gx = 0, gy = 0, g;
			for (int gi = 0; gi < 3; gi++)
				for (int gj = 0; gj < 3; gj++) {
					gy += op[gi][gj] convolution;
					gx += op[gj][gi] convolution;
				//Сворачиваем по x и по y
				}
			g = sqrt((double)(gx * gx + gy * gy));
			//Находим итог
			if (g > new_max)
				new_max = g;
			//В итоге яркость может оказаться больше,
			//Чем максимальная, которая была
			m[i][j] = g;
		}
	if (new_max > max)
	//Если яркость оказалась больше, чем надо
	//убавим
		for (int i = 1; i < y; i++)
			for (int j = 1; j < x; j++)
				m[i][j] = (((float) m[i][j]) / new_max) * max;
}

void filter_p6(int input, char* input_file)
{
	char buf[6], check, max, *out_file, strx[6], stry[6];
	int x, y, i, out;
	short **rgb[3];
	char pix_size;
	check = read(input, buf, 1);
	IF_ERR(check, -1, "Read error", exit(errno););
	IS_BAD(buf[0] != '\n' || !check);
	//После строки формата должен быть
	//перевод строки
	check = read(input, strx, 1);
	IF_ERR(check, -1, "Read error", exit(errno););
	//На следующей строке может быть комментарий
	if (strx[0] == '#') {
		do {
			check = read(input, strx + i, 1);
			IF_ERR(check, -1, "Read error", exit(errno););
			IS_BAD(!check);
		} while (strx[0] != '\n');
		//Пропустили комментарий
		x = read_until_space(input, strx, ' ', 0);
		//Читаем количество столбцов
	} else 
		x = read_until_space(input, strx, ' ', 1);
	//Если комментария не было, то один
	//символ из разрешения у нас уже
	//считан
	y = read_until_space(input, stry, '\n', 0);
	//Читаем количество строк
	max = read_until_space(input, buf, '\n', 0);
	//Строковые представления разрешения
	//сохраним, нам их на выход писать потом
	x += 2;
	y += 2;
	//Чтобы с рамкой в 1 пиксель
	for (i = 0; i < 3; i++) {
		rgb[i] = (short**) malloc(sizeof(short*) * y);
		for (int j = 0; j < y; j++)
			rgb[i][j] = malloc(2 * x);  
	}
	//Выделим память под матрицы
	x--;
	y--;
	//Так удобнее
	for (i = 0; i < 3; i++) {
		for (int j = 0; j <= y; j++) {
			rgb[i][j][0] = 0;
			rgb[i][j][x] = 0;
		}
		for (int j = 1; j < x; j++) {
			rgb[i][0][j] = 0;
			rgb[i][y][j] = 0;
		}
	}
	//Сделаем рамку из 0
	//толщиной в 1 пиксель
	if (max <= 255) 
		pix_size = 1;
	else pix_size = 2;
	//Предположительно(я так думаю),
	//если макс. яркость меньше
	//255, будет 1 байт на пиксель,
	//А если от 255 до 65535
	//То 2 байта на пиксель 
	for (i = 1; i < y; i++)
		for(int j = 1; j < x; j++)
			for(int col = 0; col < 3; col++) {
				if (pix_size == 1)
					rgb[col][i][j] = 0;
				//Размер элемента матрицы - 2 байта
				//если считывается 1, то в остальном
				//байте элемента остается лежать то
				//что там было (незвестно что)
				//поэтому сначала обнуляем
				check = read(input, rgb[col][i] + j, pix_size);
				if (check < 1) 
					rgb[col][i][j] = 0;
				//В принципе, в файле может быть
				//меньше пикселей, чем обещано
				//но это не должно стать преградой
	}
	for (i = 0; i < 3; i++)
		sobel(rgb[i], x, y, max);
	//Фильтруем
	out_file = malloc(strlen(input_file) + 5);
	strcpy(out_file, input_file);
	strcat(out_file, ".out");
	//Сделаем емя выходного файла
	out = open(out_file, O_CREAT|O_WRONLY|O_TRUNC, 0664);
	IF_ERR(out, -1, out_file, exit(errno););
	IF_ERR(write(out, "P6\n", 3), -1, out_file, exit(errno););
	//Обозначим формат
	IF_ERR(write(out, strx, strlen(strx)), -1, out_file, exit(errno););
	IF_ERR(write(out, " ", 1), -1, out_file, exit(errno););
	IF_ERR(write(out, stry, strlen(stry)), -1, out_file, exit(errno););
	IF_ERR(write(out, "\n", 1), -1, out_file, exit(errno););
	//Запишем разрешение
	IF_ERR(write(out, buf, strlen(buf)), -1, out_file, exit(errno););
	IF_ERR(write(out, "\n", 1), -1, out_file, exit(errno););			
	//Запишем яркость
	for (i = 1; i < y; i++)
		for(int j = 1; j < x; j++)
			for(int col = 0; col < 3; col++) {
				check = write(out, rgb[col][i] + j, pix_size);
				IF_ERR(check, -1, out_file, exit(errno););
	}
	//Запишем картинку
	IF_ERR(close(out), -1, out_file, exit(errno););
	free(out_file);
	for (i = 0; i < 3; i++) {
		for (int j = 0; j <= y; j++)
			free(rgb[i][j]);  
		free(rgb[i]);
	}
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
		filter_p6(input, argv[1]);
		break;
	default:
		printf("Sorry, this kind of ppm is not supported yet\n");
		exit(EXIT_SUCCESS);
	}
	exit(EXIT_SUCCESS);
}
