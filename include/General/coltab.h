#ifndef coltab_h
#define coltab_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: coltab.h,v 1.3 2008-10-27 11:38:39 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "color.h"

namespace ColTab
{

    const char*		defSeqName();
    float		defClipRate();
    float		defSymMidval();
    void		setMapperDefaults(float cr,float sm,bool histeq=false);

}


#endif
