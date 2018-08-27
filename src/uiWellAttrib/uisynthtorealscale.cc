/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Feb 2010
-*/


#include "uisynthtorealscale.h"

#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "survinfo.h"
#include "polygon.h"
#include "position.h"
#include "seistrc.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seisbuf.h"
#include "seisprovider.h"
#include "seisselectionimpl.h"
#include "stratlevel.h"
#include "statparallelcalc.h"
#include "picksettr.h"
#include "waveletmanager.h"
#include "dbman.h"

#include "uislider.h"
#include "uistratseisevent.h"
#include "uiseissel.h"
#include "uipicksetsel.h"
#include "uiwaveletsel.h"
#include "uiseparator.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uihistogramdisplay.h"
#include "uiaxishandler.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitaskrunnerprovider.h"
#include "od_helpids.h"

#define mErrRetBool(s)\
{ uiMSG().error(s); return false; }\

#define mErrRet(s)\
{ uiMSG().error(s); return; }\

class uiSynthToRealScaleStatsDisp : public uiGroup
{ mODTextTranslationClass(uiSynthToRealScaleStatsDisp);
public:

uiSynthToRealScaleStatsDisp( uiParent* p, const char* nm, bool left )
    : uiGroup(p,nm)
    , usrval_(mUdf(float))
    , usrValChanged(this)
{
    uiFunctionDisplay::Setup su;
    su.annoty( false ).noyaxis( true ).noy2axis( true ).drawgridlines( false );
    dispfld_ = new uiHistogramDisplay( this, su );
    dispfld_->xAxis()->setCaption( uiString::empty() );
    dispfld_->setPrefWidth( 260 );
    dispfld_->setPrefHeight( GetGoldenMinor(260) );

    uiSlider::Setup slsu;
    slsu.withedit( true );
    valueslider_ = new uiSlider( this, slsu, "Value" );
    valueslider_->valueChanged.notify(
	    mCB(this,uiSynthToRealScaleStatsDisp,sliderChgCB) );
    valueslider_->attach( alignedBelow, dispfld_ );
    valueslider_->setStretch( 2, 1 );

    uiLabel* lbl = new uiLabel( this, toUiString(nm) );
    dispfld_->attach( centeredBelow, lbl );
    setHAlignObj( dispfld_ );
}

void setValue( float val )
{
    const uiAxisHandler* xaxis = dispfld_->xAxis();
    const StepInterval<float> xrg = xaxis->range();
    valueslider_->setScale( xrg.step * 0.001f, 0 );
    valueslider_->setInterval( xrg );
    valueslider_->setValue( val );
    setFromSlider();
}

void setFromSlider()
{
    usrval_ = valueslider_->getFValue();
    dispfld_->setMarkValue( usrval_, true );
}

void sliderChgCB( CallBacker* )
{
    setFromSlider();
    usrValChanged.trigger();
}

    float		usrval_;

    uiHistogramDisplay*	dispfld_;
    uiSlider*		valueslider_;
    Notifier<uiSynthToRealScaleStatsDisp>	usrValChanged;

};


