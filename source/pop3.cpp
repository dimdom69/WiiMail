#include <cstring>

#include "pop3.h"

POP3::POP3(Internet* _internet, char* _address, int _port, char* _username, char* _password, int _ssl)
	: Server(_internet, _address, _port, _username, _password, _ssl){		
	response = new char[READ_BUFFER_SIZE];
	message = new char[SEND_BUFFER_SIZE];
	data = new char[DATA_BUFFER_SIZE];
	email = new email_t;
	numMessages = 0;
	error = false;
}

POP3::~POP3(){
	for(int i = 0; i < numMessages; i++){
		delete [] list[i];
	}
	delete [] list;
	delete [] response;
	delete [] message;
	delete [] email;
	delete [] data;
}

bool POP3::read(){
	return internet->read(READ_BUFFER_SIZE, response);
}

bool POP3::write(char* input){
	sprintf(message,"%s\r\n", input);
	return internet->write(message);
}

bool POP3::getMail(){
	connect();
	hello();
	if(ssl == USE_TLS){
		startTLS();
	}
	authorize();
	listMessages();
//	retrieveMessage(1);
	quit();
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
	char buf[256];
	data[0] = '\0';
	int lineOffset = 0;
	int lineLength = 0;
	int lineNum = 0;
	int readLen = 0;
	write("LIST");
	do{
		read();
		readLen += strlen(response);
		if(readLen > DATA_BUFFER_SIZE){
			//error (maybe realloc)
		}
		strcat(data, response);
	}while(strstr(response, ".\r\n") == NULL);
	sscanf(data, "+OK %d messages", &numMessages);
	list = new char*[numMessages];
	while(data[lineOffset] != '\r'){
		lineOffset++;
	}
	lineOffset+=2;
	for(; lineOffset < readLen; lineOffset++){
		if(data[lineOffset] == '.'){
			break;
		}
		else if(data[lineOffset] == '\r' || lineOffset >= readLen - 1){
			list[lineNum] = new char[lineLength+1];
			strncpy(list[lineNum], &data[lineOffset-lineLength], lineLength);
			list[lineNum][lineLength] = '\0';
			sprintf(buf, "Line %d: %s", lineNum+1, list[lineNum]);
			internet->setState(buf);
			lineNum++;
			lineOffset += 1;
			lineLength = 0;
			if(lineOffset >= readLen - 1){
				break;
			}
		}
		else{
			lineLength++;
		}
	}
	sprintf(buf, "%d", lineNum);
	internet->setState(buf);
	sprintf(buf, "%d", numMessages);
	internet->setState(buf);
	return true;
}

bool POP3::retrieveMessage(int num){
	sprintf(message, "RETR %d", num);
	write(message);
	read(); //reads "header"
	while(read()){
		if(strcmp(response, ".\r\n") == 0){
			break;
		}
//		strcpy(response);
	}
	return true;
}

bool POP3::updateDB(){
	return true;
}

bool POP3::quit(){
	write("QUIT");
	read();
	internet->close();
	return error;
}