/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vispseventdisplay.h"

#include "binidvalset.h"
#include "prestackevents.h"
#include "survinfo.h"
#include "valseries.h"
#include "velocitycalc.h"
#include "viscoord.h"
#include "visdatagroup.h"
#include "visdrawstyle.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "visprestackdisplay.h"
#include "vistransform.h"

 
mCreateFactoryEntry( visSurvey::PSEventDisplay );

namespace visSurvey
{

DefineEnumNames( PSEventDisplay, MarkerColor, 0, "Marker Color" )
{ "Single", "Quality", "Velocity", "Velocity fit", 0 };

DefineEnumNames( PSEventDisplay, DisplayMode, 0, "Display Mode" )
{ "None","Zero offset", "Sticks from sections", "Zero offset on sections", 
  "Sticks to gathers", 0 };

PSEventDisplay::PSEventDisplay()
    : VisualObjectImpl( false )
    , displaymode_( ZeroOffsetOnSections )
    , eventman_( 0 )
    , qualityrange_( 0, 1 )
    , displaytransform_( 0 )
    , linestyle_( new visBase::DrawStyle )
    , horid_( -1 )
    , offsetscale_( 1 )
    , markercolor_( Single )
    , eventmarkerset_( visBase::MarkerSet::create() )
{
    setLockable();
    linestyle_->ref();
    addNodeState( linestyle_ );
    eventmarkerset_->ref();
    eventmarkerset_->setMarkerStyle( markerstyle_ );
    eventmarkerset_->setScreenSize( 26 );
    eventmarkerset_->setMaterial( getMaterial() );

    addChild( eventmarkerset_->osgNode() );
    ctabmapper_.setup_.type( ColTab::MapperSetup::Auto );

}


PSEventDisplay::~PSEventDisplay()
{
    clearAll();

    setDisplayTransformation( 0 );
    setEventManager( 0 );
    linestyle_->unRef();

    removeChild( eventmarkerset_->osgNode() );
    eventmarkerset_->unRef();
}


void PSEventDisplay::clearAll()
{
    for ( int idx=parentattached_.size()-1; idx>=0; idx-- )
    {
	ParentAttachedObject* pao = parentattached_[idx];
	removeChild( pao->objectgroup_->osgNode() );
    }

    deepErase( parentattached_ );
}


Color PSEventDisplay::getColor() const
{ 
    return getMaterial()->getColor();
}


const char** PSEventDisplay::markerColorNames() const
{
    return PSEventDisplay::MarkerColorNames();
}


const char** PSEventDisplay::displayModeNames() const
{
    return PSEventDisplay::DisplayModeNames();
}


void PSEventDisplay::setEventManager( PreStack::EventManager* em )
{
    if ( eventman_==em )
	return;

    clearAll();

    if ( eventman_ )
    {
	eventman_->change.remove( mCB(this,PSEventDisplay,eventChangeCB) );
	eventman_->forceReload.remove(
		mCB(this,PSEventDisplay,eventForceReloadCB) );
	eventman_->unRef();
    }

    eventman_ = em;

    if ( eventman_ )
    {
	eventman_->ref();
	eventman_->change.notify( mCB(this,PSEventDisplay,eventChangeCB) );
	eventman_->forceReload.notify(
		mCB(this,PSEventDisplay,eventForceReloadCB) );
	getMaterial()->setColor( eventman_->getColor() );
    }
    
    
    updateDisplay();
}


void PSEventDisplay::setHorizonID( int horid )
{
    if ( horid_==horid )
	return;

    horid_ = horid;

    updateDisplay();
}


void PSEventDisplay::setMarkerColor( MarkerColor n, bool update )
{
    if ( markercolor_==n )
	return;

    markercolor_ = n;

    if ( update )
	updateDisplay();
}


PSEventDisplay::MarkerColor PSEventDisplay::getMarkerColor() const
{ return markercolor_; }


void PSEventDisplay::setColTabMapper( const ColTab::MapperSetup& n,
				      bool update )
{
    if ( ctabmapper_.setup_==n )
	return;

    ctabmapper_.setup_ = n;

    if ( update )
	updateDisplay();
}


const ColTab::MapperSetup& PSEventDisplay::getColTabMapper() const
{ return ctabmapper_.setup_; }

const ColTab::MapperSetup* PSEventDisplay::getColTabMapperSetup(
						int visid, int attr ) const
{ return &ctabmapper_.setup_; }

void PSEventDisplay::setColTabSequence( int ch, const ColTab::Sequence& n,
					TaskRunner* tr )
{ setColTabSequence( n, true ); }

void PSEventDisplay::setColTabSequence( const ColTab::Sequence& n, bool update )
{
    if ( ctabsequence_==n )
	return;

    ctabsequence_ = n;

    if ( update )
	updateDisplay();
}


const ColTab::Sequence* PSEventDisplay::getColTabSequence( int ) const
{ return &ctabsequence_; }


void PSEventDisplay::setDisplayMode( DisplayMode dm )
{
    if ( dm==displaymode_ )
	return;

    displaymode_ = dm;

    updateDisplay();
}


PSEventDisplay::DisplayMode PSEventDisplay::getDisplayMode() const
{ return displaymode_; }


void PSEventDisplay::setLineStyle( const LineStyle& ls )
{
    linestyle_->setLineStyle( ls );
}


LineStyle PSEventDisplay::getLineStyle() const
{ 
    return linestyle_->lineStyle();
} 


void PSEventDisplay::setMarkerStyle( const MarkerStyle3D& st, bool update )
{
    if ( markerstyle_==st )
	return;

    markerstyle_ = st;

    if ( update )
    {
	for ( int idx=0; idx<parentattached_.size(); idx++ )
	{
	    ParentAttachedObject* pao = parentattached_[idx];
		pao->markerset_->setMarkerStyle( st );
	}
    }
}


//bool PSEventDisplay::filterBinID( const BinID& bid ) const
//{
    //for ( int idx=0; idx<sectionranges_.size(); idx++ )
    //{
	//if ( sectionranges_[idx].includes( bid ) )
	    //return false;
    //}
//
    //return true;
//}


void PSEventDisplay::updateDisplay()
{
    if ( !tryWriteLock() )
	return;

    if ( displaymode_==ZeroOffset )
    {
	eventChangeCB( 0 );
	updateDisplay( 0 );
	writeUnLock();
	return;
    }

    for ( int idx=0; idx<parentattached_.size(); idx++ )
	updateDisplay( parentattached_[idx] );

    writeUnLock();
}


#define mRemoveParAttached( obj ) \
    removeChild( obj->objectgroup_->osgNode() ); \
    delete obj; \
    parentattached_ -= obj


void PSEventDisplay::otherObjectsMoved(
	const ObjectSet<const SurveyObject>& objs, int whichid )
{
    TypeSet<int> newparentsid;
    for ( int idx=objs.size()-1; idx>=0; idx-- )
    {
	int objid = -1;
	    
	mDynamicCastGet(const visSurvey::PreStackDisplay*,gather,objs[idx]);
	if ( gather )
	    objid = gather->id();
	else
	{
	    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,objs[idx]);
	    if ( pdd && pdd->getOrientation() != 
		    visSurvey::PlaneDataDisplay::Zslice )
		objid = pdd->id();
	}

	if ( objid==-1 )
	    continue;

	if ( whichid!=-1 )
	{
	    if ( objid==whichid )
	    {
		newparentsid += objid;
		break;
	    }
	}
	else
	{
	    newparentsid += objid;
	}
    }

