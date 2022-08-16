#ifndef BHBB_APPS_PAGE_H
#define BHBB_APPS_PAGE_H

#include <kernel.h>
#include <paf.h>

#include "page.h"
#include "db.h"

namespace apps
{
    class Page : public generic::Page
    {
    public:

        enum PageMode
        {
            PageMode_Browse,
            PageMode_Search
        };

        class Body 
        {
        public:
            Body(Page *page);
            ~Body();

            paf::ui::Plane *GetBody();

        private:
            Body *prev, *next;
            paf::ui::Plane *widget;
            Page *page;
        };

        static SceVoid SearchCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);

        SceVoid NewPage();
        SceVoid SetMode(PageMode mode);
        SceVoid Load();

        Page();
        virtual ~Page();
    
    private:
        enum
        {
            Hash_All = 0x59E75663,
            Hash_Game = 0x6222A81A,
            Hash_Emu = 0xD1A8D19,
            Hash_Port = 0xADC6272A,
            Hash_Util = 0x1EFEFBA6,

            Hash_SearchButton = 0xCCCE2527,
            Hash_SearchEnterButton = 0xAB8CB65E,
            Hash_SearchBackButton = 0x6A2C094C,
        } ButtonHash;

        PageMode mode;

        db::List appList;

        Body *body;
    };
}

#endif