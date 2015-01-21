/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Dec 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisynthseis.h"

#include "uibutton.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uisplitter.h"
#include "uispinbox.h"
#include "uiseiswvltsel.h"

#include "hiddenparam.h"
#include "synthseis.h"

#include "od_helpids.h"


#define mErrRet(s,act) \
{ uiMsgMainWinSetter mws( mainwin() ); if ( s ) uiMSG().error(s); act; }

uiSynthSeisGrp::uiSynthSeisGrp( uiParent* p, const uiRayTracer1D::Setup& su )
    : uiGroup(p)
    , internalmultiplebox_(0)
    , surfreflcoeffld_(0)
    , parsChanged(this)
{
    rtsel_ = new uiRayTracerSel( this, su );
    rtsel_->offsetChanged.notify( mCB(this,uiSynthSeisGrp,parsChangedCB) );

    wvltfld_ = new uiSeisWaveletSel( this );
    wvltfld_->newSelection.notify( mCB(this,uiSynthSeisGrp,parsChangedCB) );
    wvltfld_->attach( alignedBelow, rtsel_ );
    wvltfld_->setFrame( false );

    uiSeparator* lastsep = 0;
    if ( su.doreflectivity_ )
    {
	uiSeparator* sep = new uiSeparator( this, "Reflectivity sep" );
	sep->attach( stretchedBelow, wvltfld_ );

	internalmultiplebox_ = new uiCheckBox( this,
				    tr("Compute internal multiples") );
	internalmultiplebox_->attach( alignedBelow, wvltfld_ );
	internalmultiplebox_->attach( ensureBelow, sep );
	surfreflcoeffld_ = new uiLabeledSpinBox( this,
			    tr("Surface reflection coefficient"), 1 );
	surfreflcoeffld_->box()->setInterval( (double)-1, (double)1, 0.1 );
	surfreflcoeffld_->box()->setValue( 1 );
	surfreflcoeffld_->attach( alignedBelow, internalmultiplebox_ );

	lastsep = new uiSeparator( this, "Internal sep" );
	lastsep->attach( stretchedBelow, surfreflcoeffld_ );
    }

    uisynthcorrgrp_ = new uiSynthCorrectionsGrp( this );
    uisynthcorrgrp_->attach( alignedBelow, wvltfld_ );
    if ( su.doreflectivity_ )
    {
	uisynthcorrgrp_->attach( alignedBelow, surfreflcoeffld_ );
	uisynthcorrgrp_->attach( ensureBelow, lastsep );
    }
    else
	uisynthcorrgrp_->attach( alignedBelow, wvltfld_ );

    uisynthcorrgrp_->nmoparsChanged_.notify(
	    mCB(this,uiSynthSeisGrp,parsChangedCB) );
    setHAlignObj( wvltfld_ );
}


void uiSynthSeisGrp::parsChangedCB( CallBacker* )
{
    updateFieldDisplay();
    parsChanged.trigger();
}


void uiSynthSeisGrp::updateDisplayForPSBased()
{
    wvltfld_->display( false );
    if ( internalmultiplebox_ )
    {
	internalmultiplebox_->display( false );
	surfreflcoeffld_->display( false );
    }
    uisynthcorrgrp_->display( false );
    rtsel_->display( false );
}


void uiSynthSeisGrp::updateFieldDisplay()
{
    wvltfld_->display( true );
    const bool iszerooofset = rtsel_->current()->isZeroOffset();
    if ( !rtsel_->current()->hasZeroOffsetFld() )
    { // cannot set zerooffset from outside e.g uiSynthGenDlg
	uisynthcorrgrp_->display( !iszerooofset );
	rtsel_->display( !iszerooofset );
    }

    if ( internalmultiplebox_ )
    {
	internalmultiplebox_->display( iszerooofset );
	surfreflcoeffld_->display( iszerooofset );
    }
}


void uiSynthSeisGrp::setRayTracerType( const char* type )
{
    rtsel_->setCurrentType( type );
}


const char* uiSynthSeisGrp::getWaveletName() const
{
    return wvltfld_->getName();
}


void uiSynthSeisGrp::setWavelet( const char* wvltnm )
{
    wvltfld_->setInput( wvltnm );
}

#define mIsZeroOffset( offsets ) \
    (offsets.isEmpty() || (offsets.size()==1 && mIsZero(offsets[0],mDefEps)))

void uiSynthSeisGrp::usePar( const IOPar& iopar )
{
    TypeSet<float> offsets;
    iopar.get( RayTracer1D::sKeyOffset(), offsets );
    const bool iszerooofset = mIsZeroOffset(offsets);
    MultiID waveletid;
    if ( iopar.get(sKey::WaveletID(),waveletid) )
	wvltfld_->setInput( waveletid );

    if ( iszerooofset )
    {
	bool dointernalmultiples = false; float surfreflcoeff =1;
	iopar.getYN( Seis::SynthGenBase::sKeyInternal(), dointernalmultiples );
	iopar.get( Seis::SynthGenBase::sKeySurfRefl(), surfreflcoeff );
	internalmultiplebox_->setChecked( dointernalmultiples );
	surfreflcoeffld_->box()->setValue( surfreflcoeff );
    }
    else
    {
	bool donmo = true;
	iopar.getYN( Seis::SynthGenBase::sKeyNMO(), donmo );

	float mutelen = Seis::SynthGenBase::cStdMuteLength();
	iopar.get( Seis::SynthGenBase::sKeyMuteLength(), mutelen );
	if ( !mIsUdf(mutelen) )
	   mutelen = mutelen *ZDomain::Time().userFactor();

	float stretchlimit = Seis::SynthGenBase::cStdStretchLimit();
	iopar.get( Seis::SynthGenBase::sKeyStretchLimit(), stretchlimit );
	uisynthcorrgrp_->setValues( donmo, mutelen, mToPercent( stretchlimit ));
    }

    rtsel_->usePar( iopar );
    updateFieldDisplay();
}


