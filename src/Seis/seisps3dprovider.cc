/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/

#include "seisps3dprovider.h"
#include "seisfetcher.h"
#include "seispreload.h"
#include "seistrctr.h"
#include "uistrings.h"
#include "seispacketinfo.h"
#include "seisbuf.h"
#include "seisselection.h"
#include "seispsioprov.h"
#include "posinfo.h"
#include "prestackgather.h"
#include "keystrs.h"


namespace Seis
{

/*\brief Gets required traces from 3D data store.  */

class PS3DFetcher : public Fetcher3D
{ mODTextTranslationClass(Seis::PS3DFetcher);
public:

PS3DFetcher( PS3DProvider& p )
    : Fetcher3D(p)
    , rdr_(0)
    , cditer_(0)
{
}

~PS3DFetcher()
{
    delete cditer_;
    delete rdr_;
}

PS3DProvider& prov()
{
    return static_cast<PS3DProvider&>( prov_ );
}

const PS3DProvider& prov() const
{
    return static_cast<const PS3DProvider&>( prov_ );
}

    void		reset();
    void		findDataPack();
    TrcKeyZSampling	getDefaultCS() const;

    void		openStore();
    void		moveNextBinID();
    bool		prepGetAt(const BinID&);
    void		getAt(const BinID&,SeisTrcBuf&);
    void		getSingleAt(const BinID&,SeisTrc&);
    static SeisPS3DReader*  getReader(const IOObj&);

    void		get(const BinID&,SeisTrcBuf&);
    void		getSingle(const BinID&,SeisTrc&);
    void		getNext(SeisTrcBuf&);
    void		getNextSingle(SeisTrc&);

    SeisPS3DReader*		rdr_;
    RefMan<GatherSetDataPack>	dp_;
    PosInfo::CubeDataIterator*	cditer_;
    bool			atend_;

};

} // namespace Seis


void Seis::PS3DFetcher::reset()
{
    Fetcher3D::reset();
    delete cditer_; cditer_ = 0;
    delete rdr_; rdr_ = 0;
    atend_ = false;
    dp_ = 0;

    openStore();
}


void Seis::PS3DFetcher::findDataPack()
{
    RefMan<DataPack> dp = Seis::PLDM().get( prov().dbky_ );
    if ( !dp ) return;

    mDynamicCastGet(GatherSetDataPack*,gatherdp,dp.ptr());
    if ( !gatherdp ) return;

    dp_ = gatherdp;
}


TrcKeyZSampling Seis::PS3DFetcher::getDefaultCS() const
{
    TrcKeyZSampling ret;
    if ( !cditer_ || cditer_->cd_.isEmpty() )
	ret = TrcKeyZSampling( true );
    else
    {
	StepInterval<int> inlrg, crlrg;
	cditer_->cd_.getInlRange( inlrg, true );
	cditer_->cd_.getCrlRange( crlrg, true );
	ret.hsamp_.setInlRange( inlrg );
	ret.hsamp_.setCrlRange( crlrg );
    }
    return ret;
}



SeisPS3DReader* Seis::PS3DFetcher::getReader( const IOObj& ioobj )
{
    return SPSIOPF().get3DReader( ioobj );
}


void Seis::PS3DFetcher::openStore()
{
    if ( !fillIOObj() )
	return;

    rdr_ = getReader( *ioobj_ );
    if ( !rdr_ )
    {
	uirv_ = tr("Cannot find a reader for this type of data store");
	return;
    }

    cditer_ = new PosInfo::CubeDataIterator( rdr_->posData() );
    moveNextBinID();
    if ( atend_ )
	uirv_ = tr("No selected postion present in data store");
    else
	findDataPack();
}


void Seis::PS3DFetcher::moveNextBinID()
{
    while ( true )
    {
	atend_ = !cditer_->next( nextbid_ );
	if ( atend_ || !prov().seldata_ || prov().seldata_->isOK(nextbid_) )
	    break;
    }
}


bool Seis::PS3DFetcher::prepGetAt( const BinID& bid )
{
    if ( !rdr_ )
    {
	if ( uirv_.isOK() )
	    uirv_.set( uiStrings::phrInternalErr("PS3D Reader not created") );
	return false;
    }

    nextbid_ = bid;
    return true;
}


void Seis::PS3DFetcher::getAt( const BinID& bid, SeisTrcBuf& tbuf )
{
    tbuf.deepErase();
    if ( !prepGetAt(bid) )
	return;

    uirv_.setEmpty();
    if ( dp_ )
	dp_->fillGatherBuf( tbuf, bid );
    else if ( !rdr_->getGather(nextbid_,tbuf) )
	{ uirv_.set( rdr_->errMsg() ); return; }

    moveNextBinID();
}


void Seis::PS3DFetcher::getSingleAt( const BinID& bid, SeisTrc& trc )
{
    if ( !prepGetAt(bid) )
	return;

    uirv_.setEmpty();
    const int offsetidx = prov().haveSelComps() ? prov().selcomps_[0] : 0;
    const SeisTrc* seistrc = 0;
    if ( dp_ )
	seistrc = dp_->getTrace( bid, offsetidx );
    else if ( !seistrc )
    {
	seistrc = rdr_->getTrace( nextbid_, offsetidx );
	if ( !seistrc )
	    { uirv_.set( rdr_->errMsg() ); return; }
    }

    trc = *seistrc;
    moveNextBinID();
}


