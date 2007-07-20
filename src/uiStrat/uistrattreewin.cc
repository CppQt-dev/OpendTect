/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          July 2007
 RCS:		$Id: uistrattreewin.cc,v 1.2 2007-07-20 15:49:36 cvshelene Exp $
________________________________________________________________________

-*/

#include "uistrattreewin.h"

#include "compoundkey.h"
#include "stratlevel.h"
#include "stratunitrepos.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uistratreftree.h"
#include "uistratunitdesctab.h"
#include "uitable.h"


uiStratTreeWin::uiStratTreeWin( uiParent* p )
    : uiMainWin(p,"Strat RefTree viewer group", 0, true, true)
{
    createMenus();
    uitree_ = new uiStratRefTree( this, &Strat::RT() );
    const_cast<uiStratRefTree*>(uitree_)->listView()->selectionChanged.notify( 
	    				mCB( this,uiStratTreeWin,unitSelCB ) );
    uistrattab_ = new uiStratUnitDescTab( this, 0 );
    uistrattab_->table()->attach( alignedBelow, (uiObject*)uitree_->listView());
}


uiStratTreeWin::~uiStratTreeWin()
{
    delete uitree_;
    delete uistrattab_;
}

    
void uiStratTreeWin::createMenus()
{
    uiMenuBar* menubar =  menuBar();
    uiPopupMenu* viewmnu = new uiPopupMenu( this, "&View" );
    expandmnuitem_ = new uiMenuItem( "Expand all",
					mCB(this, uiStratTreeWin, setExpCB ) );
    viewmnu->insertItem( expandmnuitem_ );
    expandmnuitem_->setCheckable( true );
    expandmnuitem_->setChecked( true );
    menubar->insertItem( viewmnu );
	    
}


void uiStratTreeWin::setExpCB(CallBacker*)
{
}


void uiStratTreeWin::unitSelCB(CallBacker*)
{
    uiListViewItem* item = uitree_->listView()->selectedItem();
    BufferString bs = item->text();
    int itemdepth = item->depth();
    for ( int idx=itemdepth-2; idx>=0; idx-- )
    {
	item = item->parent();
	CompoundKey kc( item->text() );
	kc += bs.buf();
	bs = kc.buf();
    }
    const Strat::UnitRef* ur = uitree_->findUnit( bs.buf() ); 
    uistrattab_->setUnitRef( ur );
    const Strat::Level* toplvl = Strat::RT().getLevel( ur, true );
    const Strat::Level* botlvl = Strat::RT().getLevel( ur, false );
    if ( toplvl )
	uistrattab_->setLevDesc( true, toplvl->name_, toplvl->color() );
    if ( botlvl )
	uistrattab_->setLevDesc( false, botlvl->name_, botlvl->color() );
} 
