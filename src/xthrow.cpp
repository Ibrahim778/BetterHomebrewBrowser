#include <kernel.h>
#include "print.h"

namespace std
{
    void _Xlength_error(const char* const _Message) {
        print("%s\n", _Message);
    }

    void _Xout_of_range(const char* const _Message) {
        print("%s\n", _Message);
    }
};