
/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : Jan 2009
___________________________________________________________________

-*/


#include "emrandomposbody.h"

#include "ascstream.h"
#include "datapointset.h"
#include "embodyoperator.h"
#include "embodytr.h"
#include "emmanager.h"
#include "executor.h"
#include "dbman.h"
#include "ioobj.h"
#include "iopar.h"
#include "uistrings.h"
#include "pickset.h"
#include "streamconn.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "keystrs.h"

namespace EM
{

#define mRetErr( msg ) { errmsg_ = msg; return; }

class RandomPosBodyReader : public Executor
{ mODTextTranslationClass(EM::RandomPosBodyReader);
public:

    RandomPosBodyReader( RandomPosBody& rdposbody, Conn* conn )
	: Executor( "Random Position Body Loader" )
	, conn_( conn )
	, rdposbody_( rdposbody )
	, nrdone_( 0 )
	, totalnr_( -1 )
    {
	if ( !conn_ || !conn_->forRead() || !conn_->isStream() )
	    mRetErr( "Cannot open connection" );

	ascistream astream( ((StreamConn*)conn_)->iStream() );

	if ( !astream.isOK() )
	    mRetErr( "Cannot read from input file" );
	if ( !astream.isOfFileType( sFileType() ) &&
	     !astream.isOfFileType( sOldFileType()) )
	    mRetErr( sInvalidFile() );

	astream.next();

	if ( atEndOfSection( astream ) )
	    mRetErr( sInvalidFile() );

	IOPar iopar; iopar.getFrom( astream );
	if ( !rdposbody_.usePar( iopar ) )
	    mRetErr( sInvalidFile() );

	iopar.get( sKeyNrPositions(), totalnr_ );
    }

    uiString message() const
    { return uiStrings::phrReading(uiStrings::sPosition(mPlural)); }
    uiString nrDoneText() const
    { return uiStrings::phrRead(uiStrings::sPosition(mPlural)); }

    int nextStep()
    {
	if ( !errmsg_.isEmpty() )
	    return ErrorOccurred();

	od_istream& strm = ((StreamConn*)conn_)->iStream();
	if ( !strm.isOK() )
	{
	    rdposbody_.resetChangedFlag();
	    return Finished();
	}

	Coord3 pos;

	const char* err = "Cannot interprete file";

	strm >> pos.x_;
	if ( !strm.isOK() )
	{
	    if ( !strm.isBad() )
	    {
		rdposbody_.resetChangedFlag();
		return Finished();
	    }
	    errmsg_ = err; return ErrorOccurred();
	}

	int posid;
	strm >> pos.y_ >> pos.z_ >> posid;

	if ( !strm.isOK() )
	    { errmsg_ = err; return ErrorOccurred(); }
	rdposbody_.addPos( pos );
	nrdone_++;

	if ( nrdone_>=totalNr() )
	{
	    rdposbody_.resetChangedFlag();
	    return Finished();
	}

	return MoreToDo();
    }

    od_int64	totalNr() const		{ return totalnr_; }
    od_int64	nrDone() const		{ return nrdone_; }
    static const char*	sFileType()	{ return RandomPosBody::typeStr(); }
    static const char*	sOldFileType()	{ return "RandomPosBody"; }
    static const char*	sKeyNrPositions() { return "Nr positions"; }

protected:

    static const char*		sInvalidFile() { return "Invalid file"; }

    RandomPosBody&		rdposbody_;
    Conn*			conn_;
    int				nrdone_;
    int				totalnr_;
    BufferString		errmsg_;
};


class RandomPosBodyWriter : public Executor
{
public:

