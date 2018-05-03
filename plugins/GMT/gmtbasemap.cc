/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Jube 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "bufstringset.h"
#include "color.h"
#include "draw.h"
#include "filepath.h"
#include "gmtbasemap.h"
#include "initgmtplugin.h"
#include "keystrs.h"
#include "strmprov.h"
#include "survinfo.h"
#include "od_iostream.h"


static const int cTitleBoxHeight = 4;
static const int cTitleBoxWidth = 8;

namespace GMT {

static void setDefaults( BufferString& comm )
{
    const bool moderngmt = GMT::hasModernGMT();

    comm.add( moderngmt ? " --FONT_TITLE" : " --HEADER_FONT_SIZE" ).add( "=24");
    comm.add( " --" ).add( moderngmt ? "FONT_ANNOT" : "ANNOT_FONT_SIZE" )
	.add( "_PRIMARY=12" );
    if ( !moderngmt )
	comm.add( " --FRAME_PEN=2p" );

    comm.add( " --" )
	      .add( moderngmt ? "FONT_LABEL" : "LABEL_FONT_SIZE" )
	      .add( "=" ).add( 12 ).add( " --")
      .add( moderngmt ? "FONT_ANNOT_PRIMARY" : "ANNOT_FONT_SIZE_PRIMARY" )
	      .add( "=10 " );
}

};

int GMTBaseMap::factoryid_ = -1;

void GMTBaseMap::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Basemap", GMTBaseMap::createInstance );
}

GMTPar* GMTBaseMap::createInstance( const IOPar& iop )
{
    return new GMTBaseMap( iop );
}


bool GMTBaseMap::execute( od_ostream& strm, const char* fnm )
{
    strm << "Creating the Basemap ...  ";
    FixedString maptitle = find( ODGMT::sKeyMapTitle() );
    Interval<float> lblintv;
    if ( !get(ODGMT::sKeyLabelIntv(),lblintv) )
	mErrStrmRet("Incomplete data for basemap creation")

    bool closeps = false, dogrid = false;
    getYN( ODGMT::sKeyClosePS(), closeps );
    getYN( ODGMT::sKeyDrawGridLines(), dogrid );


    BufferString comm = "psbasemap ";
    BufferString rangestr; mGetRangeProjString( rangestr, "X" );
    comm += rangestr; comm += " -Ba";
    comm += lblintv.start;
    if ( dogrid ) { comm += "g"; comm += lblintv.start; }
    comm += "/a"; comm += lblintv.stop;
    if ( dogrid ) { comm += "g"; comm += lblintv.stop; }
    comm.add( ":\"." ).add( maptitle ).add( "\":" );
    comm.add( GMT::hasModernGMT() ? " --MAP_ANNOT_ORTHO=z"
				  : " --Y_AXIS_TYPE=ver_text" );
    GMT::setDefaults( comm );

    const bool moderngmt = GMT::hasModernGMT();

    get( ODGMT::sKeyMapDim(), mapdim );
    const float xmargin = mapdim.start > 30 ? mapdim.start/10 : 3;
    const float ymargin = mapdim.stop > 30 ? mapdim.stop/10 : 3;
    comm.add( moderngmt ? " --MAP_ORIGIN_X=" : " --X_ORIGIN=" );
    comm.add( xmargin ).add( "c" ).addSpace();
    comm.add( moderngmt ? "--MAP_ORIGIN_Y=" : " --Y_ORIGIN=" );
    comm.add( ymargin ).add( "c" ).addSpace();
    comm.add( moderngmt ? "--PS" : "--PAPER" ).add( "_MEDIA=Custom_" );
    float pagewidth = mapdim.start + 5 * xmargin;
    const float pageheight = mapdim.stop + 3 * ymargin;
    comm += pageheight < 21 ? 21 : pageheight; comm += "cx";
    comm += pagewidth < 21 ? 21 : pagewidth; comm += "c -K ";

    comm += "1> "; comm += fileName( fnm );
    if ( !execCmd(comm,strm) )
	mErrStrmRet("Failed to create Basemap")

    strm << "Done" << od_endl;

    strm << "Posting title box ...  ";
    comm.set( "@pslegend " ).add( rangestr ).add( " -F -O -Dx" );
    comm.add( mapdim.start + xmargin ).add( "c/" ).add( 0 ).add( "c" );
    comm.add( moderngmt ? "+w" : "/" ).add( cTitleBoxWidth ).add( "c/" );
    comm.add( cTitleBoxHeight ).add( "c" ).add( moderngmt ? "+j" : "/" );
    comm.add( "BL " );
    if ( !closeps ) comm.add( "-K " );

    comm.add( "-UBL/" );
    comm.add( moderngmt ? mapdim.start + xmargin : 0 ).add( "c/0c " );
    comm += "1>> "; comm += fileName( fnm );
    od_ostream procstrm = makeOStream( comm, strm );
    if ( !procstrm.isOK() ) mErrStrmRet("Failed to overlay title box")

    procstrm << "H 16 4 " << maptitle.buf() << "\n";
    procstrm << "G 0.5l" << "\n";
    int scaleval = 1;
    get( ODGMT::sKeyMapScale(), scaleval );
    procstrm << "L 10 4 C Scale  1:" << scaleval << "\n";
    procstrm << "D 0 1p" << "\n";
    BufferStringSet remset;
    get( ODGMT::sKeyRemarks(), remset );
    for ( int idx=0; idx<remset.size(); idx++ )
	procstrm << "L 12 4 C " << remset.get(idx) << "\n";

    strm << "Done" << od_endl;
    return true;
}


