#ifndef BHBB_PAGE_HPP
#define BHBB_PAGE_HPP
namespace generic
{
    class Page
    {
    public:
        typedef void(*BackButtonEventCallback)(void *pUserData);

    	paf::ui::Plane *root;

        static void Setup();
		static void DeleteCurrentPage();
        static void BackButtonEventHandler(SceInt32, paf::ui::Widget *, SceInt32, ScePVoid);

        static void SetBackButtonEvent(BackButtonEventCallback callback, void *data);

        Page(const char *pageName);
		virtual ~Page();

        virtual void OnRedisplay();
        virtual void OnDelete();

    private:
        static paf::ui::Plane *templateRoot;
        static generic::Page *currPage;
        generic::Page *prev;
    
        static BackButtonEventCallback backCallback;
        static void *backData;
    };
}

#endif