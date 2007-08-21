/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          June 2007
 RCS:		$Id: uistratreftree.cc,v 1.10 2007-08-21 12:40:10 cvshelene Exp $
________________________________________________________________________

-*/

#include "uistratreftree.h"

#include "stratunitrepos.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uistratutildlgs.h"

#define mAddCol(nm,wdth,nr) \
    lv_->addColumn( nm ); \
    lv_->setColumnWidthMode( nr, uiListView::Manual ); \
    lv_->setColumnWidth( nr, wdth )

static const int sUnitsCol	= 0;
static const int sDescCol	= 1;
static const int sLithoCol	= 2;

using namespace Strat;

uiStratRefTree::uiStratRefTree( uiParent* p, const RefTree* rt )
	: tree_(0)
{
    lv_ = new uiListView( p, "RefTree viewer" );
    mAddCol( "Unit", 300, 0 );
    mAddCol( "Description", 200, 1 );
    mAddCol( "Lithology", 150, 2 );
    lv_->setPrefWidth( 650 );
    lv_->setPrefHeight( 400 );
    lv_->setStretch( 2, 2 );
    lv_->setTreeStepSize(30);
    lv_->rightButtonClicked.notify( mCB( this,uiStratRefTree,rClickCB ) );

    setTree( rt );
}


uiStratRefTree::~uiStratRefTree()
{
    delete lv_;
}


void uiStratRefTree::setTree( const RefTree* rt, bool force )
{
    if ( !force && rt == tree_ ) return;

    tree_ = rt;
    if ( !tree_ ) return;

    lv_->clear();
    addNode( 0, *tree_, true );
}


void uiStratRefTree::addNode( uiListViewItem* parlvit,
			      const NodeUnitRef& nur, bool flattened )
{
    uiListViewItem* lvit = parlvit
	? new uiListViewItem( parlvit, uiListViewItem::Setup()
				.label(nur.code()).label(nur.description()) )
	: flattened ? 0 : new uiListViewItem( lv_,uiListViewItem::Setup()
				.label(nur.code()).label(nur.description()) );

    for ( int iref=0; iref<nur.nrRefs(); iref++ )
    {
	const UnitRef& ref = nur.ref( iref );
	if ( ref.isLeaf() )
	{
	    mDynamicCastGet(const LeafUnitRef&,lur,ref);
	    uiListViewItem::Setup setup = uiListViewItem::Setup()
				.label( lur.code() )
				.label( lur.description() )
				.label( UnRepo().getLithName(lur.lithology()) );
	    if ( lvit )
		new uiListViewItem( lvit, setup );
	    else
		new uiListViewItem( lv_, setup );
	}
	else
	{
	    mDynamicCastGet(const NodeUnitRef&,chldnur,ref);
	    addNode( lvit, chldnur, false );
	}
    }
}


const UnitRef* uiStratRefTree::findUnit( const char* s ) const
{
    return tree_->find( s );
}


void uiStratRefTree::expand( bool yn ) const
{
    uiListViewItem* lvit = lv_->firstChild();
    while ( lvit )
    {
	lvit->setOpen( yn );
	lvit = lvit->itemBelow();
    }
}


void uiStratRefTree::makeTreeEditable( bool yn ) const
{
    uiListViewItem* lvit = lv_->firstChild();
    while ( lvit )
    {
	lvit->setRenameEnabled( sUnitsCol, yn );
	lvit->setRenameEnabled( sDescCol, yn );
	lvit->setDragEnabled( yn );
	lvit->setDropEnabled( yn );
	lvit = lvit->itemBelow();
    }
}


void uiStratRefTree::rClickCB( CallBacker* )
{
    //TODO: as soon as Qt4 in, check for column number: 0,1 as follow 2 lithodlg
    uiListViewItem* lvit = lv_->itemNotified();
    if ( !lvit || !lvit->renameEnabled(sUnitsCol) ) return;

    uiPopupMenu mnu( lv_->parent(), "Action" );
    mnu.insertItem( new uiMenuItem("Add sub-unit ..."), 0 );
    mnu.insertItem( new uiMenuItem("Insert ..."), 1 );
    mnu.insertItem( new uiMenuItem("Remove"), 2 );
/*    mnu.insertSeparator();
    mnu.insertItem( new uiMenuItem("Rename"), 3 );*/

    const int mnuid = mnu.exec();
    if ( mnuid<0 ) return;
    else if ( mnuid==0 )
	insertSubUnit( 0 );
    else if ( mnuid==1 )
	insertSubUnit( lvit );
    else if ( mnuid==2 )
	removeUnit( lvit );
}


void uiStratRefTree::insertSubUnit( uiListViewItem* lvit )
{
    bool insert = lvit ? true : false;
    uiStratUnitDlg newurdlg( lv_->parent(), insert );
    newurdlg.go();
}


void uiStratRefTree::removeUnit( uiListViewItem* lvit )
{
}



