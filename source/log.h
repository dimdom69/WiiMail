#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>

class Log{
public:
	Log(const char* fn="log.txt", const char* dir="/");
	~Log();
	void write(const char* message);

private:
	void initFat(const char* dir);
	void openFile(const char* fn);
	void closeFile();

	FILE* fp;
};

#endif