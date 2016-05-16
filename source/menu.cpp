/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * menu.cpp
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>

#include "libwiigui/gui.h"
#include "menu.h"
#include "main.h"
#include "input.h"
#include "filelist.h"
#include "filebrowser.h"
#include "email.h"
#include "smtp.h"
#include "settings.h"

#define THREAD_SLEEP 100

static GuiImageData * pointer[4];
static GuiImage * bgImg = NULL;
static GuiSound * bgMusic = NULL;
static GuiWindow * mainWindow = NULL;
static lwp_t guithread = LWP_THREAD_NULL;
static bool guiHalt = true;
static GuiText* netTxt = NULL;
Internet *internet = NULL;
Settings* settings = NULL;

/****************************************************************************
 * ResumeGui
 *
 * Signals the GUI thread to start, and resumes the thread. This is called
 * after finishing the removal/insertion of new elements, and after initial
 * GUI setup.
 ***************************************************************************/
static void ResumeGui(){
	guiHalt = false;
	LWP_ResumeThread (guithread);
}

/****************************************************************************
 * HaltGui
 *
 * Signals the GUI thread to stop, and waits for GUI thread to stop
 * This is necessary whenever removing/inserting new elements into the GUI.
 * This eliminates the possibility that the GUI is in the middle of accessing
 * an element that is being changed.
 ***************************************************************************/
static void HaltGui(){
	guiHalt = true;

	// wait for thread to finish
	while(!LWP_ThreadIsSuspended(guithread))
		usleep(THREAD_SLEEP);
}

/****************************************************************************
 * WindowPrompt
 *
 * Displays a prompt window to user, with information, an error message, or
 * presenting a user with a choice
 ***************************************************************************/
int WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label){
	int choice = -1;

	GuiWindow promptWindow(448,288);
	promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	promptWindow.SetPosition(0, -10);
	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	GuiImageData dialogBox(dialogue_box_png);
	GuiImage dialogBoxImg(&dialogBox);

	GuiText titleTxt(title, 26, (GXColor){0, 0, 0, 255});
	titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	titleTxt.SetPosition(0,40);
	GuiText msgTxt(msg, 22, (GXColor){0, 0, 0, 255});
	msgTxt.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	msgTxt.SetPosition(0,-20);
	msgTxt.SetWrap(true, 400);

	GuiText btn1Txt(btn1Label, 22, (GXColor){0, 0, 0, 255});
	GuiImage btn1Img(&btnOutline);
	GuiImage btn1ImgOver(&btnOutlineOver);
	GuiButton btn1(btnOutline.GetWidth(), btnOutline.GetHeight());

	if(btn2Label)
	{
		btn1.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
		btn1.SetPosition(20, -25);
	}
	else
	{
		btn1.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
		btn1.SetPosition(0, -25);
	}

	btn1.SetLabel(&btn1Txt);
	btn1.SetImage(&btn1Img);
	btn1.SetImageOver(&btn1ImgOver);
	btn1.SetSoundOver(&btnSoundOver);
	btn1.SetTrigger(&trigA);
	btn1.SetState(STATE_SELECTED);
	btn1.SetEffectGrow();

	GuiText btn2Txt(btn2Label, 22, (GXColor){0, 0, 0, 255});
	GuiImage btn2Img(&btnOutline);
	GuiImage btn2ImgOver(&btnOutlineOver);
	GuiButton btn2(btnOutline.GetWidth(), btnOutline.GetHeight());
	btn2.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	btn2.SetPosition(-20, -25);
	btn2.SetLabel(&btn2Txt);
	btn2.SetImage(&btn2Img);
	btn2.SetImageOver(&btn2ImgOver);
	btn2.SetSoundOver(&btnSoundOver);
	btn2.SetTrigger(&trigA);
	btn2.SetEffectGrow();

	promptWindow.Append(&dialogBoxImg);
	promptWindow.Append(&titleTxt);
	promptWindow.Append(&msgTxt);
	promptWindow.Append(&btn1);

	if(btn2Label)
		promptWindow.Append(&btn2);

	promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_IN, 50);
	HaltGui();
	mainWindow->SetState(STATE_DISABLED);
	mainWindow->Append(&promptWindow);
	mainWindow->ChangeFocus(&promptWindow);
	ResumeGui();

	while(choice == -1)
	{
		usleep(THREAD_SLEEP);

		if(btn1.GetState() == STATE_CLICKED)
			choice = 1;
		else if(btn2.GetState() == STATE_CLICKED)
			choice = 0;
	}

	promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 50);
	while(promptWindow.GetEffect() > 0) usleep(THREAD_SLEEP);
	HaltGui();
	mainWindow->Remove(&promptWindow);
	mainWindow->SetState(STATE_DEFAULT);
	ResumeGui();
	return choice;
}

