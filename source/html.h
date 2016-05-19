#ifndef _HTML_H_
#define _HTML_H_

//For now the html parser only strips the html, rendering only the line breaks

typedef struct _attribute_t{
	char* name;
	char* value;
}attribute_t;

typedef struct _tag_t{
	char* name;
	attribute_t** attributes;
	int numAttributes;
}tag_t;


class HTML{
public:
	HTML();
	~HTML();
	char* parse(char* input, int inLength);

private:
	tag_t* parseTag(char* tag, int tagLen);
	void deleteTag(tag_t* tag);
	char* output;
};

#endif