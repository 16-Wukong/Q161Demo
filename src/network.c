/*
 * network.c
 *
 *  Created on: 2021Äê10ÔÂ14ÈÕ
 *      Author: vanstone
 */
#include <stdio.h>
#include <string.h>

#include "def.h"
#include "httpDownload.h"

#include <coredef.h>
#include <struct.h>
#include <poslib.h>

//int wirelessPdpOpen_lib(void);
int wirelessPppOpen_lib(unsigned char *pucApn, unsigned char *pucUserName, unsigned char *pucPassword) ;
//0-success   others-failed
int wirelessCheckPppDial_lib(void); //int wirelessCheckPdpDial_lib(int timeout);
int wirelessPppClose_lib(void) ;//int wirelessPdpRelease_lib(void);
int wirelessSocketCreate_lib(int nProtocol);
int wirelessSocketClose_lib(int sockid);
int wirelessTcpConnect_lib(int sockid, char *pucIP, char *pucPort, int timeout);//Q161
//int wirelessTcpConnect_lib(int sockid, char *pucIP, unsigned int uiPort); //Q181
int wirelessSend_lib(int sockid, unsigned char *pucdata, unsigned int iLen);
int wirelessRecv_lib(int sockid, unsigned char *pucdata, unsigned int iLen, unsigned int uiTimeOut);

//int wirelessSslSetTlsVer_lib(int ver);
int wirelessSetSslVer_lib(unsigned char ucVer) ;
void wirelessSslDefault_lib(void);
int wirelessSendSslFile_lib (unsigned char ucType, unsigned char *pucData, int iLen);
int wirelessSetSslMode_lib(unsigned char  ucMode);
int wirelessSslSocketCreate_lib(void);
int wirelessSslSocketClose_lib(int sockid);
//timeout: ms   return :0-success <0-failed
int wirelessSslConnect_lib(int sockid, unsigned char *pucDestIP, char *pucPort, int timeout);  //for Q161
//int wirelessSslConnect_lib(int sockid, char *pucDestIP, unsigned short pucDestPort); //for Q181
int wirelessSslSend_lib(int sockid, unsigned char *pucdata, unsigned int iLen);
int wirelessSslRecv_lib(int sockid, unsigned char *pucdata, unsigned int iLen, unsigned int uiTimeOut);
int wirelessSetDNS_lib(unsigned char *pucDNS1, unsigned char *pucDNS2 );


typedef struct {
	int valid;
	int socket;
	int ssl;
} NET_SOCKET_PRI;

// FIXME: What if the caller doesn't close the socket??
#define	MAX_SOCKETS		10
#define CERTI_LEN  1024*2

static NET_SOCKET_PRI sockets[MAX_SOCKETS];

void net_init(void)
{
	int i;

	for (i = 0; i < MAX_SOCKETS; i++)
		memset(&sockets[i], 0, sizeof(NET_SOCKET_PRI));
}

int getCertificate(char *cerName, unsigned char *cer){
	int Cerlen, Ret;
	u8 CerBuf[CERTI_LEN];

	Cerlen = GetFileSize_Api(cerName);
	if(Cerlen <= 0)
	{
		MAINLOG_L1("get certificate err or not exist - %s",cerName);
		return -1;
	}

	memset(CerBuf, 0 , sizeof(CerBuf));
	if(Cerlen > CERTI_LEN)
	{
		MAINLOG_L1("%s is too large", cerName);
		return -2;
	}

	Ret = ReadFile_Api(cerName, CerBuf, 0, (unsigned int *)&Cerlen);
	if(Ret != 0)
	{
		MAINLOG_L1("read %s failed", cerName);
		return -3;
	}
	memcpy(cer,CerBuf,strlen(CerBuf));

	return 0;
}

