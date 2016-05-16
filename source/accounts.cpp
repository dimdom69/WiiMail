#include "accounts.h"

Account::Account(char* name, Server* server){
	_name = name;
	_server = server;
}

Account::~Account(){

}