    ObjectSet<ParentAttachedObject> toremove;
    if ( whichid==-1 )
       toremove = parentattached_;

    for ( int idx=0; idx<newparentsid.size(); idx++ )
    {
	ParentAttachedObject* pao = 0;
	for ( int idy=0; idy<toremove.size(); idy++ )
	{
	    if ( toremove[idy]->parentid_!=newparentsid[idx] )
		continue;

	    pao = toremove[idy];

	    toremove.removeSingle( idy );
	    break;
	}

	if ( !pao )
	{
	    for ( int idy=0; idy<parentattached_.size(); idy++ )
	    {
		if ( parentattached_[idy]->parentid_ != newparentsid[idx] )
		    continue;
	
		mRemoveParAttached( parentattached_[idy] );
	    }

	    pao = new ParentAttachedObject( newparentsid[idx] );
	    addChild( pao->objectgroup_->osgNode() );
	    parentattached_ += pao;
	    pao->objectgroup_->setDisplayTransformation( displaytransform_ );
	}
	
	if ( displaymode_==FullOnSections || displaymode_==FullOnGathers ||
       	     displaymode_==ZeroOffsetOnSections )
	    updateDisplay( pao );
    }

    for ( int idx=0; idx<toremove.size(); idx++ )
    {
	mRemoveParAttached( toremove[idx] );
    }
}


