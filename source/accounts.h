#ifndef _ACCOUNTS_H_
#define _ACCOUNTS_H_

#include "server.h"

class Account{
public:
	Account(char* name, Server* server);
	~Account();

private:
	Server* _server;
	char* _name;
};

#endif