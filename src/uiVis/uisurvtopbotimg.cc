/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2009
________________________________________________________________________

-*/

#include "uisurvtopbotimg.h"
#include "vistopbotimage.h"
#include "vissurvscene.h"
#include "uislider.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uifiledlg.h"
#include "uiseparator.h"
#include "vismaterial.h"

#include "survinfo.h"
#include "oddirs.h"
#include "od_helpids.h"


class uiSurvTopBotImageGrp : public uiGroup
{ mODTextTranslationClass(uiSurvTopBotImageGrp);
public:

uiSurvTopBotImageGrp( uiSurvTopBotImageDlg* p, bool istop,
			const StepInterval<float>& zrng )
    : uiGroup(p, istop ? "Top img grp" : "Bot img grp")
    , dlg_(p)
    , istop_(istop)
    , img_(p->scene_->getTopBotImage(istop))
    , zrng_(zrng)
    , tlfld_(0)
{
    uiFileSel::Setup fssu( OD::ImageContent );
    fssu.checkable( true );
    fnmfld_ = new uiFileSel( this,
		    istop_ ? tr("Top image") : tr("Bottom image"), fssu );
    fnmfld_->newSelection.notify( mCB(this,uiSurvTopBotImageGrp,newFile) );
    fnmfld_->setChecked( img_ && img_->isOn() );
    mAttachCB( fnmfld_->checked, uiSurvTopBotImageGrp::onOff );

#define mAddCoordchgCB( notif ) \
     mAttachCB( notif, uiSurvTopBotImageGrp::coordChg );

    uiSlider::Setup slsu( tr("Vertical position (Z)") );
    slsu.withedit( true );
    slsu.isvertical( true );
    slsu.sldrsize( 100 );
    zposfld_ = new uiSlider( this, slsu );
    zposfld_->setInverted( true );
    zposfld_->setScale( zrng_.step, zrng_.start );
    zposfld_->setInterval( zrng_ );
    zposfld_->attach( rightOf, fnmfld_ );
    zposfld_->setValue( istop_ ? zrng_.start : zrng_.stop );
    mAddCoordchgCB( zposfld_->valueChanged );

    const Coord mincrd = SI().minCoord( OD::UsrWork );
    const Coord maxcrd = SI().maxCoord( OD::UsrWork );
    tlfld_ = new uiGenInput( this, tr("NorthWest (TopLeft) Coordinate"),
			     PositionInpSpec(Coord(mincrd.x_,maxcrd.y_)) );
    tlfld_->attach( leftAlignedBelow, fnmfld_ );
    mAddCoordchgCB( tlfld_->valuechanged );

    brfld_ = new uiGenInput( this, tr("SouthEast (BottomRight) Coordinate"),
			     PositionInpSpec(Coord(maxcrd.x_,mincrd.y_)) );
    brfld_->attach( alignedBelow, tlfld_ );
    mAddCoordchgCB( brfld_->valuechanged );

    transpfld_ = new uiSlider( this,
			    uiSlider::Setup(uiStrings::sTransparency())
                            .sldrsize(150), "Transparency slider" );
    transpfld_->attach( alignedBelow, brfld_ );
    transpfld_->setMinValue( 0 );
    transpfld_->setMaxValue( 100 );
    transpfld_->setStep( 1 );
    mAttachCB( transpfld_->valueChanged,
	       uiSurvTopBotImageGrp::transpChg );

    mAttachCB( postFinalise(), uiSurvTopBotImageGrp::finalisedCB );
}

~uiSurvTopBotImageGrp()
{
    detachAllNotifiers();
}


void finalisedCB( CallBacker* cb )
{
    fillCurrent();
}

void fillCurrent()
{
    if ( img_ )
    {
	fnmfld_->setChecked( img_->isOn() );
	fnmfld_->setFileName( img_->getImageFilename() );
	tlfld_->setValue( img_->topLeft().getXY() );
	brfld_->setValue( img_->bottomRight().getXY() );
	transpfld_->setValue( img_->getTransparency()*100 );
	zposfld_->setValue( mCast(float,img_->topLeft().z_) );
    }
}

void newFile( CallBacker* )
{
    dlg_->newFile( istop_, fnmfld_->fileName() );
}

void onOff( CallBacker* cb  )
{
    if ( !tlfld_ )
	return;

    const bool ison = fnmfld_->isChecked();

    if ( !img_ && ison )
    {
	dlg_->scene_->createTopBotImage( istop_ );
	img_ = dlg_->scene_->getTopBotImage( istop_ );
	coordChg( 0 );
    }

    dlg_->setOn( istop_, ison );
    tlfld_->display( ison );
    brfld_->display( ison );
    transpfld_->display( ison );
    zposfld_->display( ison );
}

void coordChg( CallBacker* cb )
{
    const Coord3 tlcoord( tlfld_->getCoord(), zposfld_->getValue() );
    const Coord3 brcoord( brfld_->getCoord(), zposfld_->getValue() );
    dlg_->setCoord( istop_, tlcoord, brcoord );
}

void transpChg( CallBacker* )
{
    dlg_->setTransparency( istop_,
			   float(transpfld_->getIntValue()/100.) );
}

    const bool		istop_;

    uiSurvTopBotImageDlg* dlg_;
    visBase::TopBotImage* img_;

    uiFileSel*		fnmfld_;
    uiGenInput*		tlfld_;
    uiGenInput*		brfld_;
    uiSlider*		transpfld_;
    uiSlider*		zposfld_;
    const StepInterval<float>&	 zrng_;
};


uiSurvTopBotImageDlg::uiSurvTopBotImageDlg( uiParent* p,
					    visSurvey::Scene* scene )
    : uiDialog(p, uiDialog::Setup(tr("Top/Bottom Images"),
				  tr("Set Top and/or Bottom Images"),
				  mODHelpKey(mSurvTopBotImageDlgHelpID) ) )
    , scene_( scene )
{
    setCtrlStyle( CloseOnly );

    topfld_ = new uiSurvTopBotImageGrp( this, true,
					scene_->getTrcKeyZSampling().zsamp_ );
    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, topfld_ );
    botfld_ = new uiSurvTopBotImageGrp( this, false,
					scene_->getTrcKeyZSampling().zsamp_ );
    botfld_->attach( alignedBelow, topfld_ );
    botfld_->attach( ensureBelow, sep );
}



void uiSurvTopBotImageDlg::newFile( bool istop, const char* fnm )
{
    if ( scene_->getTopBotImage(istop) )
	scene_->getTopBotImage(istop)->setRGBImageFromFile( fnm );
}


void uiSurvTopBotImageDlg::setOn( bool istop, bool ison )
{
    if ( scene_->getTopBotImage(istop) )
	scene_->getTopBotImage(istop)->turnOn( ison );
}


void uiSurvTopBotImageDlg::setCoord( bool istop, const Coord3& tl,
						 const Coord3& br  )
{
    if ( scene_->getTopBotImage(istop) )
	scene_->getTopBotImage(istop)->setPos( tl, br );
}


void uiSurvTopBotImageDlg::setTransparency( bool istop, float val )
{
    if ( scene_->getTopBotImage(istop) )
	scene_->getTopBotImage(istop)->setTransparency( val );
}
