/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : June 2001
-*/
 
static const char* rcsID = "$Id: nlacrdesc.cc,v 1.6 2005-01-31 16:03:19 bert Exp $";

#include "nlacrdesc.h"
#include "featset.h"
#include "featsettr.h"
#include "ioobj.h"
#include "ioman.h"
#include "iodir.h"
#include "iopar.h"
#include "errh.h"
#include "ptrman.h"
#include "stats.h"

NLACreationDesc& NLACreationDesc::operator =(
	const NLACreationDesc& sd )
{
    if ( this != &sd )
    {
	design = sd.design;
	deepCopy( outids, sd.outids );
	doextraction = sd.doextraction;
	fsid = sd.fsid;
	ratiotst = sd.ratiotst;
	isdirect = sd.isdirect;
    }
    return *this;
}


void NLACreationDesc::clear()
{
    design.clear();
    deepErase(outids);
    fsid = "";
    doextraction = true;
    isdirect = false;
}


const char* NLACreationDesc::transferData( const ObjectSet<FeatureSet>& fss,
	FeatureSet& fstrain, FeatureSet& fstest ) const
{
    fstrain.erase(); fstest.erase();
    const char* res = 0;
    const int nrout = fss.size();
    if ( !nrout )
	{ return "Internal: No input Feature Sets to transfer data from"; }

    bool writevecs = doextraction && fsid != "";

    // For direct prediction, the sets are ready. If not, add a FeatureDesc
    // for each output node
    fstrain.copyStructure( *fss[0] );
    if ( doextraction && !isdirect )
    {
        for ( int iout=0; iout<nrout; iout++ )
            fstrain.descs() += new FeatureDesc( *outids[iout] );
    }
    fstest.copyStructure( fstrain );

    // Get the data into train and test set
    for ( int idx=0; idx<fss.size(); idx++ )
    {
        if ( !addFeatData(fstrain,fstest,*fss[idx],nrout,idx) )
        {
            BufferString msg( "No values collected for '" );
            msg += IOM().nameOf( *outids[idx] );
            msg += "'";
            UsrMsg( msg );
        }
	if ( !idx )
	{
	    fstrain.pars().clear();
	    fstrain.pars().merge( fss[idx]->pars() );
	}
	 
    }

    if ( res && *res ) return res;

    // Correct for shortcomings of random. Make sure ratio test/train is
    // almost exactly as specified
    const int nrtest = (int)((fstrain.size()+fstest.size()) * ratiotst + .5);
    int nrdiff = nrtest - fstest.size();
    FeatureSet& to = nrdiff < 0 ? fstrain : fstest;
    FeatureSet& from = nrdiff > 0 ? fstrain : fstest;
    while ( nrdiff && from.size() )
    {
	int fsidx = Stat_getIndex( from.size() );
	FeatureVec* fv = from[fsidx];
	from -= fv;
	to += fv;
	nrdiff = nrtest - fstest.size();
    }

    return res;
}


int NLACreationDesc::addFeatData( FeatureSet& fstrain,
				FeatureSet& fstest, const FeatureSet& fs,
				int nrout, int iout ) const
{
    FeatureVec fvin( FVPos(0,0) );
    fs.startGet();
    int nradded = 0;
    while ( 1 )
    {
	int res = fs.get( fvin );
	if ( res < 0 ) break; if ( res == 0 ) continue;

	FeatureVec* newfv = new FeatureVec( fvin );
	if ( !isdirect )
	{
	    for ( int idx=0; idx<nrout; idx++ )
		*newfv += idx == iout ? 1 : 0;
	}
	FeatureSet& addset = Stat_getRandom() < ratiotst ? fstest : fstrain;
	addset += newfv;
	nradded++;
    }
    return nradded;
}
