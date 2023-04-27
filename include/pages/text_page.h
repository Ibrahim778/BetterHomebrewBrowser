#ifndef _TEXT_PAGE_H_
#define _TEXT_PAGE_H_

#include <paf.h>

#include "page.h"

namespace page 
{
    class TextPage : public page::Base
    {
    public:
        TextPage();

        TextPage(const paf::string& txt);
        TextPage(const paf::wstring& txt);
        TextPage(uint32_t strHash);

        virtual ~TextPage(){}

        virtual void SetText(const paf::string& txt);
        virtual void SetText(uint32_t hash);
        virtual void SetText(const paf::wstring& str);

    protected:
        paf::ui::Text *text;
    };
};

#endif