/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/

#include "uiconvpos.h"
#include "survinfo.h"
#include "strmprov.h"
#include "oddirs.h"
#include "uitoolbutton.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "od_iostream.h"
#include "od_helpids.h"

#define mMaxLineBuf 32000
static BufferString lastinpfile;
static BufferString lastoutfile;


uiConvertPos::uiConvertPos( uiParent* p, const SurveyInfo& si, bool mod )
	: uiDialog(p, uiDialog::Setup(tr("Convert Positions"),
		   mNoDlgTitle, mODHelpKey(mConvertPosHelpID) ).modal(mod))
	, survinfo(si)
{
    ismanfld = new uiGenInput( this, uiStrings::sConversion(),
	           BoolInpSpec(true,uiStrings::sManual(),uiStrings::sFile()) );
    ismanfld->valuechanged.notify( mCB(this,uiConvertPos,selChg) );

    mangrp = new uiGroup( this, "Manual group" );
    uiGroup* inlcrlgrp = new uiGroup( mangrp, "InlCrl group" );
    const Interval<int> intv( -mUdf(int), mUdf(int) );
    inlfld = new uiGenInput( inlcrlgrp, uiStrings::sInline(),
		IntInpSpec(0,intv).setName("Inl-field") );
    inlfld->setValue( si.inlRange(false).start );
    inlfld->setElemSzPol( uiObject::Medium );
    crlfld = new uiGenInput( inlcrlgrp, uiStrings::sCrossline(),
		IntInpSpec(0,intv).setName("Crl-field") );
    crlfld->setValue( si.crlRange(false).start );
    crlfld->setElemSzPol( uiObject::Medium );
    crlfld->attach( alignedBelow, inlfld );

    uiGroup* xygrp = new uiGroup( mangrp, "XY group" );
    xfld = new uiGenInput( xygrp, tr("X-coordinate"),
			   DoubleInpSpec().setName("X-field") );
    xfld->setElemSzPol( uiObject::Medium );
    yfld = new uiGenInput( xygrp, tr("Y-coordinate"),
			   DoubleInpSpec().setName("Y-field") );
    yfld->setElemSzPol( uiObject::Medium );
    yfld->attach( alignedBelow, xfld );

    uiGroup* butgrp = new uiGroup( mangrp, "Buttons" );
    uiToolButton* dobinidbut = new uiToolButton( butgrp,
			uiToolButton::LeftArrow, tr("Convert (X,Y) to Inl/Crl"),
			mCB(this,uiConvertPos,getBinID) );
    uiToolButton* docoordbut = new uiToolButton( butgrp,
			uiToolButton::RightArrow,tr("Convert Inl/Crl to (X,Y)"),
			mCB(this,uiConvertPos,getCoord) );
    docoordbut->attach( rightTo, dobinidbut );
    butgrp->attach( centeredRightOf, inlcrlgrp );
    xygrp->attach( centeredRightOf, butgrp );

    mangrp->setHAlignObj( inlfld );
    mangrp->attach( alignedBelow, ismanfld );

    filegrp = new uiGroup( this, "File group" );
    uiFileSel::Setup fssu( lastinpfile );
    fssu.withexamine( true ).examstyle( File::Table );
    inpfilefld = new uiFileSel( filegrp, uiStrings::phrInput(
					   uiStrings::sFile()), fssu );

    fssu.setFileName( lastoutfile );
    fssu.setForWrite();
    outfilefld = new uiFileSel( filegrp, uiStrings::phrOutput(
					   uiStrings::sFile()), fssu );
    outfilefld->attach( alignedBelow, inpfilefld );
    isxy2bidfld = new uiGenInput( filegrp, uiStrings::sType(),
	           BoolInpSpec(true,tr("X/Y to I/C"),tr("I/C to X/Y")) );
    isxy2bidfld->attach( alignedBelow, outfilefld );
    uiPushButton* pb = new uiPushButton( filegrp, uiStrings::sGo(),
				mCB(this,uiConvertPos,convFile), true );
    pb->attach( alignedBelow, isxy2bidfld );
    filegrp->setHAlignObj( inpfilefld );
    filegrp->attach( alignedBelow, ismanfld );

    setCtrlStyle( CloseOnly );
    postFinalise().notify( mCB(this,uiConvertPos,selChg) );
}


void uiConvertPos::selChg( CallBacker* )
{
    const bool isman = ismanfld->getBoolValue();
    mangrp->display( isman );
    filegrp->display( !isman );
}


void uiConvertPos::getCoord( CallBacker* )
{
    BinID binid( inlfld->getIntValue(), crlfld->getIntValue() );
    if ( binid == BinID::udf() )
    {
	uiMSG().error( tr("Cannot convert this position") );
	xfld->setText( "" ); yfld->setText( "" );
	return;
    }

    Coord coord( survinfo.transform( binid ) );
    xfld->setValue( coord.x_ );
    yfld->setValue( coord.y_ );
    inlfld->setValue( binid.inl() );
    crlfld->setValue( binid.crl() );
}


void uiConvertPos::getBinID( CallBacker* )
{
    Coord coord( xfld->getDValue(), yfld->getDValue() );
    if ( coord == Coord::udf() )
    {
	uiMSG().error( tr("Cannot convert this position") );
	inlfld->setText( "" ); crlfld->setText( "" );
	return;
    }

    BinID binid( survinfo.transform( coord ) );
    inlfld->setValue( binid.inl() );
    crlfld->setValue( binid.crl() );
    xfld->setValue( coord.x_ );
    yfld->setValue( coord.y_ );
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiConvertPos::convFile( CallBacker* )
{
    const BufferString inpfnm = inpfilefld->fileName();

    od_istream istream( inpfnm );
    if ( !istream.isOK() )
	mErrRet(tr("Input file is not readable") );

    const BufferString outfnm = outfilefld->fileName();
    od_ostream ostream( outfnm );
    if ( !ostream.isOK() )
    { mErrRet(uiStrings::phrCannotOpenOutpFile()); }

    lastinpfile = inpfnm; lastoutfile = outfnm;

    BufferString linebuf; Coord c;
    const bool xy2ic = isxy2bidfld->getBoolValue();
    int nrln = 0;
    while ( istream.isOK() )
    {
	istream.get( c.x_ );
	istream.get( c.y_ );
	if ( !istream.isOK() ) break;

        istream.getLine( linebuf );
        if ( istream.isBad() ) break;
	if ( xy2ic )
	{
	    BinID bid( SI().transform(c) );
	    ostream << bid.inl() << ' ' << bid.crl() << linebuf << '\n';
	}
	else
	{
	    BinID bid( mNINT32(c.x_), mNINT32(c.y_) );
	    c = SI().transform( bid );
	    ostream << toString(c.x_) << ' '; // keep on sep line: toString()
	    ostream << toString(c.y_) << linebuf << '\n';
	}
	nrln++;
    }

    uiMSG().message(tr("Total number of converted lines: %1")
		  .arg(toString(nrln)));
}
