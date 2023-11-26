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