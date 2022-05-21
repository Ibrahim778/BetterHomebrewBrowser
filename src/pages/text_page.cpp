#include <paf.h>

#include "utils.hpp"
#include "pages/page.hpp"
#include "pages/text_page.hpp"

text::Page::Page(const char *text):generic::Page::Page("text_page_template")
{
    paf::wstring wstr;
    paf::wstring::CharToNewWString(text, &wstr);
    Utils::GetChildByHash(root, Utils::GetHashById("page_text"))->SetLabel(&wstr);
}

text::Page::~Page()
{

}