/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
________________________________________________________________________

-*/

#include "uimapperrangeeditor.h"

#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uihistogramdisplay.h"
#include "uicolseqdisp.h"
#include "uipixmap.h"
#include "uistrings.h"

#include "coltabmapper.h"
#include "coltabseqmgr.h"
#include "datapackbase.h"
#include "datadistributiontools.h"
#include "mousecursor.h"
#include "keystrs.h"
#include <math.h>


uiMapperRangeEditor::uiMapperRangeEditor( uiParent* p, int id, bool fixdrawrg )
    : uiGroup( p, "Mapper with color slider group" )
    , id_(id)
    , mapper_(new ColTab::Mapper)
    , ctseq_(ColTab::SeqMGR().getDefault())
    , startpix_(mUdf(int))
    , stoppix_(mUdf(int))
    , mousedown_(false)
{
    uiHistogramDisplay::Setup hsu;
    hsu.border( uiBorder(20,20,20,40) );
    hsu.fixdrawrg(fixdrawrg);
    histogramdisp_ = new uiHistogramDisplay( this, hsu, true );
    histogramdisp_->getMouseEventHandler().buttonPressed.notify(
			     mCB(this,uiMapperRangeEditor,mousePressed) );
    histogramdisp_->getMouseEventHandler().buttonReleased.notify(
			     mCB(this,uiMapperRangeEditor,mouseReleased) );
    histogramdisp_->getMouseEventHandler().movement.notify(
			     mCB(this,uiMapperRangeEditor,mouseMoved) );
    histogramdisp_->reSize.notify(
			     mCB(this,uiMapperRangeEditor,histogramResized));
    histogramdisp_->drawRangeChanged.notify(
			     mCB(this,uiMapperRangeEditor,histDRChanged));
    xax_ = histogramdisp_->xAxis();

    init();
}


uiMapperRangeEditor::~uiMapperRangeEditor()
{
    detachAllNotifiers();

    delete minhandle_; delete maxhandle_;
    delete leftcoltab_; delete centercoltab_; delete rightcoltab_;
    delete minvaltext_; delete maxvaltext_;
}


void uiMapperRangeEditor::setEmpty()
{
    histogramdisp_->setEmpty();
    minhandle_->hide(); maxhandle_->hide();
    minvaltext_->hide(); maxvaltext_->hide();
}


bool uiMapperRangeEditor::setDataPackID(
	DataPack::ID dpid, DataPackMgr::ID dmid, int version )
{
    const bool retval = histogramdisp_->setDataPackID( dpid, dmid,version);
    const bool nodata = histogramdisp_->xVals().isEmpty();
    datarg_.start = nodata ? 0 : histogramdisp_->xVals().first();
    datarg_.stop = nodata ? 1 : histogramdisp_->xVals().last();
    if ( retval )
	drawAgain();
    return retval;
}


void uiMapperRangeEditor::setData( const Array2D<float>* data )
{
    histogramdisp_->setData( data );
    const bool nodata = histogramdisp_->xVals().isEmpty();
    datarg_.start = nodata ? 0 : histogramdisp_->xVals().first();
    datarg_.stop = nodata ? 1 : histogramdisp_->xVals().last();
    drawAgain();
}


void uiMapperRangeEditor::setData( const float* array, od_int64 sz )
{
    histogramdisp_->setData( array, sz );
    const bool nodata = histogramdisp_->xVals().isEmpty();
    datarg_.start = nodata ? 0 : histogramdisp_->xVals().first();
    datarg_.stop = nodata ? 1 : histogramdisp_->xVals().last();
    drawAgain();
}


bool uiMapperRangeEditor::setData( const IOPar& iop )
{
    RefMan<DataDistribution<float> > distr = new DataDistribution<float>;
    DataDistributionChanger<float>( *distr ).usePar( iop );
    if ( distr->isEmpty() )
	return false;

    histogramdisp_->setDistribution( *distr );
    const bool nodata = histogramdisp_->xVals().isEmpty();
    datarg_.start = nodata ? 0 : histogramdisp_->xVals().first();
    datarg_.stop = nodata ? 1 : histogramdisp_->xVals().last();
    drawAgain();
    return true;
}


void uiMapperRangeEditor::setMarkValue( float val, bool forx )
{
    if ( histogramdisp_ )
	histogramdisp_->setMarkValue( val, forx );
}