/****************************************************************************
 * UpdateGUI
 *
 * Primary thread to allow GUI to respond to state changes, and draws GUI
 ***************************************************************************/

static void *UpdateGUI (void *arg){
	int i;

	while(1)
	{
		if(guiHalt)
		{
			LWP_SuspendThread(guithread);
		}
		else
		{
			UpdatePads();
			mainWindow->Draw();

			#ifdef HW_RVL
			for(i=3; i >= 0; i--) // so that player 1's cursor appears on top!
			{
				if(userInput[i].wpad->ir.valid)
					Menu_DrawImg(userInput[i].wpad->ir.x-48, userInput[i].wpad->ir.y-48,
						96, 96, pointer[i]->GetImage(), userInput[i].wpad->ir.angle, 1, 1, 255);
				DoRumble(i);
			}
			#endif

			Menu_Render();

			for(i=0; i < 4; i++)
				mainWindow->Update(&userInput[i]);

			if(ExitRequested)
			{
				for(i = 0; i <= 255; i += 15)
				{
					mainWindow->Draw();
					Menu_DrawRectangle(0,0,screenwidth,screenheight,(GXColor){0, 0, 0, u8(i)},1);
					Menu_Render();
				}
				ExitApp();
			}
		}
		if(netTxt && internet){
			netTxt->SetText(internet->getState());
		}
	}
	return NULL;
}

/****************************************************************************
 * InitGUIThread
 *
 * Startup GUI threads
 ***************************************************************************/
void InitGUIThreads(){
	LWP_CreateThread (&guithread, UpdateGUI, NULL, NULL, 0, 70);
}

/****************************************************************************
 * OnScreenKeyboard
 *
 * Opens an on-screen keyboard window, with the data entered being stored
 * into the specified variable.
 ***************************************************************************/
static void OnScreenKeyboard(char * var, u16 maxlen){
	int save = -1;

	GuiKeyboard keyboard(var, maxlen);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	GuiText okBtnTxt("OK", 22, (GXColor){0, 0, 0, 255});
	GuiImage okBtnImg(&btnOutline);
	GuiImage okBtnImgOver(&btnOutlineOver);
	GuiButton okBtn(btnOutline.GetWidth(), btnOutline.GetHeight());

	okBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	okBtn.SetPosition(25, -25);

	okBtn.SetLabel(&okBtnTxt);
	okBtn.SetImage(&okBtnImg);
	okBtn.SetImageOver(&okBtnImgOver);
	okBtn.SetSoundOver(&btnSoundOver);
	okBtn.SetTrigger(&trigA);
	okBtn.SetEffectGrow();

	GuiText cancelBtnTxt("Cancel", 22, (GXColor){0, 0, 0, 255});
	GuiImage cancelBtnImg(&btnOutline);
	GuiImage cancelBtnImgOver(&btnOutlineOver);
	GuiButton cancelBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	cancelBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	cancelBtn.SetPosition(-25, -25);
	cancelBtn.SetLabel(&cancelBtnTxt);
	cancelBtn.SetImage(&cancelBtnImg);
	cancelBtn.SetImageOver(&cancelBtnImgOver);
	cancelBtn.SetSoundOver(&btnSoundOver);
	cancelBtn.SetTrigger(&trigA);
	cancelBtn.SetEffectGrow();

	keyboard.Append(&okBtn);
	keyboard.Append(&cancelBtn);

	HaltGui();
	mainWindow->SetState(STATE_DISABLED);
	mainWindow->Append(&keyboard);
	mainWindow->ChangeFocus(&keyboard);
	ResumeGui();

	while(save == -1)
	{
		usleep(THREAD_SLEEP);

		if(okBtn.GetState() == STATE_CLICKED)
			save = 1;
		else if(cancelBtn.GetState() == STATE_CLICKED)
			save = 0;
	}

	if(save)
	{
		snprintf(var, maxlen, "%s", keyboard.kbtextstr);
	}

	HaltGui();
	mainWindow->Remove(&keyboard);
	mainWindow->SetState(STATE_DEFAULT);
	ResumeGui();
}

/****************************************************************************
 * MenuSettings
 ***************************************************************************/