void *net_connect(void* attch, const char *host,const char *port, int timerOutMs, int ssl, int *errCode)
{
	int ret;
	int port_int;
	int sock;
	int i;
	u8 CerBuf[CERTI_LEN];
	int timeid = 0;
	
#ifdef __WIFI__

	// Find an empty socket slot
	for (i = 0; i < MAX_SOCKETS; i++) {
		if (sockets[i].valid == 0)
			break;
	}

	if (i >= MAX_SOCKETS) {
		*errCode = -3;
		return NULL;
	}

	// >=0 success  , others -failed
	sock = wifiSocketCreate_lib(0); //0-TCP   1-UDP
	MAINLOG_L1("wifiSocketCreate_lib:%d", ret);
	if(sock < 0) {
		*errCode = -4;
		return NULL;
	}

	ret = wifiTCPConnect_lib(sock, host, port, 60*1000);
	MAINLOG_L1("wifiTCPConnect_lib:%d", ret);
	if(ret != 0)
	{
		*errCode = -5;
		ret =  wifiSocketClose_lib(sock);
		MAINLOG_L1("wifiSocketClose_lib:%d", ret);
		return NULL;
	}

	sockets[i].valid = 1;
	sockets[i].ssl = ssl;
	sockets[i].socket = sock;

	*errCode = 0;
	return &sockets[i];

#else

	timeid = TimerSet_Api();
	while(1)
	{
		ret = wirelessCheckPppDial_lib();
		if(ret == 0)
			break;
		else{
			MAINLOG_L1("wirelessCheckPppDial_lib 1 = %d  timerOutMs = %d" , ret , timerOutMs );
			ret = wirelessPppOpen_lib(NULL, NULL, NULL);
			MAINLOG_L1("wirelessPppOpen_lib = %d" , ret );
			Delay_Api(1000);
		}
		if(TimerCheck_Api(timeid , timerOutMs) == 1)
		{
			MAINLOG_L1("wirelessCheckPppDial_lib 2 = %d" , ret);
			*errCode = -1;
			return NULL;
		}
	}
#ifdef __SETDNS__
	ret =  wirelessSetDNS_lib("114.114.114.114", "8.8.8.8" );
	MAINLOG_L1("wirelessSetDNS_lib = %d" , ret);
#endif
	// Find an empty socket slot
	for (i = 0; i < MAX_SOCKETS; i++) {
		if (sockets[i].valid == 0)
			break;
	}

	if (i >= MAX_SOCKETS) {
		*errCode = -3;

		return NULL;
	}

	if (ssl == 0) {
		sock = wirelessSocketCreate_lib(0);
		if (sock < 0) {
			*errCode = -4;
			MAINLOG_L1("wirelessSocketCreate_lib = %d" , sock );
			return NULL;
		}
	}
	else {
		wirelessSslDefault_lib();		
		wirelessSetSslVer_lib(0);
		
		if (ssl == 2) {

			ret = wirelessSetSslMode_lib(1);
			memset(CerBuf, 0 , sizeof(CerBuf));
			ret = getCertificate(FILE_CERT_ROOT, CerBuf);
			MAINLOG_L1("getCertificate === %d", ret);
			ret = wirelessSendSslFile_lib(2, CerBuf, strlen((char *)CerBuf));
			memset(CerBuf, 0 , sizeof(CerBuf));
			ret = getCertificate(FILE_CERT_PRIVATE, CerBuf);
			MAINLOG_L1("getCertificate === %d", ret);
			ret = wirelessSendSslFile_lib(1, CerBuf, strlen((char *)CerBuf));
			memset(CerBuf, 0 , sizeof(CerBuf));
			ret = getCertificate(FILE_CERT_CHAIN, CerBuf);
			MAINLOG_L1("getCertificate === %d", ret);
			ret = wirelessSendSslFile_lib(0, CerBuf, strlen((char *)CerBuf));
			MAINLOG_L1("wirelessSendSslFile_lib = %d" , ret );
		}
		else
		{
			ret = wirelessSetSslMode_lib(0);
			MAINLOG_L1("wirelessSetSslMode_lib = %d" , ret );
		}

		sock = wirelessSslSocketCreate_lib();
		if (sock == -1) {
			*errCode = -4;
			MAINLOG_L1("wirelessSslSocketCreate_lib = %d" , sock );
			return NULL;
		}
	}

	if (ssl == 0)
		ret = wirelessTcpConnect_lib(sock, (char *)host, port, 60*1000);
	else
	{
		ret = wirelessSslConnect_lib(sock, (char *)host, port, 60*1000);
		MAINLOG_L1("wirelessSslConnect_lib ret:%d host:%s  port:%s  ", ret , host, port );
	}

	if (ret != 0) {
		if (ssl == 0)
			wirelessSocketClose_lib(sock);
		else
			wirelessSslSocketClose_lib(sock);

		*errCode = -5;

		return NULL;
	}

	sockets[i].valid = 1;
	sockets[i].ssl = ssl;
	sockets[i].socket = sock;

	*errCode = 0;

	return &sockets[i];

#endif
}

