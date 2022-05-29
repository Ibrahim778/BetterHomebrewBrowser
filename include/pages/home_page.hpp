#ifndef HOME_PAGE_HPP
#define HOME_PAGE_HPP

#include <paf.h>

#include "page.hpp"
#include "parser.hpp"

namespace home
{
    class Page : public generic::Page
    {
    public: 
        Page();
        virtual ~Page();

        void OnRedisplay() override; 

        SceVoid Load();
        SceVoid Redisplay();

        static SceVoid DeleteBody(void *body);
        static SceVoid ForwardButtonCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
    
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

            parser::HomebrewList::node *startNode;
        };

        struct PageBody 
        {
            PageBody *prev;
            paf::ui::Plane *widget;
            IconLoadThread *iconThread;
            parser::HomebrewList::node *startNode;
        };


        PageBody *MakeNewBody();
        
        paf::thread::JobQueue *loadQueue;
        PageBody *body;

    private:
        paf::string dbIndex;

        paf::ui::BusyIndicator *busyIndicator;
    };

    class MoreButtonEventHandler : public paf::ui::Widget::EventCallback
    {
    public:
        MoreButtonEventHandler();

        static void OnGet(SceInt32 eventID, paf::ui::Widget *self, SceInt32, ScePVoid puserData);
    };
}

#endif