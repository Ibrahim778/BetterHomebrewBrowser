/* 
    BetterHomebrewBrowser, A homebrew browser for the PlayStation Vita with background downloading support
    Copyright (C) 2023 Muhammad Ibrahim
    Copyright (C) 2023 GrapheneCt

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

// This file is an extended version of @GrapheneCt's paf_runtime.cpp used in NetStream

#include <paf.h>
#include <kernel.h>
#include <libsysmodule.h>

#include "print.h"

extern "C" {
    typedef struct SceSysmoduleOpt {
		int flags;
		int *result;
		int unused[2];
	} SceSysmoduleOpt;

	typedef struct ScePafInit {
		SceSize global_heap_size;
		int a2;
		int a3;
		int cdlg_mode;
		int heap_opt_param1;
		int heap_opt_param2;
	} ScePafInit;

	int sceAppMgrGrowMemory3(unsigned int a1, int a2);

	void __cxa_set_dso_handle_main(void *dso)
	{

	}

	int _sceLdTlsRegisterModuleInfo()
	{
		return 0;
	}

	int __at_quick_exit()
	{
		return 0;
	}

    void * memmove(void *to, const void *from, size_t numBytes)
    {
        return sce_paf_memmove(to, from, numBytes);
    }
    
    int __aeabi_unwind_cpp_pr0()
    {
        return 9;
    }

    int __aeabi_unwind_cpp_pr1()
    {
        return 9;
    }

}

#ifndef __INTELLISENSE__
__attribute__((constructor(101)))
#endif 
void preloadPaf()
{
	SceInt32 ret = -1, load_res;
	void* pRet = 0;

	ScePafInit initParam;
    SceSysmoduleOpt opt;

    initParam.global_heap_size = 10 * 1024 * 1024;
    
    initParam.a2 = 0x0000EA60;
    initParam.a3 = 0x00040000;

    initParam.cdlg_mode = SCE_FALSE;

    initParam.heap_opt_param1 = 0;
    initParam.heap_opt_param2 = 0;

    //Specify that we will pass some arguments
    opt.flags = 0;
    opt.result = &load_res;

	ret = _sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(initParam), &initParam, &opt);

	if (ret < 0) {
		print("[PAF PRX] Loader: 0x%x\n", ret);
		print("[PAF PRX] Loader result: 0x%x\n", load_res);
	}
}

namespace std {
    const char * _Syserror_map(int)
    {
        return NULL;
    }
    size_t strlen(const char *a1)
    {
        return sce_paf_strlen(a1);
    }
};