float PSEventDisplay::getMoveoutComp( const TypeSet<float>& offsets,
				    const TypeSet<float>& picks ) const
{
    float variables[] = { picks[0], 0, 3000 };
    PtrMan<MoveoutComputer> moveoutcomp = new RMOComputer;
    const float error = moveoutcomp->findBestVariable(
		    variables, 1, ctabmapper_.setup_.range_,
		    offsets.size(), offsets.arr(), picks.arr() );
    return markercolor_==Velocity ? variables[1] : error;
}


void PSEventDisplay::updateDisplay( ParentAttachedObject* pao )
{
    if ( !eventman_ )
	return;

    if ( displaymode_==None )
    {
	for ( int idx=0; idx<parentattached_.size(); idx++ )
	    clearDisplay( parentattached_[idx] );
	
	return;
    }
    else if ( displaymode_==ZeroOffset )
    {
	for ( int idx=0; idx<parentattached_.size(); idx++ )
    	    clearDisplay( parentattached_[idx] );

	BinIDValueSet locations( 0, false );
	eventman_->getLocations( locations );
	TypeSet<float> vals;
	for ( int lidx=0; lidx<locations.totalSize(); lidx++ )
	{
	    const BinID bid = locations.getBinID( locations.getPos(lidx) );
	    ConstRefMan<PreStack::EventSet> eventset
		    = eventman_->getEvents(bid, true );
	    if ( !eventset )
		return clearAll();

	    const int size = eventset->events_.size();
	    
	    for ( int idx=0; idx<size; idx++ )
	    {
		const PreStack::Event* psevent = eventset->events_[idx];
		if ( !psevent->sz_ )
		    continue;
		Coord3 pos( bid.inl(), bid.crl(), psevent->pick_[0] );
		eventmarkerset_->getCoordinates()->addPos( pos );

		TypeSet<float> offsets;
		TypeSet<float> picks;
		for ( int idy=0; idy<psevent->sz_; idy++ )
		{
		    offsets += psevent->offsetazimuth_[idy].offset();
		    picks += psevent->pick_[idy];
		}
		sort_coupled( offsets.arr(), picks.arr(), picks.size() );

		vals += (markercolor_==Quality ? psevent->quality_
					       : getMoveoutComp(offsets,picks));
	    }
	}

	if (  markercolor_ == Single )
	{
	    getMaterial()->setColor( eventman_->getColor() );
	    eventmarkerset_->setMarkersSingleColor( eventman_->getColor() );
	}
	else
	{
	    eventmarkerset_->setMaterial( new visBase::Material );
	    const ArrayValueSeries<float,float> vs(vals.arr(),0,vals.size());
	    ctabmapper_.setData( &vs, vals.size() );
	    for (int idx=0;idx<eventmarkerset_->getCoordinates()->size();idx++)
	    {
		const Color col = ctabsequence_.color(
		    ctabmapper_.position( vals[idx]) );
		 eventmarkerset_->getMaterial()->setColor( col,idx) ;
	    }
	}
	return;
    }
    
    clearDisplay( pao );
    CubeSampling cs( false );
    bool fullevent;
    Coord dir;
    if ( displaymode_==FullOnGathers )
    {
	fullevent = true;
	mDynamicCastGet(const visSurvey::PreStackDisplay*,gather,
		visBase::DM().getObject(pao->parentid_) );
	if ( !gather )
	{
	    pao->objectgroup_->removeObject(
		pao->objectgroup_->getFirstIdx( pao->markerset_ ) );

	    pao->markerset_->clearMarkers();

	    if ( pao->lines_ )
	    {
		pao->lines_->removeAllPrimitiveSets();
		pao->lines_->getCoordinates()->setEmpty();
	    }
	    
	    return;
	}

	const BinID bid = gather->is3DSeis() ? gather->getPosition()
					     : BinID(-1,-1);
	cs.hrg.setInlRange( Interval<int>(bid.inl(), bid.inl()) );
	cs.hrg.setCrlRange( Interval<int>(bid.crl(), bid.crl()) );

	const bool isinl = gather->isOrientationInline();
	dir.x = (isinl ? offsetscale_ : 0) / SI().inlDistance();
	dir.y = (isinl ? 0 : offsetscale_ ) / SI().crlDistance();
    }
    else
    {
	mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,
			visBase::DM().getObject( pao->parentid_ ) );
	if ( !pdd )
	    return;

	cs = pdd->getCubeSampling();
	const bool isinl =
	    pdd->getOrientation()==visSurvey::PlaneDataDisplay::Inline;

	fullevent = displaymode_==FullOnSections;

	dir.x = (isinl ? offsetscale_ : 0) / SI().inlDistance();
	dir.y = (isinl ? 0 : offsetscale_ ) / SI().crlDistance();
    }

    HorSamplingIterator iter( cs.hrg );

    BinID bid = cs.hrg.start;
    int cii = 0;
    int lastmarker = 0;

    PtrMan<MoveoutComputer> moveoutcomp = new RMOComputer;

    ObjectSet<PreStack::EventSet> eventsetstounref = pao->eventsets_;
    pao->eventsets_.erase();
    pao->hrg_ = cs.hrg;

    if ( fullevent && !pao->lines_ )
    {
	pao->lines_ = visBase::PolyLine::create();
	pao->objectgroup_->addObject( pao->lines_ );
    }

    TypeSet<float> values;

    do
    {
	RefMan<PreStack::EventSet> eventset = eventman_ ?
	    eventman_->getEvents( bid, true, false ) : 0;
	if ( !eventset )
	    continue;

	Interval<int> eventrg( 0, eventset->events_.size()-1 );
	if ( horid_!=-1 )
	{
	    const int eventidx = eventset->indexOf( horid_ );
	    if ( eventidx==-1 )
		continue;

	    eventrg.start = eventrg.stop = eventidx;
	}

	pao->eventsets_ += eventset;
	eventset->ref();

	if ( markercolor_==Single )
	    pao->markerset_->setMaterial( 0 );
	else
	{
	    if ( !pao->markerset_->getMaterial() )
		pao->markerset_->setMaterial(
		new visBase::Material );
	}

	for ( int idx=eventrg.start; idx<=eventrg.stop; idx++ )
	{
	    const PreStack::Event* event = eventset->events_[idx];
	    if ( !event->sz_ )
		continue;

	    TypeSet<float> offsets( event->sz_, 0 );
	    TypeSet<float> picks( event->sz_, 0 );
	    for ( int idy=0; idy<event->sz_; idy++ )
	    {
		offsets[idy] = event->offsetazimuth_[idy].offset();
		picks[idy] = event->pick_[idy];
	    }
	    sort_coupled( offsets.arr(), picks.arr(), picks.size() );

	    float value = mUdf(float);
	    if ( markercolor_==Quality )
		value = event->quality_;
	    else if ( markercolor_!=Single && event->sz_>1 )
		value = getMoveoutComp( offsets, picks );  

	    Interval<int> pickrg( 0, fullevent ? event->sz_-1 : 0 );
	    const bool doline = pickrg.start!=pickrg.stop;
	    for ( int idy=pickrg.start; idy<=pickrg.stop; idy++ )
	    {
		Coord3 pos( bid.inl(), bid.crl(),  picks[idy] );
		if ( fullevent )
		{
		    const Coord offset = dir*offsets[idy];
		    pos.x += offset.x;
		    pos.y += offset.y;
		}

		pao->markerset_->getCoordinates()->addPos( pos );

		if ( markercolor_ != Single )
		    values += value;

		lastmarker++;
		if ( doline )
		    pao->lines_->addPoint( pos );
	    }

	    const int size = pao->lines_->getCoordinates()->size();
	    if ( doline && size > 0 )
	    {
		Geometry::RangePrimitiveSet* ps =
		    Geometry::RangePrimitiveSet::create();
		Interval<int> range( cii,size - 1 );
		ps->setRange( range );
		ps->ref();
		cii = pao->lines_->getCoordinates()->size();
	    }
	}

    } while ( iter.next( bid ) );

    if ( markercolor_ != Single )
    {
	if ( ctabmapper_.setup_.type_!=ColTab::MapperSetup::Fixed )
	{
	    const ArrayValueSeries<float,float>
		vs(values.arr(),0,values.size());
	    ctabmapper_.setData( &vs, values.size() );
	}

	for ( int idx =0; idx<lastmarker; idx++ )
	{
	    Color color = ctabsequence_.color(
		ctabmapper_.position(values[idx]) );
	    pao->markerset_->getMaterial()->setColor(color, idx );
	}

    }

    for ( int idx=pao->markerset_->getCoordinates()->size()-1;
	idx>=lastmarker; idx-- )
	pao->markerset_->removeMarker( idx );

    deepUnRef( eventsetstounref );
    
}


