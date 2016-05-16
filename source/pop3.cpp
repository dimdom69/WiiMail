#include <cstring>

#include "pop3.h"

POP3::POP3(Internet* _internet, char* _address, int _port, char* _username, char* _password, int _ssl)
	: Server(_internet, _address, _port, _username, _password, _ssl){		
	response = new char[READ_BUFFER_SIZE];
	message = new char[SEND_BUFFER_SIZE];
	email = new email_t;
	numMessages = 0;
	error = false;
}

POP3::~POP3(){
	delete [] response;
	delete [] message;
	delete [] email;
}

bool POP3::read(){
	return internet->read(READ_BUFFER_SIZE, response);
}

bool POP3::write(char* input){
	sprintf(message,"%s\r\n", input);
	return internet->write(message);
}

bool POP3::parse(){
	if(response[0] == '+'){
		return true;
	}else{
		error = true;
		return false;
	}
}

bool POP3::connect(){
	if(ssl == USE_SSL){
		return internet->connect(address, port, TCP, true);
	}
	else{
		return internet->connect(address, port, TCP, false);
	}
}

bool POP3::hello(){
	return read(); //Read the server's welcome message first
}

bool POP3::startTLS(){
	write("STARTTLS");
	read();
	internet->startSsl();
	hello();
	return error;
}

bool POP3::authorize(){
	sprintf(message, "USER %s", username);
	write(message);
	read();
	sprintf(message, "PASS %s", password);
	write(message);
	read();
}

bool POP3::listMessages(){
	write("LIST");
	read(); //reads "header"
	while(read()){
		if(strcmp(response, ".\r\n") == 0){
			break;
		}
	}
}

bool POP3::retrieveMessage(int num){
	sprintf(message, "RETR %d", num);
	write(message);
	read(); //reads "header"
	while(read()){
		if(strcmp(response, ".\r\n") == 0){
			break;
		}
	}
}

bool POP3::updateDB(){

}

bool POP3::quit(){
	write("QUIT");
	read();
	internet->close();
	return error;
}