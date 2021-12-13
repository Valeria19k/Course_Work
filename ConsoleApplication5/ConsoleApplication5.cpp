#include <omp.h>
#include <iostream>
#include <time.h>
#include <locale.h>
#include <windows.h>
using namespace std;

FILE* file; // Файловый поток.                            

void CurrentTime() // Функция, используемая для фиксирования времени результатов работы потоков.
{
    SYSTEMTIME time; // Структура для хранения текущего времени.
    GetLocalTime(&time); // Определение текущего времени.
    printf("%02d:%02d:%02d.%03d", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
    fprintf(file, "%02d:%02d:%02d.%03d", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
}

void writer(int& resource, int writers, int& active_readers, omp_lock_t& lock_writers, bool& lock_readers) // Функция моделирования потоков-писателей.
{
#pragma omp parallel num_threads(writers) // Создание параллельных потоков-писателей в количестве, хранимом переменной writers.
    {
        bool flag; // Переменная, используемая для предотвращения повторного вывода сообщений.
        int wnum = omp_get_thread_num(); // Определение номера потока-писателя.
        while (true) // Бесконечный цикл.
        {
            flag = false;
            Sleep(rand() * wnum % 15000 + 5000); //Ожидание случайного времени от 5 до 20 секунд.
#pragma omp critical // Объявление критической секции. Вывод сообщения об обращении к ресурсу.
            {
                CurrentTime();
                printf(" Обращение писателя %d к ресурсу.\n", wnum);
                fprintf(file, " Обращение писателя %d к ресурсу.\n", wnum);
            }
            if (!omp_test_lock(&lock_writers)) // Проверка состояния замка без блокировки потока.
            {
#pragma omp critical // Если замок установлен, вывести сообщение о использовании ресурса другим потоком.
                {
                    printf("Другой поток-писатель обратился к ресурсу раньше. Ожидание освобождения ресурса.\n");
                    fprintf(file, "Другой поток-писатель обратился к ресурсу раньше. Ожидание освобождения ресурса.\n");
                }
                omp_set_lock(&lock_writers); // Блокировка потока.
            }
            lock_readers = true; // Запрет доступа для читателей.
            while (active_readers != 0) // Ожидание освобождения читателями ресурса.
            {
                if (!flag)
                {
#pragma omp critical // Разовый вывод сообщения о использовании ресурса читателями.
                    {
                        printf("Ожидание писателем %d окончания работы потоков-читателей.\n", wnum);
                        fprintf(file, "Ожидание писателем %d окончания работы потоков-читателей.\n", wnum);
                    }
                }
                flag = true;
                Sleep(100); // Ожидание  0,1 секунды.
            }
#pragma omp critical // Вывод сообщения о получении доступа к ресурсу.
            {
                CurrentTime();
                printf(" Получение писателем %d доступа к ресурсу.\n", wnum);
                fprintf(file, " Получение писателем %d доступа к ресурсу.\n", wnum);
            }
            Sleep(3000); // Ожидание 3 секунды.
            resource = rand() + wnum;  // Присвоение случайного числа переменной.
#pragma omp critical // Вывод значения переменной и сообщения об окончании работы.
            {
                CurrentTime();
                printf(" Писатель %d присвоил переменной ", wnum);
                printf("значение %d. Работа потока окончена.\n", resource);
                fprintf(file, " Писатель %d присвоил переменной ", wnum);
                fprintf(file, "значение %d. Работа потока окончена.\n", resource);
            }
            lock_readers = false; // Разрешение доступа к ресурсу для потоков-читателей.
            omp_unset_lock(&lock_writers); // Разрешение доступа к ресурсу для потоков-писателей.
        }
    }
}

void reader(int& resource, int readers, int& active_readers, bool& lock_readers) // Функция моделирования потоков-читателей.
{
#pragma omp parallel num_threads(readers) // Создание параллельных потоков-читателей в количестве, хранимом переменной readers.
    {
        bool flag; // Переменная, используемая для предотвращения повторного вывода сообщений.
        int rnum = omp_get_thread_num(); // Определение номера потока-читателя.
        while (true) //Бесконечный цикл.
        {
            flag = false;
            Sleep(rand() * rnum % 10000 + 5000); //Ожидание случайного времени от 5 до 15 секунд.
#pragma omp critical // Объявление критической секции. Вывод сообщения об обращении к ресурсу.
            {
                CurrentTime();
                printf(" Обращение читателя %d к ресурсу.\n", rnum);
                fprintf(file, " Обращение читателя %d к ресурсу.\n", rnum);
            }
            while (lock_readers == true) // Ожидание освобождения писателями ресурса.
            {
                if (!flag)
                {
#pragma omp critical // Разовый вывод сообщения о использовании ресурса потоком-писателем.
                    {
                        printf("Ресурс занят потоком-писателем. Ожидание освобождения ресурса.\n");
                        fprintf(file, "Ресурс занят потоком-писателем. Ожидание освобождения ресурса.\n");
                    }
                }
                flag = true;
                Sleep(100);  // Ожидание 0,1 секунды.
            }
#pragma omp critical  // Вывод сообщения о получении доступа к ресурсу.
            {
                CurrentTime();
                printf(" Получение читателем %d доступа к ресурсу.\n", rnum);
                fprintf(file, " Получение читателем %d доступа к ресурсу.\n", rnum);
            }
#pragma omp atomic
            active_readers++; // Инкрементировать переменную, хранящую количество активных читателей.   
            Sleep(3000); // Ожидание 3 секунды.
#pragma omp critical // Вывод значения переменной и сообщения об окончании работы.
            {
                CurrentTime();
                printf(" Читатель %d прочитал ", rnum);
                printf("значение переменной %d. Работа потока окончена.\n", resource);
                fprintf(file, " Читатель %d прочитал ", rnum);
                fprintf(file, "значение переменной %d. Работа потока окончена.\n", resource);
            }
#pragma omp atomic
            active_readers--; // Декрементировать переменную, хранящую количество активных читателей.                  
        }
    }
}

void main()
{
    setlocale(LC_ALL, "Russian"); // Выбор русского языка в консоли.
    srand(time(NULL)); // Установка текущего времени в качесве базы генератора случайных чисел.

    int resource = 0; // Целочисленная переменная для хранения значения.
    int writers = 0; // Переменная, хранящая количество писателей.
    int readers = 0; // Переменная, хранящая количество читателей.
    int active_readers = 0; // Количество читателей, считывающих значение перемнной в данный момент.

    omp_lock_t lock_writers; // Замок для блокировки потоков-писателей.
    bool lock_readers = false; // Переменная для блокировки читателей.

    fopen_s(&file, "test.txt", "w"); // Определение текстового файла для фиксирования результатов.
    omp_init_lock(&lock_writers); // Инициализация замка.
    omp_set_nested(true); // Включение вложенного режима параллельной обработки потоков.

    printf("Модель взаимоисключающего взаимодействия \"Задача о читателях и писателях\".\n\n");
    fprintf(file, "Модель взаимоисключающего взаимодействия \"Задача о читателях и писателях\".\n\n");
    printf("Введите количество потоков-писателей: ");
    cin >> writers;
    fprintf(file, "Количество писателей: %d.\n", writers);
    printf("Введите количество потоков-читателей: ");
    cin >> readers;
    fprintf(file, "Количество читателей: %d.\n\n", readers);
    printf("\n\nРезультат работы:\n\n");
    fprintf(file, "Результат работы:\n\n");

#pragma omp parallel sections // Объявление параллельной секции.
    {
#pragma omp section // Моделирование писателей.
        {
            writer(resource, writers, active_readers, lock_writers, lock_readers);
        }
#pragma omp section // Моделирование читателей.
        {
            reader(resource, readers, active_readers, lock_readers);
        }
    }
}
