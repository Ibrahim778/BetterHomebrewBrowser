#ifndef BHBB_PAGE_H
#define BHBB_PAGE_H

#include <paf.h>

namespace generic
{
    class Page
    {
    public:
        typedef void(*ButtonEventCallback)(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);

    	paf::ui::Plane *root;

        static void Setup();
		static void DeleteCurrentPage();
        static void ResetBackButton();
        static Page *GetCurrentPage();

        static void SetForwardButtonEvent(ButtonEventCallback callback, void *data);
        static void SetBackButtonEvent(ButtonEventCallback callback, void *data);

        Page(const char *pageName);
		virtual ~Page();

        SceUInt64 GetHash();

        virtual void OnRedisplay();
        virtual void OnDelete();

    private:
        static void BackButtonEventHandler(SceInt32, paf::ui::Widget *, SceInt32, ScePVoid);
        static void ForwardButtonEventHandler(SceInt32, paf::ui::Widget *, SceInt32, ScePVoid);
        
        static paf::ui::Plane *templateRoot;
        static generic::Page *currPage;
        generic::Page *prev;
    
        static ButtonEventCallback backCallback;
        static ButtonEventCallback forwardCallback;
        static void *backData;
        static void *forwardData;

        SceUInt64 hash;
    };
}

#endif