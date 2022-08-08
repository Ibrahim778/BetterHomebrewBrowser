#include <paf.h>

#include "utils.h"
#include "pages/page.h"
#include "pages/text_page.h"

text::Page::Page(const char *text):generic::Page::Page("text_page_template")
{
    paf::wstring wstr;
    paf::wstring::CharToNewWString(text, &wstr);
    Utils::GetChildByHash(root, Utils::GetHashById("page_text"))->SetLabel(&wstr);
}

text::Page::~Page()
{

}