// From NetStream, by @GrapheneCt

#include <kernel.h>
#include <libdbg.h>
#include <paf.h>
#include <common_gui_dialog.h>

#include "dialog.h"

#define CURRENT_DIALOG_NONE -1

using namespace paf;
using namespace sce;

static SceInt32 s_currentDialog = CURRENT_DIALOG_NONE;
static dialog::EventHandler s_currentEventHandler = SCE_NULL;

static SceUInt32 s_twoButtonContTable[12];
static SceUInt32 s_threeButtonContTable[16];

namespace dialog {
	SceVoid CommonGuiEventHandler(SceInt32 instanceSlot, CommonGuiDialog::DIALOG_CB buttonCode, ScePVoid pUserArg)
	{
		CommonGuiDialog::Dialog::Close(instanceSlot);
		s_currentDialog = CURRENT_DIALOG_NONE;

		if (s_currentEventHandler) {
			s_currentEventHandler((dialog::ButtonCode)buttonCode, pUserArg);
			s_currentEventHandler = SCE_NULL;
		}
	}
}

SceVoid dialog::OpenPleaseWait(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText, SceBool withCancel, EventHandler eventHandler, ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::ThreadIDCache::Check(thread::ThreadIDCache::Type_Main);

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Lock();
	if (withCancel)
		s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogCancelBusy, CommonGuiEventHandler, userArg);
	else
		s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogTextSmallBusy, CommonGuiEventHandler, userArg);

    if (!isMainThread)
		thread::RMutex::main_thread_mutex.Unlock();
}

SceVoid dialog::OpenYesNo(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText, EventHandler eventHandler, ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::ThreadIDCache::Check(thread::ThreadIDCache::Type_Main);

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Lock();
	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogYesNo, CommonGuiEventHandler, userArg);
	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Unlock();
}

SceVoid dialog::OpenOk(Plugin *workPlugin, const wchar_t *titleText, const wchar_t *messageText, EventHandler eventHandler, ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::ThreadIDCache::Check(thread::ThreadIDCache::Type_Main);

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Lock();
	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &CommonGuiDialog::Param::s_dialogOk, CommonGuiEventHandler, userArg);
	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Unlock();
}

SceVoid dialog::OpenError(Plugin *workPlugin, SceInt32 errorCode, const wchar_t *messageText, EventHandler eventHandler, ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::ThreadIDCache::Check(thread::ThreadIDCache::Type_Main);

	CommonGuiDialog::ErrorDialog dialog;

	auto cb = new CommonGuiDialog::EventCBListener(CommonGuiEventHandler, userArg);

	dialog.work_plugin = workPlugin;
	dialog.error = errorCode;
	dialog.listener = cb;
	if (messageText)
		dialog.message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Lock();
	s_currentDialog = dialog.Show();
	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Unlock();
}

SceVoid dialog::OpenThreeButton(
	Plugin *workPlugin,
	const wchar_t *titleText,
	const wchar_t *messageText,
	SceUInt32 button1TextHashref,
	SceUInt32 button2TextHashref,
	SceUInt32 button3TextHashref,
	EventHandler eventHandler,
	ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::ThreadIDCache::Check(thread::ThreadIDCache::Type_Main);

	CommonGuiDialog::Param dparam;

	dparam = CommonGuiDialog::Param::s_dialogYesNoCancel;
	sce_paf_memcpy(s_threeButtonContTable, CommonGuiDialog::Param::s_dialogYesNoCancel.contents_list, sizeof(s_threeButtonContTable));
	s_threeButtonContTable[1] = button1TextHashref;
	s_threeButtonContTable[5] = button2TextHashref;
	s_threeButtonContTable[9] = button3TextHashref;
	s_threeButtonContTable[7] = 0x20413274;
	s_threeButtonContTable[11] = 0x20413274;
	dparam.contents_list = (CommonGuiDialog::ContentsHashTable *)s_threeButtonContTable;

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Lock();
	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &dparam, CommonGuiEventHandler, userArg);
	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Unlock();
}

SceVoid dialog::OpenTwoButton(
	Plugin *workPlugin,
	const wchar_t *titleText,
	const wchar_t *messageText,
	SceUInt32 button1TextHashref,
	SceUInt32 button2TextHashref,
	EventHandler eventHandler,
	ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return;

	SceBool isMainThread = thread::ThreadIDCache::Check(thread::ThreadIDCache::Type_Main);

	CommonGuiDialog::Param dparam;

	dparam = CommonGuiDialog::Param::s_dialogYesNo;
	sce_paf_memcpy(s_twoButtonContTable, CommonGuiDialog::Param::s_dialogYesNo.contents_list, sizeof(s_twoButtonContTable));
	s_twoButtonContTable[1] = button2TextHashref;
	s_twoButtonContTable[5] = button1TextHashref;
	s_twoButtonContTable[3] = 0x20413274;
	dparam.contents_list = (CommonGuiDialog::ContentsHashTable *)s_twoButtonContTable;

	wstring title = titleText;
	wstring message = messageText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Lock();
	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, &message, &dparam, CommonGuiEventHandler, userArg);
	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Unlock();
}

ui::ListView *dialog::OpenListView(
	Plugin *workPlugin,
	const wchar_t *titleText,
	EventHandler eventHandler,
	ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return SCE_NULL;

	SceBool isMainThread = thread::ThreadIDCache::Check(thread::ThreadIDCache::Type_Main);

	wstring title = titleText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Lock();

	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, SCE_NULL, &CommonGuiDialog::Param::s_dialogXLView, CommonGuiEventHandler, userArg);
	ui::Widget *ret = CommonGuiDialog::Dialog::GetWidget(s_currentDialog, CommonGuiDialog::REGISTER_ID_LIST_VIEW);

	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Unlock();

	return (ui::ListView *)ret;
}

ui::ScrollView *dialog::OpenScrollView(
	Plugin *workPlugin,
	const wchar_t *titleText,
	EventHandler eventHandler,
	ScePVoid userArg)
{
	if (s_currentDialog != CURRENT_DIALOG_NONE)
		return SCE_NULL;

	SceBool isMainThread = thread::ThreadIDCache::Check(thread::ThreadIDCache::Type_Main);

	wstring title = titleText;

	s_currentEventHandler = eventHandler;

	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Lock();

	s_currentDialog = CommonGuiDialog::Dialog::Show(workPlugin, &title, SCE_NULL, &CommonGuiDialog::Param::s_dialogXView, CommonGuiEventHandler, userArg);
	ui::Widget *ret = CommonGuiDialog::Dialog::GetWidget(s_currentDialog, CommonGuiDialog::REGISTER_ID_SCROLL_VIEW);

	if (!isMainThread)
		thread::RMutex::main_thread_mutex.Unlock();

	return (ui::ScrollView *)ret;
}

SceVoid dialog::Close()
{
	if (s_currentDialog == CURRENT_DIALOG_NONE)
		return;

	CommonGuiDialog::Dialog::Close(s_currentDialog);
	s_currentDialog = CURRENT_DIALOG_NONE;
	s_currentEventHandler = SCE_NULL;
}

SceVoid dialog::WaitEnd()
{
	if (s_currentDialog == CURRENT_DIALOG_NONE)
		return;

	while (s_currentDialog != CURRENT_DIALOG_NONE) {
		thread::Sleep(100);
	}
}