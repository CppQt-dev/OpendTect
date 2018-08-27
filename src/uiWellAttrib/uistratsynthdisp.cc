/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uistratsynthdisp.h"
#include "uistratlaymodtools.h"
#include "uistratlvlsel.h"
#include "uisynthgendlg.h"
#include "uistratsynthexport.h"
#include "uiwaveletsel.h"
#include "uisynthtorealscale.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uiflatviewmainwin.h"
#include "uiflatviewslicepos.h"
#include "uigeninput.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uimultiflatviewcontrol.h"
#include "uipsviewer2dmainwin.h"
#include "uispinbox.h"
#include "uistratlayermodel.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "envvars.h"
#include "prestacksyntheticdata.h"
#include "stratsynth.h"
#include "coltabsequence.h"
#include "stratsynthlevel.h"
#include "stratsynthlevelset.h"
#include "stratlith.h"
#include "stratunitref.h"
#include "syntheticdataimpl.h"
#include "flatviewzoommgr.h"
#include "flatposdata.h"
#include "ptrman.h"
#include "propertyref.h"
#include "prestackgather.h"
#include "survinfo.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "synthseis.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "velocitycalc.h"
#include "waveletio.h"


static const int cMarkerSize = 6;

static const char* sKeySnapLevel()	{ return "Snap Level"; }
static const char* sKeyNrSynthetics()	{ return "Nr of Synthetics"; }
static const char* sKeySyntheticNr()	{ return "Synthetics Nr"; }
static const char* sKeySynthetics()	{ return "Synthetics"; }
static const char* sKeyViewArea()	{ return "Start View Area"; }
static const char* sKeyNone()		{ return "None"; }
static const char* sKeyDecimation()	{ return "Decimation"; }


uiStratSynthDisp::uiStratSynthDisp( uiParent* p, uiStratLayerModel& uislm,
				    const Strat::LayerModelProvider& lmp )
    : uiGroup(p,"LayerModel synthetics display")
    , lmp_(lmp)
    , d2tmodels_(0)
    , stratsynth_(new StratSynth(lmp,false))
    , edstratsynth_(new StratSynth(lmp,true))
    , useed_(false)
    , dispeach_(1)
    , dispskipz_(0)
    , dispflattened_(false)
    , selectedtrace_(-1)
    , selectedtraceaux_(0)
    , frtxtitm_(0)
    , wvltChanged(this)
    , viewChanged(this)
    , modSelChanged(this)
    , synthsChanged(this)
    , dispParsChanged(this)
    , layerPropSelNeeded(this)
    , longestaimdl_(0)
    , lasttool_(0)
    , synthgendlg_(0)
    , prestackwin_(0)
    , currentwvasynthetic_(0)
    , currentvdsynthetic_(0)
    , autoupdate_(true)
    , forceupdate_(false)
    , relzoomwr_(0,0,1,1)
    , savedzoomwr_(mUdf(double),0,0,0)
    , flattenlvl_(Strat::Level::undef())
    , trprov_(p)
{
    stratsynth_->setRunnerProvider( trprov_ );
    edstratsynth_->setRunnerProvider( trprov_ );

    topgrp_ = new uiGroup( this, "Top group" );
    topgrp_->setFrame( true );
    topgrp_->setStretch( 2, 0 );

    uiLabeledComboBox* datalblcbx =
	new uiLabeledComboBox( topgrp_, tr("Wiggle View"), "" );
    wvadatalist_ = datalblcbx->box();
    wvadatalist_->selectionChanged.notify(
	    mCB(this,uiStratSynthDisp,wvDataSetSel) );
    wvadatalist_->setHSzPol( uiObject::Wide );

    uiToolButton* edbut = new uiToolButton( topgrp_, "edit",
				tr("Add/Edit Synthetic DataSet"),
				mCB(this,uiStratSynthDisp,addEditSynth) );

    edbut->attach( leftOf, datalblcbx );

    uiGroup* dataselgrp = new uiGroup( this, "Data Selection" );
    dataselgrp->attach( rightBorder );
    dataselgrp->attach( ensureRightOf, topgrp_ );

    uiLabeledComboBox* prdatalblcbx =
	new uiLabeledComboBox( dataselgrp, tr("Variable Density View"), "" );
    vddatalist_ = prdatalblcbx->box();
    vddatalist_->selectionChanged.notify(
	    mCB(this,uiStratSynthDisp,vdDataSetSel) );
    vddatalist_->setHSzPol( uiObject::Wide );
    prdatalblcbx->attach( leftBorder );

    uiToolButton* expbut = new uiToolButton( prdatalblcbx, "export",
			    uiStrings::phrExport( tr("Synthetic DataSet(s)")),
			    mCB(this,uiStratSynthDisp,exportSynth) );
    expbut->attach( rightOf, vddatalist_ );

    datagrp_ = new uiGroup( this, "DataSet group" );
    datagrp_->attach( ensureBelow, topgrp_ );
    datagrp_->attach( ensureBelow, dataselgrp );
    datagrp_->setFrame( true );
    datagrp_->setStretch( 2, 0 );

    uiToolButton* layertb = new uiToolButton( datagrp_, "defraytraceprops",
				    tr("Specify input for synthetic creation"),
				    mCB(this,uiStratSynthDisp,layerPropsPush));

    uiWaveletIOObjSel::Setup wvsu; wvsu.compact( true );
    wvltfld_ = new uiWaveletIOObjSel( datagrp_, wvsu );
    wvltfld_->selectionDone.notify( mCB(this,uiStratSynthDisp,wvltChg) );
    wvltfld_->setFrame( false );
    wvltfld_->attach( rightOf, layertb );
    curSS().setWavelet( wvltfld_->getWavelet() );

    scalebut_ = new uiPushButton( datagrp_, uiStrings::sScale(), false );
    scalebut_->activated.notify( mCB(this,uiStratSynthDisp,scalePush) );
    scalebut_->attach( rightOf, wvltfld_ );

    uiLabeledComboBox* lvlsnapcbx =
	new uiLabeledComboBox( datagrp_, VSEvent::TypeDef(), tr("Snap level") );
    levelsnapselfld_ = lvlsnapcbx->box();
    lvlsnapcbx->attach( rightOf, scalebut_ );
    lvlsnapcbx->setStretch( 2, 0 );
    levelsnapselfld_->selectionChanged.notify(
				mCB(this,uiStratSynthDisp,levelSnapChanged) );

    prestackgrp_ = new uiGroup( datagrp_, "Prestack View Group" );
    prestackgrp_->attach( rightOf, lvlsnapcbx, 20 );

    offsetposfld_ = new uiSynthSlicePos( prestackgrp_, uiStrings::sOffset() );
    offsetposfld_->positionChg.notify( mCB(this,uiStratSynthDisp,offsetChged));

    prestackbut_ = new uiToolButton( prestackgrp_, "nonmocorr64",
				tr("View Offset Direction"),
				mCB(this,uiStratSynthDisp,viewPreStackPush) );
    prestackbut_->attach( rightOf, offsetposfld_);

    vwr_ = new uiFlatViewer( this );
    vwr_->rgbCanvas().disableImageSave();
    vwr_->setInitialSize( uiSize(800,300) ); //TODO get hor sz from laymod disp
    vwr_->setStretch( 2, 2 );
    vwr_->attach( ensureBelow, datagrp_ );
    mAttachCB( vwr_->dispPropChanged, uiStratSynthDisp::parsChangedCB );
    FlatView::Appearance& app = vwr_->appearance();
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.allowuserchangereversedaxis_ = false;
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.annot_.x1_.annotinint_ = true;
    app.annot_.x2_.name_ = uiStrings::sTWT();
    app.ddpars_.show( true, true );
    app.ddpars_.wva_.allowuserchangedata_ = false;
    app.ddpars_.vd_.allowuserchangedata_ = false;
    mAttachCB( vwr_->viewChanged, uiStratSynthDisp::viewChg );
    mAttachCB( vwr_->rgbCanvas().reSize, uiStratSynthDisp::updateTextPosCB );
    setDefaultAppearance( vwr_->appearance() );

    mAttachCB( uislm.newModels, uiStratSynthDisp::doModelChange );

    uiFlatViewStdControl::Setup fvsu( this );
    fvsu.withcoltabed(false).tba((int)uiToolBar::Right)
	.withflip(false).withsnapshot(false);
    control_ = new uiMultiFlatViewControl( *vwr_, fvsu );
    control_->setViewerType( vwr_, true );
}


uiStratSynthDisp::~uiStratSynthDisp()
{
    detachAllNotifiers();
    delete stratsynth_;
    delete edstratsynth_;
    delete d2tmodels_;
}


