#pragma once

using namespace std;

// ��������� ICMP Echo Request (Ping)
struct icmp_echo_packet
{
    unsigned char type;       // ��� ��������� (8 ��� echo request, 0 ��� echo reply) (1 ����)
    unsigned char code;       // ������ ��������� (0 ��� �����) (1 ����)
    unsigned short checksum;   // ����������� ����� ��������� (2 �����)
    unsigned short identifier; // �������������
    unsigned short sequence;   // ���������� �����
    char* data;   // �������������� ������. ������ ����� ������
};

// ��������� IP-��������� (��� ���������� TTL)
struct ip_header
{
    unsigned char ip_verlen;       // IP Version and Length
    unsigned char ip_tos;          // Type of Service
    unsigned short ip_totallength; // Total Length
    unsigned short ip_id;          // Identification
    unsigned short ip_offset;      // Flags and Fragment Offset
    unsigned char ip_ttl;          // Time To Live
    unsigned char ip_protocol;     // ��������
    unsigned short ip_checksum;    // ����������� �����
    unsigned int ip_srcaddr;     // ����� ���������
    unsigned int ip_destaddr;    // ����� ����������
};