void uiMapperRangeEditor::setMapper( ColTab::Mapper& mpr )
{
    uiAxisHandler* axhndler = histogramdisp_->xAxis();
    if ( !axhndler )
	return;

    StepInterval<float> axrange = axhndler->range();

    const Interval<float> rg = mpr.getRange();
    if ( !rg.includes(axrange.start,true) || !rg.includes(axrange.stop,true) )
    {
	axrange.include( rg );
	histogramdisp_->setup().xrg( axrange );
	histogramdisp_->gatherInfo();
	histogramdisp_->draw();
    }

    replaceMonitoredRef( mapper_, mpr, this );
    mapper_->setup().setFixedRange( rg );
    cliprg_.start = rg.isRev() ? rg.stop : rg.start;
    cliprg_.stop = rg.isRev() ? rg.start : rg.stop;
    drawAgain();
}


void uiMapperRangeEditor::setColTabSeq( const ColTab::Sequence& cseq )
{
    replaceMonitoredRef( ctseq_, cseq, this );
    drawAgain();
}


void uiMapperRangeEditor::init()
{
    uiGraphicsScene& scene = histogramdisp_->scene();
    const int zval = 4;

    leftcoltab_ = scene.addItem( new uiPixmapItem() );
    leftcoltab_->setZValue( zval );
    centercoltab_ = scene.addItem( new uiPixmapItem() );
    centercoltab_->setZValue( zval );
    rightcoltab_ = scene.addItem( new uiPixmapItem() );
    rightcoltab_->setZValue( zval );

    minvaltext_ = scene.addItem(
	new uiTextItem(uiString::empty(),OD::Alignment::Right) );
    maxvaltext_ = scene.addItem( new uiTextItem() );

    uiManipHandleItem::Setup mhisu;
    mhisu.color_ = Color::DgbColor();
    mhisu.thickness_ = 2;
    mhisu.zval_ = zval+2;
    minhandle_ = scene.addItem( new uiManipHandleItem(mhisu) );
    maxhandle_ = scene.addItem( new uiManipHandleItem(mhisu) );

    mAttachCB( mapper_->objectChanged(), uiMapperRangeEditor::mapperChg );
    mAttachCB( ctseq_->objectChanged(), uiMapperRangeEditor::colSeqChg );
}


void uiMapperRangeEditor::drawText()
{
    if ( mIsUdf(startpix_) || mIsUdf(stoppix_) )
	return;

    const int posy = histogramdisp_->height() / 3;
    minvaltext_->setText( toUiString(cliprg_.start) );
    minvaltext_->setPos( uiPoint(startpix_-2,posy) );
    minvaltext_->show();

    maxvaltext_->setText( toUiString(cliprg_.stop) );
    maxvaltext_->setPos( uiPoint(stoppix_+2,posy) );
    maxvaltext_->show();
}


void uiMapperRangeEditor::drawPixmaps()
{
    if ( !ctseq_ || mIsUdf(startpix_) || mIsUdf(stoppix_) )
	return;

    const int disph = histogramdisp_->height();
    const int pmh = 20;
    const int datastartpix = xax_->getPix( datarg_.start );
    const int datastoppix = xax_->getPix( datarg_.stop );

    const Interval<float> mapperrg = mapper_->getRange();
    uiPixmap leftpixmap( startpix_-datastartpix, pmh );
    leftpixmap.fill( ctseq_->color( mapperrg.width()>0 ? 0.f :1.f ) );
    leftcoltab_->setPixmap( leftpixmap );
    leftcoltab_->setOffset( datastartpix, disph-pmh-1 );

    uiPixmap* pm = ColTab::getuiPixmap( *ctseq_, stoppix_-startpix_, pmh,
					mapper_ );
    if ( pm )
	centercoltab_->setPixmap( *pm );
    centercoltab_->setOffset( startpix_, disph-pmh-1 );
    delete pm;

    uiPixmap rightpixmap( datastoppix-stoppix_, pmh );
    rightpixmap.fill( ctseq_->color( mapperrg.width()>0 ? 1.f : 0.f ) );
    rightcoltab_->setPixmap( rightpixmap );
    rightcoltab_->setOffset( stoppix_, disph-pmh-1 );
}


void uiMapperRangeEditor::drawLines()
{
    if ( mIsUdf(startpix_) || mIsUdf(stoppix_) )
	return;

    minhandle_->setPixPos( startpix_ );
    maxhandle_->setPixPos( stoppix_ );
}


void uiMapperRangeEditor::drawAgain()
{
    startpix_ = xax_->getPix( cliprg_.start );
    stoppix_ = xax_->getPix( cliprg_.stop );

    drawText();
    drawLines();
    drawPixmaps();
}


