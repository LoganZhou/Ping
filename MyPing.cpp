//
// Created by ZhouHeng on 2017/10/29.
//

#include "MyPing.h"
#include <iostream>

USHORT MyPing::s_usPacketSeq = 0;

/**
构造函数
初始化socket，注册事件，当前进程ID，如果socket创建成功，则创建ICMP报文缓冲区
*/
MyPing::MyPing() :m_szICMPData(NULL), m_bIsInitSucc(FALSE){
    WSADATA WSAData;
    WSAStartup(MAKEWORD(1,1), &WSAData);    //启动异步socket

    m_event = WSACreateEvent();
    m_usCurrentProcID = (USHORT)GetCurrentProcessId();

    if ((m_sockRaw = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP, NULL, 0, 0)) != SOCKET_ERROR) {
        WSAEventSelect(m_sockRaw, m_event, FD_READ);//将事件指定为FD_READ(该信号量表示有数据可读)
        m_bIsInitSucc = TRUE;

        m_szICMPData = (char*)malloc((DEF_PACKET_SIZE + sizeof(ICMPHeader)));

        if (m_szICMPData == NULL) {
            m_bIsInitSucc = FALSE;
        }
    }
}

/**
析构函数
释放socket
释放ICMP报文缓冲区
*/
MyPing::~MyPing() {
    WSACleanup();

    if (NULL != m_szICMPData){
        free(m_szICMPData);
        m_szICMPData = NULL;
    }
}

/**
启动Ping，调用Ping Core
*/
BOOL MyPing::Ping(DWORD dwDestIP, PingReply *pPingReply, DWORD dwTimeout) {
    return PingCore(dwDestIP, pPingReply, dwTimeout);
}

/**
启动Ping，将字符串地址转换为DWORD，并调用Ping Core
*/
BOOL MyPing::Ping(char *szDestIP, PingReply *pPingReply, DWORD dwTimeout) {
    if (NULL != szDestIP) {
        return PingCore(inet_addr(szDestIP), pPingReply, dwTimeout);
    } else {
        return FALSE;
    }
}

