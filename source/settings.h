#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <stdio.h>

#include "tinyxml2.h"
#include "server.h"

using namespace tinyxml2;

class Settings{
public:
	Settings();
	~Settings();
	bool load(char* file);
	bool save(char* file);
	void loadDefaults();

	char* smtpServer;
	int smtpPort;
	char* smtpUsername;
	char* smtpPassword;
	int smtpSSL;

	char* popServer;
	int popPort;
	char* popUsername;
	char* popPassword;
	int popSSL;

	char* name;
	char* email;
	char* signature;

private:
	FILE* fp;
    XMLDocument* doc;
};

#endif