int GMTLegend::factoryid_ = -1;

void GMTLegend::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Legend", GMTLegend::createInstance );
}

GMTPar* GMTLegend::createInstance( const IOPar& iop )
{
    return new GMTLegend( iop );
}


bool GMTLegend::execute( od_ostream& strm, const char* fnm )
{
    strm << "Posting legends ...  ";
    bool hascolbar = false;
    ObjectSet<IOPar> parset;
    int parwithcolorbar = -1;
    for ( int idx=0; idx<100; idx++ )
    {
	IOPar* par = subselect( idx );
	if ( !par ) break;

	if ( par->find(ODGMT::sKeyPostColorBar()) )
	    parwithcolorbar = idx;

	parset += par;
    }

    const bool moderngmt = GMT::hasModernGMT();

    BufferString rangestr; mGetRangeProjString( rangestr, "X" );
    get( ODGMT::sKeyMapDim(), mapdim );
    const float xmargin = mapdim.start > 30 ? mapdim.start/10 : 3;
    const float ymargin = mapdim.stop > 30 ? mapdim.stop/10 : 3;
    if ( parwithcolorbar >= 0 )
    {
	hascolbar = true;
	const IOPar* par = parset[parwithcolorbar];
	StepInterval<float> rg;
	par->get( ODGMT::sKeyDataRange(), rg );
	FilePath fp( fnm );
	fp.setExtension( "cpt" );
	BufferString colbarcomm( "psscale" );
	GMT::setDefaults( colbarcomm );
	colbarcomm.add( "-D" );
	colbarcomm += mapdim.start + xmargin; colbarcomm += "c/";
	colbarcomm += 1.2 * ymargin + cTitleBoxHeight; colbarcomm += "c/";
	colbarcomm += 2 * ymargin; colbarcomm += "c/";
	colbarcomm += xmargin / 2; colbarcomm += "c -O -C";
	colbarcomm += fileName( fp.fullPath() ); colbarcomm += " -B";
	colbarcomm += rg.step * 5; colbarcomm += ":\"";
	colbarcomm += par->find( sKey::Name() ); colbarcomm += "\":/:";
	colbarcomm += par->find( ODGMT::sKeyAttribName() );
	colbarcomm += ": -K 1>> "; colbarcomm += fileName( fnm );
	if ( !execCmd(colbarcomm,strm) )
	    mErrStrmRet("Failed to post color bar")

	StreamProvider( fp.fullPath() ).remove();
    }

    const int nritems = parset.size();
    BufferString comm = "@pslegend ";
    comm.add( rangestr ).add( " -O -Dx" );
    comm.add( mapdim.start + xmargin ).add( "c/" );
    comm.add( ymargin / 2 + cTitleBoxHeight + ( hascolbar ? 2 * ymargin : 0 ) );
    comm.add( "c" ).add( moderngmt ? "+w" : "/" ).add( 10 ).add( "c/" );
    comm.add( nritems ? nritems : 1 ).add( "c" );
    comm.add( moderngmt ? "+j" : "/" ).add( "BL" ).addSpace();

    comm += "1>> "; comm += fileName( fnm );
    od_ostream procstrm = makeOStream( comm, strm );
    if ( !procstrm.isOK() ) mErrStrmRet("Failed to overlay legend")

    for ( int idx=0; idx<nritems; idx++ )
    {
	IOPar* par = parset[idx];
	FixedString namestr = par->find( sKey::Name() );
	if ( namestr.isEmpty() )
	    continue;

	float sz = 1;
	BufferString symbstr, penstr;
	bool usewellsymbol = false;
	par->getYN( ODGMT::sKeyUseWellSymbolsYN(), usewellsymbol );
	FixedString shapestr = par->find( ODGMT::sKeyShape() );
	if ( !usewellsymbol && !shapestr ) continue;
	ODGMT::Shape shape = ODGMT::parseEnumShape( shapestr.str() );
	symbstr = ODGMT::sShapeKeys()[(int)shape];
	par->get( sKey::Size(), sz );
	if ( shape == ODGMT::Polygon || shape == ODGMT::Line )
	{
	    const char* lsstr = par->find( ODGMT::sKeyLineStyle() );
	    if ( !lsstr ) continue;

	    OD::LineStyle ls;
	    ls.fromString( lsstr );
	    if ( ls.type_ != OD::LineStyle::None )
	    {
		mGetLineStyleString( ls, penstr );
	    }
	    else if ( shape == ODGMT::Line )
		continue;
	}
	else
	{
	    Color pencol;
	    par->get( sKey::Color(), pencol );
	    BufferString colstr;
	    mGetColorString( pencol, colstr );
	    penstr = "1p,"; penstr += colstr;
	}

	bool dofill;
	par->getYN( ODGMT::sKeyFill(), dofill );
	BufferString legendstring = "S 0.6c ";

	if ( !usewellsymbol )
	    legendstring += symbstr;
	else
	{
	    BufferString symbolname;
	    par->get( ODGMT::sKeyWellSymbolName(), symbolname );
	    BufferString deffilenm = GMTWSR().get( symbolname )->deffilenm_;
	    legendstring += "k"; legendstring += deffilenm;
	    par->get( sKey::Size(), sz );
	}

	legendstring += " ";
	legendstring += sz > 1 ? 1 : sz;
	legendstring += "c ";

	if ( !usewellsymbol && dofill )
	{
	    BufferString fillcolstr;
	    Color fillcol;
	    par->get( ODGMT::sKeyFillColor(), fillcol );
	    mGetColorString( fillcol, fillcolstr );
	    legendstring += fillcolstr;
	}
	else
	    legendstring += "-";

	legendstring += " ";
	if ( penstr.isEmpty() )
	    legendstring += "-";
	else
	    legendstring += penstr;

	legendstring += " "; legendstring += 1.3;
	legendstring += " "; legendstring += namestr;
	DBG::message( DBG_PROGSTART, legendstring.str() );
	procstrm << legendstring << "\n";
	procstrm << "G0.2c" << "\n";
    }

    strm << "Done" << od_endl;
    return true;
}