void uiStratSynthDisp::makeInfoMsg( uiString& msg, IOPar& pars )
{
    FixedString valstr = pars.find( sKey::TraceNr() );
    if ( valstr.isEmpty() )
	return;

    const int modelidx = toInt( valstr );
    const int seqidx = modelidx - 1;
    if ( seqidx<0 || seqidx>=layerModel().size() )
	return;

    msg.postFixWord( uiStrings::sModelNumber().addMoreInfo( modelidx ) );

    valstr = pars.find( sKey::Z() );
    if ( !valstr )
	valstr = pars.find( "Z-Coord" );
    const float zval = valstr && *valstr ? toFloat(valstr) : mUdf(float);
    if ( mIsUdf(zval) )
      return;
    uiString zstr = uiStrings::sZ().addMoreInfo(mNINT32(zval)).withSurvZUnit();
    msg.postFixWord( zstr );

    FixedString vdstr = pars.find( "Variable density data" );
    FixedString wvastr = pars.find( "Wiggle/VA data" );
    FixedString vdvalstr = pars.find( "VD Value" );
    FixedString wvavalstr = pars.find( "WVA Value" );
    const bool vdsameaswva = vdstr == wvastr;
    if ( !vdvalstr.isEmpty() )
    {
	if ( vdsameaswva && vdstr.isEmpty() )
	    vdstr = wvastr;
	if ( vdstr.isEmpty() )
	    vdstr = "VD";
	const float val = vdvalstr.toFloat();
	uiString toadd = toUiString( vdstr );
	if ( mIsUdf(val) )
	    toadd.addMoreInfo( uiStrings::sUndef() );
	else
	    toadd.addMoreInfo( val );
	msg.postFixWord( toadd );
    }

    if ( !wvavalstr.isEmpty() && !vdsameaswva )
    {
	if ( wvastr.isEmpty() )
	    wvastr = "WVA";
	const float val = wvavalstr.toFloat();
	uiString toadd = toUiString( wvastr );
	if ( mIsUdf(val) )
	    toadd.addMoreInfo( uiStrings::sUndef() );
	else
	    toadd.addMoreInfo( val );
	msg.postFixWord( toadd );
    }

    float val;
    if ( pars.get(sKey::Offset(),val) && !mIsUdf(val) )
    {
	if ( SI().xyInFeet() )
	    val *= mToFeetFactorF;
	uiString toadd = uiStrings::sOffset().addMoreInfo(val).withSurvXYUnit();
	msg.postFixWord( toadd );
    }

    if ( d2tmodels_ && d2tmodels_->validIdx(seqidx) )
    {
	const float realzval = zval / SI().showZ2UserFactor();
	const float depth = d2tmodels_->get(seqidx)->getDepth( realzval );
	const Strat::LayerSequence& curseq = layerModel().sequence( seqidx );
	for ( int lidx=0; lidx<curseq.size(); lidx++ )
	{
	    const auto& lay = *curseq.layers().get( lidx );
	    if ( lay.zTop()<=depth && lay.zBot()>depth )
	    {
		msg.postFixWord( uiStrings::sLayer().addMoreInfo(lay.name()) );
		break;
	    }
	}
    }
}


void uiStratSynthDisp::addViewerToControl( uiFlatViewer& vwr )
{
    if ( control_ )
    {
	control_->addViewer( vwr );
	control_->setViewerType( &vwr, false );
    }
}


const Strat::LayerModel& uiStratSynthDisp::layerModel() const
{
    return lmp_.getCurrent();
}


void uiStratSynthDisp::layerPropsPush( CallBacker* )
{
    layerPropSelNeeded.trigger();
}


void uiStratSynthDisp::addTool( const uiToolButtonSetup& bsu )
{
    uiButton* tb = bsu.getButton( datagrp_ );
    if ( lasttool_ )
	tb->attach( leftOf, lasttool_ );
    else
	tb->attach( rightBorder );

    tb->attach( ensureRightOf, prestackbut_ );

    lasttool_ = tb;
}

#define mDelD2TM \
    delete d2tmodels_; \
    d2tmodels_ = 0;

void uiStratSynthDisp::cleanSynthetics()
{
    currentwvasynthetic_ = 0;
    currentvdsynthetic_ = 0;
    curSS().clearSynthetics();
    mDelD2TM
    wvadatalist_->setEmpty();
    vddatalist_->setEmpty();
}


void uiStratSynthDisp::updateSyntheticList( bool wva )
{
    uiComboBox* datalist = wva ? wvadatalist_ : vddatalist_;
    BufferString curitem = datalist->text();
    datalist->setEmpty();
    datalist->addItem( uiStrings::sNone() );
    for ( int idx=0; idx<curSS().nrSynthetics(); idx ++)
    {
	ConstRefMan<SyntheticData> sd = curSS().getSyntheticByIdx( idx );
	if ( !sd ) continue;

	mDynamicCastGet(const StratPropSyntheticData*,prsd,sd.ptr());
	if ( wva && prsd ) continue;
	datalist->addItem( toUiString(sd->name()) );
    }

    datalist->setCurrentItem( curitem );
}


void uiStratSynthDisp::setDisplayZSkip( float zskip, bool withmodchg )
{
    dispskipz_ = zskip;
    if ( withmodchg )
	doModelChange(0);
}


