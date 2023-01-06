#include <paf.h>

#include "utils.h"
#include "pages/page.h"
#include "pages/text_page.h"
#include "bhbb_plugin.h"

text::Page::Page(const char *text):generic::Page::Page(text_page, paf::Plugin::PageOpenParam(false, 200.0f, paf::Plugin::PageEffectType_SlideFromLeft), paf::Plugin::PageCloseParam(false, 200.0f, paf::Plugin::PageEffectType_SlideFromLeft))
{
    Utils::Widget::GetChild(root, back_button)->RegisterEventCallback(paf::ui::EventMain_Decide, new Utils::SimpleEventCallback(generic::Page::DefaultBackButtonCB));
    Utils::Widget::SetLabel(Utils::Widget::GetChild(root, page_text), text);
}

text::Page::~Page()
{

}