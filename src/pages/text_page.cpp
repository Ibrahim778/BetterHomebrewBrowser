/* 
    BetterHomebrewBrowser, A homebrew browser for the PlayStation Vita with background downloading support
    Copyright (C) 2023 Muhammad Ibrahim

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.    
*/

#include <paf.h>

#include "pages/text_page.h"
#include "bhbb_plugin.h"
#include "common.h"
#include "print.h"

using namespace paf;

page::TextPage::TextPage():
page::Base(
    text_page, 
    Plugin::PageOpenParam(false, 200, Plugin::TransitionType_SlideFromBottom),  
    Plugin::PageCloseParam(true)
), text(SCE_NULL)
{
    text = (ui::Text *)root->FindChild(IDParam(page_text));
}

page::TextPage::TextPage(const string& displayText):TextPage()
{
    SetText(displayText);
}

page::TextPage::TextPage(const wstring& displayText):TextPage()
{
    SetText(displayText);
}

void page::TextPage::SetText(const string& text)
{
    wstring text16;
    common::Utf8ToUtf16(text, &text16);

    this->text->SetString(text16);
}

void page::TextPage::SetText(const wstring& text)
{
    this->text->SetString(text);
}

void page::TextPage::SetText(uint32_t hash)
{
    text->SetString(g_appPlugin->GetString(IDParam(hash)));
}

page::TextPage::TextPage(uint32_t hash):TextPage()
{
    SetText(hash);
}
