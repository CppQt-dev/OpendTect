/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.cc,v 1.6 2003-08-15 13:19:42 nanne Exp $
________________________________________________________________________

-*/

#include "uiiosurfacedlg.h"
#include "uiiosurface.h"

#include "emsurfaceiodata.h"
#include "uimsg.h"
#include "ioobj.h"
#include "emhorizon.h"
#include "emmanager.h"


uiWriteSurfaceDlg::uiWriteSurfaceDlg( uiParent* p, const EM::Horizon& hor_ )
    : uiDialog(p,uiDialog::Setup("Output selection","",""))
    , hor(hor_)
    , auxdataidx(-1)
{
    iogrp = new uiSurfaceOutSel( this, hor );
}


bool uiWriteSurfaceDlg::acceptOK( CallBacker* )
{
    if ( !iogrp->processInput() )
	return false;

    if ( !auxDataOnly() )
	return true;

    BufferString attrnm = iogrp->auxDataName();
    bool rv = checkIfAlreadyPresent( attrnm.buf() );
    BufferString msg( "This surface already has an attribute called:\n" );
    msg += attrnm;
    msg += "\nDo you wish to overwrite this data?";
    if ( rv && !uiMSG().askGoOn(msg) )
	return false;

    if ( attrnm != hor.auxDataName(0) )
	const_cast<EM::Horizon&>(hor).setAuxDataName( 0, attrnm.buf() );

    return true;
}


void uiWriteSurfaceDlg::getSelection( EM::SurfaceIODataSelection& sels )
{
    iogrp->getSelection( sels );
    sels.selvalues.erase();
    if ( auxDataOnly() )
	sels.selvalues += auxdataidx;
    else if ( surfaceAndData() )
	sels.selvalues += 0;
}


bool uiWriteSurfaceDlg::surfaceOnly() const
{
    return iogrp->surfaceOnly();
}


bool uiWriteSurfaceDlg::auxDataOnly() const
{
    return iogrp->saveAuxDataOnly();
}


bool uiWriteSurfaceDlg::surfaceAndData() const
{
    return iogrp->surfaceAndData();
}


IOObj* uiWriteSurfaceDlg::ioObj() const
{
    return iogrp->selIOObj();
}


bool uiWriteSurfaceDlg::checkIfAlreadyPresent( const char* attrnm )
{
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( hor.id(), sd );

    bool present = false;
    auxdataidx = -1;
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
    {
	if ( *sd.valnames[idx] == attrnm )
	{
	    present = true;
	    auxdataidx = idx;
	}
    }

    return present;
}



uiReadSurfaceDlg::uiReadSurfaceDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Input selection","",""))
{
    iogrp = new uiSurfaceSel( this, false );
}


bool uiReadSurfaceDlg::acceptOK( CallBacker* )
{
    return iogrp->processInput();
}


IOObj* uiReadSurfaceDlg::ioObj() const
{
    return iogrp->selIOObj();
}


void uiReadSurfaceDlg::getSelection( EM::SurfaceIODataSelection& sels )
{
    iogrp->getSelection( sels );
}
