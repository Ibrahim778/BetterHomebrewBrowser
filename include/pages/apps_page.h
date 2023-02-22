#ifndef BHBB_APPS_PAGE_H
#define BHBB_APPS_PAGE_H

#include <kernel.h>
#include <paf.h>
#include <vector>

#include "db.h"
#include "dialog.h"
#include "page.h"

namespace apps
{
    class Page : public generic::Page
    {
    public:
        class IconZipJob : public paf::job::JobItem
        {
        public:
            using paf::job::JobItem::JobItem;

            SceVoid Run();
            SceVoid Finish(){}
        };

        class LoadJob : public paf::job::JobItem
        {
        public:
            using paf::job::JobItem::JobItem;

            SceVoid Run();
            SceVoid Finish(){}

            Page *callingPage;

            LoadJob(const char *name, Page *caller):job::JobItem(name),callingPage(caller){}
        };

        class ListViewCB : public paf::ui::ListView::ItemCallback
        {
        public:
            ListViewCB(apps::Page *workPage)
            {
                this->workPage = workPage;
            }

            ~ListViewCB()
            {

            }

            ui::ListItem *Create(Param *info);

            SceVoid Start(Param *info)
            {
                info->parent->PlayEffect(0.0f, effect::EffectType_Fadein1);
            }

            apps::Page *workPage;
        };

        // (Thanks Graphene (pt 3))
        class AsyncIconLoader
        {
        public:

            AsyncIconLoader(const char *path, paf::ui::Widget *target, paf::graph::Surface **surf = SCE_NULL, SceBool autoLoad = true);

            ~AsyncIconLoader();

            SceVoid Load();

            SceVoid Abort();

        private:

            class TargetDeleteEventCallback : public paf::ui::EventCallback
            {
            public:

                TargetDeleteEventCallback(AsyncIconLoader *parent)
                {
                    workObj = parent;
                }

                virtual ~TargetDeleteEventCallback();

                AsyncIconLoader *workObj;
            };

            class Job : public paf::job::JobItem
            {
            public:

                using paf::job::JobItem::JobItem;

                ~Job() { }

                SceVoid Run();

                SceVoid Finish();

                static SceBool DownloadCancelCheck(ScePVoid pUserData);

                AsyncIconLoader *workObj;
                paf::ui::Widget *target;
                paf::string fPath;
                paf::graph::Surface **loadedSurface;
            };

            Job *item;
        };

        enum PageMode
        {
            PageMode_Browse,
            PageMode_Search
        };

        static SceVoid IconDownloadDecideCB(Dialog::ButtonCode buttonResult, ScePVoid userDat);
        static SceVoid ErrorRetryCB(Dialog::ButtonCode buttonResult, ScePVoid userDat);
        static SceVoid SearchCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
        static SceVoid CategoryCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
        static SceVoid RefreshCB(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
        static SceVoid QuickCategoryCB(paf::input::GamePad::GamePadData *data, ScePVoid pUserData);

        //Sets page mode: Search or Browse
        SceVoid SetMode(PageMode mode);
        //Cancels any icon download jobs, Redownloads and parses index and calls Redisplay()
        SceVoid Load();

        //Locks the current cateogry, (Prevents SetCategory from working)
        SceVoid LockCategory();
        //Unlocks the current category, (Restores SetCategory functionality)
        SceVoid ReleaseCategory();

        //Retrieve the current target list
        db::List *GetTargetList();
        //Set the current target list
        SceVoid SetTargetList(db::List *list);

        //Hides the list 
        SceVoid ClearList();
        //Show the list according to the current target list
        SceVoid DisplayList();
        //Redisplays the list (will also reflect page number & category updates)
        SceVoid Redisplay();

        //Hides the IME keyboard if opened for search
        SceVoid HideKeyboard();

        //Set the current category
        SceBool SetCategory(int category);
        //Get the current category
        int GetCategory();
        //Set the current category list
        SceVoid SetCategories(const std::vector<db::Category>& categoryList);

        Page();
        virtual ~Page();
    
    private:

        //Current PageMode
        PageMode mode;
        
        //Full parsed list
        db::List appList;
        //List for current search items
        db::List searchList;
        //Pointer to current list in use
        db::List *targetList;

        //Current Category
        int category;
        //Category lock
        bool locked;

        //Widgets
        paf::ui::BusyIndicator *busyIndicator;
        paf::ui::CornerButton *optionsButton;
        paf::ui::Button *refreshButton;
        paf::ui::Button *searchButton;
        paf::ui::Button *searchEnterButton;
        paf::ui::Button *searchBackButton;
        paf::ui::TextBox *searchBox;
        paf::ui::ListView *listView;
    };

    namespace button
    {
        class Callback : public paf::ui::EventCallback
        {
        public:
            Callback(apps::Page *page){
                eventHandler = OnGet;
                pUserData = page;
            }

            static void OnGet(SceInt32 eventID, paf::ui::Widget *self, SceInt32 unk, ScePVoid pUserData);
        };
    };
}

#endif