    RandomPosBodyWriter( RandomPosBody& rdposbody, Conn* conn )
	: Executor( "Random Position Body Writer" )
	, conn_( conn )
	, rdposbody_( rdposbody )
	, nrdone_( 0 )
    {
	if ( !conn_->forWrite() || !conn_->isStream() )
	    mRetErr( "Internal error: bad connection" );

	ascostream astream( ((StreamConn*)conn_)->oStream() );
	astream.putHeader( RandomPosBody::typeStr() );
	if ( !astream.isOK() )
	    mRetErr( "Cannot write to output Pick Set file" );

	IOPar pars;
	rdposbody.fillPar( pars );
	pars.set( RandomPosBodyReader::sKeyNrPositions(), totalNr() );
	pars.set( sKey::ZUnit(),
		  UnitOfMeasure::surveyDefZStorageUnit()->name() );

	pars.putTo( astream );
    }

    uiString message() const
    { return uiStrings::phrWriting(uiStrings::sPosition(mPlural)); }
    uiString nrDoneText() const
    { return uiStrings::phrWritten(uiStrings::sPosition(mPlural)); }

    int nextStep()
    {
	if ( nrdone_>=totalNr() )
	{
	    ascostream astream( ((StreamConn*)conn_)->oStream() );
	    astream.newParagraph();
	    return Finished();
	}

	const Coord3& crd( rdposbody_.getPositions()[nrdone_] );
	const int idx = mCast( int, rdposbody_.posIDs()[nrdone_] );
	BufferString str;
	str.add( crd.x_ ).add( " " ).add( crd.y_ ).add( " " ).add( crd.z_ )
	    .add( " " ).add ( idx );
	((StreamConn*)conn_)->oStream() << str.buf() << '\n';
	nrdone_++;

	return MoreToDo();
    }

    od_int64    totalNr() const	{ return rdposbody_.getPositions().size(); }
    od_int64    nrDone() const	{ return nrdone_; }

protected:

