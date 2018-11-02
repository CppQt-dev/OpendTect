/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne
 Date:          April 2017
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "survgeom2d.h"

#include "dbkey.h"
#include "posinfo2d.h"


#define mSetSampling \
const StepInterval<Pos::TraceID> trcrg = data_.trcNrRange(); \
sampling_.zsamp_ = data_.zRange(); \
sampling_.hsamp_.start_.lineNr() = sampling_.hsamp_.stop_.lineNr() = id(); \
sampling_.hsamp_.start_.trcNr() = trcrg.start; \
sampling_.hsamp_.stop_.trcNr() = trcrg.stop; \
sampling_.hsamp_.step_.trcNr() = trcrg.step; \
trcnrrg_ = sampling_.hsamp_.trcRange(); \
zrg_ = sampling_.zsamp_

Survey::Geometry2D::Geometry2D( const char* lnm )
    : data_(*new PosInfo::Line2DData(lnm))
    , trcdist_(mUdf(float))
    , linelength_(mUdf(float))
{
    sampling_.hsamp_.survid_ = TrcKey::std2DSurvID();
    mSetSampling;
}


Survey::Geometry2D::Geometry2D( PosInfo::Line2DData* l2d )
    : data_(l2d ? *l2d : *new PosInfo::Line2DData)
    , trcdist_(mUdf(float))
    , linelength_(mUdf(float))
{
    sampling_.hsamp_.survid_ = TrcKey::std2DSurvID();
    mSetSampling;
}


Survey::Geometry2D::~Geometry2D()
{ delete &data_; }


Survey::Geometry::ID Survey::Geometry2D::getIDFrom( const DBKey& dbky )
{
    return dbky.objID().getI();
}


void Survey::Geometry2D::add( const Coord& crd, int trcnr, int spnr )
{
    add( crd.x_, crd.y_, trcnr, spnr );
}


void Survey::Geometry2D::add( double x, double y, int trcnr, int spnr )
{
    PosInfo::Line2DPos pos( trcnr );
    pos.coord_.x_ = x;
    pos.coord_.y_ = y;
    data_.add( pos );
    spnrs_ += spnr;
}


int Survey::Geometry2D::size() const
{
    return data_.size();
}


void Survey::Geometry2D::setEmpty()
{
    data_.setEmpty();
    spnrs_.erase();
}


/*
bool Survey::Geometry2D::getPosByTrcNr( int trcnr, Coord& crd, int& spnr ) const
{
    const int posidx = data_.indexOf( trcnr );
    if ( !data_.positions().validIdx(posidx) )
	return false;

    crd = data_.positions()[posidx].coord_;
    spnr = spnrs_.validIdx(posidx) ? spnrs_[posidx] : mUdf(int);
    return true;
}


bool Survey::Geometry2D::getPosBySPNr( int spnr, Coord& crd, int& trcnr ) const
{
    const int posidx = spnrs_.indexOf( spnr );
    if ( !data_.positions().validIdx(posidx) )
	return false;

    crd = data_.positions()[posidx].coord_;
    trcnr = data_.positions()[posidx].nr_;
    return true;
}


bool Survey::Geometry2D::getPosByCoord( const Coord& crd, int& trcnr,
					int& spnr ) const
{
    const int posidx = data_.nearestIdx( crd );
    if ( !data_.positions().validIdx(posidx) )
	return false;

    trcnr = data_.positions()[posidx].nr_;
    spnr = spnrs_.validIdx(posidx) ? spnrs_[posidx] : mUdf(int);
    return true;
}
*/


const OD::String& Survey::Geometry2D::name() const
{
    return data_.lineName();
}


Coord Survey::Geometry2D::toCoord( int, int trcnr ) const
{
    PosInfo::Line2DPos pos;
    return data_.getPos(trcnr,pos) ? pos.coord_ : Coord::udf();
}


TrcKey Survey::Geometry2D::nearestTrace( const Coord& crd, float* dist ) const
{
    PosInfo::Line2DPos pos;
    if ( !data_.getPos(crd,pos,dist) )
	return TrcKey::udf();

    return Survey::GM().traceKey( id(), pos.nr_ );
}


void Survey::Geometry2D::touch()
{
    Threads::Locker locker( lock_ );
    mSetSampling;
    trcdist_ = mUdf(float);
    linelength_ = mUdf(float);
}


float Survey::Geometry2D::averageTrcDist() const
{
    Threads::Locker locker( lock_ );
    if ( !mIsUdf(trcdist_) )
	return trcdist_;

    float max;
    data_.compDistBetwTrcsStats( max, trcdist_ );
    return trcdist_;
}


void Survey::Geometry2D::setAverageTrcDist( float trcdist )
{
    Threads::Locker locker( lock_ );
    trcdist_ = trcdist;
}


float Survey::Geometry2D::lineLength() const
{
    Threads::Locker locker( lock_ );
    if ( !mIsUdf(linelength_) )
	return linelength_;

    linelength_ = 0;
    for ( int idx=1; idx<data_.positions().size(); idx++ )
	linelength_ += data_.positions()[idx].coord_.distTo<float>(
					data_.positions()[idx-1].coord_);

    return linelength_;
}


void Survey::Geometry2D::setLineLength( float ll )
{
    Threads::Locker locker( lock_ );
    linelength_ = ll;
}


Survey::Geometry::RelationType Survey::Geometry2D::compare(
				const Geometry& geom, bool usezrg ) const
{
    mDynamicCastGet( const Survey::Geometry2D*, geom2d, &geom );
    if ( !geom2d )
	return UnRelated;

    const PosInfo::Line2DData& mydata = data();
    const PosInfo::Line2DData& otherdata = geom2d->data();
    if ( !mydata.coincidesWith(otherdata) )
	return UnRelated;

    const StepInterval<int> mytrcrg = mydata.trcNrRange();
    const StepInterval<int> othtrcrg = otherdata.trcNrRange();
    const StepInterval<float> myzrg = mydata.zRange();
    const StepInterval<float> othzrg = otherdata.zRange();
    if ( mytrcrg == othtrcrg && (!usezrg || myzrg.isEqual(othzrg,1e-3)) )
	return Identical;
    if ( mytrcrg.includes(othtrcrg) && (!usezrg || myzrg.includes(othzrg)) )
	return SuperSet;
    if ( othtrcrg.includes(mytrcrg) && (!usezrg || othzrg.includes(myzrg)) )
	return SubSet;

    return Related;
}


StepInterval<float> Survey::Geometry2D::zRange() const
{
    return data_.zRange();
}


bool Survey::Geometry2D::includes( int line, int tracenr ) const
{
    return data_.indexOf(tracenr) >= 0;
}


BufferString Survey::Geometry2D::makeUniqueLineName( const char* lsnm,
						     const char* lnm )
{
    BufferString newlnm( lsnm );
    newlnm.add( "-" );
    newlnm.add( lnm );
    return newlnm;
}
