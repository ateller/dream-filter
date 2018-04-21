#include <pthread.h>
#include <semaphore.h>
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
#include <time.h>

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
		} \
	} while (0)
#endif
//Макрос, чтобы писать, что файл плохой

#define TEMP_PIXEL m[i + gi - 1][j + gj - 1]
//Иначе не лезет просто в 80

//Чтобы читать число от 0 до
//65535 в буфер, размером 6
//байт из файла
int f, n_of_thr;

struct sems_for_neigh {sem_t prv; sem_t nxt; };

struct args {short ***mtrx;
int x;
int start;
int end;
int max;
int col_amount;
struct sems_for_neigh *s; };

int read_until_space(int input, char *buf, char space, char i)
{
	int value;

	//Начинаем с того, что дадут
	for (; i < 6; i++) {
		char check;

		check = read(input, buf + i, 1);
		//Читаем очередной символ
		IF_ERR(check, -1, "Read error", exit(errno););
		IS_BAD(!check);
		if (buf[i] == space)
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

short **sobel(short **m, int x, int y, int *new_max)
{
	short op[3][3] = {
	{ -1, -2, -1}
	, {0,  0,  0}
	, {1,  2,  1} };
	short *temp[3];
	//Массив под указатели на последние 3
	//новые строки
	short **borders = (short **) malloc(sizeof(short *) * 2);
	//Массив под указатели на первую
	//и последнюю строку

	IF_ERR(borders, NULL, "Malloc error", exit(errno););
	for (int i = 1; i < y; i++) {
		int tempindex = i % 3;
		//В массиве 3 места. Пишем по индексу
		//остатку от деления номера текущей
		//строки на 3

		temp[tempindex] = malloc(sizeof(short *) * (x + 1));
		//Память под новую строку
		IF_ERR(temp[tempindex], NULL, "Malloc error", exit(errno););
		for (int j = 1; j < x; j++) {
			int gx = 0, gy = 0, g;

			for (int gi = 0; gi < 3; gi++)
				for (int gj = 0; gj < 3; gj++) {
					gy += op[gi][gj] * TEMP_PIXEL;
					gx += op[gj][gi] * TEMP_PIXEL;
				//Сворачиваем по x и по y
				}
			g = sqrt((double)(gx * gx + gy * gy));
			//Находим итог
			if (g > 65535)
				g = 65535;
			//Да, костыль, но память нужна
			if (g > *new_max)
				*new_max = g;
			//В итоге яркость может стать
			//больше, чем максимальная,
			//которая была раньше
			temp[tempindex][j] = g;
		}
		switch (i) {
		case 1:
			borders[0] = temp[1];
			break;
		//Если у нас первая строка,
		//указатель на нее надо сохранить
		case 3:
		case 2:
			break;
		//Здесь ничего не надо делать
		default:
			free(m[i - 2]);
			m[i - 2] = temp[(i - 2) % 3];

			if (i == (y - 1)) {
				free(m[i - 1]);
				m[i - 1] = temp[(i - 1) % 3];
				borders[1] = temp[tempindex];
			}
			//Если мы на последней строке,
			//то и прошлую. А еще
			//сохраняем индекс
			break;
		}
		//Заменяем в картинке старую строку
		//на новую. Заменяем позапрошлую
		//строку (она больше не нужна)
	}
	return borders;
}

//Поправить яркость
void dim(short **m, int x, int y, int max, int new_max)
{
	if (f) {
	//Нормализация
		for (int i = 0; i < y; i++)
			for (int j = 1; j < x; j++)
				m[i][j] = (((float) m[i][j]) / new_max) * max;
	} else {
	//Если яркость писеля больше
	//максимальной,
	//откручивам до максимальной
		for (int i = 0; i < y; i++)
			for (int j = 1; j < x; j++)
				if (m[i][j] > max)
					m[i][j] = max;
	}
}

void *sobel_in_th(void *pointer)
{
	struct args *a = (struct args *) pointer;
	short **borders;

	IF_ERR(sem_init(&(a->s->prv), 1, 0), -1, "Sem_init", exit(errno););
	IF_ERR(sem_init(&(a->s->nxt), 1, 0), -1, "Sem_init", exit(errno););
	//Инициализируем свои семафоры
	for (int i = 0; i < a->col_amount; i++) {
		int max = 0;

		borders = sobel(a->mtrx[i] + a->start, a->x, a->end, &max);
		//Фильтрация возвращает указатели
		//на первую и последниюю
		//новую строку картинки,
		//их она не заменяет на новое сама
		IF_ERR(sem_post(&a->s->prv), -1, "Sem_post", exit(errno););
		IF_ERR(sem_post(&a->s->nxt), -1, "Sem_post", exit(errno););
		//Говорим соседям, что нам их края
		//больше не нужны

		IF_ERR(sem_wait(&(a->s + 1)->prv), -1, "Semwait", exit(errno););
		//Ждем, когда сосед сверху закончит
		//хотеть нашу последнюю строку
		free(a->mtrx[i][a->end + a->start - 1]);
		a->mtrx[i][a->end + a->start - 1] = borders[1];
		//Заменяем последнуюю строку

		IF_ERR(sem_wait(&(a->s - 1)->nxt), -1, "Semwait", exit(errno););
		//Ждем, когда сосед снизу закончит
		//хотеть нашу первую строку
		free(a->mtrx[i][a->start + 1]);
		a->mtrx[i][a->start + 1] = borders[0];
		//Заменяем первую строку на новую

		free(borders);
		//Освобождаем пару

		dim(a->mtrx[i] + a->start + 1, a->x, a->end - 1, a->max, max);
		//Откручиваем яркость
	}
}

long int delta_time(struct timespec s, struct timespec f)
{
	long int result = (f.tv_sec * 1000000000) + f.tv_nsec;

	return result - ((s.tv_sec * 1000000000) + s.tv_nsec);
}

void filter_p6(int input, char *input_file)
{
	char buf[6], check, *out_nm, strx[6], stry[6];
	//Буферы для чтения из файла и
	//имя нового файла
	int x, y, i, out, max, str_per_th, thr_j;
	//Размеры файла, счетчик,
	//выходной дескриптор,
	//максимальная яркость,
	//Количество строк на поток
	short **rgb[3];
	//По матрице на канал
	char pix_size;
	//Размер пикселя
	struct sems_for_neigh *sems;
	//Массив семафоров
	struct args *a;
	//Массив аргументов
	pthread_t *ths;
	//Массив потоков
	struct timespec start, finish;
	//Замер

	check = read(input, buf, 1);
	IF_ERR(check, -1, "Read error", exit(errno););
	IS_BAD(buf[0] != '\n' || !check);
	//После строки формата должен быть
	//перевод строки
	check = read(input, strx, 1);
	IF_ERR(check, -1, "Read error", exit(errno););
	IS_BAD(!check);
	//Дальше может быть комментарий
	if (strx[0] == '#') {
		do {
			check = read(input, strx, 1);
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

	//Это была загрузка параметров файла
	x += 2;
	y += 2;
	//Чтобы с рамкой в 1 пиксель
	for (i = 0; i < 3; i++) {
		rgb[i] = (short **) malloc(sizeof(short *) * y);
		//Выделяем память под массив строк
		IF_ERR(rgb[i], NULL, "Malloc error", exit(errno););
		for (int j = 0; j < y; j++) {
			rgb[i][j] = malloc(2 * x);
			//В каждый элемент массива
			//Указатель на строку
			IF_ERR(rgb[i][j], NULL, "Malloc error", exit(errno););
		}
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
	else
		pix_size = 2;
	//Предположительно(я так думаю),
	//если макс. яркость меньше
	//255, будет 1 байт на пиксель,
	//А если от 255 до 65535
	//То 2 байта на пиксель
	for (i = 1; i < y; i++)
		for (int j = 1; j < x; j++)
			for (int col = 0; col < 3; col++) {
				if (pix_size == 1)
					rgb[col][i][j] = 0;
				//Размер элемента - 2б
				//и если считывается 1,
				//то в другом байте
				//остается то что там было
				//поэтому сначала обнуляем
				check = read(input, rgb[col][i] + j, pix_size);
				if (check < 1)
					rgb[col][i][j] = 0;
				//В файле может быть меньше
				//пикселей, чем обещано
				//но это не должно помешать
	}

	//Это была загрузка файла и подготовка
	/*
	 * for (i = 0; i < 3; i++) {
	 *	borders = sobel(rgb[i], x, y, max);
	 *	//Фильтрация возвращает указатели
	 *	//на первую и последниюю
	 *	//новую строку картинки,
	 *	//их она не заменяет на новое сама
	 *	free(rgb[i][1]);
	 *	rgb[i][1] = borders[0];
	 *	//Заменяем первую строку на новую
	 *	free(rgb[i][y - 1]);
	 *	rgb[i][y - 1] = borders[1];
	 *	//Заменяем последнуюю строку
	 *	free(borders);
	 * //Фильтруем
	 * }
	 */
	//Не больше потоков, чем строк в файле
	str_per_th = ((y - 1) / n_of_thr);
	//Строк на поток
	if (str_per_th < 4) {
		n_of_thr = 1;
		str_per_th = 4;
	}
	thr_j = (y - 1) % str_per_th;
	//Остаток
	if ((str_per_th * n_of_thr) < y - 1 - thr_j) {
		n_of_thr = (y - 1 - thr_j) / str_per_th;
		thr_j = (y - 1) % str_per_th;
		if (thr_j)
			n_of_thr++;
	} else {
		if ((str_per_th * n_of_thr) < y - 1)
			str_per_th++;
		for (; (str_per_th * (n_of_thr - 1)) >= y;)
			n_of_thr--;
		thr_j = (y - 1) % str_per_th;
	}
	//Если строки не кратны
	//потокам
	printf("Number of threads: %d\n", n_of_thr);
	printf("Str per thread: %d, Остаток: %d\n", str_per_th, thr_j);
	sems = malloc(sizeof(struct sems_for_neigh) * (n_of_thr + 2));
	//Массив семафоров
	IF_ERR(sems, NULL, "Malloc error", exit(errno););
	a = malloc(sizeof(struct args) * n_of_thr);
	//Массив параметров
	IF_ERR(a, NULL, "Malloc error", exit(errno););
	IF_ERR(sem_init(&sems[0].nxt, 1, 3), -1, "Sem_init", exit(errno););
	IF_ERR(sem_init(&sems[n_of_thr + 1].prv, 1, 3), -1, "Se", exit(errno););
	//Два крайних уже на финише
	a[0].mtrx = rgb;
	//Матрицу
	a[0].x = x;
	//Ширину
	a[0].start = 0;
	//Начало
	if (thr_j)
		a[0].end = thr_j + 1;
		//Первый поток заберет остаток
	else
		a[0].end = str_per_th + 1;
	//End - смещение последней строки
	//от первой
	a[0].max = max;
	//Макс. яркость
	a[0].col_amount = 3;
	//Количство цветов (для ргб)
	a[0].s = sems + 1;
	//Свой семафор

	ths = malloc(sizeof(pthread_t) * n_of_thr);
	//Массив адресов
	IF_ERR(ths, NULL, "Malloc error", exit(errno););
	clock_gettime(CLOCK_REALTIME, &start);
	for (i = 0; true; i++) {
		check = pthread_create(ths + i, NULL, sobel_in_th, a + i);
		//Создаем поток с теми аргументами
		//которые есть
		IF_ERR(check, -1, "Pthread_create error", exit(errno););

		//printf ("i = %d, start = %d\n", i, a[i].start);
		if (i == (n_of_thr - 1))
			break;
		//Выход из цикла
		//Дальше готовим параметры
		//для следующего потока
		a[i + 1].mtrx = rgb;
		a[i + 1].x = x;
		a[i + 1].start = a[i].start + a[i].end - 1;
		//Старт следующиего потока - наша
		//последняя строка
		a[i + 1].end = str_per_th + 1;
		//У нас n строк, а n + 1-я
		//это наш финиш и чья-то первая
		//строка
		a[i + 1].max = max;
		a[i + 1].col_amount = 3;
		a[i + 1].s = a[i].s + 1;
		//Следующий семафор
	}
	for (i = 0; i < n_of_thr; i++) {
		//printf("T%d wait\n", i + 1);
		pthread_join(ths[i], NULL);
		//printf("T%d OK\n", i + 1);
	}
	//Собираем потоки вместе
	clock_gettime(CLOCK_REALTIME, &finish);
	printf("Time: %ld\n", delta_time(start, finish));
	for (i = 0; i < (n_of_thr + 2); i++) {
		sem_destroy(&sems[i].prv);
		sem_destroy(&sems[i].nxt);
		//Ломаем семафоры
	}
	free(sems);
	free(ths);
	free(a);
	//Освобождаем все, что было
	//выделено для потоков

	out_nm = malloc(strlen(input_file) + 5);
	IF_ERR(out_nm, NULL, "Malloc error", exit(errno););
	strcpy(out_nm, input_file);
	strcat(out_nm, ".out");
	//Сделаем емя выходного файла
	out = open(out_nm, O_CREAT|O_WRONLY|O_TRUNC, 0664);
	IF_ERR(out, -1, out_nm, exit(errno););
	IF_ERR(write(out, "P6\n", 3), -1, out_nm, exit(errno););
	//Обозначим формат
	IF_ERR(write(out, strx, strlen(strx)), -1, out_nm, exit(errno););
	IF_ERR(write(out, " ", 1), -1, out_nm, exit(errno););
	IF_ERR(write(out, stry, strlen(stry)), -1, out_nm, exit(errno););
	IF_ERR(write(out, "\n", 1), -1, out_nm, exit(errno););
	//Запишем разрешение
	IF_ERR(write(out, buf, strlen(buf)), -1, out_nm, exit(errno););
	IF_ERR(write(out, "\n", 1), -1, out_nm, exit(errno););
	//Запишем яркость
	for (i = 1; i < y; i++)
		for (int j = 1; j < x; j++)
			for (int col = 0; col < 3; col++) {
				check = write(out, rgb[col][i] + j, pix_size);
				IF_ERR(check, -1, out_nm, exit(errno););
	}
	//Запишем картинку
	IF_ERR(close(out), -1, out_nm, exit(errno););
	free(out_nm);
	for (i = 0; i < 3; i++) {
		for (int j = 0; j <= y; j++)
			free(rgb[i][j]);
		free(rgb[i]);
	}
	//Освободим память
}

//Телятников, группа 3О-309Б

int main(int argc, char *argv[])
{
	int input;
	char buf[2];

	f = 0;
	n_of_thr = 1;
	//Изначально 1 поток
	if (argc == 1) {
		printf("Error: there is no arguments\n");
		exit(EXIT_FAILURE);
	}
	//Проверяем, а есть ли аргументы
	if (argc > 2) {
		for (int i = 2; i < 4; i++)
			if (argv[2][0] == '-')
				switch (argv[2][1]) {
				case 'n':
					if (!f)
						f = 1;
					//Флаг - использовать
					//нормализацию
					//вместо простого
					//откручивания
					//до максимума яркости
					break;
				case 'j':
					if (n_of_thr == 1)
						n_of_thr = atoi(argv[i] + 2);
					if (n_of_thr < 1)
						n_of_thr = 1;
					//Количество потоков
				}
	}
	input = open(argv[1], O_RDONLY);
	IF_ERR(input, -1, argv[1], exit(errno););
	//Открываем файл, если он есть
	IF_ERR(read(input, buf, 2), -1, "Read error", exit(errno););
	//Читаем строку формата
	IS_BAD(buf[0] != 'P' || buf[1] < '1' || buf[1] > '6');
	//Проверяем, а строка ли формата
	//это вообще
	switch (buf[1]) {
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
