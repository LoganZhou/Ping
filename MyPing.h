//
// Created by ZhouHeng on 2017/10/29.
//

#ifndef PING_MYPING_H
#define PING_MYPING_H

#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")

#define DEF_PACKET_SIZE 32
#define ECHO_REQUEST 8
#define ECHO_REPLY 0

struct IPHeader {
    BYTE m_byVerHLen;           //4位版本号+4位首部长度
    BYTE m_byTOS;               //区分服务类型
    USHORT m_usTotalLen;        //6位DSCP+2位CU
    USHORT m_usID;              //标识
    USHORT m_usFlagFragOffset;  //3位标志+13位片偏移
    BYTE m_byTTL;               //TTL
    BYTE m_byProtocol;          //协议
    USHORT m_usHChecksum;       //首部校验和
    ULONG m_usSrcIP;            //源IP
    ULONG m_usDestIP;           //目的IP
};

struct ICMPHeader {
    BYTE m_byType;      //类型
    BYTE m_byCode;      //代码
    USHORT m_usChecksum;//校验和
    USHORT m_usID;      //标识
    USHORT m_usSeq;     //序号
    ULONG m_ulTimeStamp;//时间戳（非标准ICMP首部）
};

struct PingReply {
    USHORT m_usSeq;
    DWORD m_dwRoundTripTime;
    DWORD m_dwBytes;
    DWORD m_dwTTL;
};


class MyPing {
public:
    MyPing();
    ~MyPing();
    BOOL Ping(DWORD dwDestIP, PingReply *pPingReply = NULL, DWORD dwTimeout = 2000);
    BOOL Ping(char *szDestIP, PingReply *pPingReply = NULL, DWORD dwTimeout = 2000);
private:
    BOOL PingCore(DWORD dwDestIP, PingReply *pPingReply, DWORD dwTimeout);
    USHORT CalCheckSum(USHORT *pBuffer, int nSize);
    ULONG GetTickCountCalibrate();

private:
    SOCKET m_sockRaw;
    WSAEVENT m_event;
    USHORT m_usCurrentProcID;
    char *m_szICMPData;
    BOOL m_bIsInitSucc;
private:
    static USHORT s_usPacketSeq;
};


#endif //PING_MYPING_H
