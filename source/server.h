#ifndef _SERVER_H_
#define _SERVER_H_

#include "internet.h"

typedef enum{
	POP3_SERVER,
	SMTP_SERVER
} server_t;

class Server{
public:
	Server(Internet* _internet, char* _address, int _port, char* _username, char* _password, bool _ssl);
	~Server();

protected:
	Internet* internet;
	int port;
	char* address;
	char* username;
	char* password;
	server_t type;
	bool ssl;
};

#endif