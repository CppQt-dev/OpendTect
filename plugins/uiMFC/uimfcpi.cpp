/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          May 2010
________________________________________________________________________

-*/

#include "uimfcdialog.h"
#include "uimenu.h"
#include "uiodmenumgr.h"
#include "plugins.h"
#include "uimainwin.h"



class MnuMgr : public CallBacker
{
public:
		MnuMgr(uiODMain&);

protected:
    void	Mnufnctn(CallBacker*);
    uiODMain&	appl_;
};


MnuMgr::MnuMgr( uiODMain& a )
    : appl_(a)
{
    uiAction* newitem =
	new uiAction( "Launch MFC Dialog", mCB(this,MnuMgr,Mnufnctn) );
    appl_.menuMgr().utilMnu()->insertItem( newitem );
}


void MnuMgr::Mnufnctn( CallBacker* )
{
    initMFCDialog( NULL );
}


mExternC  int GetuiMFCPluginType()
{
    return PI_AUTO_INIT_LATE;
}

mExternC PluginInfo* GetuiMFCPluginInfo()
{
    static PluginInfo retpi = {
        "MFC Classes for UI -- Demo",
        "dGB Earth Sciences",
        "1.0",
        "Shows simple MFC Dialog Class built in.\nDeveloper Studio" };
    return &retpi;
}


mExternC const char* InituiMFCPlugin( int, char** )
{
    (void)new MnuMgr( *ODMainWin() );
    return 0; // All OK - no error messages
}