static int MenuSettings()
{
	int menu = MENU_NONE;

	GuiText titleTxt("Settings", 28, (GXColor){255, 255, 255, 255});
	titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	titleTxt.SetPosition(50,50);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiImageData btnLargeOutline(button_large_png);
	GuiImageData btnLargeOutlineOver(button_large_over_png);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	GuiTrigger trigHome;
	trigHome.SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, 0);

	GuiText smtpBtnTxt("SMTP", 22, (GXColor){0, 0, 0, 255});
	smtpBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage smtpBtnImg(&btnLargeOutline);
	GuiImage smtpBtnImgOver(&btnLargeOutlineOver);
	GuiButton smtpBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	smtpBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	smtpBtn.SetPosition(50, 150);
	smtpBtn.SetLabel(&smtpBtnTxt);
	smtpBtn.SetImage(&smtpBtnImg);
	smtpBtn.SetImageOver(&smtpBtnImgOver);
	smtpBtn.SetSoundOver(&btnSoundOver);
	smtpBtn.SetTrigger(&trigA);
	smtpBtn.SetEffectGrow();

	GuiText popBtnTxt("POP", 22, (GXColor){0, 0, 0, 255});
	popBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage popBtnImg(&btnLargeOutline);
	GuiImage popBtnImgOver(&btnLargeOutlineOver);
	GuiButton popBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	popBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	popBtn.SetPosition(250, 150);
	popBtn.SetLabel(&popBtnTxt);
	popBtn.SetImage(&popBtnImg);
	popBtn.SetImageOver(&popBtnImgOver);
	popBtn.SetSoundOver(&btnSoundOver);
	popBtn.SetTrigger(&trigA);
	popBtn.SetEffectGrow();

	GuiText backBtnTxt("Back", 22, (GXColor){0, 0, 0, 255});
	backBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage backBtnImg(&btnLargeOutline);
	GuiImage backBtnImgOver(&btnLargeOutlineOver);
	GuiButton backBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	backBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	backBtn.SetPosition(50, -80);
	backBtn.SetLabel(&backBtnTxt);
	backBtn.SetImage(&backBtnImg);
	backBtn.SetImageOver(&backBtnImgOver);
	backBtn.SetSoundOver(&btnSoundOver);
	backBtn.SetTrigger(&trigA);
	backBtn.SetEffectGrow();

	HaltGui();
	GuiWindow w(screenwidth, screenheight);
	w.Append(&titleTxt);
	w.Append(&smtpBtn);
	w.Append(&popBtn);
	w.Append(&backBtn);

	mainWindow->Append(&w);

	ResumeGui();

	while(menu == MENU_NONE)
	{
		usleep(THREAD_SLEEP);

		if(smtpBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_SMTP_SETTINGS;
		}

		if(popBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_POP_SETTINGS;
		}

		if(backBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_HOME;
		}
	}

	HaltGui();
	mainWindow->Remove(&w);
	return menu;
}


/****************************************************************************
 * MenuSmtpSettings
 ***************************************************************************/
