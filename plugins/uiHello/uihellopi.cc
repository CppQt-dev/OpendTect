/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2003
-*/


#include "odplugin.h"
#include "uimsg.h"
#include "uihellomod.h"

#ifdef PLAN_A

mDefODInitPlugin(uiHello)
{
    uiMSG().message( toUiString("Hello world") );
    return 0; // All OK - no error messages
}


#else /* PLAN_A is not defined */


#include "odplugin.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uistrings.h"

mDefODPluginInfo(uiHello)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"uiHello plugin - plan B",
	mODPluginTutorialsPackage,
	mODPluginCreator, mODPluginVersion,
	"This is the more extensive variant of the uiHello example.\n"
	"See the plugin manual for details.") );
    return &retpi;
}


// OK: we need an object to receive the CallBacks. In real situations,
// that may be a 'normal' object inheriting from CallBacker.

class uiHelloMgr :  public CallBacker
{ mODTextTranslationClass(uiHelloMgr);
public:

			uiHelloMgr(uiODMain&);

    uiODMain&		appl;
    void		dispMsg(CallBacker*);
};


uiHelloMgr::uiHelloMgr( uiODMain& a )
	: appl(a)
{
    uiAction* newitem = new uiAction( m3Dots(tr("Display Hello Message")),
					  mCB(this,uiHelloMgr,dispMsg) );
    appl.menuMgr().utilMnu()->insertAction( newitem );
}


class uiHelloMsgBringer : public uiDialog
{ mODTextTranslationClass(uiHelloMsgBringer);
public:

uiHelloMsgBringer( uiParent* p )
    : uiDialog(p,Setup(tr("Hello Message Window"),tr("Specify hello message"),
			mNoHelpKey))
{
    txtfld = new uiGenInput( this, tr("Hello message"),
				StringInpSpec("Hello world") );
    typfld = new uiGenInput( this, tr("Message type"),
				BoolInpSpec(true,uiStrings::sInfo(),
					    uiStrings::sWarning()) );
    typfld->attach( alignedBelow, txtfld );
}

bool acceptOK()
{
    const char* typedtxt = txtfld->text();
    if ( ! *typedtxt )
    {
	uiMSG().error( tr("Please type a message text") );
	return false;
    }
    if ( typfld->getBoolValue() )
	uiMSG().message( toUiString(typedtxt) );
    else
	uiMSG().warning( toUiString(typedtxt) );
    return true;
}

    uiGenInput*	txtfld;
    uiGenInput*	typfld;

};


void uiHelloMgr::dispMsg( CallBacker* )
{
    uiHelloMsgBringer dlg( &appl );
    dlg.go();
}


mDefODInitPlugin(uiHello)
{
    mDefineStaticLocalObject( PtrMan<uiHelloMgr>, theinst_, = 0 );
    if ( theinst_ ) return 0;

    theinst_ = new uiHelloMgr( *ODMainWin() );
    if ( !theinst_ )
	return "Cannot instantiate Hello plugin";

    return 0; // All OK - no error messages
}

#endif /* ifdef PLAN_A */
