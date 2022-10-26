#ifndef BHBB_MULTI_PAGE_LIST_H
#define BHBB_MULTI_PAGE_LIST_H

#include <kernel.h>
#include <paf.h>

#include "page.h"
#include "db.h"

namespace generic
{
    class MultiPageAppList : public generic::Page
    {
    public:
        class ClearJob : public paf::job::JobItem
        {
        public:
            using paf::job::JobItem::JobItem;

            SceVoid Run();
            SceVoid Finish(){}

            MultiPageAppList *callingPage;
            ClearJob(const char *name, MultiPageAppList *caller):paf::job::JobItem(name),callingPage(caller){}
        };

        class NewPageJob : public paf::job::JobItem
        {
        public:
            using paf::job::JobItem::JobItem;

            SceVoid Run();
            SceVoid Finish(){}

            SceBool populate;
            MultiPageAppList *callingPage;
            NewPageJob(const char *name, MultiPageAppList *caller, SceBool _populate):paf::job::JobItem(name),callingPage(caller),populate(_populate){}
        };

        class DeletePageJob : public paf::job::JobItem
        {
        public:
            using paf::job::JobItem::JobItem;

            SceVoid Run();
            SceVoid Finish(){}

            SceBool animate;
            MultiPageAppList *callingPage;
            DeletePageJob(const char *name, MultiPageAppList *caller, SceBool _animate):paf::job::JobItem(name),callingPage(caller),animate(_animate){}
        };

        static SceVoid ForwardButtonCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
        static SceVoid BackCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);

        //Redisplays the elements in the parsed db list according to category, also resets the current page number to 0 (1)
        SceVoid Redisplay();

        //Gets the current list widget
        paf::ui::Widget *GetCurrentPage();
        //Clears all lists
        SceVoid ClearPages();

        //Sets the category and updates button colour
        SceBool SetCategory(int category);

        //Creates a new empty page
        SceVoid NewPage(SceBool populate = SCE_TRUE);
        //Deletes the current list page
        SceVoid DeletePage(SceBool animate = SCE_TRUE);
        
        SceVoid SetTargetList(db::List *targetList);
        db::List *GetTargetList();

        int GetCategory();
        //Returns the number of pages in the list, calculated using the page list (currBody)
        SceUInt32 GetPageCount();

        //Called BEFORE pages are going to be deleted
        virtual SceVoid OnClear();
        //Called AFTER pages have been deleted
        virtual SceVoid OnCleared();
        //Called whenever the category is changed from SetCategory()
        virtual SceVoid OnCategoryChanged(int prevCategory, int currentCategory);
        //Called to populate pages made with NewPage()
        virtual SceVoid PopulatePage(paf::ui::Widget *scrollBox);
        
        MultiPageAppList(db::List *targetList = SCE_NULL, const char *templateName = "");
        virtual ~MultiPageAppList();
        
    private:
        struct Body {
            paf::ui::Widget *widget;
            Body *prev;

            Body(Body *_prev = SCE_NULL):prev(_prev){}
        };

        SceVoid ClearInternal();
        SceVoid HandleForwardButton();
        SceVoid CreateListWrapper();

        SceVoid _NewPage(SceBool populate);
        SceVoid _DeletePage(SceBool animate);

        //Called whenever this page becomes the main page on screen. Handles forward button
        void OnRedisplay() override;

        Body *currBody;

        paf::ui::Plane *listWrapperPlane;
        paf::ui::Plane *listRootPlane;

        db::List *targetList;

        SceInt32 category;

    };
}

#endif