static int MenuSmtpSettings()
{
	int menu = MENU_NONE;
	char buffer[128];

	GuiText titleTxt("SMTP Settings", 28, (GXColor){255, 255, 255, 255});
	titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	titleTxt.SetPosition(50,50);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiImageData btnLargeOutline(button_large_png);
	GuiImageData btnLargeOutlineOver(button_large_over_png);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	GuiTrigger trigHome;
	trigHome.SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, 0);

	GuiText serverBtnTxt("Server", 22, (GXColor){0, 0, 0, 255});
	serverBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage serverBtnImg(&btnLargeOutline);
	GuiImage serverBtnImgOver(&btnLargeOutlineOver);
	GuiButton serverBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	serverBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	serverBtn.SetPosition(50, 100);
	serverBtn.SetLabel(&serverBtnTxt);
	serverBtn.SetImage(&serverBtnImg);
	serverBtn.SetImageOver(&serverBtnImgOver);
	serverBtn.SetSoundOver(&btnSoundOver);
	serverBtn.SetTrigger(&trigA);
	serverBtn.SetEffectGrow();

	GuiText portBtnTxt("Port", 22, (GXColor){0, 0, 0, 255});
	portBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage portBtnImg(&btnLargeOutline);
	GuiImage portBtnImgOver(&btnLargeOutlineOver);
	GuiButton portBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	portBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	portBtn.SetPosition(250, 100);
	portBtn.SetLabel(&portBtnTxt);
	portBtn.SetImage(&portBtnImg);
	portBtn.SetImageOver(&portBtnImgOver);
	portBtn.SetSoundOver(&btnSoundOver);
	portBtn.SetTrigger(&trigA);
	portBtn.SetEffectGrow();

	GuiText userBtnTxt("Username", 22, (GXColor){0, 0, 0, 255});
	userBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage userBtnImg(&btnLargeOutline);
	GuiImage userBtnImgOver(&btnLargeOutlineOver);
	GuiButton userBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	userBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	userBtn.SetPosition(50, 200);
	userBtn.SetLabel(&userBtnTxt);
	userBtn.SetImage(&userBtnImg);
	userBtn.SetImageOver(&userBtnImgOver);
	userBtn.SetSoundOver(&btnSoundOver);
	userBtn.SetTrigger(&trigA);
	userBtn.SetEffectGrow();

	GuiText passBtnTxt("Password", 22, (GXColor){0, 0, 0, 255});
	passBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage passBtnImg(&btnLargeOutline);
	GuiImage passBtnImgOver(&btnLargeOutlineOver);
	GuiButton passBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	passBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	passBtn.SetPosition(250, 200);
	passBtn.SetLabel(&passBtnTxt);
	passBtn.SetImage(&passBtnImg);
	passBtn.SetImageOver(&passBtnImgOver);
	passBtn.SetSoundOver(&btnSoundOver);
	passBtn.SetTrigger(&trigA);
	passBtn.SetEffectGrow();

	GuiText nameBtnTxt("Name", 22, (GXColor){0, 0, 0, 255});
	GuiImage nameBtnImg(&btnOutline);
	GuiImage nameBtnImgOver(&btnOutlineOver);
	GuiButton nameBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	nameBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	nameBtn.SetPosition(50, 300);
	nameBtn.SetLabel(&nameBtnTxt);
	nameBtn.SetImage(&nameBtnImg);
	nameBtn.SetImageOver(&nameBtnImgOver);
	nameBtn.SetSoundOver(&btnSoundOver);
	nameBtn.SetTrigger(&trigA);
	nameBtn.SetTrigger(&trigHome);
	nameBtn.SetEffectGrow();

	GuiText emailBtnTxt("Email", 22, (GXColor){0, 0, 0, 255});
	GuiImage emailBtnImg(&btnOutline);
	GuiImage emailBtnImgOver(&btnOutlineOver);
	GuiButton emailBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	emailBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	emailBtn.SetPosition(250, 300);
	emailBtn.SetLabel(&emailBtnTxt);
	emailBtn.SetImage(&emailBtnImg);
	emailBtn.SetImageOver(&emailBtnImgOver);
	emailBtn.SetSoundOver(&btnSoundOver);
	emailBtn.SetTrigger(&trigA);
	emailBtn.SetTrigger(&trigHome);
	emailBtn.SetEffectGrow();

	GuiText sigBtnTxt("Signature", 22, (GXColor){0, 0, 0, 255});
	GuiImage sigBtnImg(&btnOutline);
	GuiImage sigBtnImgOver(&btnOutlineOver);
	GuiButton sigBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	sigBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	sigBtn.SetPosition(450, 300);
	sigBtn.SetLabel(&sigBtnTxt);
	sigBtn.SetImage(&sigBtnImg);
	sigBtn.SetImageOver(&sigBtnImgOver);
	sigBtn.SetSoundOver(&btnSoundOver);
	sigBtn.SetTrigger(&trigA);
	sigBtn.SetTrigger(&trigHome);
	sigBtn.SetEffectGrow();

	GuiText backBtnTxt("Back", 22, (GXColor){0, 0, 0, 255});
	GuiImage backBtnImg(&btnOutline);
	GuiImage backBtnImgOver(&btnOutlineOver);
	GuiButton backBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	backBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	backBtn.SetPosition(50, -35);
	backBtn.SetLabel(&backBtnTxt);
	backBtn.SetImage(&backBtnImg);
	backBtn.SetImageOver(&backBtnImgOver);
	backBtn.SetSoundOver(&btnSoundOver);
	backBtn.SetTrigger(&trigA);
	backBtn.SetTrigger(&trigHome);
	backBtn.SetEffectGrow();

	GuiText saveBtnTxt("Save", 22, (GXColor){0, 0, 0, 255});
	GuiImage saveBtnImg(&btnOutline);
	GuiImage saveBtnImgOver(&btnOutlineOver);
	GuiButton saveBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	saveBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	saveBtn.SetPosition(250, -35);
	saveBtn.SetLabel(&saveBtnTxt);
	saveBtn.SetImage(&saveBtnImg);
	saveBtn.SetImageOver(&saveBtnImgOver);
	saveBtn.SetSoundOver(&btnSoundOver);
	saveBtn.SetTrigger(&trigA);
	saveBtn.SetTrigger(&trigHome);
	saveBtn.SetEffectGrow();

	HaltGui();
	GuiWindow w(screenwidth, screenheight);
	w.Append(&titleTxt);
	w.Append(&serverBtn);
	w.Append(&portBtn);
	w.Append(&userBtn);
	w.Append(&passBtn);
	w.Append(&nameBtn);
	w.Append(&emailBtn);
	w.Append(&sigBtn);
	w.Append(&backBtn);
	w.Append(&saveBtn);

	mainWindow->Append(&w);

	ResumeGui();

	while(menu == MENU_NONE)
	{
		usleep(THREAD_SLEEP);

		if(serverBtn.GetState() == STATE_CLICKED)
		{
			OnScreenKeyboard(settings->smtpServer, 128);
		}

		if(portBtn.GetState() == STATE_CLICKED)
		{
			sprintf(buffer, "%d", settings->smtpPort);
			OnScreenKeyboard(buffer, 128);
			settings->smtpPort = strtol(buffer, NULL, 10);
		}

		if(userBtn.GetState() == STATE_CLICKED)
		{
			OnScreenKeyboard(settings->smtpUsername, 128);
		}

		if(passBtn.GetState() == STATE_CLICKED)
		{
			OnScreenKeyboard(settings->smtpPassword, 128);
		}

		if(nameBtn.GetState() == STATE_CLICKED)
		{
			OnScreenKeyboard(settings->name, 128);
		}

		if(emailBtn.GetState() == STATE_CLICKED)
		{
			OnScreenKeyboard(settings->email, 128);
		}

		if(sigBtn.GetState() == STATE_CLICKED)
		{
			OnScreenKeyboard(settings->signature, 1024);
		}

		if(saveBtn.GetState() == STATE_CLICKED)
		{
			settings->save("WiiMail.xml");
			menu = MENU_SETTINGS;
		}

		if(backBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_SETTINGS;
		}
	}

	HaltGui();
	mainWindow->Remove(&w);
	return menu;
}


