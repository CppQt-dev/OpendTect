/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uiconvpos.cc,v 1.2 2001-08-23 14:59:17 windev Exp $
________________________________________________________________________

-*/

#include "uiconvpos.h"
#include "survinfo.h"
#include "uibutton.h"
#include "uidialog.h"
#include "uigeninput.h"

uiConvertPos::uiConvertPos( uiParent* p, SurveyInfo* si )
	: uiDialog(p, "Convert positions")
	, survinfo(si)

{
    inlfld = new uiGenInput( this, "In-line", IntInpSpec() );
    crlfld = new uiGenInput( this, "Cross-line", IntInpSpec() );
    xfld = new uiGenInput( this, "X-coordinate", DoubleInpSpec() );
    yfld = new uiGenInput( this, "Y-coordinate", DoubleInpSpec() );
    docoordbut = new uiPushButton( this, "->" );
    docoordbut->activated.notify( mCB(this,uiConvertPos, getCoord) );
    dobinidbut = new uiPushButton( this, "<-" );
    dobinidbut->activated.notify( mCB(this,uiConvertPos, getBinID) );
    crlfld->attach( alignedBelow, inlfld );
    docoordbut->attach( rightTo, inlfld );
    dobinidbut->attach( alignedBelow, docoordbut );
    xfld->attach( rightTo, docoordbut );
    yfld->attach( alignedBelow, xfld );
    setOkText("Quit");
    setCancelText("");
}

void uiConvertPos::getCoord()
{
    BinID binid( inlfld->getIntValue(), crlfld->getIntValue() );
    Coord coord( survinfo->transform( binid ) );
    xfld->setValue( coord.x );
    yfld->setValue( coord.y );
}

void uiConvertPos::getBinID()
{
    Coord coord( xfld->getValue(), yfld->getValue() );
    BinID binid( survinfo->transform( coord ) );
    inlfld->setValue( binid.inl );
    crlfld->setValue( binid.crl );
}

