#include <paf.h>

#include "utils.h"
#include "pages/page.h"
#include "pages/text_page.h"

text::Page::Page(const char *text):generic::Page::Page("text_page_template")
{
    Utils::Widget::SetLabel(Utils::Widget::GetChild(root, Utils::Misc::GetHash("page_text")), text);
}

text::Page::~Page()
{

}