void uiSynthSeisGrp::fillPar( IOPar& iopar ) const
{
    iopar.setEmpty();

    rtsel_->fillPar( iopar );
    iopar.set( sKey::WaveletID(), wvltfld_->getID() );

    const bool iszeroffset = rtsel_->current()->isZeroOffset();
    if ( iszeroffset )
    {
	RayTracer1D::setIOParsToZeroOffset( iopar );
	const bool dointernal = internalmultiplebox_->isChecked();
	const float coeff = surfreflcoeffld_->box()->getFValue();
	iopar.setYN( Seis::SynthGenBase::sKeyInternal(), dointernal );
	iopar.set( Seis::SynthGenBase::sKeySurfRefl(), coeff );
    }
    else
    {
	bool donmo = !iszeroffset ? uisynthcorrgrp_->wantNMOCorr() : false;
	iopar.setYN( Seis::SynthGenBase::sKeyNMO(), donmo );
	iopar.set( Seis::SynthGenBase::sKeyMuteLength(),
	     uisynthcorrgrp_->getMuteLength() / ZDomain::Time().userFactor() );
	iopar.set( Seis::SynthGenBase::sKeyStretchLimit(),
		   mFromPercent( uisynthcorrgrp_->getStrechtMutePerc()) );
    }

}


class uiSynthCorrAdvancedDlg : public uiDialog
{ mODTextTranslationClass(uiSynthCorrAdvancedDlg);
    public:
				uiSynthCorrAdvancedDlg(uiParent*);

    uiGenInput*		stretchmutelimitfld_;
    uiGenInput*		mutelenfld_;

    protected:

    bool			acceptOK(CallBacker*);
};


uiSynthCorrectionsGrp::uiSynthCorrectionsGrp( uiParent* p )
    : uiGroup( p, "Synth corrections parameters" )
    , nmoparsChanged_(this)
{
    nmofld_ = new uiGenInput( this, tr("Apply NMO corrections"),
			      BoolInpSpec(true) );
    mAttachCB( nmofld_->valuechanged, uiSynthCorrectionsGrp::parsChanged);
    nmofld_->setValue( true );

    CallBack cbadv = mCB(this,uiSynthCorrectionsGrp,getAdvancedPush);
    advbut_ = new uiPushButton( this, "Advanced", cbadv, false );
    advbut_->attach( alignedBelow, nmofld_ );

    uiscadvdlg_ = new uiSynthCorrAdvancedDlg( this );
    setHAlignObj( nmofld_ );
}


void uiSynthCorrectionsGrp::parsChanged( CallBacker* )
{
    advbut_->display( wantNMOCorr() );
    nmoparsChanged_.trigger();
}


bool uiSynthCorrectionsGrp::wantNMOCorr() const
{
    return nmofld_->getBoolValue();
}


float uiSynthCorrectionsGrp::getStrechtMutePerc() const
{
    return uiscadvdlg_->stretchmutelimitfld_->getfValue();
}


float uiSynthCorrectionsGrp::getMuteLength() const
{
    return uiscadvdlg_->mutelenfld_->getfValue();
}


void uiSynthCorrectionsGrp::getAdvancedPush( CallBacker* )
{
    uiscadvdlg_->go();
}


void uiSynthCorrectionsGrp::setValues( bool donmo, float mutelen,
				       float stretchlim )
{
    nmofld_->setValue( donmo );
    uiscadvdlg_->mutelenfld_->setValue( mutelen );
    uiscadvdlg_->stretchmutelimitfld_->setValue( stretchlim );
}


uiSynthCorrAdvancedDlg::uiSynthCorrAdvancedDlg( uiParent* p )
    : uiDialog( p, uiDialog::Setup(tr("Synthetic Corrections advanced options"),
		tr("Specify advanced options"), mTODOHelpKey) )
{
    FloatInpSpec inpspec;
    inpspec.setLimits( Interval<float>(1,500) );
    stretchmutelimitfld_ = new uiGenInput(this, tr("Stretch mute (%)"),
					  inpspec );

    mutelenfld_ = new uiGenInput( this, tr("Mute taper-length (ms)"),
				  FloatInpSpec() );
    mutelenfld_->attach( alignedBelow, stretchmutelimitfld_ );
}


bool uiSynthCorrAdvancedDlg::acceptOK( CallBacker* )
{
    if ( mIsUdf(mutelenfld_->getfValue() ) || mutelenfld_->getfValue()<0 )
	mErrRet( "The mutelength must be more than zero.", return false );

    if ( mIsUdf(stretchmutelimitfld_->getfValue()) ||
	 stretchmutelimitfld_->getfValue()<0 )
	mErrRet( "The stretch mute must be more than 0%", return false );

    return true;
}

