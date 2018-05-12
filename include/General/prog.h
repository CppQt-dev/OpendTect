#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		5-12-1995
________________________________________________________________________

 Include this file in any executable program you make. The file is actually
 pretty empty ....

-*/

#include "plugins.h"
#include "debug.h"
#include "od_ostream.h"
#include "odruncontext.h"
#include "genc.h"


#ifdef __msvc__
# ifndef _CONSOLE
#  include "winmain.h"
# endif
#endif
