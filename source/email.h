#ifndef _EMAIL_H_
#define _EMAIL_H_

typedef struct email{
	bool read;
	char* subject;
	char* message;
	char* to;
	char* from;
}email_t;

#endif