uiSynthToRealScale::uiSynthToRealScale( uiParent* p, bool is2d,
					const SeisTrcBuf& tb,
					const DBKey& wid, const char* lvlnm )
    : uiDialog(p,Setup(tr("Scale synthetics"),
		       tr("Determine scaling for synthetics"),
			mODHelpKey(mSynthToRealScaleHelpID) ))
    , seisev_(*new Strat::SeisEvent)
    , is2d_(is2d)
    , synth_(tb)
    , inpwvltid_(wid)
    , seisfld_(0)
    , horizon_(0)
    , horiter_(0)
    , polygon_(0)
{
#define mNoDealRet(cond,msg) \
    if ( cond ) \
	{ new uiLabel( this, msg ); return; }
    mNoDealRet( Strat::LVLS().isEmpty(), tr("No Stratigraphic Levels defined") )
    mNoDealRet( tb.isEmpty(), tr("Generate models first") )
    mNoDealRet( inpwvltid_.isInvalid(), uiStrings::phrCreate(
							tr("a Wavelet first")) )
    mNoDealRet( !lvlnm || !*lvlnm || (*lvlnm == '-' && *(lvlnm+1) == '-'),
				   uiStrings::phrSelect(tr("Stratigraphic Level"
				   "\nbefore starting this tool")) )

    uiString wintitle = tr("Determine scaling for synthetics using '%1'")
				    .arg(toUiString(DBM().nameOf(inpwvltid_)));
    setTitleText( wintitle );

    uiSeisSel::Setup sssu( is2d_, false );
    seisfld_ = new uiSeisSel( this, uiSeisSel::ioContext(sssu.geom_,true),
			      sssu );

    const IOObjContext horctxt( is2d_ ? mIOObjContext(EMHorizon2D)
				      : mIOObjContext(EMHorizon3D) );
    uiIOObjSel::Setup horsu( tr("Horizon for '%1'").arg(lvlnm));
    horfld_ = new uiIOObjSel( this, horctxt, horsu );
    horfld_->attach( alignedBelow, seisfld_ );

    uiIOObjSel::Setup polysu( tr("Within Polygon") ); polysu.optional( true );
    polyfld_ = new uiPickSetIOObjSel( this, polysu, true,
				      uiPickSetIOObjSel::PolygonOnly );
    polyfld_->attach( alignedBelow, horfld_ );

    uiStratSeisEvent::Setup ssesu( true );
    ssesu.fixedlevelid( Strat::LVLS().getIDByName(lvlnm) );
    evfld_ = new uiStratSeisEvent( this, ssesu );
    evfld_->attach( alignedBelow, polyfld_ );

    uiPushButton* gobut = new uiPushButton( this, tr("Extract amplitudes"),
				mCB(this,uiSynthToRealScale,goPush), true );
    gobut->attach( alignedBelow, evfld_ );

    uiSeparator* sep = new uiSeparator( this, "separator" );
    sep->attach( stretchedBelow, gobut );

    valislbl_ = new uiLabel( this, tr("       [Amplitude values]       ") );
    valislbl_->setAlignment( OD::Alignment::HCenter );
    valislbl_->attach( centeredBelow, sep );

    uiGroup* statsgrp = new uiGroup( this, "Stats displays" );

    synthstatsfld_ = new uiSynthToRealScaleStatsDisp( statsgrp, "Synthetics",
						      true );
    realstatsfld_ = new uiSynthToRealScaleStatsDisp( statsgrp, "Real Seismics",
						     false );
    realstatsfld_->attach( rightOf, synthstatsfld_ );
    const CallBack setsclcb( mCB(this,uiSynthToRealScale,setScaleFld) );
    synthstatsfld_->usrValChanged.notify( setsclcb );
    statsgrp->attach( centeredBelow, valislbl_ );
    realstatsfld_->usrValChanged.notify( setsclcb );
    statsgrp->setHAlignObj( realstatsfld_ );

    finalscalefld_ = new uiGenInput( this, uiString::empty(),
                                     FloatInpSpec() );
    finalscalefld_->attach( centeredBelow, statsgrp );
    new uiLabel( this, tr("Scaling factor"), finalscalefld_ );

    uiWaveletIOObjSel::Setup wvsu( tr("Save scaled Wavelet as") );
    wvltfld_ = new uiWaveletIOObjSel( this, wvsu, false );
    wvltfld_->attach( alignedBelow, finalscalefld_ );

    postFinalise().notify( mCB(this,uiSynthToRealScale,initWin) );
}


uiSynthToRealScale::~uiSynthToRealScale()
{
    delete horiter_;
    if ( horizon_ )
	horizon_->unRef();
    delete polygon_;
    delete &seisev_;
}


void uiSynthToRealScale::initWin( CallBacker* )
{
    updSynthStats();
}

#define mSetFldValue( fld, val ) \
     if ( !mIsUdf(val) ) \
	fld->setValue( (float)val ); \

void uiSynthToRealScale::setScaleFld( CallBacker* )
{
    const float synthval = synthstatsfld_->usrval_;
    const float realval = realstatsfld_->usrval_;
    if ( mIsUdf(synthval) || mIsUdf(realval) || synthval == 0 )
	finalscalefld_->setValue( mUdf(float) );
    else
	finalscalefld_->setValue( realval / synthval );

    mSetFldValue( synthstatsfld_, synthval );
    mSetFldValue( realstatsfld_, realval );
}


bool uiSynthToRealScale::getEvent()
{
    if ( !evfld_->getFromScreen() )
	return false;
    seisev_ = evfld_->event();
    const bool isrms = evfld_->getFullExtrWin().nrSteps() > 0;
    valislbl_->setText( (isrms ? tr("Amplitude RMS values")
			       : tr("Amplitude values")).optional() );
    return true;
}