/****************************************************************************
 * MenuPopSettings
 ***************************************************************************/
static int MenuPopSettings()
{
	int menu = MENU_NONE;

	GuiText titleTxt("Settings", 28, (GXColor){255, 255, 255, 255});
	titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	titleTxt.SetPosition(50,50);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiImageData btnLargeOutline(button_large_png);
	GuiImageData btnLargeOutlineOver(button_large_over_png);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	GuiTrigger trigHome;
	trigHome.SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, 0);

	GuiText smtpBtnTxt("SMTP", 22, (GXColor){0, 0, 0, 255});
	smtpBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage smtpBtnImg(&btnLargeOutline);
	GuiImage smtpBtnImgOver(&btnLargeOutlineOver);
	GuiButton smtpBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	smtpBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	smtpBtn.SetPosition(50, 50);
	smtpBtn.SetLabel(&smtpBtnTxt);
	smtpBtn.SetImage(&smtpBtnImg);
	smtpBtn.SetImageOver(&smtpBtnImgOver);
	smtpBtn.SetSoundOver(&btnSoundOver);
	smtpBtn.SetTrigger(&trigA);
	smtpBtn.SetEffectGrow();

	GuiText popBtnTxt("POP", 22, (GXColor){0, 0, 0, 255});
	popBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage popBtnImg(&btnLargeOutline);
	GuiImage popBtnImgOver(&btnLargeOutlineOver);
	GuiButton popBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	popBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	popBtn.SetPosition(150, 50);
	popBtn.SetLabel(&popBtnTxt);
	popBtn.SetImage(&popBtnImg);
	popBtn.SetImageOver(&popBtnImgOver);
	popBtn.SetSoundOver(&btnSoundOver);
	popBtn.SetTrigger(&trigA);
	popBtn.SetEffectGrow();

	GuiText backBtnTxt("Back", 22, (GXColor){0, 0, 0, 255});
	backBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage backBtnImg(&btnLargeOutline);
	GuiImage backBtnImgOver(&btnLargeOutlineOver);
	GuiButton backBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	backBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	backBtn.SetPosition(50, 120);
	backBtn.SetLabel(&backBtnTxt);
	backBtn.SetImage(&backBtnImg);
	backBtn.SetImageOver(&backBtnImgOver);
	backBtn.SetSoundOver(&btnSoundOver);
	backBtn.SetTrigger(&trigA);
	backBtn.SetEffectGrow();

	HaltGui();
	GuiWindow w(screenwidth, screenheight);
	w.Append(&titleTxt);
	w.Append(&smtpBtn);
	w.Append(&popBtn);
	w.Append(&backBtn);

	mainWindow->Append(&w);

	ResumeGui();

	while(menu == MENU_NONE)
	{
		usleep(THREAD_SLEEP);

		if(smtpBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_SMTP_SETTINGS;
		}

		if(popBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_POP_SETTINGS;
		}

		if(backBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_HOME;
		}
	}

	HaltGui();
	mainWindow->Remove(&w);
	return menu;
}

