#include "server.h"

Server::Server(Internet* _internet, char* _address, int _port, char* _username, char* _password, bool _ssl){
	internet = _internet;
	address = _address;
	port = _port;
	username = _username;
	password = _password;
	ssl = _ssl;
}

Server::~Server(){

}