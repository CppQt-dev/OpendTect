/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2001
________________________________________________________________________

-*/



#include "uifreqfilterattrib.h"
#include "freqfilterattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uifreqfilter.h"
#include "uigeninput.h"
#include "uiwindowfunctionsel.h"
#include "uifreqtaper.h"
#include "od_helpids.h"

using namespace Attrib;

static uiWord sDispName()
{
    return od_static_tr("sDispName","Frequency Filter");
}

mInitGrpDefAttribUI(uiFreqFilterAttrib,FreqFilter,sDispName(),sFilterGrp())


uiFreqFilterAttrib::uiFreqFilterAttrib( uiParent* p, bool is2d )
	: uiAttrDescEd(p,is2d, mODHelpKey(mFreqFilterAttribHelpID) )

{
    inpfld_ = createImagInpFld( is2d );
    inpfld_->selectionChanged.notify(
		mCB(this,uiFreqFilterAttrib,selectionDoneCB) );

    isfftfld_ = new uiGenInput( this, tr("Filtering method"),
			BoolInpSpec(true,uiStrings::sFFT(),tr("ButterWorth")) );
    isfftfld_->attach( alignedBelow, inpfld_ );
    isfftfld_->valuechanged.notify( mCB(this,uiFreqFilterAttrib,isfftSel) );

    freqfld_ = new uiFreqFilterSelFreq( this );
    freqfld_->parchanged.notify( mCB(this,uiFreqFilterAttrib,typeSel) );
    freqfld_->parchanged.notify(mCB(this,uiFreqFilterAttrib,freqChanged) );
    freqfld_->parchanged.notify(mCB(this,uiFreqFilterAttrib,updateTaperFreqs));
    freqfld_->attach( alignedBelow, isfftfld_ );

    polesfld_ = new uiGenInput( this, tr("Nr of poles"),
		IntInpSpec(2).setLimits(Interval<int>(2,99)) );
    polesfld_->attach( ensureBelow, freqfld_ );
    polesfld_->attach( alignedBelow, isfftfld_ );

    uiWindowFunctionSel::Setup su;
    su.label_ = "Window/Taper";
    su.winname_ = 0;

    for ( int idx=0; idx<2; idx++ )
    {
	if ( idx )
	{
	    su.label_ = "Taper";
	    su.onlytaper_ = true;
	    su.with2fldsinput_ = true;
	    su.inpfldtxt_ = "Min/max frequency(Hz)";

	    FreqTaperSetup freqsu;
	    const Attrib::DescSet& attrset = inpfld_->attrSet();
	    const Attrib::Desc* inpdesc = attrset.getDesc(inpfld_->attribID());
	    if ( inpdesc )
		freqsu.dbkey_ = inpdesc->getStoredID( true );
	    winflds_ += new uiFreqTaperSel( this, su, freqsu );
	}
	else
	    winflds_ += new uiWindowFunctionSel( this, su );
	winflds_[idx]->display (false);
    }
    uiString seltxt = uiStrings::sTaper();
    freqwinselfld_ = new uiCheckBox( this, seltxt );
    freqwinselfld_->attach( alignedBelow, winflds_[0] );
    freqwinselfld_->activated.notify( mCB(this,uiFreqFilterAttrib,freqWinSel) );

    winflds_[0]->attach( alignedBelow, polesfld_ );
    winflds_[1]->attach( alignedBelow, freqwinselfld_ );
    winflds_[1]->setSensitive( false );

    postFinalise().notify( mCB(this,uiFreqFilterAttrib,finaliseCB));
    setHAlignObj( inpfld_ );
}


void uiFreqFilterAttrib::finaliseCB( CallBacker* )
{
    typeSel(0);
    isfftSel(0);
    freqChanged(0);
    freqWinSel(0);
}


void uiFreqFilterAttrib::selectionDoneCB( CallBacker* cb )
{
    mDynamicCastGet(uiFreqTaperSel*,freqtapersel,winflds_[1]);
    if ( !freqtapersel ) return;

    const Attrib::DescSet& attrset = inpfld_->attrSet();
    const Attrib::Desc* inpdesc = attrset.getDesc( inpfld_->attribID() );
    const DBKey dbkey = inpdesc ? DBKey(inpdesc->getStoredID(true))
				    : DBKey::getInvalid();
    freqtapersel->setDBKey( dbkey );
}


