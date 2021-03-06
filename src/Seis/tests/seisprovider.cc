/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2016
-*/

#include "seisprovidertester.h"
#include "seisprovider.h"
#include "trckey.h"

#include "testprog.h"
#include "moddepmgr.h"

/*!

Test program for Seis::Provider for testing standard usages of provider
class for all four geom types: Vol, Line, VolPS, LinePS. Uses
Seis::ProviderTester for testing various functionalities.

The required input is the output generated by the test_seisstorer program.

*/

mUseType( Seis, GeomType );
mUseType( Seis, ProviderTester );


#define mGetTester(gt) \
    ProviderTester tester( gt, quiet_ ); \
    logStream() << "\n---- " << Seis::nameOf(gt) << "----\n" << od_endl

#define mTestGetAt(tk,exist) \
if ( !tester.testGetAt(tk,exist) ) \
    return false;

#define mTestGetAll() \
if ( !tester.testGetAll() ) \
    return false;

#define mTestIOParUsage() \
if ( !tester.testIOParUsage() ) \
    return false;

#define mTestComponentSelection() \
if ( !tester.testComponentSelection() ) \
    return false;

#define mDo3DTests() \
    mTestGetAt( TrcKey(tester.existingbid_), true ); \
    mTestGetAt( TrcKey(tester.nonexistingbid_), false ); \
    mTestGetAll()

#define mDo2DTests() \
    mTestGetAt( TrcKey(gid,tnr), true ); \
    mTestGetAt( TrcKey(gid,tester.nonexistingtrcnr_), false ); \
    mTestGetAt( TrcKey(tester.nonexistinggeomid_,tnr), false ); \
    mTestGetAll()

#define mStartPreLoad3D() \
    auto* pl = tester.preLoad( tester.prov_->as3D()->cubeSubSel() ); \
    logStream() << "\n\t>> PreLoad" << od_endl \

#define mStartPreLoad2D() \
    auto* pl = tester.preLoad( tester.prov_->as2D()->lineSubSelSet() ); \
    logStream() << "\n\t>> PreLoad" << od_endl \

#define mStopPreLoad() \
    logStream() << "\t>> Unload\n" << od_endl; \
    tester.removePreLoad( pl )


static bool doFull3DTest( GeomType gt )
{
    mGetTester( gt );

    mDo3DTests();
    mStartPreLoad3D();
    mDo3DTests();
    mStopPreLoad();

    return tester.testIOParUsage();
}


static bool doFull2DTest( GeomType gt )
{
    mGetTester( gt );

    const auto gid = tester.existinggeomid_;
    const auto tnr = tester.existingTrcNr( gid );

    mDo2DTests();
    mStartPreLoad2D();
    mDo2DTests();
    mStopPreLoad();

    return tester.testIOParUsage();
}


static bool testVol()
{
    return doFull3DTest( Seis::Vol );
}


static bool testPS3D()
{
    return doFull3DTest( Seis::VolPS );
}


static bool testLine()
{
    return doFull2DTest( Seis::Line );
}


static bool testPS2D()
{
    /*TODO enable
    return doFull2DTest( Seis::LinePS );
    */
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();
    OD::ModDeps().ensureLoaded("Seis");

    if ( !testVol() )
	ExitProgram( 1 );
    if ( !testLine() )
	ExitProgram( 1 );
    if ( !testPS3D() )
	ExitProgram( 1 );
    if ( !testPS2D() )
	ExitProgram( 1 );

    return ExitProgram( 0 );
}