int net_close(void *netContext)
{
	int ret;
	NET_SOCKET_PRI *sock = (NET_SOCKET_PRI *)netContext;

	if (sock == NULL)
		return -1;

	if (sock->valid == 0)
		return 0;

#ifdef __WIFI__
	ret = wifiTCPClose_lib(sock->socket);
	MAINLOG_L1("wifiTCPClose_lib:%d", ret);

	ret =  wifiSocketClose_lib(sock->socket);
	MAINLOG_L1("wifiSocketClose_lib:%d", ret);

#else
	if (sock->ssl == 0)
		wirelessSocketClose_lib(sock->socket);
	else
		wirelessSslSocketClose_lib(sock->socket);
#endif
	sock->valid = 0;

	return 0;
}

int net_read(void *netContext, unsigned char* recvBuf, int needLen, int timeOutMs)
{
	int ret;
	NET_SOCKET_PRI *sock = (NET_SOCKET_PRI *)netContext;

	if (sock == NULL)
		return -1;

	if (sock->valid == 0)
		return -1;

#ifdef __WIFI__
	ret = wifiRecv_lib(sock->socket, recvBuf, needLen, timeOutMs);
	if(ret == 0)
		return 0;
	else if(ret < 0)
	{
		MAINLOG_L1("wifiRecv_lib:%d", ret);
		return -1;
	}
	else
		return ret;

#else
	if (sock->ssl == 0)
		ret = wirelessRecv_lib(sock->socket, recvBuf, 1, timeOutMs);
	else
		ret = wirelessSslRecv_lib(sock->socket, recvBuf, 1, timeOutMs);

	if (ret == 0)
		return 0;

	if (ret != 1)
		return -1;

	if (needLen == 1)
		return 1;

	if (sock->ssl == 0)
		ret = wirelessRecv_lib(sock->socket, recvBuf + 1, needLen - 1, 10);
	else
		ret = wirelessSslRecv_lib(sock->socket, recvBuf + 1, needLen - 1, 10);

	if(ret < 0)
		return -1;

	return ret + 1;
#endif
}

int net_write(void *netContext, unsigned char* sendBuf, int sendLen, int timeOutMs)
{
	int ret;
	NET_SOCKET_PRI *sock = (NET_SOCKET_PRI *)netContext;

	if (sock == NULL)
		return -1;

	if (sock->valid == 0)
		return -1;

#ifdef __WIFI__

	ret = wifiSend_lib(sock->socket, sendBuf, sendLen, timeOutMs);
	MAINLOG_L1("wifiSend_lib :%d" , ret);
	if(ret == 0)
		return sendLen;
	else
		return -1;

#else
	if (sock->ssl == 0)
		return wirelessSend_lib(sock->socket, sendBuf, sendLen);
	else
		return wirelessSslSend_lib(sock->socket, sendBuf, sendLen);
#endif
}

int ApConnect()
{
	int ret;

#ifdef __WIFI__
	memset(wifiId, 0, sizeof(wifiId));
	memset(wifiPwd, 0, sizeof(wifiPwd));
	readWifiParam();
	ret = wifiAPConnect_lib(wifiId, wifiPwd);
	MAINLOG_L1("wifiAPConnect_lib:%d", ret);
#endif
	return ret;
}