void uiStratSynthDisp::setDispEach( int de )
{
    dispeach_ = de;
    displayPostStackSynthetic( currentwvasynthetic_, true );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::setSelectedTrace( int st )
{
    selectedtrace_ = st;

    delete vwr_->removeAuxData( selectedtraceaux_ );
    selectedtraceaux_ = 0;

    const StepInterval<double> xrg = vwr_->getDataPackRange(true);
    const StepInterval<double> zrg = vwr_->getDataPackRange(false);

    const float offset = mCast( float, xrg.start );
    if ( !xrg.includes( selectedtrace_ + offset, true ) )
	return;

    selectedtraceaux_ = vwr_->createAuxData( "Selected trace" );
    selectedtraceaux_->zvalue_ = 2;
    vwr_->addAuxData( selectedtraceaux_ );

    const double ptx = selectedtrace_ + offset;
    const double ptz1 = zrg.start;
    const double ptz2 = zrg.stop;

    Geom::Point2D<double> pt1 = Geom::Point2D<double>( ptx, ptz1 );
    Geom::Point2D<double> pt2 = Geom::Point2D<double>( ptx, ptz2 );

    selectedtraceaux_->poly_ += pt1;
    selectedtraceaux_->poly_ += pt2;
    selectedtraceaux_->linestyle_ =
	OD::LineStyle( OD::LineStyle::Dot, 2, Color::DgbColor() );

    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
}



void uiStratSynthDisp::handleFlattenChange()
{
    resetRelativeViewRect();
    doModelChange(0);
    control_->zoomMgr().toStart();
}


void uiStratSynthDisp::setFlattened( bool flattened, bool trigger )
{
    dispflattened_ = flattened;
    control_->setFlattened( flattened );
    if ( trigger )
	handleFlattenChange();
}


void uiStratSynthDisp::setDispMrkrs( const BufferStringSet& lvlnmset,
			    const uiStratLayerModelDisp::LVLZValsSet& zvalset )
{
    curSS().setLevels( lvlnmset, zvalset );
    levelSnapChanged(0);
}


void uiStratSynthDisp::setRelativeViewRect( const uiWorldRect& relwr )
{
    relzoomwr_ = relwr;
    uiWorldRect abswr;
    getAbsoluteViewRect( abswr );
    vwr_->setView( abswr );
}


void uiStratSynthDisp::setAbsoluteViewRect( const uiWorldRect& wr )
{
    uiWorldRect abswr = wr;
    uiWorldRect bbwr = vwr_->boundingBox();
    bbwr.sortCorners();
    abswr.sortCorners();
    if ( mIsZero(bbwr.width(),1e-3) || mIsZero(bbwr.height(),1e-3) ||
	 !bbwr.contains(abswr,1e-3) )
	return;

    const double left = (abswr.left() - bbwr.left())/bbwr.width();
    const double right = (abswr.right() - bbwr.left())/bbwr.width();
    const double top = (abswr.top() - bbwr.top())/bbwr.height();
    const double bottom = (abswr.bottom() - bbwr.top())/bbwr.height();
    relzoomwr_ = uiWorldRect( left, top, right, bottom );
}


void uiStratSynthDisp::getAbsoluteViewRect( uiWorldRect& abswr ) const
{
    uiWorldRect bbwr = vwr_->boundingBox();
    bbwr.sortCorners();
    if ( mIsZero(bbwr.width(),1e-3) || mIsZero(bbwr.height(),1e-3) )
	return;
    const double left = bbwr.left() + relzoomwr_.left()*bbwr.width();
    const double right = bbwr.left() + relzoomwr_.right()*bbwr.width();
    const double top = bbwr.top() + relzoomwr_.top()*bbwr.height();
    const double bottom = bbwr.top() + relzoomwr_.bottom()*bbwr.height();
    abswr = uiWorldRect( left, top, right, bottom );
}


void uiStratSynthDisp::resetRelativeViewRect()
{
    relzoomwr_ = uiWorldRect( 0, 0, 1, 1 );
}


void uiStratSynthDisp::updateRelativeViewRect()
{
    setAbsoluteViewRect( curView(false) );
}


void uiStratSynthDisp::setZoomView( const uiWorldRect& relwr )
{
    relzoomwr_ = relwr;
    uiWorldRect abswr;
    getAbsoluteViewRect( abswr );
    Geom::Point2D<double> centre = abswr.centre();
    Geom::Size2D<double> newsz = abswr.size();
    control_->setActiveVwr( 0 );
    control_->setNewView( centre, newsz, *control_->activeVwr() );
}


void uiStratSynthDisp::setZDataRange( const Interval<double>& zrg, bool indpth )
{
    Interval<double> newzrg; newzrg.set( zrg.start, zrg.stop );
    if ( indpth && d2tmodels_ && !d2tmodels_->isEmpty() )
    {
	int mdlidx = longestaimdl_;
	if ( !d2tmodels_->validIdx(mdlidx) )
	    mdlidx = d2tmodels_->size()-1;

	const TimeDepthModel& d2t = *(*d2tmodels_)[mdlidx];
	newzrg.start = d2t.getTime( (float)zrg.start );
	newzrg.stop = d2t.getTime( (float)zrg.stop );
    }
    const Interval<double> xrg = vwr_->getDataPackRange( true );
    vwr_->setSelDataRanges( xrg, newzrg );
    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
}


void uiStratSynthDisp::levelSnapChanged( CallBacker* )
{
    const StratSynthLevelSet* lvl = curSS().getLevels();
    if ( !lvl )  return;

    StratSynthLevelSet* edlvl = const_cast<StratSynthLevelSet*>( lvl );
    edlvl->setSnapEv( VSEvent::TypeDef().parse(levelsnapselfld_->text()) );
    drawLevel();
}


const char* uiStratSynthDisp::levelName( const int idx )  const
{
    const StratSynthLevel* lvl = curSS().getLevel(idx);
    return lvl ? lvl->name().buf() : 0;
}


void uiStratSynthDisp::displayFRText( bool yn, bool isbrine )
{
    if ( !frtxtitm_ )
    {
	uiGraphicsScene& scene = vwr_->rgbCanvas().scene();
	const uiPoint pos( mNINT32( scene.nrPixX()/2 ),
			   mNINT32( scene.nrPixY()-10 ) );
	frtxtitm_ = scene.addItem(
				new uiTextItem(pos,uiString::empty(),
					       mAlignment(HCenter,VCenter)) );
	frtxtitm_->setPenColor( Color::Black() );
	frtxtitm_->setZValue( 999999 );
	frtxtitm_->setMovable( true );
    }

    frtxtitm_->setVisible( yn );
    if ( yn )
    {
	frtxtitm_->setText( isbrine ? tr("Brine filled")
				   : tr("Hydrocarbon filled") );
    }
}


void uiStratSynthDisp::updateTextPosCB( CallBacker* )
{
    if ( !frtxtitm_ )
	return;

    const uiGraphicsScene& scene = vwr_->rgbCanvas().scene();
    const uiPoint pos( mNINT32( scene.nrPixX()/2 ),
		       mNINT32( scene.nrPixY()-10 ) );
    frtxtitm_->setPos( pos );
}


void uiStratSynthDisp::drawLevel()
{
    vwr_->removeAuxDatas( levelaux_ );

    const float offset = prestackgrp_->isSensitive()
		       ? (float)offsetposfld_->getValue() : 0.0f;
    const auto& cursynth = currentwvasynthetic_ ? currentwvasynthetic_
						: currentvdsynthetic_;
    ObjectSet<const TimeDepthModel> curd2tmodels;
    getCurD2TModel( cursynth, curd2tmodels, offset );

    if ( !curd2tmodels.isEmpty() )
    {
	const bool canshowflatten = dispflattened_ && !flattenlvl_.isUndef();
	TypeSet<float> fltlvltimevals;
	if ( canshowflatten )
	    curSS().getLevelTimes( flattenlvl_, curd2tmodels, fltlvltimevals );

	const StratSynthLevelSet* lvls = curSS().getLevels();
	for( int lvlidx=0; lvlidx<lvls->size(); lvlidx++ )
	{
	    const StratSynthLevel* lvl = curSS().getLevel( lvlidx );
	    if ( !lvl )
		continue;
	    const Strat::Level stratlvl
				= Strat::LVLS().getByName( lvl->getName()) ;
	    if ( stratlvl.isUndef() )
		continue;
	    TypeSet<float> strattimevals;
	    curSS().getLevelTimes( stratlvl, curd2tmodels, strattimevals );
	    if ( strattimevals.isEmpty() )
		continue;

	    const bool issellvl = stratlvl.id() == flattenlvl_.id();
	    FlatView::AuxData* auxd = vwr_->createAuxData("Level markers");
	    auxd->linestyle_.type_ = OD::LineStyle::None;
	    for ( int imdl=0; imdl<strattimevals.size(); imdl++ )
	    {
		float tval = strattimevals[imdl];
		if ( canshowflatten )
		    tval -= fltlvltimevals[imdl];

		int mrkrsz = cMarkerSize;
		auto mrkrstyletype = OD::MarkerStyle2D::Target;
		if ( issellvl )
		    mrkrsz *= 2;

		auxd->markerstyles_ +=
			OD::MarkerStyle2D( mrkrstyletype, mrkrsz, lvl->col_ );
		auxd->poly_ += FlatView::Point( (imdl*dispeach_)+1, tval );
		auxd->zvalue_ = 3;
	    }

	    vwr_->addAuxData( auxd );
	    levelaux_ += auxd;
	}
    }

    vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::Auxdata) );
}


void uiStratSynthDisp::setCurrentWavelet()
{
    currentwvasynthetic_ = 0;
    curSS().setWavelet( wvltfld_->getWavelet() );
    RefMan<SyntheticData> wvasd = curSS().getSynthetic( wvadatalist_->text() );
    RefMan<SyntheticData> vdsd = curSS().getSynthetic( vddatalist_->text() );
    if ( !vdsd && !wvasd ) return;
    const BufferString wvasynthnm( wvasd ? wvasd->name().buf() : "" );
    const BufferString vdsynthnm( vdsd ? vdsd->name().buf() : "" );

    if ( wvasd )
    {
	wvasd->setWavelet( wvltfld_->getInput() );
	currentwvasynthetic_ = wvasd;
	if ( synthgendlg_ )
	    synthgendlg_->updateWaveletName();
	currentwvasynthetic_->fillGenParams( curSS().genParams() );
	wvltChanged.trigger();
	updateSynthetic( wvasynthnm, true );
    }

    if ( vdsynthnm == wvasynthnm )
    {
	setCurrentSynthetic( false );
	return;
    }

    mDynamicCastGet(const StratPropSyntheticData*,prsd,vdsd.ptr());
    if ( vdsd && !prsd )
    {
	vdsd->setWavelet( wvltfld_->getInput() );
	currentvdsynthetic_ = vdsd;
	if ( vdsynthnm != wvasynthnm )
	{
	    currentvdsynthetic_->fillGenParams( curSS().genParams() );
	    updateSynthetic( vdsynthnm, false );
	}
    }
}


