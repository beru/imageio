// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#pragma once

// Change these values to use different versions
#define WINVER		0x0500
#define _WIN32_WINNT	0x0500
#define _WIN32_IE	0x0400
#define _RICHEDIT_VER	0x0100

#define NOMINMAX

#include <atlbase.h>
#if _ATL_VER >= 0x0700
	#include <atlcoll.h>
	#include <atlstr.h>
	#include <atltypes.h>
	#define _WTL_NO_CSTRING
	#define END_MSG_MAP_EX END_MSG_MAP
	#define _WTL_NO_WTYPES
#else
	#define _WTL_USE_CSTRING
#endif

#include <atlapp.h>

#include <atlframe.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atlctrlw.h>
#include <atldlgs.h>
#include <atlscrl.h>
#include <atlmisc.h>
#include <assert.h>
#include <atlcomtime.h>
#include <atlddx.h>
#include <atlcrack.h>
#include <atlsplit.h>

extern CAppModule _Module;

#include <atlwin.h>

#include "Timer.h"
#include <assert.h>

#include <memory>
#include "fastdelegate.h"

#include <vector>
#include <list>
#include <map>
#include <deque>

#include "Trace.h"