void uiFreqFilterAttrib::typeSel( CallBacker* )
{
    const int type = (int)freqfld_->filterType();
    const bool hasmin = type==1 || type==2;
    const bool hasmax = !type || type==2;
    mDynamicCastGet( uiFreqTaperSel*, tap, winflds_[1] );
    if ( tap ) tap->setIsMinMaxFreq( hasmin, hasmax );
}


void uiFreqFilterAttrib::isfftSel( CallBacker* )
{
    const bool isfft = isfftfld_->getBoolValue();
    for ( int idx=0; idx<winflds_.size(); idx++)
	winflds_[idx]->display( isfft );
    freqwinselfld_->display( isfft );
    polesfld_->display( !isfft );
}


void uiFreqFilterAttrib::freqChanged( CallBacker* )
{
    mDynamicCastGet( uiFreqTaperSel*, tap, winflds_[1] );
    if ( tap )
    {
	tap->setRefFreqs( freqfld_->freqRange() );
    }
}


void uiFreqFilterAttrib::updateTaperFreqs( CallBacker* )
{
    mDynamicCastGet( uiFreqTaperSel*, tap, winflds_[1] );
    if ( tap )
    {
	const bool costaper = FixedString(tap->windowName()) == "CosTaper";
	Interval<float> frg( freqfld_->freqRange() );
	if ( costaper ) { frg.start-=5; frg.stop+=10; }
	tap->setInputFreqValue( frg.start > 0 ? frg.start : 0, 0 );
	tap->setInputFreqValue( frg.stop , 1 );
    }
}


void uiFreqFilterAttrib::freqWinSel( CallBacker* )
{
    const bool isfreqtaper = freqwinselfld_->isChecked();
    winflds_[1]->setSensitive( isfreqtaper );
    if ( !isfreqtaper )
	updateTaperFreqs(0);
}


bool uiFreqFilterAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != FreqFilter::attribName() )
	return false;

    mIfGetEnum( FreqFilter::filtertypeStr(), filtertype,
	        freqfld_->setFilterType((FFTFilter::Type)filtertype) );
    mIfGetFloat( FreqFilter::minfreqStr(), minfreq,
		 freqfld_->setMinFreq(minfreq) );
    mIfGetFloat( FreqFilter::maxfreqStr(), maxfreq,
		 freqfld_->setMaxFreq(maxfreq) );
    mIfGetInt( FreqFilter::nrpolesStr(), nrpoles,
	       polesfld_->setValue(nrpoles) )

    mIfGetString( FreqFilter::windowStr(), window,
			    winflds_[0]->setWindowName(window) );
    mIfGetString( FreqFilter::fwindowStr(), fwindow,
			    winflds_[1]->setWindowName(fwindow) );
    mIfGetFloat( FreqFilter::paramvalStr(), variable,
	    const float resvar = float( mNINT32((1-variable)*1000) )/1000.0f;
	    winflds_[0]->setWindowParamValue(resvar) );
    mDynamicCastGet( uiFreqTaperSel*, taper, winflds_[1] );
    if ( taper )
    {
	mIfGetFloat( FreqFilter::freqf1Str(), freqf1,
	    const float res = float( freqf1) ;
	    taper->setInputFreqValue( res, 0 ) );
	mIfGetFloat( FreqFilter::freqf4Str(), freqf4,
	    const float res = float( freqf4 );
	    taper->setInputFreqValue( res, 1 ) );
    }
    mIfGetBool( FreqFilter::isfftfilterStr(), isfftfilter,
		isfftfld_->setValue(isfftfilter) );
    mIfGetBool( FreqFilter::isfreqtaperStr(), isfreqtaper,
		freqwinselfld_->setChecked(isfreqtaper) );

    finaliseCB(0);
    return true;
}


