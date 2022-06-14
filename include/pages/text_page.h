#ifndef TEXT_PAGE_H
#define TEXT_PAGE_H

#include "page.h"

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