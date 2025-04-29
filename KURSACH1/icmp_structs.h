#pragma once

using namespace std;

// Структура ICMP Echo Request (Ping)
struct icmp_echo_packet
{
    unsigned char type;       // Тип сообщения (8 для echo request, 0 для echo reply) (1 байт)
    unsigned char code;       // Подтип сообщения (0 для всего) (1 байт)
    unsigned short checksum;   // Контрольная сумма сообщения (2 байта)
    unsigned short identifier; // Идентификатор
    unsigned short sequence;   // Порядковый номер
    char* data;   // Дополнительные данные. Размер можно менять
};

// Структура IP-заголовка (для извлечения TTL)
struct ip_header
{
    unsigned char ip_verlen;       // IP Version and Length
    unsigned char ip_tos;          // Type of Service
    unsigned short ip_totallength; // Total Length
    unsigned short ip_id;          // Identification
    unsigned short ip_offset;      // Flags and Fragment Offset
    unsigned char ip_ttl;          // Time To Live
    unsigned char ip_protocol;     // Протокол
    unsigned short ip_checksum;    // Контрольная сумма
    unsigned int ip_srcaddr;     // Адрес источника
    unsigned int ip_destaddr;    // Адрес назначения
};