bool uiFreqFilterAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiFreqFilterAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != FreqFilter::attribName() )
	return false;

    const Interval<float> freqrg = freqfld_->freqRange();
    mSetEnum( FreqFilter::filtertypeStr(), freqfld_->filterType() );
    mSetFloat( FreqFilter::minfreqStr(), freqrg.start );
    mSetFloat( FreqFilter::maxfreqStr(), freqrg.stop );
    mSetInt( FreqFilter::nrpolesStr(), polesfld_->getIntValue() );
    mSetString( FreqFilter::windowStr(), winflds_[0]->windowName() );
    mSetString( FreqFilter::fwindowStr(), winflds_[1]->windowName() );

    const float resvar =
	float( mNINT32((1-winflds_[0]->windowParamValue())*1000) )/1000.0f;
    mSetFloat( FreqFilter::paramvalStr(), resvar );
    mDynamicCastGet( uiFreqTaperSel*, taper, winflds_[1] );
    if ( taper )
    {
	Interval<float> freqresvar = taper->freqValues();
	const bool istaper = FixedString(winflds_[1]->windowName())=="CosTaper";
	freqresvar.start = istaper ? (freqresvar.start) : freqrg.start;
	freqresvar.stop = istaper ? (freqresvar.stop) : freqrg.stop;
	mSetFloat( FreqFilter::freqf1Str(), freqresvar.start );
	mSetFloat( FreqFilter::freqf4Str(), freqresvar.stop );
    }
    mSetBool( FreqFilter::isfftfilterStr(), isfftfld_->getBoolValue() );
    mSetBool( FreqFilter::isfreqtaperStr(), freqwinselfld_->isChecked() );

    return true;
}


uiRetVal uiFreqFilterAttrib::getInput( Desc& desc )
{
    const bool needimag = isfftfld_->getBoolValue();
    return fillInp( needimag ? inpfld_ : (uiAttrSel*)inpfld_, desc, 0 );
}


void uiFreqFilterAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    const int passtype = (int)freqfld_->filterType();
    if ( passtype != 0 )
	params += EvalParam( "Min frequency", FreqFilter::minfreqStr() );
    if ( passtype != 1 )
	params += EvalParam( "Max frequency", FreqFilter::maxfreqStr() );
    if ( !isfftfld_->getBoolValue() )
	params += EvalParam( "Nr poles", FreqFilter::nrpolesStr() );
}


#define mErrWinFreqMsg(s) { updateTaperFreqs(0); return s; }

uiRetVal uiFreqFilterAttrib::areUIParsOK()
{
    const bool isfft = isfftfld_->getBoolValue();
    if ( isfft && FixedString(winflds_[0]->windowName())=="CosTaper" )
    {
	float paramval = winflds_[0]->windowParamValue();
	if ( paramval<0 || paramval>1  )
	    return uiRetVal( tr("Variable 'Taper length' is not\n"
			 "within the allowed range: 0 to 100 (%)." ) );
    }

    mDynamicCastGet(uiFreqTaperSel*,taper,winflds_[1]);
    if ( isfft && taper )
    {
	Interval<float> freqresvar = taper->freqValues();
	if ( freqresvar.start < 0 )
	    mErrWinFreqMsg( tr("min frequency cannot be negative") )

	if ( freqresvar.start > freqfld_->freqRange().start )
	    mErrWinFreqMsg(
		tr("Taper min frequency must be lower than this of the"
		" filter frequency.\n Please select a different frequency.") );
	if ( freqresvar.stop < freqfld_->freqRange().stop )
	    mErrWinFreqMsg(
		tr("Taper max frequency must be higher than this of the"
		" filter frequency.\n Please select a different frequency.") );
    }

    uiRetVal uirv;
    const int passtype = (int)freqfld_->filterType();
    if ( passtype==2 && mIsZero(freqfld_->freqRange().width(),1e-3) )
    {
	if ( mIsZero(freqfld_->freqRange().width(),1e-3) )
	    uirv.add( tr("min and max frequencies should be different") );
	if ( freqfld_->freqRange().isRev() )
	    uirv.add( tr("Min frequency must be lower than max frequency") );
    }

    return uirv;
}
