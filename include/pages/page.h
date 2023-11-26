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

#ifndef _PAGE_H_
#define _PAGE_H_

#include <paf.h>

namespace page
{
    class Base
    {
    public:
        Base(uint32_t hash, paf::Plugin::PageOpenParam openParam, paf::Plugin::PageCloseParam closeParam);
		virtual ~Base();

        uint32_t GetHash();

        paf::ui::Scene *root;

        static void DeleteCurrentPage();
        static page::Base *GetCurrentPage();
        static void DefaultBackButtonCB(uint32_t eventID, paf::ui::Handler *self, paf::ui::Event *event, ScePVoid pUserData);
    
    protected:
        paf::ui::CornerButton *backButton;
        paf::Plugin::PageCloseParam closeParam;
    };
    
}

#endif