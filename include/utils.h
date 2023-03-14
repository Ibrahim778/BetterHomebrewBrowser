#ifndef BHBB_UTILS_H
#define BHBB_UTILS_H

#include <paf.h>
#include <vector>

#include "db.h"

namespace Utils
{
    //Why didn't I think of this either? (Thanks Graphene (pt 2))
    class SimpleEventCallback : public paf::ui::EventCallback
	{
	public:

		SimpleEventCallback(paf::ui::EventCallback::EventHandler function, ScePVoid userArg = SCE_NULL)
		{
			eventHandler = function;
			pUserData = userArg;
		};

		virtual ~SimpleEventCallback()
		{

		};
	};

    namespace Misc
    {
        void InitMusic();
        SceVoid StartBGDL();    
        SceUInt32 GetHash(const char *id);
    }
    namespace str 
    {
        void ToLowerCase(char *string);
        SceVoid GetFromID(const char *id, paf::string *out);
        SceVoid GetFromHash(SceUInt64 id, paf::string *out);
        SceVoid GetfFromID(const char *id, paf::string *out);
        SceVoid GetfFromHash(SceUInt64 id, paf::string *out);
        wchar_t *GetPFromID(const char *id);
        wchar_t *GetPFromHash(SceUInt64 Hash);
    };

    namespace Net 
    {
        SceVoid HttpsToHttp(paf::string &str);
        SceBool IsValidURLSCE(const char *url); //Can this URL be used with SceHttp?
    };
    
    namespace Widget
    {
        SceInt32 PlayEffect(paf::ui::Widget *widget, SceFloat32 param, paf::effect::EffectType animId, paf::ui::EventCallback::EventHandler animCB = (paf::ui::EventCallback::EventHandler)SCE_NULL, ScePVoid pUserData = SCE_NULL);
        SceInt32 PlayEffectReverse(paf::ui::Widget *widget, SceFloat32 param, paf::effect::EffectType animId, paf::ui::EventCallback::EventHandler animCB = (paf::ui::EventCallback::EventHandler)SCE_NULL, ScePVoid pUserData = SCE_NULL);

        template<class T = paf::ui::Widget>
        T *GetChild(paf::ui::Widget *parent, SceUInt32 hash)
        {
            paf::rco::Element search;
            search.hash = hash;
            return (T*)parent->GetChild(&search, 0);
        }

        template<class T = paf::ui::Widget>
        T *Create(const char *id, const char *type, const char *style, paf::ui::Widget *parent)
        {
            rco::Element styleInfo;
            styleInfo.hash = Utils::Misc::GetHash(style);
            rco::Element widgetInfo;
            widgetInfo.hash = Utils::Misc::GetHash(id);

            return (T*)g_appPlugin->CreateWidgetWithStyle(parent, type, &widgetInfo, &styleInfo);
        }        
        SceInt32 SetLabel(paf::ui::Widget *widget, const char *text);
        SceInt32 SetLabel(paf::ui::Widget *widget, paf::string &text);
        SceVoid DeleteTexture(paf::graph::Surface **tex);
    };
    
    namespace GamePad 
    {
        typedef void(*CallbackFunction)(paf::input::GamePad::GamePadData *pData, ScePVoid pUserData);
        
        typedef struct GamePadCB
        {
            CallbackFunction func;
            ScePVoid pUserData;

            GamePadCB(CallbackFunction _func, ScePVoid _pUserData):func(_func),pUserData(_pUserData){}
        };

        static std::vector<GamePadCB> cbList;
        static void CB(paf::input::GamePad::GamePadData *pData);
        static SceInt32 buttons = 0;

        SceVoid RegisterButtonUpCB(CallbackFunction cb, ScePVoid pUserData = SCE_NULL);
    };

    namespace Time
    {
        static const char *SavePath = "savedata0:/previous_dl_time";

        void ResetPreviousDLTime();
        void LoadPreviousDLTime();
        void SavePreviousDLTime();

        paf::rtc::Tick GetPreviousDLTime(db::Id source);
        SceVoid SetPreviousDLTime(db::Id source, paf::rtc::Tick time);

        //Should not be directly accessed
        static paf::rtc::Tick previousDownloadTime[sizeof(db::info) / sizeof(db::dbInfo)];
    };
#ifdef _DEBUG
    SceVoid PrintAllChildren(paf::ui::Widget *widget, int offset = 0);
#endif

};
#endif