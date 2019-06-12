/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          February 2003
________________________________________________________________________

-*/


#include "freqfilterattrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "fftfilter.h"
#include "fourier.h"
#include "genericnumer.h"
#include "timeser.h"
#include "transform.h"
#include "ptrman.h"
#include "arrayndinfo.h"
#include "survinfo.h"

#include <math.h>


#define mMINNRSAMPLES		100


namespace Attrib
{

mAttrDefCreateInstance(FreqFilter)

void FreqFilter::initClass()
{
    mAttrStartInitClassWithUpdate

    //Note: Ordering must be the same as numbering!
    EnumParam* filtertype_ = new EnumParam( filtertypeStr() );
    filtertype_->setEnums( FFTFilter::TypeDef() );
    filtertype_->setDefaultValue( FFTFilter::LowPass );
    desc->addParam( filtertype_ );

    FloatParam* minfreq_ = new FloatParam( minfreqStr() );
    minfreq_->setLimits( Interval<float>(0,mUdf(float)) );
    minfreq_->setDefaultValue(15);
    desc->addParam( minfreq_ );

    FloatParam* maxfreq_ = new FloatParam( maxfreqStr() );
    maxfreq_->setLimits( Interval<float>(0,mUdf(float)) );
    maxfreq_->setDefaultValue(50);
    desc->addParam( maxfreq_ );

    IntParam* nrpoles_ = new IntParam( nrpolesStr() );
    nrpoles_->setLimits( Interval<int>(2,20) );
    nrpoles_->setDefaultValue( 4 );
    desc->addParam( nrpoles_ );

    BoolParam* isfftfilter_ = new BoolParam( isfftfilterStr() );
    isfftfilter_->setDefaultValue( true );
    desc->addParam( isfftfilter_ );

    StringParam* window_ = new StringParam( windowStr() );
    window_->setDefaultValue( "CosTaper" );
    desc->addParam( window_ );

    FloatParam* variable_ = new FloatParam( paramvalStr() );
    const float defval = 0.95f;
    variable_->setDefaultValue( defval );
    variable_->setRequired( false );
    desc->addParam(variable_);

    BoolParam* isfreqtaper = new BoolParam( isfreqtaperStr() );
    isfreqtaper->setDefaultValue( true );
    desc->addParam( isfreqtaper );

    StringParam* fwindow_ = new StringParam( fwindowStr() );
    fwindow_->setDefaultValue( "CosTaper" );
    fwindow_->setRequired( false );
    desc->addParam( fwindow_ );

    FloatParam* freqf1_ = new FloatParam( freqf1Str() );
    freqf1_->setLimits( Interval<float>(0,mUdf(float)) );
    freqf1_->setDefaultValue( 10 );
    freqf1_->setRequired( false );
    desc->addParam( freqf1_ );

    FloatParam* freqf4_ = new FloatParam( freqf4Str() );
    freqf4_->setLimits( Interval<float>(0,mUdf(float)) );
    freqf4_->setDefaultValue( 60 );
    freqf4_->setRequired( false );
    desc->addParam( freqf4_ );

    desc->addOutputDataType( Seis::UnknownData );

    desc->addInput( InputSpec("Real data",true) );
    desc->addInput( InputSpec("Imag data",true) );

    desc->setIsSingleTrace( true );
    mAttrEndInitClass
}


void FreqFilter::updateDesc( Desc& desc )
{
    Attrib::ValParam* valpar = desc.getValParam( FreqFilter::paramvalStr() );
    Attrib::ValParam* winpar = desc.getValParam( FreqFilter::windowStr() );
    if ( !valpar || !winpar ) return;

    BufferString winstr = winpar->getStringValue();
    if ( winstr == "CosTaper5" )
    { winpar->setValue( "CosTaper" ); valpar->setValue( (float)0.95 ); }
    else if ( winstr == "CosTaper10" )
    { winpar->setValue( "CosTaper" ); valpar->setValue( (float)0.9 ); }
    else if ( winstr == "CosTaper20" )
    { winpar->setValue( "CosTaper" ); valpar->setValue( (float)0.8 ); }

    const ValParam* ftype = desc.getValParam( filtertypeStr() );
    const BufferString type = ftype->getStringValue();

    desc.setParamEnabled( minfreqStr(),
		      type != FFTFilter::toString( FFTFilter::LowPass ) );
    desc.setParamEnabled( maxfreqStr(),
		      type != FFTFilter::toString( FFTFilter::HighPass ) );

    const bool isfft = desc.getValParam(isfftfilterStr())->getBoolValue();
    desc.setParamEnabled( nrpolesStr(), !isfft );
    desc.inputSpec(1).enabled_ = isfft;
}



FreqFilter::FreqFilter( Desc& ds )
    : Provider( ds )
    , fftsz_(-1)
    , minfreq_(mUdf(float))
    , maxfreq_(mUdf(float))
    , nrpoles_(mUdf(int))
    , signal_(0)
    , window_(0)
    , windowtype_(0)
    , variable_(mUdf(float))
    , freqf1_(mUdf(float))
    , freqf4_(mUdf(float))
{
    if ( !isOK() ) return;

    mGetEnum( filtertype_, filtertypeStr() );
    if ( filtertype_ != FFTFilter::LowPass )
	mGetFloat( minfreq_, minfreqStr() );

    if ( filtertype_ != FFTFilter::HighPass )
	mGetFloat( maxfreq_, maxfreqStr() );

    if ( filtertype_ != FFTFilter::HighPass &&
	 filtertype_ != FFTFilter::LowPass &&
	 mIsEqual( minfreq_, maxfreq_, 1e-3) )
	{ uirv_ = tr("Minimum and maximum frequencies are the same."); return; }

    mGetBool( isfftfilter_, isfftfilterStr() );
    if ( isfftfilter_ )
    {
	if ( filtertype_ != FFTFilter::LowPass )
	    mGetFloat( freqf1_, freqf1Str() );

	if ( filtertype_ != FFTFilter::HighPass )
	    mGetFloat( freqf4_, freqf4Str() );

	mGetString( windowtype_, windowStr() );
	mGetFloat( variable_, paramvalStr() );
    }
    else
	mGetInt( nrpoles_, nrpolesStr() );

    zmargin_ = Interval<int>( -mNINT32((float) mMINNRSAMPLES/2),
			       mNINT32((float) mMINNRSAMPLES/2) );
}


FreqFilter::~FreqFilter()
{
    delete window_;
}


bool FreqFilter::getInputOutput( int input, TypeSet<int>& res ) const
{
    return Provider::getInputOutput( input, res );
}


bool FreqFilter::getInputData( const BinID& relpos, int idx )
{
    redata_ = inputs_[0]->getData(relpos, idx);
    if ( !redata_ ) return false;

    realidx_ = getDataIndex( 0 );

    imdata_ = isfftfilter_ ? inputs_[1]->getData(relpos, idx) : 0;
    imagidx_ = isfftfilter_ ? getDataIndex( 1 ) : -1;
    if ( isfftfilter_ && !imdata_ ) return false;

    return true;
}


bool FreqFilter::computeData( const DataHolder& output, const BinID& relpos,
				int z0, int nrsamples, int threadid ) const
{
    if ( isfftfilter_ )
	const_cast<FreqFilter*>(this)->fftFilter( output, z0, nrsamples );
    else
	const_cast<FreqFilter*>(this)->butterWorthFilter(output,z0,nrsamples);

    return true;
}


void FreqFilter::butterWorthFilter( const DataHolder& output,
				    int z0, int nrsamples )
{
    int nrsamp = nrsamples;
    int csamp = z0;
    if ( nrsamples < mMINNRSAMPLES )
    {
	nrsamp = mMINNRSAMPLES;
	csamp = z0 - mNINT32((float) nrsamp/2) + mNINT32((float) nrsamples/2);
    }

    mAllocLargeVarLenArr( float, data, nrsamp );
    mAllocLargeVarLenArr( float, outp, nrsamp );

    for ( int idx=0; idx<nrsamp; idx++ )
    {
	int reidx = idx + csamp - z0;
	int checkidx = idx + csamp - redata_->z0_;
	int maxidx = redata_->nrsamples_-1;
	data[idx] = checkidx<0 ? getInputValue( *redata_, realidx_, 0, z0 )
	    : checkidx>maxidx ? getInputValue( *redata_, realidx_, maxidx, z0 )
			      : getInputValue( *redata_, realidx_, reidx, z0 );
    }

    if ( filtertype_ == FFTFilter::LowPass )
    {
	float cutoff = refzstep_ * maxfreq_;
	BFlowpass( nrpoles_, cutoff, nrsamp, data, outp );
	reverseArray( outp.ptr(), nrsamp );
	BFlowpass( nrpoles_, cutoff, nrsamp, outp, outp );
	reverseArray( outp.ptr(), nrsamp );
    }
    else if ( filtertype_ == FFTFilter::HighPass )
    {
	float cutoff = refzstep_ * minfreq_;
	BFhighpass( nrpoles_, cutoff, nrsamp, data, outp );
	reverseArray( outp.ptr(), nrsamp );
	BFhighpass( nrpoles_, cutoff, nrsamp, outp, outp );
	reverseArray( outp.ptr(), nrsamp );
    }
    else
    {
	float cutoff = refzstep_ * maxfreq_;
	mAllocLargeVarLenArr( float, tmp, nrsamp );
	BFlowpass( nrpoles_, cutoff, nrsamp, data, tmp );
	cutoff = refzstep_ * minfreq_;
	BFhighpass( nrpoles_, cutoff, nrsamp, tmp, outp );
	reverseArray( outp.ptr(), nrsamp );
	cutoff = refzstep_ * maxfreq_;
	BFlowpass( nrpoles_, cutoff, nrsamp, outp, outp );
	cutoff = refzstep_ * minfreq_;
	BFhighpass( nrpoles_, cutoff, nrsamp, outp, outp );
	reverseArray( outp.ptr(), nrsamp );
    }

    if ( nrsamples < mMINNRSAMPLES )
    {
	int offset = mNINT32((float) nrsamp/2) - mNINT32((float) nrsamples/2);
	for ( int idx=0; idx<nrsamples; idx++ )
	    setOutputValue( output, 0, idx, z0, outp[offset - 1 + idx] );
    }
    else
    {
	float* out = output.series(0)->arr();
	OD::memCopy(out,outp,nrsamp*sizeof(float));
    }
}


void FreqFilter::fftFilter( const DataHolder& output, int z0, int nrsamples )
{
    signal_.setInfo( Array1DInfoImpl( nrsamples ) );
    bool isimagudf = false;
    Array1DImpl<float> realsignal( nrsamples );
    for ( int idx=0; idx<nrsamples; idx++ )
    {
	const float real = getInputValue( *redata_, realidx_, idx, z0 );
	const float imag = getInputValue( *imdata_, imagidx_, idx, z0 );
	signal_.set( idx, float_complex(real,-imag) );
	if ( imag > 1e20 )
	    isimagudf = true;

	realsignal.set( idx, real );
    }

    FFTFilter filter( nrsamples, refzstep_);
    if ( windowtype_ != "None" )
	if ( !filter.setTimeTaperWindow(nrsamples,windowtype_,variable_) )
	    return;

    if ( filtertype_ == FFTFilter::HighPass )
	filter.setHighPass( freqf1_, minfreq_ );
    else if ( filtertype_ == FFTFilter::LowPass )
	filter.setLowPass( maxfreq_, freqf4_ );
    else if ( filtertype_ == FFTFilter::BandPass )
	filter.setBandPass( freqf1_, minfreq_, maxfreq_, freqf4_ );

    if ( isimagudf )
    {
	if ( !filter.apply(realsignal) )
	    return;
    }
    else
	if ( !filter.apply(signal_) )
	    return;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	if ( isimagudf )
	    setOutputValue( output, 0, idx, z0, realsignal.get( idx ) );
	else
	    setOutputValue( output, 0, idx, z0, signal_.get( idx ).real() );
    }
}


const Interval<int>* FreqFilter::desZSampMargin(int input, int output) const
{
    return &zmargin_;
}


};//namespace
