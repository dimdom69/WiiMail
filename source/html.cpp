#include <cstring>

#include "html.h"

HTML::HTML(){

}

HTML::~HTML(){

}

char* HTML::parse(char* input, int inLength){
	output = new char[inLength];
	char* tag;
	int tagLen = 0;
	int outPos = 0;
	tag_t* tagBuf;
	bool tagOpen = false;

	for(int i = 0; i < inLength; i++){
		if(input[i] == '<'){ //Open a tag
			tagLen = 0;
			tag = input + i;
			tagOpen = true;
		}
		else if(input[i] == '>' && input[i] != '/'){
			tagBuf = parseTag(tag, tagLen);
			if(strcmp(tagBuf->name, "br") != 0){
				output[outPos] = '\n';
				outPos++;
			}
			deleteTag(tagBuf);
			tagOpen = false;
		}
		else if(!tagOpen){
			output[outPos] = input[i];
			outPos++;
		}
		else if(tagOpen){
			tagLen++;
		}
	}
	output[outPos] = '\0';
	return output;
}

tag_t* HTML::parseTag(char* tag, int tagLen){
	tag_t* tagReturn = new tag_t;
	int pos = 0;
	int nameStart = 0;
	while(tag[pos] == ' ' && pos < tagLen){ //Delete any whitespace before tag name
		pos++;
	}
	if(pos == tagLen){
		return tagReturn; //error
	}
	nameStart = pos;
	while(tag[pos] != ' ' && pos < tagLen){
		pos++;
	}
	tagReturn->name = new char[pos - nameStart];
	strncpy(tagReturn->name, tag+nameStart, pos-nameStart);
	tagReturn->attributes = NULL;
	tagReturn->numAttributes = 0; //for now
	if(pos == tagLen){
		return tagReturn; //nothing left
	}
	return tagReturn; //stoppig here anyways
}

void HTML::deleteTag(tag_t* tag){
	if(tag != NULL){
		if(tag->name != NULL){
			delete [] tag->name;
		}
		if(tag->attributes != NULL){
			for(int i = 0; i<tag->numAttributes; i++){
				if(tag->attributes[i] != NULL){
					if(tag->attributes[i]->name != NULL){
						delete [] tag->attributes[i]->name;
					}
					if(tag->attributes[i]->value != NULL){
						delete [] tag->attributes[i]->value;
					}
				}
			}
			delete [] tag->attributes;
		}
		delete [] tag;
	}
}