int GMTCommand::factoryid_ = -1;

void GMTCommand::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Advanced", GMTCommand::createInstance );
}

GMTPar* GMTCommand::createInstance( const IOPar& iop )
{
    return new GMTCommand( iop );
}


const char* GMTCommand::userRef() const
{
    BufferString* str = new BufferString( "GMT Command: " );
    const char* res = find( ODGMT::sKeyCustomComm() );
    *str += res;
    *( str->getCStr() + 25 ) = '\0';
    return str->buf();
}


bool GMTCommand::execute( od_ostream& strm, const char* fnm )
{
    strm << "Executing custom command" << od_endl;
    const char* res = find( ODGMT::sKeyCustomComm() );
    if ( !res || !*res )
	mErrStrmRet("No command to execute")

    strm << res << od_endl;
    BufferString comm = res;
    char* ptr = firstOcc( comm.getCStr(), " -R" );
    if ( ptr )
    {
	BufferString oldstr( ptr );
	ptr = oldstr.getCStr();
	ptr++;
	while ( ptr && *ptr != ' ' )
	    ptr++;

	*ptr = '\0';
	BufferString newstr;
	mGetRangeString( newstr )
	newstr.insertAt( 0, " " );
	comm.replace( oldstr.buf(), newstr.buf() );
    }

    ptr = comm.find( " -J" );
    if ( ptr )
    {
	BufferString oldstr( ptr );
	ptr = oldstr.getCStr();
	ptr++;
	while ( ptr && *ptr != ' ' )
	    ptr++;

	*ptr = '\0';
	BufferString newstr;
	mGetProjString( newstr, "X" )
	newstr.insertAt( 0, " " );
	comm.replace( oldstr.buf(), newstr.buf() );
    }

    ptr = comm.find( ">>" );
    if ( !ptr )
	ptr = comm.find( ">" );

    if ( ptr )
    {
	BufferString outputarg = ptr;
	const BufferString filename = FilePath(fnm).fileName();
	char* fptr = outputarg.find( filename );
	if ( fptr )
	{
	    fptr += filename.size();
	    *ptr = '\0';
	    comm += " -O -K >> ";
	    comm += fileName( fnm ); // Ensures full path for output file.
	    comm += fptr;
	    strm << " Command: " << comm.buf() << od_endl;
	}
	else
	    strm << " Output file different from main map " << fnm << od_endl;
    }
    else
	strm << " Opps!...no output file" << od_endl;

    if ( !execCmd(comm.buf(),strm) )
	mErrStrmRet("... Failed")

    strm << "... Done" << od_endl;
    return true;
}
