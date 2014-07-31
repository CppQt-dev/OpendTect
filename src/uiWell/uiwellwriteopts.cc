/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwellwriteopts.h"
#include "settings.h"
#include "welllog.h"
#include "welltransl.h"
#include "wellodwriter.h"
#include "uigeninput.h"

#define mODWellTranslInstance mTranslTemplInstance(Well,od)


uiODWellWriteOpts::uiODWellWriteOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,mODWellTranslInstance)
    , defbinwrite_(true)
{
    mSettUse(getYN,"dTect.Well logs","Binary format",defbinwrite_);
    wrlogbinfld_ = new uiGenInput( this, "Well log storage",
		 BoolInpSpec(defbinwrite_,"Binary","Ascii") );

    setHAlignObj( wrlogbinfld_ );
}


void uiODWellWriteOpts::use( const IOPar& iop )
{
    const char* res = iop.find( Well::odWriter::sKeyLogStorage() );
    bool binwr = defbinwrite_;
    if ( res && *res )
	binwr = *res != 'A';
    wrlogbinfld_->setValue( binwr );
}


bool uiODWellWriteOpts::fill( IOPar& iop ) const
{
    const bool wantbin = wrlogbinfld_->getBoolValue();
    iop.set( Well::odWriter::sKeyLogStorage(), wantbin ? "Binary" : "Ascii" );
    return true;
}


void uiODWellWriteOpts::initClass()
{
    factory().addCreator( create, mODWellTranslInstance.getDisplayName() );
}
