#ifndef _INBOX_H_
#define _INBOX_H_

#include <time.h>

#include "accounts.h"

/*
Inboxes are "slices" from the database. All emails stored in an SQL db, and inbox
shows those that are to the user. DB stored as accountname.db
*/
class Inbox{
public:
	Inbox(Account* account);
	~Inbox();
	bool getInbox(char* filter);
	bool getNew();
	bool getAll();
	
private:
	Account* _account;
	time_t lastUpdate;
};

#endif