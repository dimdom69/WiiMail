#ifndef _SMTP_H_
#define _SMTP_H_

#include "email.h"
#include "server.h"

//SMTP status codes. Some are slightly ambiguous, but helpful impformation can be derived from all.

//Connection errors
#define SMTP_CONNECTION_ERROR 			101
#define SMTP_CONNECTION_REFUSED 		111

//Successful responses
#define SMTP_STATUS 					211
#define SMTP_HELP						214
#define SMTP_SERVER_READY				220
#define SMTP_GOODBYE					221
#define SMTP_DELIVERED					250
#define SMTP_RELAYED					251
#define SMTP_DELIVERED_UNVERIFIED		252

//Sometimes sent after from and to are sent to server
#define SMTP_START_DATA					354

//Local Errors
#define SMTP_ERROR_BLOCKED				420
#define SMTP_ERROR_UNAVAILABLE			421
#define SMTP_ERROR_STORAGE				422
#define SMTP_ERROR_MEMORY				432
#define SMTP_ERROR_NO_RESPONSE			441
#define SMTP_ERROR_CONNECTION_DROPPED	442
#define SMTP_ERROR_LOOP					446
#define SMTP_ERROR_TIMED_OUT			447
#define SMTP_ERROR_ROUTING				449
#define SMTP_ERROR_IP 					450
#define SMTP_ERROR_ABORTED				451
#define SMTP_ERROR_OVERLOAD				452
#define SMTP_ERROR_SPAM					471

//Remote errors
#define SMTP_ERROR_SYNTAX				500
#define SMTP_ERROR_PARAMETERS			501
#define SMTP_ERROR_NOT_IMPL				502
#define SMTP_ERROR_AUTH					503
#define SMTP_ERROR_PARAM_NOT_IMPL		504
#define SMTP_ERROR_BAD_EMAIL			510
#define SMTP_ERROR_BAD_EMAIL_2			511
#define SMTP_ERROR_DNS					512
#define SMTP_ERROR_ADDRESS_TYPE			513
#define SMTP_ERROR_SIZE					523
#define SMTP_ERROR_AUTH_2				530
#define SMTP_ERROR_REJECTED				541
#define SMTP_ERROR_NOT_EXIST			550
#define SMTP_ERROR_RELAY_DENIED			551
#define SMTP_ERROR_ABORTED_2			552
#define SMTP_ERROR_INVALID				553
#define SMTP_ERROR_FAILED				554

#define READ_BUFFER_SIZE 1024
#define SEND_BUFFER_SIZE 128

class SMTP : public Server{
public:
	SMTP(Internet* _internet, char* _address, int _port, char* _username, char* _password, int _ssl);
	~SMTP();
	bool sendMail(email_t* _email);

private:
	bool write(char* input);
	bool read();
	bool parse();

	bool connect();
	bool hello();
	bool parseHello();
	bool startTLS();
	bool authorizePlain();
	bool authorizeLogin();
	bool sendHeader();
	bool sendMessage();
	bool quit();

	email_t* email;
	char* response;
	char* message;
	int statusCode;
	bool error;
	bool authPlain;
	bool authLogin;
};

#endif