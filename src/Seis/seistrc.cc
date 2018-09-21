/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : 28-2-1996
 * FUNCTION : Seismic trace functions
-*/


#include "seistrc.h"
#include "simpnumer.h"
#include "iopar.h"
#include "valseriesinterpol.h"
#include "arraynd.h"
#include <math.h>
#include <float.h>


SeisTrc::~SeisTrc()
{
}


SeisTrc& SeisTrc::operator =( const SeisTrc& trc )
{
    if ( &trc == this ) return *this;

    while ( nrComponents() > trc.nrComponents() )
	data_.delComponent( 0 );

    info_ = trc.info_;
    copyDataFrom( trc, -1, false );

    intpol_ = 0;
    if ( trc.intpol_ )
	intpol_ = new ValueSeriesInterpolator<float>( *trc.intpol_ );

    return *this;
}


bool SeisTrc::reSize( int sz, bool copydata )
{
    for ( int idx=0; idx<nrComponents(); idx++ )
	data_.reSize( sz, idx, copydata );
    return data_.allOk();
}


float* SeisTrc::arr( int icomp )
{
    float val;
    const DataCharacteristics dc( val );
    const TraceDataInterpreter* tdi = data_.getInterpreter( icomp );
    if ( !tdi ) return 0;

    if ( tdi->dataChar() !=dc )
	return 0;

    return (float*) data_.getComponent( icomp )->data();
}


const float* SeisTrc::arr( int icomp ) const
{ return const_cast<SeisTrc*>( this )->arr(icomp); }


bool SeisTrc::copytoArray( Array1D<float>& seistrcarr, int icomp ) const
{
    const int trcsz = size();
    const Array1DInfoImpl trcinfo( trcsz );
    if ( seistrcarr.getSize(0) != trcsz && !seistrcarr.setInfo(trcinfo) )
	return false;

    if ( arr(icomp) )
    {
	const void* srcptr = data_.getComponent(icomp)->data();
	OD::memCopy( seistrcarr.arr(), srcptr, trcsz*sizeof(float) );
	return true;
    }

    for ( int idx=0; idx<trcsz; idx++ )
	seistrcarr.set( idx, get(idx,icomp) );

    return true;
}


void SeisTrc::copyFromArray( const Array1D<float>& seistrcarr, int icomp )
{
    const int trcsz = size();
    if ( trcsz != seistrcarr.getSize(0) )
	return;

    if ( arr(icomp) )
    {
	void* destptr = data_.getComponent(icomp)->data();
	OD::memCopy( destptr, seistrcarr.arr(), trcsz*sizeof(float) );
	return;
    }

    for ( int idx=0; idx<trcsz; idx++ )
	set( idx, icomp, seistrcarr.get( idx ) );
}


const ValueSeriesInterpolator<float>& SeisTrc::interpolator() const
{
    if ( !intpol_ )
    {
	ValueSeriesInterpolator<float>* newintpol =
				new ValueSeriesInterpolator<float>();
	newintpol->snapdist_ = Seis::cDefSampleSnapDist();
	newintpol->smooth_ = true;
	newintpol->extrapol_ = false;
	newintpol->udfval_ = 0;

	intpol_.setIfNull(newintpol,true);
    }

    intpol_->maxidx_ = size()-1;

    return *intpol_;
}


void SeisTrc::setInterpolator( ValueSeriesInterpolator<float>* intpol )
{
    intpol_ = intpol;
}


bool SeisTrc::isNull( int icomp ) const
{
    return chkForSpecVal( icomp, true );
}


bool SeisTrc::hasUndef( int icomp ) const
{
    return chkForSpecVal( icomp, false );
}


bool SeisTrc::chkForSpecVal( int icomp, bool isnull ) const
{
    if ( icomp >= nrComponents() )
	return isnull;

    Interval<int> comps( icomp, icomp );
    if ( icomp < 0 )
	{ comps.start = 0; comps.stop = nrComponents()-1; }

    const int sz = size();
    for ( icomp=comps.start; icomp<=comps.stop; icomp++ )
    {
	for ( int isamp=0; isamp<sz; isamp++ )
	{
	    const float val = get( isamp, icomp );
	    if ( (isnull && val) || (!isnull && mIsUdf(val)) )
		return !isnull;
	}
    }

    return isnull;
}


float SeisTrc::getValue( float t, int icomp ) const
{
    const int sz = size();
    int sampidx = nearestSample( t );
    if ( sampidx < 0 || sampidx >= sz )
	return interpolator().udfval_;

    const float pos = ( t - startPos() ) / info_.sampling_.step;
    if ( sampidx-pos > -Seis::cDefSampleSnapDist()
      && sampidx-pos < Seis::cDefSampleSnapDist() )
	return get( sampidx, icomp );

    return interpolator().value( SeisTrcValueSeries(*this,icomp), pos );
}


void SeisTrc::setAll( float v, int compnr )
{
    const auto sz = size();
    for ( int icomp=0; icomp<nrComponents(); icomp++ )
	if ( compnr == icomp || compnr < 0 )
	    for ( int isamp=0; isamp<sz; isamp++ )
		set( isamp, v, icomp );
}


SampleGate SeisTrc::sampleGate( const Interval<float>& tg, bool check ) const
{
    SampleGate sg( info_.sampleGate(tg) );
    if ( !check )
	return sg;

    const int maxsz = size() - 1;
    if ( sg.start > maxsz )
	sg.start = maxsz;
    if ( sg.stop > maxsz )
	sg.stop = maxsz;
    return sg;
}