/****************************************************************************
 * MenuHome
 ***************************************************************************/
static int MenuHome()
{
	int menu = MENU_NONE;

	GuiText titleTxt("WiiMail", 28, (GXColor){255, 255, 255, 255});
	titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	titleTxt.SetPosition(50,50);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiImageData btnLargeOutline(button_large_png);
	GuiImageData btnLargeOutlineOver(button_large_over_png);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	GuiTrigger trigHome;
	trigHome.SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, 0);

	GuiText checkBtnTxt("Check Mail", 22, (GXColor){0, 0, 0, 255});
	checkBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage checkBtnImg(&btnLargeOutline);
	GuiImage checkBtnImgOver(&btnLargeOutlineOver);
	GuiButton checkBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	checkBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	checkBtn.SetPosition(50, 120);
	checkBtn.SetLabel(&checkBtnTxt);
	checkBtn.SetImage(&checkBtnImg);
	checkBtn.SetImageOver(&checkBtnImgOver);
	checkBtn.SetSoundOver(&btnSoundOver);
	checkBtn.SetTrigger(&trigA);
	checkBtn.SetEffectGrow();

	GuiText sendBtnTxt("Send Mail", 22, (GXColor){0, 0, 0, 255});
	sendBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage sendBtnImg(&btnLargeOutline);
	GuiImage sendBtnImgOver(&btnLargeOutlineOver);
	GuiButton sendBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	sendBtn.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	sendBtn.SetPosition(0, 120);
	sendBtn.SetLabel(&sendBtnTxt);
	sendBtn.SetImage(&sendBtnImg);
	sendBtn.SetImageOver(&sendBtnImgOver);
	sendBtn.SetSoundOver(&btnSoundOver);
	sendBtn.SetTrigger(&trigA);
	sendBtn.SetEffectGrow();

	GuiText exitBtnTxt("Exit", 22, (GXColor){0, 0, 0, 255});
	GuiImage exitBtnImg(&btnOutline);
	GuiImage exitBtnImgOver(&btnOutlineOver);
	GuiButton exitBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	exitBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	exitBtn.SetPosition(100, -35);
	exitBtn.SetLabel(&exitBtnTxt);
	exitBtn.SetImage(&exitBtnImg);
	exitBtn.SetImageOver(&exitBtnImgOver);
	exitBtn.SetSoundOver(&btnSoundOver);
	exitBtn.SetTrigger(&trigA);
	exitBtn.SetTrigger(&trigHome);
	exitBtn.SetEffectGrow();

	GuiText settingsBtnTxt("Settings", 22, (GXColor){0, 0, 0, 255});
	GuiImage settingsBtnImg(&btnOutline);
	GuiImage settingsBtnImgOver(&btnOutlineOver);
	GuiButton settingsBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
	settingsBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	settingsBtn.SetPosition(100, -90);
	settingsBtn.SetLabel(&settingsBtnTxt);
	settingsBtn.SetImage(&settingsBtnImg);
	settingsBtn.SetImageOver(&settingsBtnImgOver);
	settingsBtn.SetSoundOver(&btnSoundOver);
	settingsBtn.SetTrigger(&trigA);
	settingsBtn.SetTrigger(&trigHome);
	settingsBtn.SetEffectGrow();

	HaltGui();
	GuiWindow w(screenwidth, screenheight);
	w.Append(&titleTxt);
	w.Append(&checkBtn);
	w.Append(&sendBtn);
	w.Append(&exitBtn);
	w.Append(&settingsBtn);

	mainWindow->Append(&w);

	ResumeGui();

	while(menu == MENU_NONE)
	{
		usleep(THREAD_SLEEP);

		if(checkBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_INBOX;
		}
		else if(sendBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_COMPOSE;
		}
		else if(settingsBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_SETTINGS;
		}
		else if(exitBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_EXIT;
		}
	}

	HaltGui();
	mainWindow->Remove(&w);
	return menu;
}

/****************************************************************************
 * MenuInbox
 ***************************************************************************/
