
//By GrapheneCT (From VBackup)

#include <kernel.h>
#include <message_dialog.h>
#include <libdbg.h>
#include <paf.h>
#include <stdlib.h>
#include <common_gui_dialog.h>

#include "main.h"
#include "dialog.h"

#define CURRENT_DIALOG_NONE -1

using namespace paf;
using namespace sce;

static SceInt32 s_currentDialog = CURRENT_DIALOG_NONE;
static Dialog::EventHandler s_currentEventHandler = SCE_NULL;

static SceUInt32 s_twoButtonContTable[12];
static SceUInt32 s_threeButtonContTable[16];

SceVoid Dialog::CommonGuiEventHandler(SceInt32 instanceSlot, CommonGuiDialog::ButtonCode buttonCode, ScePVoid pUserArg)
{
	CommonGuiDialog::Dialog::Close(instanceSlot);
	s_currentDialog = CURRENT_DIALOG_NONE;

	if (s_currentEventHandler) {
		s_currentEventHandler((Dialog::ButtonCode)buttonCode, pUserArg);
		s_currentEventHandler = SCE_NULL;
	}
}

SceVoid Dialog::OpenPleaseWait(Plugin *workPlugin, wchar_t *titleText, wchar_t *messageText, SceBool withCancel, EventHandler eventHandler, ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::IsMainThread();

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::s_mainThreadMutex.Lock();

	if (withCancel)
		s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogCancelBusy, CommonGuiEventHandler, userArg);
	else
		s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogTextSmallBusy, CommonGuiEventHandler, userArg);
	if (!isMainThread)
		thread::s_mainThreadMutex.Unlock();
}

SceVoid Dialog::OpenYesNo(Plugin *workPlugin, wchar_t *titleText, wchar_t *messageText, EventHandler eventHandler, ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::IsMainThread();

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::s_mainThreadMutex.Lock();
	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogYesNo, CommonGuiEventHandler, userArg);
	if (!isMainThread)
		thread::s_mainThreadMutex.Unlock();
}

SceVoid Dialog::OpenError(Plugin *workPlugin, SceInt32 errorCode, wchar_t *messageText, EventHandler eventHandler, ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::IsMainThread();

	CommonGuiDialog::ErrorDialog dialog;

	auto cb = new CommonGuiDialog::EventCallback();
	cb->eventHandler = CommonGuiEventHandler;
	cb->pUserData = userArg;

	dialog.workPlugin = workPlugin;
	dialog.errorCode = errorCode;
	dialog.eventHandler = cb;
	if (messageText)
		dialog.message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::s_mainThreadMutex.Lock();
	s_currentDialog = dialog.Show();
	if (!isMainThread)
		thread::s_mainThreadMutex.Unlock();
}

SceVoid Dialog::OpenThreeButton(
	Plugin *workPlugin,
	wchar_t *titleText,
	wchar_t *messageText,
	SceUInt32 button1TextHashref,
	SceUInt32 button2TextHashref,
	SceUInt32 button3TextHashref,
	EventHandler eventHandler,
	ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::IsMainThread();

	CommonGuiDialog::Param dparam;

	dparam = CommonGuiDialog::Param::s_dialogYesNoCancel;
	sce_paf_memcpy(s_threeButtonContTable, CommonGuiDialog::Param::s_dialogYesNoCancel.contentsList, sizeof(s_threeButtonContTable));
	s_threeButtonContTable[1] = button1TextHashref;
	s_threeButtonContTable[5] = button2TextHashref;
	s_threeButtonContTable[9] = button3TextHashref;
	s_threeButtonContTable[7] = 0x20413274;
	s_threeButtonContTable[11] = 0x20413274;
	dparam.contentsList = (CommonGuiDialog::ContentsHashTable *)s_threeButtonContTable;

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::s_mainThreadMutex.Lock();
	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &dparam, CommonGuiEventHandler, userArg);
	if (!isMainThread)
		thread::s_mainThreadMutex.Unlock();
}

SceVoid Dialog::OpenTwoButton(
	Plugin *workPlugin,
	wchar_t *titleText,
	wchar_t *messageText,
	SceUInt32 button1TextHashref,
	SceUInt32 button2TextHashref,
	EventHandler eventHandler,
	ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::IsMainThread();

	CommonGuiDialog::Param dparam;

	dparam = CommonGuiDialog::Param::s_dialogYesNo;
	sce_paf_memcpy(s_twoButtonContTable, CommonGuiDialog::Param::s_dialogYesNo.contentsList, sizeof(s_twoButtonContTable));
	s_twoButtonContTable[1] = button2TextHashref;
	s_twoButtonContTable[5] = button1TextHashref;
	s_twoButtonContTable[3] = 0x20413274;
	dparam.contentsList = (CommonGuiDialog::ContentsHashTable *)s_twoButtonContTable;

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::s_mainThreadMutex.Lock();
	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &dparam, CommonGuiEventHandler, userArg);
	if (!isMainThread)
		thread::s_mainThreadMutex.Unlock();
}

SceVoid Dialog::Close()
{
	if (s_currentDialog == CURRENT_DIALOG_NONE)
		return;

	CommonGuiDialog::Dialog::Close(s_currentDialog);
	s_currentDialog = CURRENT_DIALOG_NONE;
	s_currentEventHandler = SCE_NULL;
}

SceVoid Dialog::WaitEnd()
{
	if (s_currentDialog == CURRENT_DIALOG_NONE)
		return;

	while (s_currentDialog != CURRENT_DIALOG_NONE) {
		thread::Sleep(100);
	}
}