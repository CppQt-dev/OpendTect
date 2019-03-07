/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uipickpropdlg.h"

#include "color.h"
#include "draw.h"
#include "pickset.h"
#include "settings.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimarkerstyle.h"
#include "uislider.h"
#include "vispicksetdisplay.h"
#include "vistristripset.h"


uiPickPropDlg::uiPickPropDlg( uiParent* p, Pick::Set& set,
			      visSurvey::PickSetDisplay* psd )
    : uiMarkerStyleDlg( p, tr("PointSet Display Properties") )
    , set_( set )
    , psd_( psd )
{
    usedrawstylefld_ = new uiCheckBox( this, tr("Connect picks") );
    const bool hasbody = psd && psd->isBodyDisplayed();
    const bool hassty = set_.disp_.connect_==Pick::Set::Disp::Close || hasbody;
    usedrawstylefld_->setChecked( hassty );
    usedrawstylefld_->activated.notify( mCB(this,uiPickPropDlg,drawSel) );

    drawstylefld_ = new uiGenInput( this, tr("with"),
				    BoolInpSpec( true, tr("Line"),
				    tr("Surface") ) );
    drawstylefld_->setValue( !hasbody );
    drawstylefld_->valuechanged.notify( mCB(this,uiPickPropDlg,drawStyleCB) );
    drawstylefld_->attach( rightOf, usedrawstylefld_ );

    stylefld_->attach( alignedBelow, usedrawstylefld_ );

    bool usethreshold = true;
    Settings::common().getYN( Pick::Set::sKeyUseThreshold(), usethreshold );
    usethresholdfld_ =
     new uiCheckBox( this, tr("Switch to Point mode for all large PointSets") );
    usethresholdfld_->setChecked( usethreshold );
    usethresholdfld_->activated.notify( mCB(this,uiPickPropDlg,useThresholdCB));
    usethresholdfld_->attach( alignedBelow, stylefld_ );

    thresholdfld_ =  new uiGenInput( this, tr("Threshold size for Point mode"));
    thresholdfld_->attach( rightAlignedBelow, usethresholdfld_ );
    thresholdfld_->valuechanged.notify(
				     mCB(this,uiPickPropDlg,thresholdChangeCB));
    thresholdfld_->setSensitive( usethreshold );
    thresholdfld_->setValue( Pick::Set::getSizeThreshold() );

    drawSel( 0 );
}


void uiPickPropDlg::drawSel( CallBacker* )
{
    const bool usestyle = usedrawstylefld_->isChecked();
    drawstylefld_->display( usestyle );

    if ( !usestyle )
    {
	if ( set_.disp_.connect_==Pick::Set::Disp::Close )
	{
	    set_.disp_.connect_ = Pick::Set::Disp::None;
	    Pick::Mgr().reportDispChange( this, set_ );
	}

	if ( psd_ )
	    psd_->displayBody( false );
    }
    else
	drawStyleCB( 0 );
}


void uiPickPropDlg::drawStyleCB( CallBacker* )
{
    const bool showline = drawstylefld_->getBoolValue();
    if ( psd_ )
	psd_->displayBody( !showline );

    if ( showline )
    {
	set_.disp_.connect_ = Pick::Set::Disp::Close;
	Pick::Mgr().reportDispChange( this, set_ );
    }
    else
    {
	if ( !psd_ ) return;
	set_.disp_.connect_ = Pick::Set::Disp::None;
	Pick::Mgr().reportDispChange( this, set_ );

	if ( !psd_->getDisplayBody() )
	    psd_->setBodyDisplay();
    }
}


void uiPickPropDlg::doFinalise( CallBacker* )
{
    const MarkerStyle3D style( (MarkerStyle3D::Type) set_.disp_.markertype_,
	    set_.disp_.pixsize_, set_.disp_.color_ );
    stylefld_->setMarkerStyle( style );
}


void uiPickPropDlg::sliderMove( CallBacker* )
{
    MarkerStyle3D style;
    stylefld_->getMarkerStyle( style );

    if ( set_.disp_.pixsize_ == style.size_ )
	return;

    set_.disp_.pixsize_ = style.size_;
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::typeSel( CallBacker* )
{
    MarkerStyle3D style;
    stylefld_->getMarkerStyle( style );

    set_.disp_.markertype_ = style.type_;
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::colSel( CallBacker* )
{
    MarkerStyle3D style;
    stylefld_->getMarkerStyle( style );
    set_.disp_.color_ = style.color_;
    Pick::Mgr().reportDispChange( this, set_ );
}


void uiPickPropDlg::useThresholdCB( CallBacker* )
{
    const bool usesthreshold = usethresholdfld_->isChecked();
    thresholdfld_->setSensitive( usesthreshold );
    Settings::common().setYN( Pick::Set::sKeyUseThreshold(), usesthreshold );
    Settings::common().write();
}


void uiPickPropDlg::thresholdChangeCB( CallBacker* )
{
    const int thresholdval = thresholdfld_->getIntValue();
    Settings::common().set( Pick::Set::sKeyThresholdSize(), thresholdval );
    Settings::common().write();
}


bool uiPickPropDlg::acceptOK( CallBacker* )
{
    set_.writeDisplayPars();
    return true;
}