static int MenuInbox()
{
	int menu = MENU_NONE;

	GuiText titleTxt("Inbox", 28, (GXColor){255, 255, 255, 255});
	titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	titleTxt.SetPosition(50,50);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiImageData btnLargeOutline(button_large_png);
	GuiImageData btnLargeOutlineOver(button_large_over_png);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	GuiTrigger trigHome;
	trigHome.SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, 0);

	GuiText backBtnTxt("Back", 22, (GXColor){0, 0, 0, 255});
	backBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage backBtnImg(&btnLargeOutline);
	GuiImage backBtnImgOver(&btnLargeOutlineOver);
	GuiButton backBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	backBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	backBtn.SetPosition(50, 120);
	backBtn.SetLabel(&backBtnTxt);
	backBtn.SetImage(&backBtnImg);
	backBtn.SetImageOver(&backBtnImgOver);
	backBtn.SetSoundOver(&btnSoundOver);
	backBtn.SetTrigger(&trigA);
	backBtn.SetEffectGrow();

	HaltGui();
	GuiWindow w(screenwidth, screenheight);
	w.Append(&titleTxt);
	w.Append(&backBtn);

	mainWindow->Append(&w);

	ResumeGui();

	while(menu == MENU_NONE)
	{
		usleep(THREAD_SLEEP);

		if(backBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_HOME;
		}
	}

	HaltGui();
	mainWindow->Remove(&w);
	return menu;
}

/****************************************************************************
 * MenuCompose
 ***************************************************************************/
static int MenuCompose()
{
	email_t* email = new email_t;
	email->from = new char[128];
	strcpy(email->from, settings->email);
	email->to = new char[128];
	email->to[0] = '\0';
	email->subject = new char[128];
	email->subject[0] = '\0';
	email->message = new char[1024];
	email->message[0] = '\0';

	SMTP* smtp = new SMTP(internet, settings->smtpServer, settings->smtpPort, settings->smtpUsername, settings->smtpPassword, true, false);

	int menu = MENU_NONE;

	GuiText titleTxt("Compose Mail", 28, (GXColor){255, 255, 255, 255});
	titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	titleTxt.SetPosition(50,50);

	GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
	GuiImageData btnOutline(button_png);
	GuiImageData btnOutlineOver(button_over_png);
	GuiImageData btnLargeOutline(button_large_png);
	GuiImageData btnLargeOutlineOver(button_large_over_png);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
	GuiTrigger trigHome;
	trigHome.SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME | WPAD_CLASSIC_BUTTON_HOME, 0);

	GuiText toBtnTxt("To", 22, (GXColor){0, 0, 0, 255});
	toBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage toBtnImg(&btnLargeOutline);
	GuiImage toBtnImgOver(&btnLargeOutlineOver);
	GuiButton toBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	toBtn.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
	toBtn.SetPosition(50, 80);
	toBtn.SetLabel(&toBtnTxt);
	toBtn.SetImage(&toBtnImg);
	toBtn.SetImageOver(&toBtnImgOver);
	toBtn.SetSoundOver(&btnSoundOver);
	toBtn.SetTrigger(&trigA);
	toBtn.SetEffectGrow();

	GuiText subBtnTxt("Subject", 22, (GXColor){0, 0, 0, 255});
	subBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage subBtnImg(&btnLargeOutline);
	GuiImage subBtnImgOver(&btnLargeOutlineOver);
	GuiButton subBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	subBtn.SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
	subBtn.SetPosition(50, 0);
	subBtn.SetLabel(&subBtnTxt);
	subBtn.SetImage(&subBtnImg);
	subBtn.SetImageOver(&subBtnImgOver);
	subBtn.SetSoundOver(&btnSoundOver);
	subBtn.SetTrigger(&trigA);
	subBtn.SetEffectGrow();

	GuiText messBtnTxt("Message", 22, (GXColor){0, 0, 0, 255});
	messBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage messBtnImg(&btnLargeOutline);
	GuiImage messBtnImgOver(&btnLargeOutlineOver);
	GuiButton messBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	messBtn.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	messBtn.SetPosition(0, 0);
	messBtn.SetLabel(&messBtnTxt);
	messBtn.SetImage(&messBtnImg);
	messBtn.SetImageOver(&messBtnImgOver);
	messBtn.SetSoundOver(&btnSoundOver);
	messBtn.SetTrigger(&trigA);
	messBtn.SetEffectGrow();

	GuiText backBtnTxt("Back", 22, (GXColor){0, 0, 0, 255});
	backBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage backBtnImg(&btnLargeOutline);
	GuiImage backBtnImgOver(&btnLargeOutlineOver);
	GuiButton backBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	backBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	backBtn.SetPosition(50, -50);
	backBtn.SetLabel(&backBtnTxt);
	backBtn.SetImage(&backBtnImg);
	backBtn.SetImageOver(&backBtnImgOver);
	backBtn.SetSoundOver(&btnSoundOver);
	backBtn.SetTrigger(&trigA);
	backBtn.SetEffectGrow();

	GuiText sendBtnTxt("Send", 22, (GXColor){0, 0, 0, 255});
	sendBtnTxt.SetWrap(true, btnLargeOutline.GetWidth()-30);
	GuiImage sendBtnImg(&btnLargeOutline);
	GuiImage sendBtnImgOver(&btnLargeOutlineOver);
	GuiButton sendBtn(btnLargeOutline.GetWidth(), btnLargeOutline.GetHeight());
	sendBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	sendBtn.SetPosition(-50, -50);
	sendBtn.SetLabel(&sendBtnTxt);
	sendBtn.SetImage(&sendBtnImg);
	sendBtn.SetImageOver(&sendBtnImgOver);
	sendBtn.SetSoundOver(&btnSoundOver);
	sendBtn.SetTrigger(&trigA);
	sendBtn.SetEffectGrow();

	HaltGui();
	GuiWindow w(screenwidth, screenheight);
	w.Append(&titleTxt);
	w.Append(&toBtn);
	w.Append(&subBtn);
	w.Append(&messBtn);
	w.Append(&backBtn);
	w.Append(&sendBtn);

	mainWindow->Append(&w);

	ResumeGui();

	while(menu == MENU_NONE)
	{
		usleep(THREAD_SLEEP);

		if(backBtn.GetState() == STATE_CLICKED)
		{
			menu = MENU_HOME;
		}

		if(toBtn.GetState() == STATE_CLICKED)
		{
			OnScreenKeyboard(email->to, 256);
		}

		if(subBtn.GetState() == STATE_CLICKED)
		{
			OnScreenKeyboard(email->subject, 256);
		}

		if(messBtn.GetState() == STATE_CLICKED)
		{
			OnScreenKeyboard(email->message, 1024);
		}

		if(sendBtn.GetState() == STATE_CLICKED)
		{
			strcat(email->message, "\n\n--\n");
			strcat(email->message, settings->signature);
			smtp->sendMail(email);
			menu = MENU_HOME;
		}
	}

	HaltGui();
	mainWindow->Remove(&w);
	delete [] email->to;
	delete [] email->subject;
	delete [] email->message;
	delete email;
	delete smtp;

	return menu;
}

