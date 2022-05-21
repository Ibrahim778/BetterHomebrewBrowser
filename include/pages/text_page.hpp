#ifndef TEXT_PAGE_HPP
#define TEXT_PAGE_HPP

#include "page.hpp"

namespace text
{
    class Page : public generic::Page
    {
    public: 
        Page(const char *text);
        virtual ~Page();
    };
}

#endif