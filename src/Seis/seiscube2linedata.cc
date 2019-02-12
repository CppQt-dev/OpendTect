/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Apr 2010
-*/


#include "seiscube2linedata.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "seisprovider.h"
#include "seisselection.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisinfo.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "ioobj.h"

Seis2DFrom3DExtractor::Seis2DFrom3DExtractor(
			const IOObj& cubein, const IOObj& dsout,
			const GeomIDSet& geomids )
    : Executor("Extract 3D data into 2D lines")
    , wrr_(*new SeisTrcWriter(&dsout))
    , geomids_(geomids)
    , prov_(0)
    , nrdone_(0)
    , totalnr_(0)
    , curgeom2d_(0)
    , curlineidx_(-1)
    , curtrcidx_(-1)
{
    uiRetVal uirv;
    prov_ = Seis::Provider::create( cubein, &uirv );
    if ( !prov_ )
	{ msg_ = uirv; return; }

    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	const auto& geom2d = Survey::Geometry::get2D( geomids[idx] );
	totalnr_ += geom2d.data().positions().size();
    }
}


Seis2DFrom3DExtractor::~Seis2DFrom3DExtractor()
{
    delete &wrr_; delete prov_;
}


Pos::GeomID Seis2DFrom3DExtractor::curGeomID() const
{
    return geomids_.validIdx(curlineidx_) ? geomids_[curlineidx_] : mUdfGeomID;
}

#define mErrRet(s) { msg_ = s; return ErrorOccurred(); }

int Seis2DFrom3DExtractor::goToNextLine()
{
    if ( !prov_ )
	return ErrorOccurred();

    curlineidx_++;
    if ( curlineidx_ >= geomids_.size() )
	return Finished();

    const auto geomid = geomids_[curlineidx_];
    curgeom2d_ = &Survey::Geometry::get2D( geomid );
    if ( curgeom2d_->isEmpty() )
	mErrRet(tr("Line geometry not available"))

    Seis::SelData* newseldata = Seis::SelData::get( Seis::Range );
    newseldata->setGeomID( geomids_[curlineidx_] );
    wrr_.setSelData( newseldata );
    curtrcidx_ = 0;
    return MoreToDo();
}


int Seis2DFrom3DExtractor::nextStep()
{
    if ( !curgeom2d_ || curtrcidx_ >= curgeom2d_->data().positions().size() )
	return goToNextLine();

    return handleTrace();
}


int Seis2DFrom3DExtractor::handleTrace()
{
    SeisTrc trc;
    const uiRetVal uirv = prov_->getNext( trc );
    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	    return Finished();

	mErrRet( uirv );
    }

    const PosInfo::Line2DPos& curpos =
		curgeom2d_->data().positions()[curtrcidx_++];
    TrcKey curtrckey( curgeom2d_->geomID(), curpos.nr_ );
    trc.info().trckey_ = curtrckey;
    trc.info().coord_ = curpos.coord_;
    if ( !wrr_.put(trc) )
	mErrRet( wrr_.errMsg() )

    nrdone_++;
    return MoreToDo();
}