/****************************************************************************
 * MainMenu
 ***************************************************************************/
void MainMenu(int menu)
{
	int currentMenu = menu;

	#ifdef HW_RVL
	pointer[0] = new GuiImageData(player1_point_png);
	pointer[1] = new GuiImageData(player2_point_png);
	pointer[2] = new GuiImageData(player3_point_png);
	pointer[3] = new GuiImageData(player4_point_png);
	#endif

	mainWindow = new GuiWindow(screenwidth, screenheight);

	bgImg = new GuiImage(screenwidth, screenheight, (GXColor){50, 50, 50, 255});
	bgImg->ColorStripe(30);
	mainWindow->Append(bgImg);


	netTxt = new GuiText("Geting network state...", 10, (GXColor){0, 0, 0, 255});
	netTxt->SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
	netTxt->SetPosition(0, 0);
	mainWindow->Append(netTxt);

	GuiTrigger trigA;
	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);

	ResumeGui();

	bgMusic = new GuiSound(bg_music_ogg, bg_music_ogg_size, SOUND_OGG);
	bgMusic->SetVolume(50);
	bgMusic->Play(); // startup music

	internet = new Internet();
	settings = new Settings();
	settings->load("WiiMail.xml");

	while(currentMenu != MENU_EXIT)
	{
		switch (currentMenu)
		{
			case MENU_HOME:
				currentMenu = MenuHome();
				break;
			case MENU_INBOX:
				currentMenu = MenuInbox();
				break;
			case MENU_COMPOSE:
				currentMenu = MenuCompose();
				break;
			case MENU_SETTINGS:
				currentMenu = MenuSettings();
				break;
			case MENU_SMTP_SETTINGS:
				currentMenu = MenuSmtpSettings();
				break;
			case MENU_POP_SETTINGS:
				currentMenu = MenuPopSettings();
				break;
			default: // unrecognized menu
				currentMenu = WindowPrompt("Error", "Unrecognized menu selected.", "Okay", NULL);
				break;
		}
	}

	ResumeGui();
	ExitRequested = 1;
	while(1) usleep(THREAD_SLEEP);

	HaltGui();

	bgMusic->Stop();
	delete bgMusic;
	delete bgImg;
	delete mainWindow;
	delete netTxt;

	delete pointer[0];
	delete pointer[1];
	delete pointer[2];
	delete pointer[3];

	delete internet;
	delete settings;

	mainWindow = NULL;
}