void uiMapperRangeEditor::colSeqChg( CallBacker* cb )
{
    drawAgain();
}


void uiMapperRangeEditor::mapperChg( CallBacker* cb )
{
    const Interval<float> rg = mapper_->getRange();
    cliprg_.start = rg.isRev() ? rg.stop : rg.start;
    cliprg_.stop = rg.isRev() ? rg.start : rg.stop;
    drawAgain();
}


void uiMapperRangeEditor::histogramResized( CallBacker* cb )
{
    drawAgain();
}


bool uiMapperRangeEditor::changeLinePos( bool firstclick )
{
    MouseEventHandler& meh = histogramdisp_->getMouseEventHandler();
    if ( meh.isHandled() )
	return false;

    const MouseEvent& ev = meh.event();
    if ( !(ev.buttonState() & OD::LeftButton ) ||
	  (ev.buttonState() & OD::MidButton ) ||
	  (ev.buttonState() & OD::RightButton ) )
	return false;

    const int diff = stoppix_ - startpix_;
    if ( !firstclick && fabs(float(diff)) <= 1 )
	return false;

    const int mousepix = ev.pos().x_;
    const float mouseposval = xax_->getVal( ev.pos().x_ );

    const Interval<float> histxrg = histogramdisp_->xAxis()->range();
    const bool insiderg = datarg_.includes(mouseposval,true) &&
			  histxrg.includes(mouseposval,true);
    if ( !firstclick && !insiderg )
	return false;

#define clickrg 5
    if ( mouseposval < (cliprg_.start+cliprg_.stop)/2 )
    {
	const bool faraway = (mousepix > startpix_+clickrg) ||
			     (mousepix < startpix_-clickrg);
	if ( firstclick && faraway )
	    return false;

	cliprg_.start = mouseposval;
    }
    else
    {
	const bool faraway = (mousepix > stoppix_+clickrg) ||
			     (mousepix < stoppix_-clickrg);
	if ( firstclick && faraway )
	    return false;

	cliprg_.stop = mouseposval;
    }

    return true;
}


void uiMapperRangeEditor::mousePressed( CallBacker* cb )
{
    MouseEventHandler& meh = histogramdisp_->getMouseEventHandler();
    if ( meh.isHandled() || mousedown_ ) return;

    mousedown_ = true;
    if ( changeLinePos(true) )
    {
	drawAgain();
	meh.setHandled( true );
    }
    else
	mousedown_ = false;
}


void uiMapperRangeEditor::mouseMoved( CallBacker* )
{
    MouseEventHandler& meh = histogramdisp_->getMouseEventHandler();
    if ( meh.isHandled() || !mousedown_ ) return;

    if ( changeLinePos() )
    {
	drawAgain();

	Interval<float> newrg;
	newrg.start =
	    mapper_->getRange().isRev() ? cliprg_.stop : cliprg_.start;
	newrg.stop =
	    mapper_->getRange().isRev() ? cliprg_.start : cliprg_.stop;
	mapper_->setup().setFixedRange( newrg );
    }

    meh.setHandled( true );
}


void uiMapperRangeEditor::mouseReleased( CallBacker* )
{
    MouseEventHandler& meh = histogramdisp_->getMouseEventHandler();
    if ( meh.isHandled() || !mousedown_ )
	return;

    mousedown_ = false;
    Interval<float> newrg;
    newrg.start =
	mapper_->getRange().isRev() ? cliprg_.stop : cliprg_.start;
    newrg.stop =
	mapper_->getRange().isRev() ? cliprg_.start : cliprg_.stop;
    mapper_->setup().setFixedRange( newrg );

    drawAgain();
    meh.setHandled( true );
}


void uiMapperRangeEditor::histDRChanged( CallBacker* cb )
{
    const Interval<float>& drg = histogramdisp_->getDrawRange();
    if ( cliprg_.start<drg.start )
	cliprg_.start = drg.start;
    if ( cliprg_.stop>drg.stop )
	cliprg_.stop = drg.stop;

    startpix_ = xax_->getPix( cliprg_.start );
    stoppix_ = xax_->getPix( cliprg_.stop );

    minhandle_->setPixPos( startpix_ );
    maxhandle_->setPixPos( stoppix_ );

    Interval<float> newrg;
    newrg.start =
	mapper_->getRange().isRev() ? cliprg_.stop : cliprg_.start;
    newrg.stop =
	mapper_->getRange().isRev() ? cliprg_.start : cliprg_.stop;
    mapper_->setup().setFixedRange( newrg );
}
