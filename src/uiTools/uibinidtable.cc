/*+
 ________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          February 2003
 RCS:           $Id: uibinidtable.cc,v 1.4 2003-03-12 16:24:11 arend Exp $
 ________________________________________________________________________

-*/

#include "uibinidtable.h"
#include "uitable.h"
#include "uimsg.h"
#include "position.h"
#include "survinfo.h"


uiBinIDTable::uiBinIDTable( uiParent* p, int nr )
    : uiGroup(p,"BinID table")
{
    init( nr );
}


uiBinIDTable::uiBinIDTable( uiParent* p, const TypeSet<BinID>& bids )
    : uiGroup(p,"BinID table")
{
    init( bids.size() );
    setBinIDs( bids );
}


void uiBinIDTable::init( int nrrows )
{
    ObjectSet<BufferString> colnms;
    colnms += new BufferString("Inline");
    colnms += new BufferString("Crossline");

    table = new uiTable( this, uiTable::Setup().rowdesc("Node").rowcangrow(),
		         "Table" );


    table->setColumnLabels( colnms );
    table->setNrRows( nrrows+5 );
    nodeAdded();

    deepErase( colnms );

    table->rowInserted.notify( mCB(this,uiBinIDTable,nodeAdded) );
}

void uiBinIDTable::nodeAdded(CallBacker*)
{
    const int nrrows = table->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	BufferString labl( "Node " );
	labl += idx;
	table->setRowLabel( idx, labl );
    }
}

    

void uiBinIDTable::setBinIDs( const TypeSet<BinID>& bids )
{
    const int nrbids = bids.size();
    for ( int idx=0; idx<nrbids; idx++ )
    {
	const BinID bid = bids[idx];
	table->setText( uiTable::Pos(0,idx), BufferString(bid.inl) );
	table->setText( uiTable::Pos(1,idx), BufferString(bid.crl) );
    }
}


void uiBinIDTable::getBinIDs( TypeSet<BinID>& bids )
{
    int nrrows = table->size().height();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	BinID bid(0,0);
	BufferString inlstr = table->text(uiTable::Pos(0,idx));
	BufferString crlstr = table->text(uiTable::Pos(1,idx));
	if ( !(*inlstr) || !(*crlstr) )
	    continue;
	bid.inl = atoi(inlstr);
	bid.crl = atoi(crlstr);
	if ( !SI().isReasonable(bid) )
	{
	    Coord c( atof(inlstr), atof(crlstr) );
	    if ( SI().isReasonable(c) )
		bid = SI().transform(c);
	    else
	    {
		BufferString msg( "Position " );
		msg += bid.inl; msg += "/"; msg += bid.crl;
		msg += " is probably wrong.\nDo you wish to discard it?";
		if ( uiMSG().askGoOn(msg) )
		{
		    table->removeRow( idx );
		    nrrows--; idx--;
		    continue;
		}
	    }
	}
	SI().snap( bid );
	bids += bid;
    }
}


uiBinIDTableDlg::uiBinIDTableDlg( uiParent* p, const char* title, int nr )
    : uiDialog(p,uiDialog::Setup(title,"",""))
{
    table = new uiBinIDTable( this, nr );
}


uiBinIDTableDlg::uiBinIDTableDlg( uiParent* p, const char* title, 
				  const TypeSet<BinID>& bids )
    : uiDialog(p,uiDialog::Setup(title,"",""))
{
    table = new uiBinIDTable( this, bids );
}


void uiBinIDTableDlg::setBinIDs( const TypeSet<BinID>& bids )
{
    table->setBinIDs( bids );
}


void uiBinIDTableDlg::getBinIDs( TypeSet<BinID>& bids )
{
    table->getBinIDs( bids );
}
