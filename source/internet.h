#ifndef _INTERNET_H_
#define _INTERNET_H_

#include <network.h>
#include <wolfssl/ssl.h>

#include "log.h"

#define POP_TO "To:"
#define POP_FROM "From:"
#define POP_CC "Cc:"
#define POP_DATE "Date:"
#define POP_SUBJECT "Subject:"
#define POP_BODY "\r\n\r\n"
#define ESMTP_AUTH_LOGIN 2
#define MAX_SOCKETS 99


typedef enum netProtocol{
	UDP,
	TCP
} netProtocol_t;
	
class Internet{
public:
	Internet();
	~Internet();
	bool connect(char* server, int port, netProtocol_t protocol, bool _ssl=false);
	void close();
	const char* getState();
	void setState(const char* state);
	bool write(const char *data);
	bool read(int bufferSize, char* buffer);
	char* addressToIp(char* address_in);
	bool startSsl();

	char localip[16];
	char gateway[16];
	char netmask[16];

	int socket;

private:
	int err;
	char errString[80];
	WOLFSSL_METHOD* method;
	WOLFSSL_CTX* ctx;
	WOLFSSL* sslSocket;
	bool sslSetup();
	bool ssl;
	const char* netstate;
	u32 ipaddr;
	hostent* hostip;
	char* host;
	void sslReportError();
	char* getIpByHostname(char* domain);
	Log* log;
};

#endif