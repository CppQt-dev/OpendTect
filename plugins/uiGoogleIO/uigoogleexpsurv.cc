/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
-*/


#include "uigoogleexpsurv.h"
#include "googlexmlwriter.h"
#include "uisurveymanager.h"
#include "uisellinest.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "oddirs.h"
#include "strmprov.h"
#include "survinfo.h"
#include "latlong.h"
#include "draw.h"
#include "od_helpids.h"

#include <iostream>


uiGoogleExportSurvey::uiGoogleExportSurvey( uiSurveyManager* uisurv )
    : uiDialog(uisurv,
       uiDialog::Setup(uiStrings::phrExport( tr("Survey boundaries to KML")),
		      tr("Specify output parameters"),
		      mODHelpKey(mGoogleExportSurveyHelpID) ) )
    , si_(uisurv->curSurvInfo())
{
    const OD::LineStyle ls( OD::LineStyle::Solid, 20, Color(255,170,80,100) );
    uiSelLineStyle::Setup lssu; lssu.drawstyle( false ).transparency( true );
    lsfld_ = new uiSelLineStyle( this, ls, lssu );

    hghtfld_ = new uiGenInput( this, tr("Border height"), FloatInpSpec(500) );
    hghtfld_->attach( alignedBelow, lsfld_ );

    File::Path deffp( SurveyDiskLocation::currentSurvey().fullPath() );
    deffp.add( "survbounds" ).setExtension( "kml" );
    uiFileSel::Setup fssu( OD::HtmlContent, deffp.fullPath() );
    fssu.setForWrite().setFormat( kmlFileuiString(), "kml" );
    fnmfld_ = new uiFileSel( this, uiStrings::sOutputFile(), fssu );
    fnmfld_->attach( alignedBelow, hghtfld_ );
}


uiGoogleExportSurvey::~uiGoogleExportSurvey()
{
}


bool uiGoogleExportSurvey::acceptOK()
{
    mCreateWriter( "Survey area", si_->name() );

    const StepInterval<int> inlrg = si_->inlRange( false );
    const StepInterval<int> crlrg = si_->crlRange( false );
    TypeSet<Coord> coords;
    coords += si_->transform(BinID(inlrg.start,crlrg.start));
    coords += si_->transform(BinID(inlrg.start,crlrg.stop));
    coords += si_->transform(BinID(inlrg.stop,crlrg.stop));
    coords += si_->transform(BinID(inlrg.stop,crlrg.start));
    coords += si_->transform(BinID(inlrg.start,crlrg.start));

    const float reqwdth = lsfld_->getWidth() * 0.1f;
    wrr.writePolyStyle( "survey", lsfld_->getColor(), mNINT32(reqwdth) );
    wrr.writePoly( "survey", si_->name(), coords, hghtfld_->getFValue(), si_ );

    return true;
}