SeisTrc* SeisTrc::getRelTrc( const ZGate& zgate, float sr ) const
{
    const float pick = info_.pick_;
    if ( mIsUdf(pick) )
	return 0;

    ZGate zg( zgate ); zg.sort();
    SeisTrc* ret = new SeisTrc;
    ret->info_ = info_;
    ret->info_.sampling_.start = zg.start;
    if ( mIsUdf(sr) ) sr = info_.sampling_.step;
    ret->info_.sampling_.step = sr;
    ret->info_.pick_ = 0;

    const int nrsamps = (int)( (zg.stop - zg.start) / sr + 1.5);
    ret->reSize( nrsamps, false );

    while ( ret->nrComponents() < nrComponents() )
	ret->data_.addComponent( ret->size(),
		data_.getInterpreter( ret->nrComponents() )->dataChar() );

    for ( int idx=0; idx<nrsamps; idx++ )
    {
	const float curt = pick + zg.start + sr * idx;
	for ( int icomp=0; icomp<nrComponents(); icomp++ )
	    ret->set( idx, getValue(curt,icomp), icomp );
    }

    return ret;
}


SeisTrc* SeisTrc::getExtendedTo( const ZGate& zgate, bool usevals ) const
{
    const float fnrsamps = (zgate.stop-zgate.start) / info_.sampling_.step + 1;
    const int outnrsamps = mNINT32( fnrsamps );
    const TraceDataInterpreter* tdi = data_.getInterpreter(0);
    DataCharacteristics dc( tdi ? tdi->dataChar() : DataCharacteristics() );
    SeisTrc* newtrc = new SeisTrc( outnrsamps, dc );
    while ( newtrc->nrComponents() < nrComponents() )
	newtrc->data_.addComponent( newtrc->size(),
		data_.getInterpreter( newtrc->nrComponents() )->dataChar() );
    if ( size() < 1 )
	{ newtrc->zero(); return newtrc; }

    newtrc->info_ = info_;
    newtrc->info_.sampling_.start = zgate.start;
    const float z0 = startPos() - Seis::cDefSampleSnapDist()
				    * info_.sampling_.step;
    const float z1 = endPos() + Seis::cDefSampleSnapDist()*info_.sampling_.step;

    for ( int icomp=0; icomp<nrComponents(); icomp++ )
    {
	const float preval = usevals ? get(0,icomp) : 0;
	const float postval = usevals ? get(size()-1,icomp) : 0;
	for ( int isamp=0; isamp<newtrc->size(); isamp++ )
	{
	    const float z = newtrc->samplePos( isamp );
	    const float val = (z < z0 ? preval : (z > z1 ? postval
			    : getValue( z, icomp ) ) );
	    newtrc->set( isamp, val, icomp );
	}
    }

    return newtrc;
}


void SeisTrc::convertToFPs( bool pres )
{
    data_.convertToFPs( pres );
}


void SeisTrc::setNrComponents( int newnrcomps, OD::DataRepType datarep )
{
    const int oldnrcomps = nrComponents();
    const bool isautodatarep = datarep == OD::AutoDataRep;
    if ( oldnrcomps == newnrcomps )
    {
	if ( isautodatarep )
	    return;

	bool isok = true;
	for ( int icomp=0; icomp<oldnrcomps; icomp++ )
	    if ( data().getInterpreter(icomp)->dataChar().userType()!=datarep )
		{ isok = false; break; }
	if ( isok )
	    return;
    }

    const int sz = size();
    if ( isautodatarep )
	datarep = data().getInterpreter(0)->dataChar().userType();

    while ( nrComponents() > 0 )
	data().delComponent( 0 );

    for ( int icomp=0; icomp<newnrcomps; icomp++ )
	data().addComponent( sz, DataCharacteristics(datarep) );
}


void SeisTrc::copyDataFrom( const SeisTrc& trc, int tarcomp, bool forcefloats )
{
    int startcomp = tarcomp < 0 ? 0 : tarcomp;
    int stopcomp = tarcomp < 0 ? trc.nrComponents()-1 : tarcomp;
    const int sz = trc.size();
    for ( int icomp=startcomp; icomp<=stopcomp; icomp++ )
    {
	DataCharacteristics dc = forcefloats
				? DataCharacteristics()
				: trc.data().getInterpreter(icomp)->dataChar();

	if ( nrComponents() <= icomp )
	    data().addComponent( sz, dc );
	else
	{
	    if ( data().getInterpreter(icomp)->dataChar() != dc )
		data().setComponent(dc,icomp);
	    if ( data_.size(icomp) != sz )
		data().getComponent(icomp)->reSize( sz );
	}
	OD::memCopy( data().getComponent(icomp)->data(),
		trc.data().getComponent(icomp)->data(),
		sz * (int)dc.nrBytes() );
    }
}


float SeisTrcValueSeries::value( od_int64 idx ) const
{
    return trc_.get((int) idx,icomp_);
}


void SeisTrcValueSeries::setValue( od_int64 idx,float v )
{
    trc_.set((int) idx,v,icomp_);
}


float* SeisTrcValueSeries::arr()
{
    return trc_.arr( icomp_ );
}


bool SeisTrcValueSeries::copytoArray( Array1D<float>& seistrcarr )
{
    return trc_.copytoArray( seistrcarr, icomp_ );
}


const float* SeisTrcValueSeries::arr() const
{ return const_cast<SeisTrcValueSeries*>( this )->arr(); }
