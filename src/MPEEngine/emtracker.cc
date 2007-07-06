/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2004
___________________________________________________________________

-*/

static const char* rcsID = "$Id: emtracker.cc,v 1.34 2007-07-06 14:11:05 cvskris Exp $";

#include "emtracker.h"

#include "attribsel.h"
#include "autotracker.h"
#include "consistencychecker.h"
#include "undo.h"
#include "emmanager.h"
#include "emobject.h"
#include "executor.h"
#include "mpeengine.h"
#include "sectionextender.h"
#include "sectionselector.h"
#include "sectionadjuster.h"
#include "sectiontracker.h"
#include "survinfo.h"
#include "trackplane.h"
#include "iopar.h"

#include "ioman.h"
#include "ioobj.h"
#include "mpesetup.h"


namespace MPE 
{

EMTracker::EMTracker( EM::EMObject* emo )
    : isenabled_(true)
    , emobject_(0)
{
    setEMObject(emo);
}


EMTracker::~EMTracker()
{
    deepErase( sectiontrackers_ );
    setEMObject(0);
}


BufferString EMTracker::objectName() const
{ return emobject_ ? emobject_->name() : 0; }



EM::ObjectID EMTracker::objectID() const
{ return emobject_ ? emobject_->id() : -1; }


bool EMTracker::trackSections( const TrackPlane& plane )
{
    if ( !emobject_ || !isenabled_ || plane.getTrackMode()==TrackPlane::Move ||
	    plane.getTrackMode()==TrackPlane::None )
	return true;


    const int initialhistnr = EM::EMM().undo().currentEventID();
    ConsistencyChecker* consistencychecker = getConsistencyChecker();
    if ( consistencychecker ) consistencychecker->reset();

    bool success = true;
    for ( int idx=0; idx<emobject_->nrSections(); idx++ )
    {
	const EM::SectionID sid = emobject_->sectionID( idx );
	SectionTracker* sectiontracker = getSectionTracker( sid, true );
	if ( !sectiontracker ) continue;
	if ( !sectiontracker->hasInitializedSetup() && 
	     plane.getTrackMode()!=TrackPlane::Erase )
	    continue;

	EM::PosID posid( emobject_->id(), sid );
	if ( !sectiontracker->trackWithPlane(plane) )
	{
	    errmsg_ = sectiontracker->errMsg();
	    success = false;
	}
	else if ( consistencychecker )
	{
	    const TypeSet<EM::SubID>& addedpos = 
		sectiontracker->extender()->getAddedPositions();
	    for ( int posidx=0; posidx<addedpos.size(); posidx++ )
	    {
		posid.setSubID( addedpos[posidx] );
		consistencychecker->addNodeToCheck( posid );
	    }
	}
    }

    if ( !success )
	return false;

    if ( consistencychecker )
	consistencychecker->nextStep();

    return true;
}


bool EMTracker::trackIntersections( const TrackPlane& )
{ return true; }


Executor* EMTracker::trackInVolume()
{
    ExecutorGroup* res = 0;
    for ( int idx=0; idx<emobject_->nrSections(); idx++ )
    {
	const EM::SectionID sid = emobject_->sectionID( idx );
	SectionTracker* sectiontracker = getSectionTracker( sid, true );
	if ( !sectiontracker || !sectiontracker->hasInitializedSetup() )
	    continue;
	
	// check whether data loading was cancelled by user
	ObjectSet<const Attrib::SelSpec> attrselset;
	sectiontracker->getNeededAttribs( attrselset );
	if ( attrselset.isEmpty() || !engine().getAttribCache(*attrselset[0]) )
	    continue;
	
	if ( !res )
	{
	    res = new ExecutorGroup("Autotracker", true );
	    res->setNrDoneText("seeds processed");
	}

	res->add( new AutoTracker( *this, sid ) );
    }

    return res;
}


bool EMTracker::snapPositions( const TypeSet<EM::PosID>& pids ) 
{
    if ( !emobject_ ) return false;

    for ( int idx=0; idx<emobject_->nrSections(); idx++ )
    {
	const EM::SectionID sid = emobject_->sectionID(idx);

	TypeSet<EM::SubID> subids;
	for ( int idy=0; idy<pids.size(); idy++ )
	{
	    const EM::PosID& pid = pids[idy];
	    if ( pid.objectID()!= emobject_->id() )
		continue;
	    if ( pid.sectionID()!=sid )
		continue;

	    subids += pid.subID();
	}

	SectionTracker* sectiontracker = getSectionTracker(sid,true);
	if ( !sectiontracker || !sectiontracker->hasInitializedSetup() )
	    continue;

	SectionAdjuster* adjuster = sectiontracker->adjuster();
	if ( !adjuster ) continue;

	adjuster->reset();
	adjuster->setPositions( subids );

	const bool didremoveonfailure = adjuster->removeOnFailure(false);

	while ( int res=adjuster->nextStep() )
	{
	    if ( res==-1 )
	    {
		errmsg_ = adjuster->errMsg();
		adjuster->removeOnFailure(didremoveonfailure);
		return false;
	    }
	}

	adjuster->removeOnFailure(didremoveonfailure);
    }

    return true;
}


CubeSampling EMTracker::getAttribCube( const Attrib::SelSpec& spec ) const
{
    CubeSampling res( engine().activeVolume() );

    for ( int sectidx=0; sectidx<sectiontrackers_.size(); sectidx++ )
    {
	CubeSampling cs = sectiontrackers_[sectidx]->getAttribCube( spec );
	res.include( cs );
    }


    return res;
}


void EMTracker::getNeededAttribs( ObjectSet<const Attrib::SelSpec>& res ) const
{
    for ( int sectidx=0; sectidx<sectiontrackers_.size(); sectidx++ )
    {
	ObjectSet<const Attrib::SelSpec> specs;
	sectiontrackers_[sectidx]->getNeededAttribs( specs );

	for ( int idx=0; idx<specs.size(); idx++ )
	{
	    const Attrib::SelSpec* as = specs[idx];
	    if ( indexOf(res,*as) < 0 )
		res += as;
	}
    }
}

const char* EMTracker::errMsg() const
{ return errmsg_[0] ? (const char*) errmsg_ : 0; }


SectionTracker* EMTracker::getSectionTracker( EM::SectionID sid, bool create )
{
    for ( int idx=0; idx<sectiontrackers_.size(); idx++ )
    {
	if ( sectiontrackers_[idx]->sectionID()==sid )
	    return sectiontrackers_[idx];
    }

    if ( !create ) return 0;

    SectionTracker* sectiontracker = createSectionTracker( sid );
    if ( !sectiontracker || !sectiontracker->init() )
    {
	delete sectiontracker;
	return 0;
    }

    const int defaultsetupidx = sectiontrackers_.size()-1;
    sectiontrackers_ += sectiontracker;

    if ( defaultsetupidx >= 0 )
	applySetupAsDefault( sectiontrackers_[defaultsetupidx]->sectionID() );
    
    return sectiontracker;
}


void EMTracker::applySetupAsDefault( const EM::SectionID sid )
{
    SectionTracker* defaultsetuptracker(0);

    for ( int idx=0; idx<sectiontrackers_.size(); idx++ )
    {
	if ( sectiontrackers_[idx]->sectionID() == sid )
	    defaultsetuptracker = sectiontrackers_[idx];
    }
    if ( !defaultsetuptracker || !defaultsetuptracker->hasInitializedSetup() ) 
	return;

    IOPar par;
    defaultsetuptracker->fillPar( par );

    for ( int idx=0; idx<sectiontrackers_.size(); idx++ )
    {
	if ( !sectiontrackers_[idx]->hasInitializedSetup() )
	    sectiontrackers_[idx]->usePar( par );
    }
}    


void EMTracker::erasePositions( EM::SectionID sectionid,
				const TypeSet<EM::SubID>& pos )
{
    EM::PosID posid( emobject_->id(), sectionid );
    for ( int idx=0; idx<pos.size(); idx++ )
    {
	posid.setSubID( pos[idx] );
	emobject_->unSetPos( posid, true );
    }
}


void EMTracker::fillPar( IOPar& iopar ) const
{
    for ( int idx=0; idx<sectiontrackers_.size(); idx++ )
    {
	const SectionTracker* st = sectiontrackers_[idx];
	IOPar localpar;
	localpar.set( sectionidStr(), st->sectionID() );
	st->fillPar( localpar );

	BufferString key( IOPar::compKey("Section",idx) );
	iopar.mergeComp( localpar, key );
    }
}


bool EMTracker::usePar( const IOPar& iopar )
{
    int idx=0;
    while ( true )
    {
	BufferString key( IOPar::compKey("Section",idx) );
	PtrMan<IOPar> localpar = iopar.subselect( key );
	if ( !localpar ) return true;

	int sid;
	if ( !localpar->get(sectionidStr(),sid) ) { idx++; continue; }
	SectionTracker* st = getSectionTracker( (EM::SectionID)sid, true );
	if ( !st ) { idx++; continue; }
	
	MultiID setupid;
	if ( !localpar->get(setupidStr(),setupid) )
	{
	    st->usePar( *localpar );    
	}
	else  // old policy for restoring session
	{
	    st->setSetupID( setupid );
	    PtrMan<IOObj> ioobj = IOM().get( setupid );
	    if ( !ioobj ) { idx++; continue; }

	    MPE::Setup setup;
	    BufferString bs;
	    if ( !MPESetupTranslator::retrieve(setup,ioobj,bs) )
		{ idx++; continue; }

	    IOPar setuppar;
	    setup.fillPar( setuppar );
	    st->usePar( setuppar );
	}
	
	idx++;
    }

    return true;
}


void EMTracker::setEMObject( EM::EMObject* no ) 
{
    if ( emobject_ ) emobject_->unRef();
    emobject_ = no;
    if ( emobject_ ) emobject_->ref();
}


// TrackerFactory +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TrackerFactory::TrackerFactory( const char* emtype, EMTrackerCreationFunc func )
    : type( emtype )
    , createfunc( func )
{}


const char* TrackerFactory::emObjectType() const
{ return type; } 


EMTracker* TrackerFactory::create( EM::EMObject* emobj ) const
{ return createfunc( emobj ); }


}; // namespace MPE
