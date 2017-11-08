#include <iostream>
#include <stdio.h>
#include "MyPing.h"

int main() {
    MyPing objPing;

    char szDestIP[] = {"218.199.68.180"};
    PingReply reply;

    printf("Pinging %s with %d bytes of data:\n", szDestIP, DEF_PACKET_SIZE);

    for(int i=0;i<5;i++)
    {
        objPing.Ping(szDestIP, &reply);
        printf("Reply from %s: bytes=%ld time=%ldms TTL=%ld\n", szDestIP, reply.m_dwBytes, reply.m_dwRoundTripTime, reply.m_dwTTL);
        Sleep(500);
    }
    return 0;
}