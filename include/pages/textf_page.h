#include <paf.h>

#include "text_page.h"

namespace page 
{
    class TextfPage : public page::TextPage
    {
    public:
        TextfPage(const paf::string& str);
        TextfPage(const paf::wstring& str);
        TextfPage(uint32_t hash);

        virtual ~TextfPage(){}

        void SetText(const paf::string& str) override;
        void SetText(const paf::wstring& str) override;
        void SetText(uint32_t hash) override;
    };
};