/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "uiprestackanglemute.h"

#include "prestackanglecomputer.h"
#include "prestackanglemute.h"
#include "raytrace1d.h"
#include "survinfo.h"
#include "windowfunction.h"
#include "uibutton.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiprestackprocessor.h"
#include "uiraytrace1d.h"
#include "uiseparator.h"
#include "uiveldesc.h"
#include "od_helpids.h"


namespace PreStack
{

uiAngleCompGrp::uiAngleCompGrp( uiParent* p, PreStack::AngleCompParams& pars,
				bool dooffset, bool isformute )
    : uiGroup(p,"Angle Mute Group")
    , params_(pars)
    , isformute_(isformute)
    , anglelbl_(0)
    , dooffset_(dooffset)
    , advpardlg_(0)
{
    velfuncsel_ = new uiVelSel( this, uiVelSel::ioContext(),
				uiSeisSel::Setup(Seis::Vol) );
    velfuncsel_->setLabelText( tr("Input Velocity") );
    if ( !params_.velvolmid_.isInvalid() )
       velfuncsel_->setInput( params_.velvolmid_ );

    if ( isformute_ )
    {
	anglefld_ = new uiGenInput( this, tr("Mute cutoff angle (degree)"),
				    FloatInpSpec(false) );
	anglefld_->attach( alignedBelow, velfuncsel_ );
	anglefld_->setValue( params_.mutecutoff_ );
    }
    else
    {
	anglefld_ = new uiGenInput( this, tr("Angle range"),
				    IntInpIntervalSpec(params_.anglerange_) );
	anglefld_->attach( alignedBelow, velfuncsel_ );
	anglelbl_ = new uiLabel( this, toUiString("(deg)") );
	anglelbl_->attach( rightOf, anglefld_ );
    }

    advpushbut_ = new uiPushButton( this,
		  uiStrings::sAdvanced(uiStrings::sParameter(mPlural)), false );
    advpushbut_->setIcon( "advanced" );
    advpushbut_->activated.notify( mCB(this,uiAngleCompGrp,advPushButCB) );
    advpushbut_->attach( rightAlignedBelow, velfuncsel_ );

    setHAlignObj( velfuncsel_ );
}


void uiAngleCompGrp::updateFromParams()
{
    velfuncsel_->setInput( params_.velvolmid_ );
    if ( isformute_ )
	anglefld_->setValue( params_.mutecutoff_ );
    else
	anglefld_->setValue( params_.anglerange_ );
}


bool uiAngleCompGrp::acceptOK()
{
    if ( !velfuncsel_->ioobj() )
	return false;

    params_.velvolmid_ = velfuncsel_->key(true);
    Interval<int> normalanglevalrange( 0, 90 );
    if ( isformute_ )
    {
	params_.mutecutoff_ = anglefld_->getFValue();
	if ( !normalanglevalrange.includes(params_.mutecutoff_,false) )
	{
	    uiMSG().error( uiStrings::phrSelect(
		tr("the mute cutoff between 0 and 90 degree")) );
	    return false;
	}
    }
    else
    {
	Interval<int> anglerange = anglefld_->getIInterval();
	anglerange.sort();

	if ( !normalanglevalrange.includes(anglerange,false) )
	{
	    uiMSG().error(
		tr("Provide angle range between 0 and 90 degree") );
	    return false;
	}

	params_.anglerange_ = anglerange;
    }

    return true;
}


void uiAngleCompGrp::advPushButCB( CallBacker* )
{
    if (!advpardlg_)
    advpardlg_ = new uiAngleCompAdvParsDlg(this, params_, dooffset_,isformute_);
    advpardlg_->updateFromParams();
    advpardlg_->go();
}


uiAngleCompAdvParsDlg::uiAngleCompAdvParsDlg( uiParent* p,
					      PreStack::AngleCompParams& pars,
					      bool offset, bool isformute )
    : uiDialog(p, uiDialog::Setup(uiStrings::sAdvanced(tr("Angle Parameters")),
				  mNoDlgTitle,
				  mODHelpKey(mAngleCompAdvParsDlgHelpID)))
    , params_(pars)
    , isformute_(isformute)
    , smoothtypefld_(0)
    , smoothwindowfld_(0)
    , smoothwinparamfld_(0)
    , smoothwinlengthfld_(0)
    , freqf3fld_(0)
    , freqf4fld_(0)
{
    uiRayTracer1D::Setup rsu;
    rsu.dooffsets_ = offset;
    rsu.doreflectivity_ = false;
    raytracerfld_ = new uiRayTracerSel( this, rsu );

    if ( !isformute_ )
	createAngleCompFields();

    postFinalise().notify( mCB(this,uiAngleCompAdvParsDlg,finaliseCB) );
}


void uiAngleCompAdvParsDlg::createAngleCompFields()
{
    smoothtypefld_ = new uiGenInput( this, tr("Smoothing Type"),
	StringListInpSpec(PreStack::AngleComputer::smoothingTypeDef()) );
    smoothtypefld_->attach( alignedBelow, raytracerfld_ );
    smoothtypefld_->setValue( PreStack::AngleComputer::FFTFilter );
    smoothtypefld_->valuechanged.notify( mCB(this,uiAngleCompAdvParsDlg,
					     smoothTypeSel) );

    const BufferStringSet& windowfunctions
			= WindowFunction::factory().getKeys();
    smoothwindowfld_ = new uiGenInput( this, tr("Window/Taper"),
				       StringListInpSpec(windowfunctions) );
    smoothwindowfld_->attach( alignedBelow, smoothtypefld_ );
    smoothwindowfld_->valuechanged.notify( mCB(this,uiAngleCompAdvParsDlg,
					       smoothWindowSel) );

    smoothwinparamfld_ = new uiGenInput( this, tr("Taper length").withUnit("%"),
					 FloatInpSpec() );
    smoothwinparamfld_->attach( rightOf, smoothwindowfld_ );

    smoothwinlengthfld_ = new uiGenInput( this,
	    tr("Window width").withSurvZUnit(), FloatInpSpec());
    smoothwinlengthfld_->attach( alignedBelow, smoothwindowfld_ );

    freqf3fld_ = new uiGenInput( this, tr("Frequency F3").withUnit("Hz"),
				 FloatInpSpec() );
    freqf3fld_->attach( alignedBelow, smoothtypefld_ );

    freqf4fld_ = new uiGenInput( this, tr("Frequency F4").withUnit("Hz"),
				 FloatInpSpec() );
    freqf4fld_->attach( alignedBelow, freqf3fld_ );
}


bool uiAngleCompAdvParsDlg::acceptOK()
{
    raytracerfld_->fillPar( params_.raypar_ );
    if ( isformute_ )
	return true;

    IOPar& iopar = params_.smoothingpar_;
    iopar.set( PreStack::AngleComputer::sKeySmoothType(),
	       smoothtypefld_->getIntValue() );
    if ( isSmoothTypeMovingAverage() )
    {
	iopar.set( PreStack::AngleComputer::sKeyWinLen(),
		smoothwinlengthfld_->getFValue()/SI().zDomain().userFactor() );
	iopar.set( PreStack::AngleComputer::sKeyWinFunc(),
		   smoothwindowfld_->text() );
	if ( smoothwindowfld_->text() == CosTaperWindow::sName() )
	{
	    const float uservalue = smoothwinparamfld_->getFValue();
	    if ( !mIsUdf(uservalue) )
		iopar.set( PreStack::AngleComputer::sKeyWinParam(),
			   1 - uservalue / 100 );
	}
    }
    else if ( isSmoothTypeFFTFilter() )
    {
	iopar.set( PreStack::AngleComputer::sKeyFreqF3(),
		   freqf3fld_->getFValue() );
	iopar.set( PreStack::AngleComputer::sKeyFreqF4(),
		   freqf4fld_->getFValue() );
    }

    return true;
}


void uiAngleCompAdvParsDlg::updateFromParams()
{
    raytracerfld_->usePar( params_.raypar_ );
    if ( isformute_ )
	return;

    const IOPar& iopar = params_.smoothingpar_;
    int smoothtype = 0;
    iopar.get( PreStack::AngleComputer::sKeySmoothType(), smoothtype );
    smoothtypefld_->setValue( smoothtype );
    BufferString windowname;
    iopar.get( PreStack::AngleComputer::sKeyWinFunc(), windowname );
    smoothwindowfld_->setText( windowname );
    float windowparam;
    iopar.get( PreStack::AngleComputer::sKeyWinParam(), windowparam );
    smoothwinparamfld_->setValue( mNINT32((1-windowparam)*100) );
    float windowlength;
    iopar.get( PreStack::AngleComputer::sKeyWinLen(), windowlength );
    smoothwinlengthfld_->setValue( windowlength * SI().zDomain().userFactor() );

    float freqf3;
    iopar.get( PreStack::AngleComputer::sKeyFreqF3(), freqf3 );
    freqf3fld_->setValue( freqf3 );
    float freqf4;
    iopar.get( PreStack::AngleComputer::sKeyFreqF4(), freqf4 );
    freqf4fld_->setValue( freqf4 );

    smoothTypeSel(0);
}


bool uiAngleCompAdvParsDlg::isSmoothTypeMovingAverage()
{
    const char* smoothtype = smoothtypefld_->text();
    const PreStack::AngleComputer::smoothingType smtype =
		PreStack::AngleComputer::smoothingTypeDef().parse( smoothtype);
    return smtype == PreStack::AngleComputer::MovingAverage;
}


bool uiAngleCompAdvParsDlg::isSmoothTypeFFTFilter()
{
    const char* smoothtype = smoothtypefld_->text();
    const PreStack::AngleComputer::smoothingType smtype =
		PreStack::AngleComputer::smoothingTypeDef().parse( smoothtype );
    return smtype == PreStack::AngleComputer::FFTFilter;
}


void uiAngleCompAdvParsDlg::smoothTypeSel( CallBacker* )
{
    const bool ismovingavg = isSmoothTypeMovingAverage();
    const bool isfftfilter = isSmoothTypeFFTFilter();
    smoothwindowfld_->display( ismovingavg );
    smoothwinlengthfld_->display( ismovingavg );
    freqf3fld_->display( isfftfilter );
    freqf4fld_->display( isfftfilter );

    smoothWindowSel(0);
}


void uiAngleCompAdvParsDlg::smoothWindowSel( CallBacker* )
{
    const bool ismovingavg = isSmoothTypeMovingAverage();
    FixedString smoothwindow = smoothwindowfld_->text();
    const bool iscostaper = smoothwindow==CosTaperWindow::sName();
    smoothwinparamfld_->display( ismovingavg && iscostaper );
}


void uiAngleCompAdvParsDlg::finaliseCB( CallBacker* )
{
    updateFromParams();
    if ( isformute_ )
	return;

    freqf3fld_->setToolTip(tr("Frequency where the cosine tapering "
			      "window starts: Amplitude=input"));
    freqf4fld_->setToolTip(tr("Frequency where the cosine tapering "
			      "window stops: Amplitude=0"));
    smoothTypeSel(0);
}


void uiAngleMute::initClass()
{
    uiProcessorManager::factory().addCreator( create,
					      AngleMute::sFactoryKeyword() );
}


uiDialog* uiAngleMute::create( uiParent* p, Processor* sgp )
{
    mDynamicCastGet( AngleMute*, sgmute, sgp );
    if ( !sgmute ) return 0;

    return new uiAngleMute( p, sgmute );
}


uiAngleMute::uiAngleMute( uiParent* p, AngleMute* rt )
    : uiDialog( p, uiDialog::Setup(tr("AngleMute setup"),mNoDlgTitle,
				   mODHelpKey(mAngleMuteHelpID)))
    , processor_( rt )
{
    anglecompgrp_ = new uiAngleCompGrp( this, processor_->params() );

    uiSeparator* sep = new uiSeparator( this, "Sep" );
    sep->attach( stretchedBelow, anglecompgrp_ );

    topfld_ = new uiGenInput( this, tr("Mute type"),
	    BoolInpSpec(!processor_->params().tail_,
			uiStrings::sOuter(),uiStrings::sInner()) );
    topfld_->attach( ensureBelow, sep );
    topfld_->attach( centeredBelow, anglecompgrp_ );

    taperlenfld_ = new uiGenInput( this, tr("Taper length (samples)"),
	    FloatInpSpec(processor_->params().taperlen_) );
    taperlenfld_->attach( alignedBelow, topfld_ );
}


bool uiAngleMute::acceptOK()
{
    if ( !anglecompgrp_->acceptOK() )
	return false;

    processor_->params().raypar_.setYN( RayTracer1D::sKeyReflectivity(), false);
    processor_->params().taperlen_ = taperlenfld_->getFValue();
    processor_->params().tail_ = !topfld_->getBoolValue();

    return true;
}

} // namespace PreStack