    RandomPosBody&		rdposbody_;
    Conn*			conn_;
    int				nrdone_;
    BufferString		errmsg_;
};


mImplementEMObjFuncs( RandomPosBody, "RandomPos" );


RandomPosBody::RandomPosBody( const char* nm )
    : EMObject(nm)
{}


RandomPosBody::~RandomPosBody()
{}


DBKey RandomPosBody::storageID() const
{ return EMObject::dbKey(); }


BufferString RandomPosBody::storageName() const
{ return EMObject::name(); }


void RandomPosBody::refBody()
{ EMObject::ref(); }


void RandomPosBody::unRefBody()
{ EMObject::unRef(); }


void RandomPosBody::copyFrom( const Pick::Set& ps )
{
    locations_.erase();
    ids_.erase();

    Pick::SetIter iter( ps );
    EM::SubID idx = 0;
    while ( iter.next() )
    {
	locations_ += iter.get().pos();
	ids_ += idx;
	idx++;
    }
}


void RandomPosBody::copyFrom( const DataPointSet& data, int selgrp )
{
    locations_.erase();
    ids_.erase();

    for ( int idx=0; idx<data.size(); idx++ )
    {
	if ( selgrp<0 || data.selGroup(idx) == selgrp )
	{
	    locations_ += Coord3( data.coord(idx), data.z(idx) );
	    ids_ += idx;
	}
    }
}


void RandomPosBody::copyFrom( const DataPointSet& data, int dpscolid,
			      const Interval<float>& valrg )
{
    locations_.erase();
    ids_.erase();

    if ( dpscolid<0 || dpscolid>data.nrCols() )
	return;

    for ( int idx=0; idx<data.size(); idx++ )
    {
	if ( valrg.includes(data.value(dpscolid,idx),true) )
	{
	    locations_ += Coord3( data.coord(idx), data.z(idx) );
	    ids_ += idx;
	}
    }
}


void RandomPosBody::setPositions( const TypeSet<Coord3>& pts )
{
    locations_.erase();
    locations_ = pts;

    ids_.erase();
    for ( int idx=0; idx<pts.size(); idx++ )
       ids_ += idx;
}


bool RandomPosBody::addPos( const Coord3& np )
{
    if ( locations_.isPresent(np) )
	return false;

    locations_ += np;
    ids_ += ids_.size();

    return true;
}


Coord3 RandomPosBody::getPos( const PosID& posid ) const
{ return getPos( posid.sectionID(), posid.subID() ); }


Coord3 RandomPosBody::getPos( const SectionID& sid, const SubID& subid ) const
{
    if ( sid!=sectionID(0) )
	return Coord3::udf();

    const int posidx = ids_.indexOf( subid );
    return posidx==-1 ? Coord3::udf() : locations_[posidx];
}


bool RandomPosBody::setPosition( const SectionID& sid, const SubID& sub,
				    const Coord3& pos, bool addtohistory,
				    NodeSourceType tp )
{
    if ( sid!=sectionID(0) || sub<0 )
	return false;

    if ( sub==ids_.size() )
	return addPos( pos );

    if ( !ids_.isPresent(sub) && sub==ids_.size() )
	return addPos( pos );
    else
	locations_[mCast(int,sub)] = pos;

    return true;
}


Executor* RandomPosBody::saver( IOObj* inpioobj )
{
    PtrMan<IOObj> myioobj = 0;
    IOObj* ioobj = 0;
    if ( inpioobj )
	ioobj = inpioobj;
    else
    {
	myioobj = DBM().get( dbKey() );
	ioobj = myioobj;
    }

    Conn* conn = ioobj ? ioobj->getConn( Conn::Write ) : 0;
    return conn ? new RandomPosBodyWriter( *this, conn ) : 0;
}


Executor* RandomPosBody::saver()
{ return saver(0); }


Executor* RandomPosBody::loader()
{
    PtrMan<IOObj> ioobj = DBM().get( dbKey() );
    Conn* conn = ioobj ? ioobj->getConn( Conn::Read ) : 0;
    return conn ? new RandomPosBodyReader( *this, conn ) : 0;
}


bool RandomPosBody::isEmpty() const
{ return locations_.isEmpty(); }


const IOObjContext& RandomPosBody::getIOObjContext() const
{
    mDefineStaticLocalObject( PtrMan<IOObjContext>, res, = 0 );
    if ( !res )
    {
	IOObjContext* newres =
	    new IOObjContext(EMBodyTranslatorGroup::ioContext() );
	newres->fixTranslator( typeStr() );

	res.setIfNull(newres,true);
    }

    return *res;
}


ImplicitBody* RandomPosBody::createImplicitBody( TaskRunner* taskrunner,
						 bool smooth ) const
{
    BodyOperator bodyopt;
    return bodyopt.createImplicitBody( locations_, taskrunner );
}


bool RandomPosBody::getBodyRange( TrcKeyZSampling& cs )
{
    for ( int idx=0; idx<locations_.size(); idx++ )
    {
	cs.hsamp_.include( SI().transform(locations_[idx].getXY()) );

	if ( idx )
	    cs.zsamp_.include( (float) locations_[idx].z_ );
	else
	    cs.zsamp_.start = cs.zsamp_.stop = (float) locations_[idx].z_;
    }

    return locations_.size();
}



bool RandomPosBody::useBodyPar( const IOPar& par )
{
    if ( !EMObject::usePar( par ) )
	return false;

    ids_.erase();
    if ( !par.get( sKeySubIDs(), ids_ ) )
	return false;

    locations_.erase();
    for ( int idx=0; idx<ids_.size(); idx++ )
    {
	Coord3 pos( Coord3::udf() );
	BufferString skeypos("Location pos"); skeypos += ids_[idx];
	if (!par.get( skeypos.buf(), pos ) )
	    return false;

	locations_ += pos;
    }

    return true;
}


void RandomPosBody::fillBodyPar( IOPar& par ) const
{
    EMObject::fillPar( par );
    par.set( sKeySubIDs(), ids_ );
    for ( int idx=0; idx<locations_.size(); idx++ )
    {
	BufferString skeypos("Location pos"); skeypos += ids_[idx];
	par.set( skeypos.buf(), locations_[idx] );
    }
}


}; // namespace EM