void PSEventDisplay::clearDisplay( ParentAttachedObject* pao )
{

    if( eventmarkerset_ )
	eventmarkerset_->clearMarkers();

    if ( !pao )
	return;

    pao->markerset_->clearMarkers();
    pao->lines_ = 0;

    if ( pao->objectgroup_ )
    	pao->objectgroup_->removeAll();

}


void PSEventDisplay::setDisplayTransformation(const mVisTrans* nt)
{ 
    for ( int idx=0; idx<parentattached_.size(); idx++ )
    {
	ParentAttachedObject* pao = parentattached_[idx];
	pao->objectgroup_->setDisplayTransformation( nt );
    }

    eventmarkerset_->setDisplayTransformation( nt );

    if ( displaytransform_ )
	displaytransform_->unRef();

    displaytransform_ = nt;

    if ( displaytransform_ )
	displaytransform_->ref();
}


const mVisTrans* PSEventDisplay::getDisplayTransformation() const
{ return displaytransform_; }


void PSEventDisplay::eventChangeCB(CallBacker*)
{
    const BinID bid = eventman_ ? eventman_->changeBid() : BinID(-1,-1);
    if ( bid.inl()<0 || bid.crl()<0 )
    {
	if ( eventman_ )
 	    getMaterial()->setColor( eventman_->getColor() );
    }

    if ( !parentattached_.size() )
    {
	retriveParents();
	return;
    }

    for ( int idx=parentattached_.size()-1; idx>=0; idx-- )
    {
	ParentAttachedObject* pao = parentattached_[idx];
	if ( !pao->hrg_.includes(bid) )
	    continue;

	updateDisplay(pao);
    }
}