void uiStratSynthDisp::wvltChg( CallBacker* )
{
    setCurrentWavelet();
    displaySynthetic( currentwvasynthetic_ );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::scalePush( CallBacker* )
{
    haveUserScaleWavelet();
}


bool uiStratSynthDisp::haveUserScaleWavelet()
{
    uiMsgMainWinSetter mws( mainwin() );

    if ( !currentwvasynthetic_ || currentwvasynthetic_->isPS() )
    {
	uiMSG().error( tr("Select a post-stack synthetic in wiggle view") );
	return false;
    }


    mDynamicCastGet(const PostStackSyntheticData*,pssd,
		    currentwvasynthetic_.ptr());
    const SeisTrcBuf& tbuf = pssd->postStackPack().trcBuf();
    if ( tbuf.isEmpty() )
    {
	uiMSG().error(tr("Synthetic seismic has no trace. "
		         "Please regenerate the synthetic."));
	return false;
    }

    const bool needknow2d3d = SI().has2D() && SI().has3D();
    uiListBox::Setup lbsu( OD::ChooseOnlyOne, uiStrings::sLevel(),
			    uiListBox::LeftMid );
    uiStratLevelSelDlg dlg( this, uiStrings::sLevel() );
    dlg.setID( flattenlvl_.id() );
    if ( !needknow2d3d )
	dlg.setTitleText( tr("Please select the stratigraphic level"
	    "\nalong which you want to compare real and synthetic amplitudes"));
    uiGenInput* use2dfld = 0;
    if ( needknow2d3d )
    {
	use2dfld = new uiGenInput( &dlg, tr("Type of seismic data to use"),
			BoolInpSpec(false,uiStrings::s2D(), uiStrings::s3D()) );
	use2dfld->attach( alignedBelow, dlg.box() );
    }
    if ( !dlg.go() )
	return false;

    const BufferString sellvlnm = dlg.getLevelName();
    bool use2d = SI().has2D();
    if ( use2dfld )
	use2d = use2dfld->getBoolValue();

    bool rv = false;
    PtrMan<SeisTrcBuf> scaletbuf = tbuf.clone();
    const Strat::Level sellvl = Strat::LVLS().getByName( sellvlnm );
    curSS().setLevelTimesInTrcs( sellvl, *scaletbuf,
				 currentwvasynthetic_->zerooffsd2tmodels_);
    uiSynthToRealScale srdlg( this, use2d, *scaletbuf, wvltfld_->key(true),
				sellvlnm );
    if ( srdlg.go() )
    {
	const DBKey wvltid( srdlg.selWvltID() );
	if ( wvltid.isInvalid() )
	    pErrMsg( "Huh" );
	else
	{
	    rv = true;
	    wvltfld_->setInput( wvltid );
	}
	vwr_->handleChange( mCast(unsigned int,FlatView::Viewer::All) );
    }
    return rv;
}


void uiStratSynthDisp::parsChangedCB( CallBacker* )
{
    if ( currentvdsynthetic_ )
    {
	SynthFVSpecificDispPars& disppars = currentvdsynthetic_->dispPars();
	disppars.colseqname_ = vwr_->appearance().ddpars_.vd_.colseqname_;
	*disppars.vdmapsetup_ = vwr_->appearance().ddpars_.vd_.mapper_->setup();
    }

    if ( currentwvasynthetic_ )
    {
	SynthFVSpecificDispPars& disppars = currentwvasynthetic_->dispPars();
	*disppars.wvamapsetup_
		= vwr_->appearance().ddpars_.wva_.mapper_->setup();
	disppars.overlap_ = vwr_->appearance().ddpars_.wva_.overlap_;
    }

    dispParsChanged.trigger();
}


void uiStratSynthDisp::viewChg( CallBacker* )
{
    updateRelativeViewRect();
    viewChanged.trigger();
}


void uiStratSynthDisp::setSnapLevelSensitive( bool yn )
{
    levelsnapselfld_->setSensitive( yn );
}


float uiStratSynthDisp::centralTrcShift() const
{
    if ( !dispflattened_ ) return 0.0;
    bool forward = false;
    int forwardidx = mNINT32( vwr_->curView().centre().x_ );
    int backwardidx = forwardidx-1;
    const SeisTrcBuf& trcbuf = curTrcBuf();
    if ( !trcbuf.size() ) return 0.0f;
    while ( true )
    {
	if ( backwardidx<0 || forwardidx>=trcbuf.size() )
	    return 0.0f;
	const int centrcidx = forward ? forwardidx : backwardidx;
	const SeisTrc* centtrc = trcbuf.size() ? trcbuf.get( centrcidx ) :  0;
	if ( centtrc && !mIsUdf(centtrc->info().pick_) )
	    return centtrc->info().pick_;
	forward ? forwardidx++ : backwardidx--;
	forward = !forward;
    }

    return 0.0f;
}


const uiWorldRect& uiStratSynthDisp::curView( bool indpth ) const
{
    mDefineStaticLocalObject( Threads::Lock, lock, (true) );
    Threads::Locker locker( lock );

    mDefineStaticLocalObject( uiWorldRect, timewr, );
    timewr = vwr_->curView();
    if ( !indpth )
	return timewr;

    mDefineStaticLocalObject( uiWorldRect, depthwr, );
    depthwr.setLeft( timewr.left() );
    depthwr.setRight( timewr.right() );
    ObjectSet<const TimeDepthModel> curd2tmodels;
    getCurD2TModel( currentwvasynthetic_, curd2tmodels, 0.0f );
    if ( !curd2tmodels.isEmpty() )
    {
	Interval<float> twtrg( mCast(float,timewr.top()),
			       mCast(float,timewr.bottom()) );
	Interval<double> depthrg( curd2tmodels[0]->getDepth(twtrg.start),
				  curd2tmodels[0]->getDepth(twtrg.stop) );
	for ( int idx=1; idx<curd2tmodels.size(); idx++ )
	{
	    const TimeDepthModel& d2t = *curd2tmodels[idx];
	    Interval<double> curdepthrg( d2t.getDepth(twtrg.start),
					 d2t.getDepth(twtrg.stop) );
	    if ( !curdepthrg.isUdf() )
		depthrg.include( curdepthrg );
	}

	if ( dispflattened_ )
	    depthrg.shift( SI().seismicReferenceDatum() );
	depthwr.setTop( depthrg.start );
	depthwr.setBottom( depthrg.stop );
    }

    return depthwr;
}


const SeisTrcBuf& uiStratSynthDisp::curTrcBuf() const
{
    ConstRefMan<SeisTrcBufDataPack> tbdp = vwr_->getPack( true, true );
    if ( !tbdp )
    {
	mDefineStaticLocalObject( SeisTrcBuf, emptybuf, (false) );
	return emptybuf;
    }

    if ( tbdp->trcBuf().isEmpty() )
	tbdp = vwr_->getPack( false, true );

    if ( !tbdp )
    {
	mDefineStaticLocalObject( SeisTrcBuf, emptybuf, (false) );
	return emptybuf;
    }
    return tbdp->trcBuf();
}


#define mErrRet(s,act) \
{ uiMsgMainWinSetter mws( mainwin() ); if (!s.isEmpty()) uiMSG().error(s); act;}

void uiStratSynthDisp::reDisplayPostStackSynthetic( bool wva )
{
    displayPostStackSynthetic( wva ? currentwvasynthetic_ : currentvdsynthetic_,
			       wva );
}


void uiStratSynthDisp::displaySynthetic( ConstRefMan<SyntheticData> sd )
{
    displayPostStackSynthetic( sd );
    displayPreStackSynthetic( sd );
}

void uiStratSynthDisp::getCurD2TModel( ConstRefMan<SyntheticData> sd,
		ObjectSet<const TimeDepthModel>& d2tmodels, float offset ) const
{
    if ( !sd )
	return;

    d2tmodels.erase();
    mDynamicCastGet(const PreStack::PreStackSyntheticData*,presd,sd.ptr());
    if ( !presd || presd->isNMOCorrected() || mIsZero(offset,mDefEps) )
    {
	d2tmodels = sd->zerooffsd2tmodels_;
	return;
    }

    StepInterval<float> offsetrg( presd->offsetRange() );
    offsetrg.step = presd->offsetRangeStep();
    int offsidx = offsetrg.getIndex( offset );
    if ( offsidx<0 )
    {
	d2tmodels = sd->zerooffsd2tmodels_;
	return;
    }

    const int nroffsets = offsetrg.nrSteps()+1;
    const SeisTrcBuf* tbuf = presd->getTrcBuf( offset );
    if ( !tbuf ) return;
    for ( int trcidx=0; trcidx<tbuf->size(); trcidx++ )
    {
	int d2tmodelidx = ( trcidx*nroffsets ) + offsidx;
	if ( !sd->d2tmodels_.validIdx(d2tmodelidx) )
	{
	    pErrMsg("Cannot find D2T Model for corresponding offset" );
	    d2tmodelidx = trcidx;
	}
	if ( !sd->d2tmodels_.validIdx(d2tmodelidx) )
	{
	    pErrMsg( "huh?" );
	    return;
	}
	d2tmodels += sd->d2tmodels_[d2tmodelidx];
    }
}


void uiStratSynthDisp::displayPostStackSynthetic( ConstRefMan<SyntheticData> sd,
						  bool wva )
{
    if ( !sd.ptr() )
	return;

    const bool hadpack = vwr_->hasPack( wva );
    if ( hadpack )
	vwr_->removePack( vwr_->packID(wva) );

    mDelD2TM

    mDynamicCastGet(const PreStack::PreStackSyntheticData*,presd,sd.ptr());
    mDynamicCastGet(const PostStackSyntheticData*,postsd,sd.ptr());

    const float offset =
	prestackgrp_->isSensitive() ? mCast( float, offsetposfld_->getValue() )
				    : 0.0f;
    const SeisTrcBuf* tbuf = presd ? presd->getTrcBuf( offset, 0 )
				   : &postsd->postStackPack().trcBuf();
    if ( !tbuf ) return;

    SeisTrcBuf* disptbuf = new SeisTrcBuf( true );
    tbuf->copyInto( *disptbuf );
    ObjectSet<const TimeDepthModel> curd2tmodels;
    getCurD2TModel( sd, curd2tmodels, offset );
    ObjectSet<const TimeDepthModel>* zerooffsd2tmodels =
	new ObjectSet<const TimeDepthModel>();
    getCurD2TModel( sd, *zerooffsd2tmodels, 0.0f );
    d2tmodels_ = zerooffsd2tmodels;
    float lasttime =  -mUdf(float);
    for ( int idx=0; idx<curd2tmodels.size(); idx++ )
    {
	if ( curd2tmodels[idx]->getLastTime() > lasttime )
	    longestaimdl_ = idx;
    }

    curSS().decimateTraces( *disptbuf, dispeach_ );
    reSampleTraces( sd, *disptbuf );
    if ( dispflattened_ )
    {
	curSS().setLevelTimesInTrcs( flattenlvl_, *disptbuf, curd2tmodels,
				     dispeach_ );
	curSS().flattenTraces( *disptbuf );
    }
    else
	curSS().trimTraces( *disptbuf, curd2tmodels, dispskipz_);


    SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( disptbuf, Seis::Line,
				    SeisTrcInfo::TrcNr, "Forward Modeling" );
    DPM( DataPackMgr::FlatID() ).add( dp );
    dp->setName( sd->name() );
    if ( !wva )
	vwr_->appearance().ddpars_.vd_.colseqname_ =
						    sd->dispPars().colseqname_;
    else
	vwr_->appearance().ddpars_.wva_.overlap_ = sd->dispPars().overlap_;
    ColTab::MapperSetup& mapsu =
	wva ? vwr_->appearance().ddpars_.wva_.mapper_->setup()
	    : vwr_->appearance().ddpars_.vd_.mapper_->setup();
    mDynamicCastGet(const StratPropSyntheticData*,prsd,sd.ptr());
    RefMan<SyntheticData> dispsd = const_cast< SyntheticData* > ( sd.ptr() );
    ColTab::MapperSetup& dispparsmapsu = !wva ? *dispsd->dispPars().vdmapsetup_
					   : *dispsd->dispPars().wvamapsetup_;
    const bool rgnotsaved = (mIsZero(dispparsmapsu.range().start,mDefEps) &&
			     mIsZero(dispparsmapsu.range().stop,mDefEps)) ||
			     dispparsmapsu.range().isUdf();
    if ( !rgnotsaved )
	mapsu = dispparsmapsu;
    else
    {
	mapsu.setNotFixed();
	const float cliprate = wva ? 0.0f : 0.025f;
	mapsu.setClipRate( ColTab::ClipRatePair(cliprate,cliprate) );
	BufferString colseqnm = vwr_->appearance().ddpars_.vd_.colseqname_;
	if ( sd->dispPars().colseqname_.isEmpty() )
	    dispsd->dispPars().colseqname_
		= colseqnm = ColTab::Sequence::sDefaultName( !prsd );
    }

    vwr_->setPack( wva, dp->id(), !hadpack );
    vwr_->setVisible( wva, true );
    control_->setD2TModels( *d2tmodels_ );
    NotifyStopper notstop( vwr_->viewChanged );
    if ( mIsZero(relzoomwr_.left(),1e-3) &&
	 mIsEqual(relzoomwr_.width(),1.0,1e-3) &&
	 mIsEqual(relzoomwr_.height(),1.0,1e-3) )
	vwr_->setViewToBoundingBox();
    else
	setRelativeViewRect( relzoomwr_ );

    if ( rgnotsaved )
    {
	mapsu.setFixedRange( dispparsmapsu.range() );
	dispparsmapsu = mapsu;
    }

    levelSnapChanged( 0 );
}


void uiStratSynthDisp::setSavedViewRect()
{
    if ( mIsUdf(savedzoomwr_.left()) )
	return;
    setAbsoluteViewRect( savedzoomwr_ );
    setZoomView( relzoomwr_ );
}


void uiStratSynthDisp::reSampleTraces( ConstRefMan<SyntheticData> sd,
				       SeisTrcBuf& tbuf ) const
{
    if ( longestaimdl_>=layerModel().size() || longestaimdl_<0 )
	return;
    Interval<float> depthrg = layerModel().sequence(longestaimdl_).zRange();
    const float offset =
	prestackgrp_->isSensitive() ? mCast( float, offsetposfld_->getValue() )
				    : 0.0f;
    ObjectSet<const TimeDepthModel> curd2tmodels;
    mDynamicCastGet(const StratPropSyntheticData*,spsd,sd.ptr());
    getCurD2TModel( sd, curd2tmodels, offset );
    if ( !curd2tmodels.validIdx(longestaimdl_) )
	return;
    const TimeDepthModel& d2t = *curd2tmodels[longestaimdl_];
    const float reqlastzval =
	d2t.getTime( layerModel().sequence(longestaimdl_).zRange().stop );
    for ( int idx=0; idx<tbuf.size(); idx++ )
    {
	SeisTrc& trc = *tbuf.get( idx );

	const float lastzval = trc.info().sampling_.atIndex( trc.size()-1 );
	const int lastsz = trc.size();
	if ( lastzval > reqlastzval )
	    continue;
	const int newsz = trc.info().sampling_.nearestIndex( reqlastzval );
	trc.reSize( newsz, true );
	if ( spsd )
	{
	    const float lastval = trc.get( lastsz-1, 0 );
	    for ( int xtrasampidx=lastsz; xtrasampidx<newsz; xtrasampidx++ )
		trc.set( xtrasampidx, lastval, 0 );
	}
    }
}


void uiStratSynthDisp::displayPreStackSynthetic( ConstRefMan<SyntheticData> sd )
{
    if ( !prestackwin_ ) return;

    if ( !sd ) return;
    mDynamicCastGet(const GatherSetDataPack*,gsetdp,&sd->getPack())
    mDynamicCastGet(const PreStack::PreStackSyntheticData*,presd,sd.ptr())
    if ( !gsetdp || !presd ) return;

    const GatherSetDataPack& angledp = presd->angleData();
    prestackwin_->removeGathers();
    TypeSet<PreStackView::GatherInfo> gatherinfos;
    const ObjectSet<Gather>& gathers = gsetdp->getGathers();
    const ObjectSet<Gather>& anglegathers = angledp.getGathers();
    for ( int idx=0; idx<gathers.size(); idx++ )
    {
	const Gather* gather = gathers[idx];
	const Gather* anglegather= anglegathers[idx];

	PreStackView::GatherInfo gatherinfo;
	gatherinfo.isstored_ = false;
	gatherinfo.gathernm_ = sd->name();
	gatherinfo.bid_ = gather->getBinID();
	gatherinfo.wvadpid_ = gather->id();
	gatherinfo.vddpid_ = anglegather->id();
	gatherinfo.isselected_ = true;
	gatherinfos += gatherinfo;
    }

    prestackwin_->setGathers( gatherinfos );
}


void uiStratSynthDisp::setPreStackMapper()
{
    for ( int idx=0; idx<prestackwin_->nrViewers(); idx++ )
    {
	uiFlatViewer& vwr = prestackwin_->viewer( idx );
	RefMan<ColTab::MapperSetup> newmapsu = new ColTab::MapperSetup(
				vwr.appearance().ddpars_.vd_.mapper_->setup() );
	newmapsu->setNotFixed();
	newmapsu->setNoClipping();
	vwr.appearance().ddpars_.vd_.mapper_->setup() = *newmapsu;
	vwr.appearance().ddpars_.vd_.colseqname_
				= ColTab::Sequence::sDefaultName();
	*newmapsu = vwr.appearance().ddpars_.wva_.mapper_->setup();
	newmapsu->setNoClipping();
	vwr.appearance().ddpars_.wva_.mapper_->setup() = *newmapsu;
	vwr.handleChange( FlatView::Viewer::DisplayPars );
    }
}


void uiStratSynthDisp::selPreStackDataCB( CallBacker* cb )
{
    BufferStringSet allgnms, selgnms;
    for ( int idx=0; idx<curSS().nrSynthetics(); idx++ )
    {
	ConstRefMan<SyntheticData> sd = curSS().getSyntheticByIdx( idx );
	mDynamicCastGet(const PreStack::PreStackSyntheticData*,presd,sd.ptr());
	if ( !presd ) continue;
	allgnms.addIfNew( sd->name() );
    }

    TypeSet<PreStackView::GatherInfo> ginfos = prestackwin_->gatherInfos();

    prestackwin_->getGatherNames( selgnms );
    for ( int idx=0; idx<selgnms.size(); idx++ )
    {
	const int gidx = allgnms.indexOf( selgnms[idx]->buf() );
	if ( gidx<0 )
	    continue;
	allgnms.removeSingle( gidx );
    }

    PreStackView::uiViewer2DSelDataDlg seldlg( prestackwin_, allgnms, selgnms );
    if ( seldlg.go() )
    {
	prestackwin_->removeGathers();
	TypeSet<PreStackView::GatherInfo> newginfos;
	for ( int synthidx=0; synthidx<selgnms.size(); synthidx++ )
	{
	    ConstRefMan<SyntheticData> sd =
		curSS().getSynthetic( selgnms[synthidx]->buf() );
	    if ( !sd ) continue;
	    mDynamicCastGet(const PreStack::PreStackSyntheticData*,presd,
			    sd.ptr());
	    mDynamicCastGet(const GatherSetDataPack*,gsetdp,
			    &sd->getPack())
	    if ( !gsetdp || !presd ) continue;
	    const GatherSetDataPack& angledp = presd->angleData();
	    for ( int idx=0; idx<ginfos.size(); idx++ )
	    {
		PreStackView::GatherInfo ginfo = ginfos[idx];
		ginfo.gathernm_ = sd->name();
		const Gather* gather = gsetdp->getGather( ginfo.bid_);
		const Gather* anglegather =
		    angledp.getGather( ginfo.bid_);
		ginfo.vddpid_ = anglegather->id();
		ginfo.wvadpid_ = gather->id();
		newginfos.addIfNew( ginfo );
	    }
	}

	prestackwin_->setGathers( newginfos, false );
    }

}


void uiStratSynthDisp::preStackWinClosedCB( CallBacker* )
{
    prestackwin_ = 0;
}


void uiStratSynthDisp::viewPreStackPush( CallBacker* cb )
{
    if ( !currentwvasynthetic_ || !currentwvasynthetic_->isPS() )
	return;
    if ( !prestackwin_ )
    {
	prestackwin_ =
	    new PreStackView::uiSyntheticViewer2DMainWin(this,"Prestack view");
	prestackwin_->seldatacalled_.notify(
		mCB(this,uiStratSynthDisp,selPreStackDataCB) );
	prestackwin_->windowClosed.notify(
		mCB(this,uiStratSynthDisp,preStackWinClosedCB) );
    }

    displayPreStackSynthetic( currentwvasynthetic_ );
    prestackwin_->show();
}


void uiStratSynthDisp::setCurrentSynthetic( bool wva )
{
    RefMan<SyntheticData> sd = curSS().getSynthetic( wva ? wvadatalist_->text()
						  : vddatalist_->text() );
    if ( wva )
	currentwvasynthetic_ = sd;
    else
	currentvdsynthetic_ = sd;
    RefMan<SyntheticData> cursynth = wva ? currentwvasynthetic_
					 : currentvdsynthetic_;

    if ( !cursynth ) return;

    NotifyStopper notstop( wvltfld_->selectionDone );
    if ( wva )
    {
	wvltfld_->setInputText( cursynth->waveletName() );
	curSS().setWavelet( wvltfld_->getWavelet() );
    }
}

void uiStratSynthDisp::updateFields()
{
    mDynamicCastGet(const PreStack::PreStackSyntheticData*,pssd,
		    currentwvasynthetic_.ptr());
    if ( pssd )
    {
	StepInterval<float> limits( pssd->offsetRange() );
	const float offsetstep = pssd->offsetRangeStep();
	limits.step = mIsUdf(offsetstep) ? 100 : offsetstep;
	offsetposfld_->setLimitSampling( limits );
    }

    prestackgrp_->setSensitive( pssd && pssd->hasOffset() );
    datagrp_->setSensitive( currentwvasynthetic_ );
    scalebut_->setSensitive( !pssd );
}


void uiStratSynthDisp::copySyntheticDispPars()
{
    for ( int sidx=0; sidx<curSS().nrSynthetics(); sidx++ )
    {
	RefMan<SyntheticData> cursd = curSS().getSyntheticByIdx( sidx );
	BufferString sdnm( cursd->name() );
	if ( useed_ )
	    sdnm.remove( StratSynth::sKeyFRNameSuffix() );
	else
	    sdnm += StratSynth::sKeyFRNameSuffix();

	ConstRefMan<SyntheticData> altsd = altSS().getSynthetic( sdnm );
	if ( !altsd ) continue;
	cursd->dispPars() = altsd->dispPars();
    }
}


void uiStratSynthDisp::showFRResults()
{
    const int wvacuritm = wvadatalist_->currentItem();
    const int vdcuritm = vddatalist_->currentItem();
    updateSyntheticList( true );
    updateSyntheticList( false );
    if ( wvadatalist_->size() <= 1 )
	return;
    copySyntheticDispPars();
    wvadatalist_->setCurrentItem( wvacuritm );
    vddatalist_->setCurrentItem( vdcuritm );
    setCurrentSynthetic( true );
    setCurrentSynthetic( false );
    displaySynthetic( currentwvasynthetic_ );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::doModelChange( CallBacker* )
{
    if ( !autoupdate_ && !forceupdate_ )
	return;

    if ( !curSS().errMsg().isEmpty() )
	mErrRet( curSS().errMsg(), return )

    uiUserShowWait usw( this, uiStrings::sUpdatingDisplay() );
    showInfoMsg( false );
    updateSyntheticList( true );
    updateSyntheticList( false );
    if ( wvadatalist_->size() <= 1 )
	return;
    wvadatalist_->setCurrentItem( 1 );
    vddatalist_->setCurrentItem( 1 );
    setCurrentSynthetic( true );
    setCurrentSynthetic( false );

    updateFields();
    displaySynthetic( currentwvasynthetic_ );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::updateSynthetic( const char* synthnm, bool wva )
{
    const BufferString curwvasdnm( currentwvasynthetic_ ?
			currentwvasynthetic_->name().buf() : "" );
    const BufferString curvdsdnm( currentvdsynthetic_ ?
			currentvdsynthetic_->name().buf() : "" );
    FixedString syntheticnm( synthnm );
    uiComboBox* datalist = wva ? wvadatalist_ : vddatalist_;
    if ( !datalist->isPresent(syntheticnm) || syntheticnm == sKeyNone() )
	return;
    if ( !curSS().removeSynthetic(syntheticnm) )
	return;

     if ( curwvasdnm==synthnm )
	 currentwvasynthetic_ = 0;
     if ( curvdsdnm==synthnm )
	 currentvdsynthetic_ = 0;
    mDelD2TM
    RefMan<SyntheticData> sd = curSS().addSynthetic();
    if ( !sd )
	mErrRet(curSS().errMsg(), return );

    showInfoMsg( false );

    if ( altSS().hasElasticModels() )
    {
	altSS().removeSynthetic( syntheticnm );
	altSS().genParams() = curSS().genParams();
	RefMan<SyntheticData> altsd = altSS().addSynthetic();
	if ( !altsd )
	    mErrRet(altSS().errMsg(), return );

	showInfoMsg( true );
    }

    updateSyntheticList( wva );
    synthsChanged.trigger();
    datalist->setCurrentItem( sd->name() );
    setCurrentSynthetic( wva );
}


void uiStratSynthDisp::syntheticChanged( CallBacker* cb )
{
    BufferString syntheticnm;
    if ( cb )
    {
	mCBCapsuleUnpack(BufferString,synthname,cb);
	syntheticnm = synthname;
    }
    else
	syntheticnm = wvadatalist_->text();

    const BufferString curvdsynthnm( currentvdsynthetic_ ?
			currentvdsynthetic_->name().buf() : "" );
    const BufferString curwvasynthnm( currentwvasynthetic_ ?
			currentwvasynthetic_->name().buf() : "" );
    RefMan<SyntheticData> cursd = curSS().getSynthetic( syntheticnm );
    if ( !cursd ) return;
    SynthGenParams curgp;
    cursd->fillGenParams( curgp );
    if ( !(curgp == curSS().genParams()) )
    {
	updateSynthetic( syntheticnm, true );
	updateSyntheticList( false );
    }
    else
    {
	wvadatalist_->setCurrentItem( syntheticnm );
	setCurrentSynthetic( true );
    }

    if ( curwvasynthnm == curvdsynthnm )
    {
	vddatalist_->setCurrentItem( currentwvasynthetic_->name() );
	setCurrentSynthetic( false );
    }

    updateFields();
    displaySynthetic( currentwvasynthetic_ );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::syntheticDisabled( CallBacker* cb )
{
    mCBCapsuleUnpack(BufferString,synthname,cb);
    curSS().disableSynthetic( synthname );
    altSS().disableSynthetic( synthname );
}


void uiStratSynthDisp::syntheticRemoved( CallBacker* cb )
{
    const BufferString curwvasdnm( currentwvasynthetic_ ?
			currentwvasynthetic_->name().buf() : "" );
    const BufferString curvdsdnm( currentvdsynthetic_ ?
			currentvdsynthetic_->name().buf() : "" );

    mCBCapsuleUnpack(BufferString,synthname,cb);
    if ( !curSS().removeSynthetic(synthname) )
	return;

    if ( curwvasdnm==synthname )
	currentwvasynthetic_ = 0;
    if ( curvdsdnm==synthname )
	currentvdsynthetic_ = 0;
    mDelD2TM
    altSS().removeSynthetic( synthname );
    synthsChanged.trigger();
    updateSyntheticList( true );
    updateSyntheticList( false );
    if ( wvadatalist_->size() <= 1 )
	return;
    wvadatalist_->setCurrentItem( 1 );
    vddatalist_->setCurrentItem( 1 );
    setCurrentSynthetic( true );
    setCurrentSynthetic( false );
    displayPostStackSynthetic( currentwvasynthetic_, true );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::addEditSynth( CallBacker* )
{
    if ( !synthgendlg_ )
    {
	synthgendlg_ = new uiSynthGenDlg( this, curSS());
	synthgendlg_->synthRemoved.notify(
		mCB(this,uiStratSynthDisp,syntheticRemoved) );
	synthgendlg_->synthDisabled.notify(
		mCB(this,uiStratSynthDisp,syntheticDisabled) );
	synthgendlg_->synthChanged.notify(
		mCB(this,uiStratSynthDisp,syntheticChanged) );
	synthgendlg_->genNewReq.notify(
			    mCB(this,uiStratSynthDisp,genNewSynthetic) );
    }

    synthgendlg_->go();
}


void uiStratSynthDisp::exportSynth( CallBacker* )
{
    if ( layerModel().isEmpty() )
	mErrRet( tr("No valid layer model present"), return )
    uiStratSynthExport dlg( this, curSS() );
    dlg.go();
}


void uiStratSynthDisp::offsetChged( CallBacker* )
{
    displayPostStackSynthetic( currentwvasynthetic_, true );
    if ( FixedString(wvadatalist_->text())==vddatalist_->text() &&
	 currentwvasynthetic_ && currentwvasynthetic_->isPS() )
	displayPostStackSynthetic( currentvdsynthetic_, false );
}


const PropertyRefSelection& uiStratSynthDisp::modelPropertyRefs() const
{ return layerModel().propertyRefs(); }


const ObjectSet<const TimeDepthModel>* uiStratSynthDisp::d2TModels() const
{ return d2tmodels_; }


void uiStratSynthDisp::vdDataSetSel( CallBacker* )
{
    setCurrentSynthetic( false );
    displayPostStackSynthetic( currentvdsynthetic_, false );
}


void uiStratSynthDisp::wvDataSetSel( CallBacker* )
{
    setCurrentSynthetic( true );
    updateFields();
    displayPostStackSynthetic( currentwvasynthetic_, true );
}


const ObjectSet<SyntheticData>& uiStratSynthDisp::getSynthetics() const
{
    return curSS().synthetics();
}


const Wavelet* uiStratSynthDisp::getWavelet() const
{
    return curSS().wavelet();
}


DBKey uiStratSynthDisp::waveletID() const
{
    return wvltfld_->key( true );
}


void uiStratSynthDisp::genNewSynthetic( CallBacker* )
{
    if ( !synthgendlg_ )
	return;

    uiUserShowWait usw( this, tr("Generating New Synthetics") );
    RefMan<SyntheticData> sd = curSS().addSynthetic();
    if ( !sd )
	mErrRet(curSS().errMsg(), return )

    showInfoMsg( false );
    if ( altSS().hasElasticModels() )
    {
	altSS().genParams() = curSS().genParams();
	RefMan<SyntheticData> altsd = altSS().addSynthetic();
	if ( !altsd )
	    mErrRet(altSS().errMsg(), return )

	showInfoMsg( true );
    }
    updateSyntheticList( true );
    updateSyntheticList( false );
    synthsChanged.trigger();
    synthgendlg_->putToScreen();
    synthgendlg_->updateSynthNames();
}


void uiStratSynthDisp::showInfoMsg( bool foralt )
{
    StratSynth& ss = foralt ? altSS() : curSS();
    if ( !ss.infoMsg().isEmpty() )
    {
	uiMsgMainWinSetter mws( mainwin() );
	uiMSG().warning( ss.infoMsg() );
	ss.clearInfoMsg();
    }
}


RefMan<SyntheticData> uiStratSynthDisp::getSyntheticData( const char* nm )
{
    return curSS().getSynthetic( nm );
}


RefMan<SyntheticData> uiStratSynthDisp::getCurrentSyntheticData(bool wva ) const
{
    return wva ? currentwvasynthetic_ : currentvdsynthetic_;
}


void uiStratSynthDisp::fillPar( IOPar& par, const StratSynth* stratsynth ) const
{
    IOPar stratsynthpar;
    stratsynthpar.set( sKeySnapLevel(), levelsnapselfld_->currentItem());
    int nr_nonproprefsynths = 0;
    for ( int idx=0; idx<stratsynth->nrSynthetics(); idx++ )
    {
	ConstRefMan<SyntheticData> sd = stratsynth->getSyntheticByIdx( idx );
	if ( !sd ) continue;
	mDynamicCastGet(const StratPropSyntheticData*,prsd,sd.ptr());
	if ( prsd ) continue;
	nr_nonproprefsynths++;
	SynthGenParams genparams;
	sd->fillGenParams( genparams );
	IOPar synthpar;
	genparams.fillPar( synthpar );
	sd->fillDispPar( synthpar );
	stratsynthpar.mergeComp( synthpar, IOPar::compKey(sKeySyntheticNr(),
				 nr_nonproprefsynths-1) );
    }

    savedzoomwr_ = curView( false );
    TypeSet<double> startviewareapts;
    startviewareapts.setSize( 4 );
    startviewareapts[0] = savedzoomwr_.left();
    startviewareapts[1] = savedzoomwr_.top();
    startviewareapts[2] = savedzoomwr_.right();
    startviewareapts[3] = savedzoomwr_.bottom();
    stratsynthpar.set( sKeyViewArea(), startviewareapts );
    stratsynthpar.set( sKeyNrSynthetics(), nr_nonproprefsynths );
    par.removeWithKey( sKeySynthetics() );
    par.mergeComp( stratsynthpar, sKeySynthetics() );
}


void uiStratSynthDisp::fillPar( IOPar& par, bool useed ) const
{
    fillPar( par, useed ? edstratsynth_ : stratsynth_ );
}


void uiStratSynthDisp::fillPar( IOPar& par ) const
{
    fillPar( par, &curSS() );
}


bool uiStratSynthDisp::prepareElasticModel()
{
    if ( !forceupdate_ && !autoupdate_ )
	return false;
    return curSS().createElasticModels();
}


bool uiStratSynthDisp::usePar( const IOPar& par )
{
    PtrMan<IOPar> stratsynthpar = par.subselect( sKeySynthetics() );
    if ( !curSS().hasElasticModels() )
	return false;
    currentwvasynthetic_ = 0;
    currentvdsynthetic_ = 0;
    curSS().clearSynthetics();
    mDelD2TM
    par.get( sKeyDecimation(), dispeach_);
    int nrsynths = 0;
    if ( stratsynthpar )
    {
	stratsynthpar->get( sKeyNrSynthetics(), nrsynths );
	currentvdsynthetic_ = 0;
	currentwvasynthetic_ = 0;
	for ( int idx=0; idx<nrsynths; idx++ )
	{
	    PtrMan<IOPar> synthpar =
		stratsynthpar->subselect(IOPar::compKey(sKeySyntheticNr(),idx));
	    if ( !synthpar ) continue;
	    SynthGenParams genparams;
	    genparams.usePar( *synthpar );
	    wvltfld_->setInputText( genparams.wvltnm_ );
	    curSS().setWavelet( wvltfld_->getWavelet() );
	    RefMan<SyntheticData> sd = curSS().addSynthetic( genparams );
	    if ( !sd )
	    {
		mErrRet(curSS().errMsg(),);
		continue;
	    }

	    showInfoMsg( false );

	    if ( useed_ )
	    {
		RefMan<SyntheticData> nonfrsd =
					stratsynth_->getSyntheticByIdx( idx );
		IOPar synthdisppar;
		if ( nonfrsd )
		    nonfrsd->fillDispPar( synthdisppar );
		sd->useDispPar( synthdisppar );
		continue;
	    }

	    sd->useDispPar( *synthpar );
	}
    }

    if ( !nrsynths )
    {
	if ( curSS().addDefaultSynthetic() ) //par file not ok, add default
	    synthsChanged.trigger(); //update synthetic WorkBenchPar
    }

    if ( !curSS().nrSynthetics() )
    {
	displaySynthetic( 0 );
	displayPostStackSynthetic( 0, false );
	return false;
    }

    if ( GetEnvVarYN("DTECT_STRAT_MAKE_PROPERTYTRACES",true) )
	curSS().generateOtherQuantities();

    if ( useed_ && GetEnvVarYN("USE_FR_DIFF",false) )
	setDiffData();

    if ( stratsynthpar )
    {
	int snaplvl = 0;
	stratsynthpar->get( sKeySnapLevel(), snaplvl );
	levelsnapselfld_->setCurrentItem( snaplvl );
	TypeSet<double> startviewareapts;
	if ( stratsynthpar->get(sKeyViewArea(),startviewareapts) &&
	     startviewareapts.size() == 4 )
	{
	    savedzoomwr_.setLeft( startviewareapts[0] );
	    savedzoomwr_.setTop( startviewareapts[1] );
	    savedzoomwr_.setRight( startviewareapts[2] );
	    savedzoomwr_.setBottom( startviewareapts[3] );
	}
    }

    return true;
}


void uiStratSynthDisp::setDiffData()
{
    for ( int idx=0; idx<curSS().nrSynthetics(); idx++ )
    {
	RefMan<SyntheticData> frsd = curSS().getSyntheticByIdx( idx );
	ConstRefMan<SyntheticData> sd = altSS().getSyntheticByIdx( idx );
	if ( !frsd || !sd ) continue;
	if ( !sd->isPS() )
	{
	    mDynamicCastGet(PostStackSyntheticData*,frpostsd,frsd.ptr())
	    mDynamicCastGet(const PostStackSyntheticData*,postsd,sd.ptr())
	    SeisTrcBuf& frsdbuf = frpostsd->postStackPack().trcBuf();
	    const SeisTrcBuf& sdbuf = postsd->postStackPack().trcBuf();
	    for ( int itrc=0; itrc<frsdbuf.size(); itrc++ )
	    {
		SeisTrc* frtrc = frsdbuf.get( itrc );
		const SeisTrc* trc = sdbuf.get( itrc );
		for ( int isamp=0; isamp<frtrc->size(); isamp++ )
		{
		    const float z = trc->samplePos( isamp );
		    const float trcval = trc->getValue( z, 0 );
		    const float frtrcval = frtrc->getValue( z, 0 );
		    frtrc->set( isamp, frtrcval - trcval , 0 );
		}
	    }
	}
	else
	{
	    mDynamicCastGet(PreStack::PreStackSyntheticData*,frpresd,frsd.ptr())
	    mDynamicCastGet(const PreStack::PreStackSyntheticData*,presd,
			    sd.ptr())
	    GatherSetDataPack& frgdp =frpresd->preStackPack();
	    ObjectSet<Gather>& frgathers = frgdp.getGathers();
	    StepInterval<float> offrg( frpresd->offsetRange() );
	    offrg.step = frpresd->offsetRangeStep();
	    const GatherSetDataPack& gdp = presd->preStackPack();
	    for ( int igather=0; igather<frgathers.size(); igather++ )
	    {
		for ( int offsidx=0; offsidx<offrg.nrSteps(); offsidx++ )
		{
		    SeisTrc* frtrc = frgdp.getTrace( igather, offsidx );
		    const SeisTrc* trc = gdp.getTrace( igather, offsidx );
		    for ( int isamp=0; isamp<frtrc->size(); isamp++ )
		    {
			const float trcval = trc->get( isamp, 0 );
			const float frtrcval = frtrc->get( isamp, 0 );
			frtrc->set( isamp, frtrcval-trcval, 0 );
		    }
		}
	    }
	}
    }
}


uiGroup* uiStratSynthDisp::getDisplayClone( uiParent* p ) const
{
    uiFlatViewer* vwr = new uiFlatViewer( p );
    vwr->rgbCanvas().disableImageSave();
    vwr->setInitialSize( uiSize(800,300) );
    vwr->setStretch( 2, 2 );
    vwr->appearance() = vwr_->appearance();
    vwr->setPack( true, vwr_->packID(true), false );
    vwr->setPack( false, vwr_->packID(false), false );
    return vwr;
}


void uiStratSynthDisp::setDefaultAppearance( FlatView::Appearance& app )
{
    app.setGeoDefaults( true );
    app.setDarkBG( false );
    app.annot_.allowuserchangereversedaxis_ = false;
    app.annot_.title_.setEmpty();
    app.annot_.x1_.showAll( true );
    app.annot_.x2_.showAll( true );
    app.annot_.x1_.annotinint_ = true;
    app.annot_.x2_.name_ = uiStrings::sTWT();
    app.ddpars_.show( true, true );
    app.ddpars_.wva_.allowuserchangedata_ = false;
    app.ddpars_.vd_.allowuserchangedata_ = false;
}


uiSynthSlicePos::uiSynthSlicePos( uiParent* p, const uiString& lbltxt )
    : uiGroup( p, "Slice Pos" )
    , positionChg(this)
{
    label_ = new uiLabel( this, lbltxt );
    sliceposbox_ = new uiSpinBox( this, 0, "Slice position" );
    sliceposbox_->valueChanging.notify( mCB(this,uiSynthSlicePos,slicePosChg));
    sliceposbox_->valueChanged.notify( mCB(this,uiSynthSlicePos,slicePosChg));
    sliceposbox_->attach( rightOf, label_ );

    uiLabel* steplabel = new uiLabel( this, uiStrings::sStep() );
    steplabel->attach( rightOf, sliceposbox_ );

    slicestepbox_ = new uiSpinBox( this, 0, "Slice step" );
    slicestepbox_->attach( rightOf, steplabel );

    prevbut_ = new uiToolButton( this, "prevpos", tr("Previous position"),
				mCB(this,uiSynthSlicePos,prevCB) );
    prevbut_->attach( rightOf, slicestepbox_ );
    nextbut_ = new uiToolButton( this, "nextpos", tr("Next position"),
				 mCB(this,uiSynthSlicePos,nextCB) );
    nextbut_->attach( rightOf, prevbut_ );
}


void uiSynthSlicePos::slicePosChg( CallBacker* )
{
    positionChg.trigger();
}


void uiSynthSlicePos::prevCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    posbox->setValue( posbox->getIntValue()-stepbox->getIntValue() );
}


void uiSynthSlicePos::nextCB( CallBacker* )
{
    uiSpinBox* posbox = sliceposbox_;
    uiSpinBox* stepbox = slicestepbox_;
    posbox->setValue( posbox->getIntValue()+stepbox->getIntValue() );
}


void uiSynthSlicePos::setLimitSampling( StepInterval<float> lms )
{
    limitsampling_ = lms;
    sliceposbox_->setInterval( lms.start, lms.stop );
    sliceposbox_->setStep( lms.step );
    slicestepbox_->setValue( lms.step );
    slicestepbox_->setStep( lms.step );
    slicestepbox_->setMinValue( lms.step );
}


void uiSynthSlicePos::setValue( int val ) const
{
    sliceposbox_->setValue( val );
}


int uiSynthSlicePos::getValue() const
{
    return sliceposbox_->getIntValue();
}
