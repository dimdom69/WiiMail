#include <fat.h>
#include <stdio.h>
#include <unistd.h>

#include "settings.h"
#include "tinyxml2.h"

using namespace tinyxml2;

Settings::Settings(){
	if (!fatInitDefault()){
	//error
	}
	doc = new XMLDocument;
	smtpServer = new char[128];
	smtpUsername = new char[128];
	smtpPassword = new char[128];

	popServer = new char[128];
	popUsername = new char[128];
	popPassword = new char[128];

	name = new char[128];
	email = new char[128];
	signature = new char[1024];
}

Settings::~Settings(){
	delete doc;
}

void Settings::loadDefaults(){
	smtpServer[0] = '\0';
	smtpPort = 25;
	smtpUsername[0] = '\0';
	smtpPassword[0] = '\0';
	smtpSSL = 0;

	popServer[0] = '\0';
	popPort = 110;
	popUsername[0] = '\0';
	popPassword[0] = '\0';
	popSSL = 0;

	name[0] = '\0';
	email[0] = '\0';
	strcpy(signature, "Sent from WiiMail");
}

bool Settings::load(char* file){
	if (fp = fopen(file, "r")){
        fclose(fp);
    }
    else{
    	loadDefaults();
    }
    doc->LoadFile(file);
    strncpy(smtpServer, doc->FirstChildElement("smtpServer")->Attribute("smtpServer"), 128);
    smtpPort = doc->FirstChildElement("smtpPort")->IntAttribute("smtpPort");
    strncpy(smtpUsername, doc->FirstChildElement("smtpUsername")->Attribute("smtpUsername"), 128);
    strncpy(smtpPassword, doc->FirstChildElement("smtpPassword")->Attribute("smtpPassword"), 128);
    smtpSSL = doc->FirstChildElement("smtpSSL")->IntAttribute("smtpSSL");

    strncpy(popServer, doc->FirstChildElement("popServer")->Attribute("popServer"), 128);
    popPort = doc->FirstChildElement("popPort")->IntAttribute("popPort");
    strncpy(popUsername, doc->FirstChildElement("popUsername")->Attribute("popUsername"), 128);
    strncpy(popPassword, doc->FirstChildElement("popPassword")->Attribute("popPassword"), 128);
    popSSL = doc->FirstChildElement("popSsl")->IntAttribute("popSSL");

    strncpy(name, doc->FirstChildElement("name")->Attribute("name"), 128);
    strncpy(email, doc->FirstChildElement("email")->Attribute("email"), 128);
    strncpy(signature, doc->FirstChildElement("signature")->Attribute("signature"), 1024);
    return true;
}

bool Settings::save(char* file){
	char buffer[24];
	fp = fopen (file, "w");
	if (fp == NULL) {
		//error
	}

	XMLPrinter printer(fp);

	printer.OpenElement("smtpServer");
	printer.PushAttribute("smtpServer", smtpServer);
	printer.CloseElement();
	sprintf(buffer, "%d", smtpPort);
	printer.OpenElement("smtpPort");
	printer.PushAttribute("smtpPort", buffer);
	printer.CloseElement();
	printer.OpenElement("smtpUsername");
	printer.PushAttribute("smtpUsername", smtpUsername);
	printer.CloseElement();
	printer.OpenElement("smtpPassword");
	printer.PushAttribute("smtpPassword", smtpPassword);
	printer.CloseElement();
	sprintf(buffer, "%d", smtpSSL);
	printer.OpenElement("smtpSSL");
	printer.PushAttribute("smtpSSL", buffer);
	printer.CloseElement();
	printer.OpenElement("smtpTls");
	printer.PushAttribute("smtpTls", buffer);
	printer.CloseElement();

	printer.OpenElement("popServer");
	printer.PushAttribute("popServer", popServer);
	printer.CloseElement();
	sprintf(buffer, "%d", popPort);
	printer.OpenElement("popPort");
	printer.PushAttribute("popPort", buffer);
	printer.CloseElement();
	printer.OpenElement("popUsername");
	printer.PushAttribute("popUsername", popUsername);
	printer.CloseElement();
	printer.OpenElement("popPassword");
	printer.PushAttribute("popPassword", popPassword);
	printer.CloseElement();
	sprintf(buffer, "%d", popSSL);
	printer.OpenElement("popSSL");
	printer.PushAttribute("popSSL", buffer);
	printer.CloseElement();

	printer.OpenElement("name");
	printer.PushAttribute("name", name);
	printer.CloseElement();
	printer.OpenElement("email");
	printer.PushAttribute("email", email);
	printer.CloseElement();
	printer.OpenElement("signature");
	printer.PushAttribute("signature", signature);
	printer.CloseElement();

	fclose(fp);

	return true;
}