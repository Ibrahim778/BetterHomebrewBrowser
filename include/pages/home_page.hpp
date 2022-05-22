#ifndef HOME_PAGE_HPP
#define HOME_PAGE_HPP

#include "page.hpp"
#include "parser.hpp"

namespace home
{
    class Page : public generic::Page
    {
    public: 
        Page();
        virtual ~Page();

        SceVoid Load();
        SceVoid Populate();

        SceVoid OnParse(parser::HomebrewList::homeBrewInfo *parsedInfo);

        paf::ui::ImageButton *AddOption(paf::wstring *title);
    
        class LoadThread : public paf::thread::Thread
        {
        public:
            using paf::thread::Thread::Thread;

            SceVoid EntryFunction();

            Page *callingPage;
        };

        class PopulateJob : public paf::thread::JobQueue::Item
        {
        public:
            paf::thread::JobQueue::Item::Item;

            ~PopulateJob();

            SceVoid Run();
            SceVoid Finish();

            static SceVoid JobKiller(paf::thread::JobQueue::Item *job)
            {
                if(job) delete job;
            }

            Page *callingPage;
        };

    private:
        paf::string dbIndex;
        home::Page::LoadThread *loadThread;
        paf::thread::JobQueue *populateQueue;
    };
}

#endif