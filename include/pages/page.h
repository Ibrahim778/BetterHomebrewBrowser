#ifndef BHBB_PAGE_H
#define BHBB_PAGE_H

#include <paf.h>

namespace page
{
    class Base
    {
    public:
        Base(uint32_t hash, paf::Plugin::PageOpenParam openParam, paf::Plugin::PageCloseParam closeParam);
		virtual ~Base();

        uint32_t GetHash();

        paf::ui::Scene *root;

        static void DeleteCurrentPage();
        static page::Base *GetCurrentPage();
        static void DefaultBackButtonCB(uint32_t eventID, paf::ui::Handler *self, paf::ui::Event *event, ScePVoid pUserData);
    
    protected:
        paf::ui::CornerButton *backButton;
        paf::Plugin::PageCloseParam closeParam;
    };
    
}

#endif