bool uiSynthToRealScale::getHorData( TaskRunnerProvider& trprov )
{
    delete polygon_; polygon_ = 0;
    if ( horizon_ )
	{ horizon_->unRef(); horizon_ = 0; }

    if ( polyfld_->isChecked() )
    {
	if ( !polyfld_->ioobj() )
	    return false;
	polygon_ = polyfld_->getSelectionPolygon();
	if ( !polygon_ )
	    mErrRetBool( tr("Selection Polygon is empty") )
    }

    const IOObj* ioobj = horfld_->ioobj();
    if ( !ioobj )
	return false;
    EM::Object* emobj = EM::MGR().loadIfNotFullyLoaded( ioobj->key(),
							  trprov );
    mDynamicCastGet(EM::Horizon*,hor,emobj);
    if ( !hor ) return false;
    horizon_ = hor;
    horizon_->ref();
    horiter_ = horizon_->createIterator();
    return true;
}


float uiSynthToRealScale::getTrcValue( const SeisTrc& trc, float reftm ) const
{
    const int lastsamp = trc.size() - 1;
    const Interval<float> trg( trc.startPos(), trc.samplePos(lastsamp) );
    const StepInterval<float> extrwin( evfld_->getFullExtrWin() );
    const int nrtms = extrwin.nrSteps() + 1;
    const bool calcrms = nrtms > 1;
    float sumsq = 0;
    for ( int itm=0; itm<nrtms; itm++ )
    {
	float extrtm = reftm + extrwin.atIndex( itm );
	float val;
	if ( extrtm <= trg.start )
	    val = trc.get( 0, 0 );
	if ( extrtm >= trg.stop )
	    val = trc.get( lastsamp, 0 );
	else
	    val = trc.getValue( extrtm, 0 );
	if ( calcrms ) val *= val;
	sumsq += val;
    }
    return calcrms ? Math::Sqrt( sumsq / nrtms ) : sumsq;
}


void uiSynthToRealScale::updSynthStats()
{
    if ( !getEvent() )
	return;

    TypeSet<float> vals;
    for ( int idx=0; idx<synth_.size(); idx++ )
    {
	const SeisTrc& trc = *synth_.get( idx );
	const float reftm = seisev_.snappedTime( trc );
	if ( !mIsUdf(reftm) )
	    vals += getTrcValue( trc, reftm );
    }

    uiHistogramDisplay& histfld = *synthstatsfld_->dispfld_;
    histfld.setData( vals.arr(), vals.size() );
    histfld.putN();
    mSetFldValue( synthstatsfld_, histfld.getStatCalc().average() );
}


class uiSynthToRealScaleRealStatCollector : public Executor
{ mODTextTranslationClass(uiSynthToRealScaleRealStatCollector);
public:
uiSynthToRealScaleRealStatCollector(
	uiSynthToRealScale& d, Seis::Provider& prov )
    : Executor( "Collect Amplitudes" )
    , dlg_(d)
    , prov_(prov)
    , msg_(uiStrings::sCollectingData())
    , nrdone_(0)
    , totalnr_(-1)
    , seldata_(0)
{
    if ( dlg_.polygon_ )
	seldata_ = new Seis::PolySelData( *dlg_.polygon_ );
    if ( seldata_ )
	totalnr_ = seldata_->expectedNrTraces( dlg_.is2d_ );
    else
	totalnr_ = dlg_.horiter_->approximateSize();
}

~uiSynthToRealScaleRealStatCollector()
{
    delete seldata_;
}

uiString message() const	{ return msg_; }
uiString nrDoneText() const	{ return tr("Traces handled"); }
od_int64 nrDone() const		{ return nrdone_; }
od_int64 totalNr() const	{ return totalnr_; }

bool getNextPos3D()
{
    while ( true )
    {
	const EM::PosID posid = dlg_.horiter_->next();
	if ( posid.isInvalid() )
	    return false;
	const Coord3 crd = dlg_.horizon_->getPos( posid );
	if ( setBinID(crd.getXY()) )
	{
	    z_ = (float)crd.z_;
	    break;
	}
    }

    return true;
}

bool setBinID( const Coord& crd )
{
    bid_ = SI().transform( crd );
    return seldata_ ? seldata_->isOK(bid_) : true;
}


int getTrc3D()
{
    while ( true )
    {
	if ( !getNextPos3D() )
	    return Finished();

	if ( !prov_.isPresent(TrcKey(bid_)) )
	    continue;

	const uiRetVal uirv = prov_.get( TrcKey(bid_), trc_ );
	if ( !uirv.isOK() )
	    { msg_ = uirv; return ErrorOccurred(); }

	break;
    }
    return MoreToDo();
}

int getTrc2D()
{
    const uiRetVal uirv = prov_.get( TrcKey(bid_), trc_ );
    if ( !uirv.isOK() )
	{ msg_ = uirv; return ErrorOccurred(); }

    if ( !setBinID(trc_.info().coord_) )
	return MoreToDo();

    mDynamicCastGet(const EM::Horizon2D*,hor2d,dlg_.horizon_)
    if ( !hor2d )
	return ErrorOccurred();
    TrcKey tk( prov_.curGeomID(), trc_.info().trcNr() );
    EM::PosID pid = hor2d->geometry().getPosID( tk );
    const Coord3 crd = dlg_.horizon_->getPos( pid );
    if ( mIsUdf(crd.z_) )
	return MoreToDo();

    z_ = (float)crd.z_;
    return MoreToDo();
}

int nextStep()
{
    const int res = dlg_.is2d_ ? getTrc2D() : getTrc3D();
    if ( res <= 0 )
	return res;
    nrdone_++;
    if ( res > 1 )
	return MoreToDo();

    const float val = dlg_.getTrcValue( trc_, z_ );
    if ( !mIsUdf(val) )
	vals_ += val;
    return MoreToDo();
}

