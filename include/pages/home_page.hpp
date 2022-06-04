#ifndef HOME_PAGE_HPP
#define HOME_PAGE_HPP

#include <paf.h>

#include "page.hpp"
#include "db.hpp"

namespace home
{
    class Page : public generic::Page
    {
    public: 

        class LoadJob : public paf::thread::JobQueue::Item
        {
        public:
            using paf::thread::JobQueue::Item::Item;

            SceVoid Run();
            SceVoid Finish();

            Page *callingPage;
        };

        class PopulateJob : public paf::thread::JobQueue::Item
        {
        public:
            using paf::thread::JobQueue::Item::Item;

            SceVoid Run();
            SceVoid Finish();

            Page *callingPage;
        };

        class IconLoadThread : public::paf::thread::Thread
        {
        public:
            using paf::thread::Thread::Thread;
            
            SceVoid EntryFunction();

            db::entryInfo *startEntry;
            Page *callingPage;
        };

        struct PageBody 
        {
            PageBody *prev;
            paf::ui::Plane *widget;
            IconLoadThread *iconThread;
            db::entryInfo *startEntry;
        };

        enum PageMode
        {
            PageMode_Browse,
            PageMode_Search
        };

        Page();
        virtual ~Page();

        void OnRedisplay() override; 

        SceVoid Load();
        SceVoid Redisplay();

        SceBool SetCategory(int category);

        SceVoid SetMode(PageMode mode);

        static SceVoid DeleteBody(void *body);
        static SceVoid ForwardButtonCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
        static SceVoid SearchCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
        static SceVoid CategoryButtonCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);

        PageBody *MakeNewBody();

        paf::thread::JobQueue *loadQueue;
        PageBody *body;

        db::entryInfo *list; //Will point to either home::Page::searchList or home::Page::parsedList
        int listSize;

        db::entryInfo *parsedList; //Will point to entire parsed db
        int parsedListSize;

        db::entryInfo *searchList; //Will point to list with matching search elements
        int searchListSize;

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

        paf::string dbIndex;
        int category;

        paf::ui::BusyIndicator *busyIndicator;
    };
}

#endif