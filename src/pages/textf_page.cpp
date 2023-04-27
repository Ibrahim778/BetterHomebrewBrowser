#include <paf.h>

#include "pages/textf_page.h"
#include "common.h"
#include "print.h"

using namespace paf;
using namespace page;

TextfPage::TextfPage(const paf::string& str)
{
    SetText(str);
}

TextfPage::TextfPage(uint32_t hash)
{
    SetText(hash);
}

TextfPage::TextfPage(const wstring& text16)
{
    SetText(text16);
}

void TextfPage::SetText(const paf::wstring& str)
{
    short slashNum = 0;
    size_t strlen = str.length();
    const wchar_t *strptr = str.c_str();
    for(int i = 0; i < strlen + 1 && strptr[i] != '\0'; i++)
        if(strptr[i] == '\\') slashNum++;

    int buffSize = (strlen + 1) - slashNum;
    wchar_t *buff = new wchar_t[buffSize];
    sce_paf_wmemset(buff, 0, buffSize);

    for(wchar_t *buffPtr = buff, *strPtr = (wchar_t *)strptr; *strPtr != '\0'; strPtr++, buffPtr++)
    {
        if(*strPtr == '\\')
        {
            switch(*++strPtr)
            {
                case 'n':
                    *buffPtr = '\n';
                    break;
                case 'a':
                    *buffPtr = '\a';
                    break;
                case 'b':
                    *buffPtr = '\b';
                    break;
                case 'e':
                    *buffPtr = '\e';
                    break;
                case 'f':
                    *buffPtr = '\f';
                    break;
                case 'r':
                    *buffPtr = '\r';
                    break;
                case 'v':
                    *buffPtr = '\v';
                    break;
                case '\\':
                    *buffPtr = '\\';
                    break;
                case '\'':
                    *buffPtr = '\'';
                    break;
                case '\"':
                    *buffPtr = '\"';
                    break;
                case '?':
                    *buffPtr = '\?';
                    break;
                case 't':
                    *buffPtr = '\t';
                    break;
            }
        }
        else *buffPtr = *strPtr;
    }    

    text->SetString(buff);
    
    delete[] buff;
}

void TextfPage::SetText(uint32_t hash)
{
    SetText(g_appPlugin->GetString(IDParam(hash)));
}

void TextfPage::SetText(const paf::string &text8)
{
    wstring text16;
    common::Utf8ToUtf16(text8, &text16); 
    SetText(text16);
}
