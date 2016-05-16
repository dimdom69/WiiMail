#include <cstring>
#include <stdio.h>
#include <wolfssl/wolfcrypt/coding.h>

#include "smtp.h"

SMTP::SMTP(Internet* _internet, char* _address, int _port, char* _username, char* _password, bool _ssl, bool _tls)
	: Server(_internet, _address, _port, _username, _password, _ssl){
	response = new char[READ_BUFFER_SIZE];
	message = new char[SEND_BUFFER_SIZE];
	error = false;
	tls = _tls;
	authPlain = false;
	authLogin = false;
}

SMTP::~SMTP(){
	delete [] response;
	delete [] message;
}

bool SMTP::sendMail(email_t* _email){
	email = _email;
	connect();
	hello();
	if(tls){
		startTLS();
	}
	parseHello();
	if(username != NULL && password != NULL){
		if(authPlain){
			authorizePlain();
		}
		else if(authLogin){
			authorizeLogin();
		}
	}
	sendMessage();
	quit();
	return true;
}

bool SMTP::read(){
	internet->read(READ_BUFFER_SIZE, response);
	return parse();
}

bool SMTP::write(char* input){
	sprintf(message,"%s\r\n", input);
	return internet->write(message);
}

bool SMTP::parse(){
	//Converts first three chars of response to an int. Should be status code
	statusCode = ((int)response[0] - 48)*100 + ((int)response[1] - 48)*10 + ((int)response[2] - 48);

	if(response[0] == '1' || response[0] == '4' || response[0] == '5'){
		error = true;
	}

	return true;
}

bool SMTP::parseHello(){
	char* authLine = strstr(response, "AUTH");
	if(authLine == NULL){
		return false;
	}
	if(strstr(authLine, "PLAIN")){
		authPlain = true;
	}
	if(strstr(authLine, "LOGIN")){
		authLogin = true;
	}
	return true;
}

bool SMTP::connect(){
	return internet->connect(address, port, TCP, ssl);
}

bool SMTP::hello(){
	read(); //Read the server's welcome message first
	sprintf(message,"EHLO %s", internet->localip);
	write(message);
	read();
	return error;
}

bool SMTP::startTLS(){
	write("STARTTLS");
	read();
	internet->startSsl();
	hello();
	return error;
}

bool SMTP::authorizeLogin(){
	//Encode username and password in base64
	byte userPlain[1024];
	byte passPlain[1024];
	byte userEncoded[1024];
	byte passEncoded[1024];
	word32 userLen = strlen(username);
	word32 passLen = strlen(password);
	word32 userEncLen = sizeof(userEncoded);
	word32 passEncLen = sizeof(passEncoded);
	Base64_Encode(userPlain, userLen, userEncoded, &userEncLen);
	Base64_Encode(passPlain, passLen, passEncoded, &passEncLen);
	userEncoded[userEncLen-1] = '\0';
	passEncoded[passEncLen-1] = '\0';

	//Send result to server
	write("AUTH LOGIN");
	read();
	write((char*)userEncoded);
	read();
	write((char*)passEncoded);
	read();
	return error;
}

bool SMTP::authorizePlain(){
	//Encode both username and password combined in base64
	char bothPlain[512];
	char bothEncoded[512];
	bothPlain[0] = '\0';
	strcpy(&bothPlain[1], username);
	strcpy(&bothPlain[strlen(username)+2], password);
	int plainLen = strlen(username) + strlen(password) + 2; //Add two for the null bytes. Don't add extra for trailing \0 as we don't want that to be encoded
	int bothLen = sizeof(bothEncoded);
	Base64_Encode((byte*)bothPlain, (word32)plainLen, (byte*)bothEncoded, (word32*)&bothLen);
	bothEncoded[bothLen-1] = '\0';

	//Send result to server
	write("AUTH PLAIN");
	read();
	write((char*)bothEncoded);
	read();
	return error;
}

bool SMTP::sendHeader(){
	sprintf(message,"MAIL FROM:<%s>",email->from);
	write(message);
	read();
	sprintf(message,"RCPT TO:<%s>",email->to);
	write(message);
	read();
	return error;
}

bool SMTP::sendMessage(){
	sendHeader();
	write("DATA");
	read();
	sprintf(message,"From: <%s>",email->from);
	write(message);
	sprintf(message,"To: <%s>",email->to);
	write(message);
	sprintf(message,"Subject: %s",email->subject);
	write(message);
	sprintf(message,"\r\n%s",email->message);
	write(message);
	write(".");
	read();
	return error;
}

bool SMTP::quit(){
	write("QUIT");
	read();
	internet->close();
	return error;
}