#ifndef BHBB_PAGE_HPP
#define BHBB_PAGE_HPP
namespace generic
{
    class Page
    {
    private:
        static paf::ui::Plane *templateRoot;
        static generic::Page *currPage;
        generic::Page *prev;
    
    public:
    	paf::ui::Plane *root;

        static void Init();
		static void DeleteCurrentPage();
        static void BackButtonEventHandler(SceInt32, paf::ui::Widget *, SceInt32, ScePVoid);
		
        Page(const char *pageName);
		virtual ~Page();
    };
}

#endif