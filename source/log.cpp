#include "log.h"
#include <time.h>
#include <fat.h>
#include <stdio.h>
#include <unistd.h>
#include <cstring>

void Log::initFat(const char* dir){
	if (!fatInitDefault()){
		//error
	}
	if (chdir(dir)){
		//error
	}
}

void Log::openFile(const char* fn){
	fp = fopen (fn, "w");

	if (fp == NULL) {
		//error
	}
}

void Log::closeFile(){
	fclose(fp);
}

Log::Log(const char* fn, const char* dir){
	initFat(dir);
	openFile(fn);
}

Log::~Log(){
	closeFile();
	fatUnmount(0);
}

void Log::write(const char* message){
	char currentTime[100];
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	strftime(currentTime, sizeof(currentTime)-1, "%d %m %Y %H:%M", t);
	int bufSize = strlen(message) + strlen(currentTime) + 10;
	char buffer[bufSize];
	snprintf(buffer, bufSize, "%s %s \r\n", currentTime, message);
	fputs(buffer, fp);
}
