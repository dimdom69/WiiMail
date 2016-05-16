/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * menu.h
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#ifndef _MENU_H_
#define _MENU_H_

#include <ogcsys.h>

void InitGUIThreads();
void MainMenu (int menuitem);

enum
{
	MENU_EXIT = -1,
	MENU_NONE,
	MENU_HOME,
	MENU_INBOX,
	MENU_COMPOSE,
	MENU_SETTINGS,
	MENU_SMTP_SETTINGS,
	MENU_POP_SETTINGS
};

#endif
