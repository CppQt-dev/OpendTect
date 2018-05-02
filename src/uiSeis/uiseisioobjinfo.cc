/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2004
________________________________________________________________________

-*/

#include "uiseisioobjinfo.h"

#include "cbvsreadmgr.h"
#include "ioobj.h"
#include "ptrman.h"
#include "seisprovider.h"
#include "separstr.h"
#include "systeminfo.h"
#include "od_ostream.h"
#include "uimsg.h"
#include "od_strstream.h"
#include "survinfo.h"


uiSeisIOObjInfo::uiSeisIOObjInfo( const IOObj& ioobj, bool errs )
	: sii(ioobj)
	, doerrs(errs)
{
}


uiSeisIOObjInfo::uiSeisIOObjInfo( const DBKey& key, bool errs )
	: sii(key)
	, doerrs(errs)
{
}


#define mChk(ret) if ( !isOK() ) return ret

bool uiSeisIOObjInfo::provideUserInfo() const
{
    mChk(false);
    if ( is2D() )
	return provideUserInfo2D();

    uiRetVal uirv;
    PtrMan<Seis::Provider> prov = Seis::Provider::create( ioObj()->key(),
							  &uirv );
    if ( !prov )
    {
	if ( !uirv.isOK() )
	    uiMSG().error( uirv );
	return false;
    }

    const Seis::GeomType gt = prov->geomType();
    const uiString datanm( Seis::dataName(gt) );
    const Seis::Provider3D& prov3d = *(Seis::Provider3D*)prov.ptr();
    PosInfo::CubeData cd;
    prov3d.getGeometryInfo( cd );
    if ( cd.isEmpty() )
    {
	uiMSG().error( tr("The %1 is empty").arg( datanm ) );
	return false;
    }

    uiStringSet msgs;
    msgs.add( tr("The %1 is usable").arg( datanm ) );
    msgs.add( tr("Total number of traces: %1").arg( cd.totalSize() ) );
    uiMSG().message( msgs.cat(uiString::CloseLine,uiString::AfterEmptyLine) );
    return true;
}


bool uiSeisIOObjInfo::provideUserInfo2D( const TypeSet<Pos::GeomID>* sel ) const
{
    mChk(false);
    //TODO: Handle errors

    TypeSet<Pos::GeomID> geomids;
    if ( sel )
	geomids = *sel;
    else
	sii.getGeomIDs( geomids );

    const int nrlines = geomids.size();
    const BufferString datanm( sii.ioObj()->name() );
    uiString msg = nrlines < 2 ?
	tr("The following line was added to dataset %2:").arg(datanm)
	: tr("%1 lines were added to dataset %2:").arg(nrlines).arg(datanm);
    const uiString zunitstr( SI().zUnitString() );
    const float zfac = SI().showZ2UserFactor();
    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	const BufferString linenm = Survey::GM().getName( geomids[idx] );
	StepInterval<int> trcrg( 0, 0, 1 );
	StepInterval<float> zrg( 0.f, 0.f, 1.f );
	if ( sii.getRanges(geomids[idx],trcrg,zrg) )
	    zrg.scale( zfac );

	msg.appendPhrase( tr("Line: %1, Trace range: %2 to %3 (step %4), "
		    "Z range %5: %6 to %7 (step %8)")
		.arg(linenm).arg(trcrg.start).arg(trcrg.stop).arg(trcrg.step)
		.arg(zunitstr).arg(zrg.start).arg(zrg.stop).arg(zrg.step) );
    }

    uiMSG().message( msg);
    return true;
}


bool uiSeisIOObjInfo::checkSpaceLeft( const SeisIOObjInfo::SpaceInfo& si ) const
{
    mChk(false);

    const int szmb = expectedMBs( si );
    if ( szmb < 0 ) // Unknown, but probably small
	return true;

    const int avszmb = System::getFreeMBOnDisk( *ioObj() );
#ifdef __win__
    const int szgb = szmb / 1024;
    if ( szgb >= 4 )
    {
	BufferString fsysname = System::getFileSystemName( ioObj()->dirName() );
	if ( fsysname == "FAT32" )
	{
	    uiMSG().error( tr("Target directory has a FAT32 File System.\n"
			      "Files larger than 4GB are not supported") );
	    return false;
	}
    }
#endif

    if ( avszmb == 0 )
    {
	if ( !doerrs ) return false;
	if ( !uiMSG().askContinue( tr("The output disk seems to be full.\n\n"
				      "Do you want to continue?") ) )
	    return false;
    }
    else if ( szmb > avszmb )
    {
	if ( !doerrs ) return false;
	uiString msg = tr( "The new cube size may exceed the space "
		       "available on disk:\n%1\n\nDo you want to continue?" );

	uiString explanationmsg = avszmb == 0 ? tr("The disk seems to be full!")
					      : tr("\nEstimated size: %1 MB\n"
						   "Available on disk: %2 MB")
					      .arg(szmb).arg(avszmb);

	msg.arg(explanationmsg);

	if ( !uiMSG().askContinue( msg ) )
	    return false;
    }
    return true;
}