void Seis::PS3DFetcher::get( const BinID& bid, SeisTrcBuf& tbuf )
{
    getAt( bid, tbuf );
}


void Seis::PS3DFetcher::getSingle( const BinID& bid, SeisTrc& trc )
{
    getSingleAt( bid, trc );
}


void Seis::PS3DFetcher::getNext( SeisTrcBuf& tbuf )
{
    if ( atend_ )
	uirv_.set( uiStrings::sFinished() );
    else
	getAt( nextbid_, tbuf );
}


void Seis::PS3DFetcher::getNextSingle( SeisTrc& trc )
{
    if ( atend_ )
	uirv_.set( uiStrings::sFinished() );
    else
	getSingleAt( nextbid_, trc );
}



Seis::PS3DProvider::PS3DProvider()
    : fetcher_(*new PS3DFetcher(*this))
{
}


Seis::PS3DProvider::~PS3DProvider()
{
    delete &fetcher_;
}


SeisPS3DReader* Seis::PS3DProvider::mkReader() const
{
    PtrMan<IOObj> ioobj = fetcher_.getIOObj();
    return ioobj ? PS3DFetcher::getReader( *ioobj ) : 0;
}


int Seis::PS3DProvider::gtNrOffsets() const
{
    PtrMan<SeisPS3DReader> rdrptrman;
    const SeisPS3DReader* rdr = 0;
    if ( fetcher_.rdr_ )
	rdr = fetcher_.rdr_;
    else
    {
	rdrptrman = mkReader();
	rdr = rdrptrman;
    }

    if ( !rdr )
	return 1;

    const PosInfo::CubeData& cd = rdr->posData();
    if ( cd.size() < 1 )
	return 1;
    const int linenr = cd.size() / 2;
    const PosInfo::LineData& ld = *cd[linenr];
    const int segnr = ld.segments_.size() / 2;
    const BinID bid( ld.linenr_, ld.segments_[segnr].center() );
    SeisTrcBuf tbuf( true );
    if ( !rdr->getGather(bid,tbuf) || tbuf.isEmpty() )
	return 1;

    return tbuf.size();
}


bool Seis::PS3DProvider::getRanges( TrcKeyZSampling& cs ) const
{
    SeisPS3DReader* rdr = fetcher_.rdr_;
    PtrMan<SeisPS3DReader> rdrptrman;
    if ( !rdr )
    {
	rdrptrman = mkReader();
	rdr = rdrptrman;
    }
    if ( !rdr )
    {
	cs = TrcKeyZSampling(true);
	return false;
    }

    cs.zsamp_ = rdr->getZRange();
    const PosInfo::CubeData& cd = rdr->posData();
    StepInterval<int> rg;
    cd.getInlRange( rg ); cs.hsamp_.setInlRange( rg );
    cd.getCrlRange( rg ); cs.hsamp_.setCrlRange( rg );

    return true;
}


void Seis::PS3DProvider::getGeometryInfo( PosInfo::CubeData& cd ) const
{
    if ( fetcher_.cditer_ )
	cd = fetcher_.cditer_->cd_;
    else
    {
	cd.setEmpty();
	PtrMan<SeisPS3DReader> rdr = mkReader();
	if ( rdr )
	    cd = rdr->posData();
	else
	    cd.setEmpty();
    }
}


void Seis::PS3DProvider::doFillPar( IOPar& iop, uiRetVal& uirv ) const
{
    Seis::Provider3D::doFillPar( iop, uirv );

    IOPar par;
    fetcher_.doFillPar( iop, uirv );
    iop.merge( par );
}


void Seis::PS3DProvider::doUsePar( const IOPar& iop, uiRetVal& uirv )
{
    Seis::Provider3D::doUsePar( iop, uirv );

    fetcher_.doUsePar( iop, uirv );
}


void Seis::PS3DProvider::doReset( uiRetVal& uirv ) const
{
    fetcher_.reset();
    uirv = fetcher_.uirv_;
}


TrcKey Seis::PS3DProvider::doGetCurPosition() const
{
    return TrcKey( fetcher_.nextbid_ );
}


bool Seis::PS3DProvider::doGoTo( const TrcKey& tk )
{
    return fetcher_.prepGetAt( tk.binID() );
}


void Seis::PS3DProvider::doGetNextGather( SeisTrcBuf& tbuf,
					  uiRetVal& uirv ) const
{
    fetcher_.getNext( tbuf );
    uirv = fetcher_.uirv_;
}


void Seis::PS3DProvider::doGetGather( const TrcKey& trcky, SeisTrcBuf& tbuf,
				  uiRetVal& uirv ) const
{
    fetcher_.get( trcky.binID(), tbuf );
    uirv = fetcher_.uirv_;
}


void Seis::PS3DProvider::doGetNext( SeisTrc& trc, uiRetVal& uirv ) const
{
    fetcher_.getNextSingle( trc );
    uirv = fetcher_.uirv_;
}


void Seis::PS3DProvider::doGet( const TrcKey& trcky, SeisTrc& trc,
				uiRetVal& uirv ) const
{
    fetcher_.getSingle( trcky.binID(), trc );
    uirv = fetcher_.uirv_;
}