    uiSynthToRealScale&	dlg_;
    Seis::Provider&	prov_;
    Seis::SelData*	seldata_;
    SeisTrc		trc_;
    uiString		msg_;
    od_int64		nrdone_;
    od_int64		totalnr_;
    BinID		bid_;
    float		z_;
    TypeSet<float>	vals_;

};


void uiSynthToRealScale::updRealStats()
{
    if ( !getEvent() )
	return;

    uiTaskRunnerProvider trprov( this );
    if ( !getHorData(trprov) )
	return;

    if ( is2d_ )
    {
	EM::IOObjInfo eminfo( horfld_->ioobj() );
	SeisIOObjInfo seisinfo( seisfld_->ioobj() );
	BufferStringSet seislnms, horlnms;
	seisinfo.getLineNames( seislnms );
	eminfo.getLineNames( horlnms );
	bool selectionvalid = false;
	for ( int lidx=0; lidx<seislnms.size(); lidx++ )
	{
	    if ( horlnms.isPresent(seislnms.get(lidx)) )
	    {
		selectionvalid = true;
		break;
	    }
	}

	if ( !selectionvalid )
	    mErrRet(tr("No common line names found in horizon & seismic data"))
    }

    uiRetVal uirv;
    PtrMan<Seis::Provider> prov = Seis::Provider::create(
					seisfld_->ioobj()->key(), &uirv );
    if ( !prov )
	mErrRet( uirv );

    uiSynthToRealScaleRealStatCollector coll( *this, *prov );
    if ( !trprov.execute(coll) )
	return;

    uiHistogramDisplay& histfld = *realstatsfld_->dispfld_;
    histfld.setData( coll.vals_.arr(), coll.vals_.size() );
    histfld.putN();
    mSetFldValue( realstatsfld_, histfld.getStatCalc().average() );
    setScaleFld( 0 );
}


bool uiSynthToRealScale::acceptOK()
{
    if ( !evfld_->getFromScreen() )
	return false;

    const float scalefac = finalscalefld_->getFValue();
    if ( mIsUdf(scalefac) )
	mErrRetBool(uiStrings::phrEnter(tr("the scale factor")))

    uiRetVal retval;
    ConstRefMan<Wavelet> inpwvlt = WaveletMGR().fetch( inpwvltid_, retval );
    if ( retval.isError() )
	mErrRetBool( retval )

    RefMan<Wavelet> wvlt = inpwvlt->clone();
    wvlt->transform( 0, scalefac );
    if ( !wvltfld_->store(*wvlt) )
	return false;

    outwvltid_ = wvltfld_->key(true);
    const DBKey horid( horfld_->key(true) );
    const DBKey seisid( seisfld_->key(true) );
    WaveletMGR().setScalingInfo( outwvltid_, &inpwvltid_, &horid, &seisid,
				 evfld_->levelName() );
    return true;
}