void PSEventDisplay::eventForceReloadCB(CallBacker*)
{
    for ( int idx=parentattached_.size()-1; idx>=0; idx-- )
    {
	ParentAttachedObject* pao = parentattached_[idx];
	deepUnRef( pao->eventsets_ );
	HorSamplingIterator iter( pao->hrg_ );

	BinID bid = pao->hrg_.start;
	do
	{
	    eventman_->addReloadPosition( bid );
	}
	while ( iter.next( bid ) );
    }

    deepErase( parentattached_ );

}


bool PSEventDisplay::hasParents() const
{
    return !parentattached_.isEmpty();
}


void PSEventDisplay::retriveParents()
{
    if ( !scene_ )
	return;
    
    TypeSet<int> visids;
    for ( int idx=0; idx<scene_->size(); idx++ )
	visids += scene_->getObject( idx )->id();

    for ( int idx=0; idx<visids.size(); idx++ )
    {
	mDynamicCastGet( visSurvey::SurveyObject*, so,
		scene_->getObject(visids[idx])  );
	if ( !so ) continue;
	
	mDynamicCastGet(const visSurvey::PreStackDisplay*,gather,so);
	mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,so);
	if ( gather || (pdd && pdd->isOn() &&
	     pdd->getOrientation()!=visSurvey::PlaneDataDisplay::Zslice) )
	{
	    ParentAttachedObject* pao = new ParentAttachedObject( visids[idx] );
	    addChild( pao->objectgroup_->osgNode() );
	    parentattached_ += pao;
	    pao->objectgroup_->setDisplayTransformation( displaytransform_ );
	    updateDisplay( pao );
	}
    }
}


PSEventDisplay::ParentAttachedObject::ParentAttachedObject( int parent )
    : parentid_( parent )
    , objectgroup_( visBase::DataObjectGroup::create() )
    , lines_( 0 )
    , markerset_( visBase::MarkerSet::create() )
{
    objectgroup_->ref();
    markerset_->ref();
    objectgroup_->addObject( markerset_ );
}


PSEventDisplay::ParentAttachedObject::~ParentAttachedObject()
{
    objectgroup_->unRef();
    deepUnRef( eventsets_ );

    markerset_->unRef();
}

} // namespace
