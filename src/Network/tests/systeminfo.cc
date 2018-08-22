/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2015
 * FUNCTION :
-*/


#include "systeminfo.h"

#include "testprog.h"


bool testSystemInfo()
{
    //Dummy test in a sense, as we cannot check the result
    mRunStandardTest( System::macAddressHash(),
		     "macAddressHash" );
    const BufferString localaddress = System::localAddress();
    mRunStandardTest( !localaddress.isEmpty(),
		     "Local address" );

    return true;
}

int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testSystemInfo() )
	return 1;

    return 0;
}
