//By GrapheneCT (From VBackup)

#ifndef BHBB_DIALOG_H
#define BHBB_DIALOG_H

#include <kernel.h>
#include <paf.h>
#include <common_gui_dialog.h>

using namespace paf;

class Dialog
{
public:

	enum ButtonCode
	{
		ButtonCode_X = 1,
		ButtonCode_Ok,
		ButtonCode_Cancel,
		ButtonCode_Yes,
		ButtonCode_No,
		ButtonCode_Button1 = ButtonCode_Yes,
		ButtonCode_Button2 = ButtonCode_No,
		ButtonCode_Button3 = ButtonCode_Cancel
	};

	typedef void(*EventHandler)(ButtonCode buttonCode, ScePVoid pUserArg);

	static SceVoid OpenPleaseWait(Plugin *workPlugin, wchar_t *titleText, wchar_t *messageText, SceBool withCancel = SCE_FALSE, EventHandler eventHandler= SCE_NULL, ScePVoid userArg = SCE_NULL);

	static SceVoid OpenYesNo(Plugin *workPlugin, wchar_t *titleText, wchar_t *messageText, EventHandler eventHandler = SCE_NULL, ScePVoid userArg = SCE_NULL);

	static SceVoid OpenError(Plugin *workPlugin, SceInt32 errorCode, wchar_t *messageText = SCE_NULL, EventHandler eventHandler = SCE_NULL, ScePVoid userArg = SCE_NULL);

	static SceVoid OpenTwoButton(
		Plugin *workPlugin,
		wchar_t *titleText,
		wchar_t *messageText,
		SceUInt32 button1TextHashref,
		SceUInt32 button2TextHashref,
		EventHandler eventHandler = SCE_NULL,
		ScePVoid userArg = SCE_NULL);

	static SceVoid OpenThreeButton(
		Plugin *workPlugin,
		wchar_t *titleText,
		wchar_t *messageText,
		SceUInt32 button1TextHashref,
		SceUInt32 button2TextHashref,
		SceUInt32 button3TextHashref,
		EventHandler eventHandler = SCE_NULL,
		ScePVoid userArg = SCE_NULL);

	static SceVoid Close();

	static SceVoid WaitEnd();

private:

	static SceVoid CommonGuiEventHandler(SceInt32 instanceSlot, sce::CommonGuiDialog::ButtonCode buttonCode, ScePVoid pUserArg);

};

#endif
