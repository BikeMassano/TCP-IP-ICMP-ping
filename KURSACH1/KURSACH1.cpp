#include <stdio.h>
#include <conio.h>
#include <winsock.h>
#include <process.h>
#include <cstdlib>
#include <iostream>
#include <windows.h>
#include <ctime>
#include "icmp_structs.h"

#pragma comment(lib, "ws2_32.lib")

#ifndef SD_SEND
#define SD_SEND 1
#endif

using namespace std;

const short TalkPort = 80; // Порт сокета
const int DEFAULT_DATA_SIZE = 32;    // Размер данных по умолчанию в ICMP пакете
const int DEFAULT_PACKET_COUNT = 4; // Количество передаваемых пакетов по умолчанию
const float DEFAULT_INTERVAL = 1.0f; // Интервал между отправкой пакетов по умолчанию

int DATA_SIZE; // Размер данных в ICMP пакете
int PACKET_COUNT; // Количество передаваемых пакетов
float INTERVAL; // Интервал между отправкой пакетов (в секундах)

// Инициализация Windows Sockets DLL
int WinSockInit()
{
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 0); /* Требуется WinSock ver 2.0*/
    //printf("Запуск Winsock...");
    // Проинициализируем Dll
    if (WSAStartup(wVersionRequested, &wsaData) != 0)
    {
        printf("\nОшибка: Не удалось найти работоспособную Winsock Dll\n");
        return 1;
    }
    // Проверка версии Dll
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0)
    {
        printf("\nОшибка: Не удалось найти работоспособную WinSock DLL\n");
        WSACleanup(); // Отключение Windows Sockets DLL
        return 1;
    }
    //printf(" Winsock запущен.\n");
    return 0;
}

// Отключение Windows Sockets DLL
void WinSockClose()
{
    WSACleanup();
    //printf("Winsock закрыт...\n");
}

// Остановка передачи данных
void stopTCP(SOCKET s)
{
    shutdown(s, SD_SEND); // Остановка передачи данных
    closesocket(s); // Закрытие сокета
    //printf("Сокет %ld закрыт.\n", s);
}

// Передача данных
int sendPing(SOCKET s, const char* message)
{
    return send(s, message, strlen(message), 0); // Отправляем сообщение 'message'
}

// Вычисление контрольной суммы ICMP
unsigned short calculate_checksum(unsigned short* buffer, int length)
{
    unsigned long sum = 0;
    unsigned short answer = 0;
    unsigned short* w = buffer;

    while (length > 1)
    {
        sum += *w++;
        length -= 2;
    }

    if (length == 1)
    {
        *(unsigned char*)&answer = *(unsigned char*)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return answer;
}

// Функция создания RAW сокета
SOCKET create_raw_socket() 
{
    SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == INVALID_SOCKET) 
    {
        cerr << "Ошибка: Не удалось создать сокет. Код ошибки: " << WSAGetLastError() << endl;
        return INVALID_SOCKET;
    }
    return sock;
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "rus");
    DATA_SIZE = DEFAULT_DATA_SIZE; // Инициализация размера данных по умолчанию
    PACKET_COUNT = DEFAULT_PACKET_COUNT; // Инициализация количества передаваемых пакетов по умолчанию
    INTERVAL = DEFAULT_INTERVAL; // Инициализация интервала между отправкой пакетов по умолчанию

    if (argc != 2) 
    {
        cerr << "Использование: ping <имя_хоста>\n";
        return 1;
    }

    if (WinSockInit() != 0) 
    {
        return 1;
    }

    const char* hostname = argv[1];

    // Преобразование имени хоста в IP-адрес
    hostent* host = gethostbyname(hostname);
    if (host == NULL) 
    {
        cerr << "Ошибка: Не удалось разрешить имя хоста.\n";
        WinSockClose();
        return 1;
    }
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(TalkPort);
    addr.sin_addr.s_addr = *(unsigned long*)host->h_addr_list[0];

    // Создание RAW сокета
    SOCKET sock = create_raw_socket();

    // Установка времени ожидания ответа
    int timeout = 1000; // миллисекунды
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    icmp_echo_packet ping_request;
    ping_request.type = 8;       // ICMP Echo Request
    ping_request.code = 0;
    ping_request.identifier = GetCurrentProcessId(); // Используем ID процесса как идентификатор
    ping_request.sequence = 0;
    ping_request.data = new char[DATA_SIZE + 1]; // Выделяем память динамически
    memset(ping_request.data, 'A', DATA_SIZE);  // Заполняем данными
    ping_request.data[DATA_SIZE] = '\0';
    ping_request.checksum = 0;
    ping_request.checksum = calculate_checksum((unsigned short*)&ping_request, sizeof(icmp_echo_packet) - sizeof(char*) + DATA_SIZE);

    cout << "Обмен пакетами с " << hostname << " [" << inet_ntoa(*(in_addr*)host->h_addr_list[0]) << "] с " << DATA_SIZE << " байтами данных:" << endl;

    for (size_t i = 0; i < PACKET_COUNT; i++)
    {
        clock_t start_time;
        // Отправка ping запроса
        start_time = clock(); // Запоминаем время отправки
        int bytes_sent = sendto(sock, (char*)&ping_request, sizeof(icmp_echo_packet) - sizeof(char*) + DATA_SIZE, 0, (sockaddr*)&addr, sizeof(addr));
        if (bytes_sent == SOCKET_ERROR)
        {
            cerr << "Ошибка: Не удалось отправить ping запрос. Код ошибки: " << WSAGetLastError() << endl;
            stopTCP(sock);
            WinSockClose();
            delete[] ping_request.data; // Освобождаем память
            return 1;
        }

        // Получение ping ответа
        char recv_buf[1024];
        sockaddr_in recv_addr;
        int recv_addr_len = sizeof(recv_addr);

        int bytes_received = recvfrom(sock, recv_buf, sizeof(recv_buf), 0, (sockaddr*)&recv_addr, &recv_addr_len);
        if (bytes_received == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSAETIMEDOUT)
            {
                cout << "Запрос превысил время ожидания." << endl;
            }
            else
            {
                cerr << "Ошибка: Не удалось получить ping ответ. Код ошибки: " << WSAGetLastError() << endl;
            }
        }
        else
        {
            clock_t end_time = clock(); // Запоминаем время получения
            double rtt = (double)(end_time - start_time) / CLOCKS_PER_SEC * 1000.0; // Время в миллисекундах

            // Обработка ICMP ответа (проверка типа, кода и т.д.)
            ip_header* ip_reply = (ip_header*)recv_buf; // Сначала IP-заголовок
            icmp_echo_packet* ping_reply = (icmp_echo_packet*)(recv_buf + sizeof(ip_header)); // Затем ICMP

            if (ping_reply->type == 0 && ping_reply->code == 0 && ping_reply->identifier == GetCurrentProcessId())
            {
                cout << "Ответ от " << inet_ntoa(recv_addr.sin_addr) << ": число байт=" << DATA_SIZE
                    << " время=" << rtt << "мс TTL=" << (int)ip_reply->ip_ttl << endl;
            }
            else
            {
                cout << "Неверный ICMP ответ." << endl;
            }
        }
        // Ожидание по интервалу
        Sleep(INTERVAL*1000);
    }

    stopTCP(sock);
    WinSockClose();

    return 0;
}