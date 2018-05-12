#include "prog.h"
#include "grav.h"


int main( int, char** )
{
    OD::SetRunContext( OD::TestProgCtxt );
    Grav::Block blk( Coord(-100,100), Coord( 300, -100 ),
			Interval<double>(50,150) );

    Coord3 c3( 0, 0, 0 );

    for ( int idx=0; idx<2001; idx++ )
    {
	c3.x = -1000 + idx;
	const double val = 1e12 * blk.calcValue(c3);
	od_cout() << c3.x << od_tab << val << od_endl;
    }

    return 0;
}