/*

uint8 pssltmpp[93] = {
	0x10, 0x5B, 0x00, 0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0x82, 0x02, 0x58, 0x00, 0x34, 0x4D, 0x73,
	0x77, 0x69, 0x70, 0x65, 0x2D, 0x53, 0x6F, 0x75, 0x6E, 0x64, 0x62, 0x6F, 0x78, 0x2D, 0x37, 0x35,
	0x37, 0x66, 0x39, 0x66, 0x61, 0x35, 0x2D, 0x38, 0x35, 0x63, 0x33, 0x2D, 0x34, 0x34, 0x38, 0x31,
	0x2D, 0x39, 0x38, 0x37, 0x33, 0x2D, 0x38, 0x30, 0x64, 0x66, 0x33, 0x30, 0x35, 0x39, 0x63, 0x34,
	0x32, 0x33, 0x00, 0x19, 0x3F, 0x53, 0x44, 0x4B, 0x3D, 0x50, 0x79, 0x74, 0x68, 0x6F, 0x6E, 0x26,
	0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x31, 0x2E, 0x34, 0x2E, 0x39
};

void DR_SSL_testi(void)
{
	//int32 iRet;
	int ret;
	//MAINLOG_L1("[%s] -%s- Line=%d:application thread enter, param 0x%x\r\n", filename(__FILE__), __FUNCTION__, __LINE__, param);
	int timeid = 0;

	timeid = TimerSet_Api();
	while(1)
	{
		ret = wirelessCheckPppDial_lib();
		MAINLOG_L1("wirelessCheckPppDial_lib = %d" , ret );
		if(ret == 0)
			break;
		else{
			ret = wirelessPppOpen_lib(NULL, NULL, NULL);
			Delay_Api(1000);
		}
		if(TimerCheck_Api(timeid , 60*1000) == 1)
		{
			return ;
		}
	}
	//ret =  wirelessSetDNS_lib("114.114.114.114", "8.8.8.8" );
	//MAINLOG_L1("wirelessSetDNS_lib = %d" , ret);


    ret = wirelessSetSslMode_lib((unsigned char)1);
    MAINLOG_L1("wirelessSetSslMode_lib :%d", ret);

	ret = wirelessSendSslFile_lib(2, TEST_CA_FILE, strlen((char *)TEST_CA_FILE));
	MAINLOG_L1("wirelessSendSslFile_lib 1:%d %d", ret, strlen((char *)TEST_CA_FILE));
	ret = wirelessSendSslFile_lib(1, TEST_CLIENT_KEY_FILE, strlen((char *)TEST_CLIENT_KEY_FILE));
	MAINLOG_L1("wirelessSendSslFile_lib 2:%d %d", ret, strlen((char *)TEST_CLIENT_KEY_FILE));
	ret = wirelessSendSslFile_lib(0, TEST_CLIENT_CRT_FILE, strlen((char *)TEST_CLIENT_CRT_FILE));
	MAINLOG_L1("wirelessSendSslFile_lib 3:%d %d", ret, strlen((char *)TEST_CLIENT_CRT_FILE));


	wirelessSslDefault_lib();
	wirelessSetSslVer_lib((unsigned char)0); //wirelessSslSetTlsVer_lib(0);

	Delay_Api(2000);

    int sock = wirelessSslSocketCreate_lib();
    if (sock == -1)
    {
    	MAINLOG_L1("[%s] -%s- Line=%d:<ERR> create ssl sock failed\r\n");
        return;//fibo_thread_delete();
    }

    MAINLOG_L1(":::fibossl fibo_ssl_sock_create %x\r\n", sock);

	ret = wirelessSslConnect_lib(sock,  "a14bkgef0ojrzw-ats.iot.us-west-2.amazonaws.com", "8883", 60*1000);

	MAINLOG_L1(":::fibossl wirelessSslConnect_lib %d\r\n", ret);

    ret = wirelessSslSend_lib(sock, pssltmpp, sizeof(pssltmpp));
	MAINLOG_L1(":::fibossl sys_sock_send %d\r\n", ret);

    ret = wirelessSslRecv_lib(sock, buf, 1, 1000);
	MAINLOG_L1(":::fibossl sys_sock_recv %d\r\n", ret);
	ret = wirelessSslRecv_lib(sock, buf, 32, 10000);
	MAINLOG_L1(":::fibossl sys_sock_recv %d\r\n", ret);

	ret = wirelessSslGetErrcode_lib();
	MAINLOG_L1(":::fibo_get_ssl_errcode, ret = %d\r\n", ret);

	uint32 port = 6500;

	MAINLOG_L1(":::wifiTCPConnect_lib, iRet = %d\r\n", iRet);
}


*/



