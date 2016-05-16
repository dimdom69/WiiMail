#ifndef _POP3_H_
#define _POP3_H_

#include "server.h"

class POP3 : public Server{
public:
	POP3(Internet* _internet, char* _address, int _port, char* _username, char* _password, int _ssl);
	~POP3();
	bool getMail();
};

#endif