/**
Ping核心函数
*/
BOOL MyPing::PingCore(DWORD dwDestIP, PingReply *pPingReply, DWORD dwTimeout) {
    //判断初始化是否成功
    if (!m_bIsInitSucc)
    {
        return FALSE;
    }

    //配置SOCKET，IP协议版本，目的地址
    sockaddr_in sockaddrDest;
    sockaddrDest.sin_family = AF_INET;
    sockaddrDest.sin_addr.s_addr = dwDestIP;    //填入目的地址IP
    int nSockaddrDestSize = sizeof(sockaddrDest);

    //构建ICMP包
    int nICMPDataSize = DEF_PACKET_SIZE + sizeof(ICMPHeader);   //大小等于ICMP数据报头部+数据部分
    ULONG ulSendTimestamp = GetTickCountCalibrate();
    USHORT usSeq = ++s_usPacketSeq;
    memset(m_szICMPData, 0, nICMPDataSize); //将ICMP数据报置为0
    ICMPHeader *pICMPHeader = (ICMPHeader*)m_szICMPData;
    pICMPHeader->m_byType = ECHO_REQUEST;       //填充ICMP报头
    pICMPHeader->m_byCode = 0;
    pICMPHeader->m_usID = m_usCurrentProcID;
    pICMPHeader->m_usSeq = usSeq;
    pICMPHeader->m_ulTimeStamp = ulSendTimestamp;
    pICMPHeader->m_usChecksum = CalCheckSum((USHORT*)m_szICMPData, nICMPDataSize);

    //发送ICMP报文
    //1. socket
    //2. pointer to buffer
    //3. data length in bytes
    //4. A set of flags that specify the way in which the call is made.
    //5. 目的主机的sockaddr指针
    //6. 目的主机的sockaddr长度
    if (sendto(m_sockRaw, m_szICMPData, nICMPDataSize, 0, (struct sockaddr*)&sockaddrDest, nSockaddrDestSize) == SOCKET_ERROR)
    {
        return FALSE;
    }

    //如果返回结果结构体为空，则返回出错
    if (pPingReply == NULL)
    {
        return FALSE;
    }

    char recvbuf[256] = {"\0"};
    while (TRUE)
    {
        //等候重叠I/O调用结束
        //参数：
        //1. event 数量
        //2. event 数组
        //3. 是否等待所有事件都被标记
        //4. 超时
        //5. 线程等待状态是否可变（不确定）
        if (WSAWaitForMultipleEvents(1, &m_event, FALSE, 100, FALSE) != WSA_WAIT_TIMEOUT)
        {
            //检测socket上FD_READ网络事件的发生
            WSANETWORKEVENTS netEvent;
            WSAEnumNetworkEvents(m_sockRaw, m_event, &netEvent);

            if (netEvent.lNetworkEvents & FD_READ)//如果有数据可读
            {
                ULONG nRecvTimestamp = GetTickCountCalibrate();//记录时间戳
                int nPacketSize = recvfrom(m_sockRaw, recvbuf, 256, 0, (struct sockaddr*)&sockaddrDest, &nSockaddrDestSize);
                if (nPacketSize != SOCKET_ERROR)
                {
                    IPHeader *pIPHeader = (IPHeader*)recvbuf;//先转换成IP数据报头部指针
                    USHORT usIPHeaderLen = (USHORT)((pIPHeader->m_byVerHLen & 0x0f) * 4);//读取IP数据报长度
                    ICMPHeader *pICMPHeader = (ICMPHeader*)(recvbuf + usIPHeaderLen);

                    if (pICMPHeader->m_usID == m_usCurrentProcID //是当前进程发出的报文
                        && pICMPHeader->m_byType == ECHO_REPLY //是ICMP响应报文
                        && pICMPHeader->m_usSeq == usSeq //是本次请求报文的响应报文
                            )
                    {
                        pPingReply->m_usSeq = usSeq;
                        pPingReply->m_dwRoundTripTime = nRecvTimestamp - pICMPHeader->m_ulTimeStamp;
                        pPingReply->m_dwBytes = nPacketSize - usIPHeaderLen - sizeof(ICMPHeader);
                        pPingReply->m_dwTTL = pIPHeader->m_byTTL;
                        return TRUE;
                    }
                }
            }
        }
        //超时
        if (GetTickCountCalibrate() - ulSendTimestamp >= dwTimeout)
        {
            return FALSE;
        }
    }
}

USHORT MyPing::CalCheckSum(USHORT *pBuffer, int nSize)
{
    unsigned long ulCheckSum=0;
    while(nSize > 1)    //按照1个字为单位（16位）求和
    {
        ulCheckSum += *pBuffer++;
        nSize -= sizeof(USHORT);
    }
    if(nSize )//如果是奇数，则把最后一个字节转换为2个字节来读取
    {
        ulCheckSum += *(UCHAR*)pBuffer;
    }
    //高16位和低16位相加，取反得到校验和
    ulCheckSum = (ulCheckSum >> 16) + (ulCheckSum & 0xffff);
    ulCheckSum += (ulCheckSum >>16);

    return (USHORT)(~ulCheckSum);
}

ULONG MyPing::GetTickCountCalibrate()
{
    static ULONG s_ulFirstCallTick = 0;
    static LONGLONG s_ullFirstCallTickMS = 0;

    SYSTEMTIME systemtime;
    FILETIME filetime;
    GetLocalTime(&systemtime);
    SystemTimeToFileTime(&systemtime, &filetime);
    LARGE_INTEGER liCurrentTime;
    liCurrentTime.HighPart = filetime.dwHighDateTime;
    liCurrentTime.LowPart = filetime.dwLowDateTime;
    LONGLONG llCurrentTimeMS = liCurrentTime.QuadPart / 10000;

    if (s_ulFirstCallTick == 0)
    {
        s_ulFirstCallTick = GetTickCount();
    }
    if (s_ullFirstCallTickMS == 0)
    {
        s_ullFirstCallTickMS = llCurrentTimeMS;
    }

    return s_ulFirstCallTick + (ULONG)(llCurrentTimeMS - s_ullFirstCallTickMS);
}