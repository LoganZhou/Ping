#include <iostream>
#include <stdio.h>
#include "MyPing.h"

int main(int argc,char *argv[]) {
    MyPing objPing;

    if (argc < 2) {
        printf("Usage: ping <host>\nFor example: ping 114.114.114.114");
        exit(1);
    }

    PingReply reply;

    printf("Pinging %s with %d bytes of data:\n", argv[1], DEF_PACKET_SIZE);

    for(int i=0;i<5;i++)
    {
        objPing.Ping(argv[1], &reply);
        printf("Reply from %s: bytes=%ld time=%ldms TTL=%ld\n", argv[1], reply.m_dwBytes, reply.m_dwRoundTripTime, reply.m_dwTTL);
        Sleep(500);
    }
    return 0;
}