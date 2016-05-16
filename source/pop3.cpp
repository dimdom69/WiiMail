#include "pop3.h"

POP3::POP3(Internet* _internet, char* _address, int _port, char* _username, char* _password, int _ssl)
	: Server(_internet, _address, _port, _username, _password, _ssl){
		
}

POP3::~POP3(){

}