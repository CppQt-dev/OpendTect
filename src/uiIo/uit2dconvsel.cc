/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uit2dconvsel.cc,v 1.2 2010-05-31 15:11:37 cvsbert Exp $";

#include "uit2dconvsel.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uicombobox.h"
#include "ioobj.h"
#include "survinfo.h"
#include "zdomain.h"

mImplFactory1Param(uiT2DConvSelGroup,uiParent*,uiT2DConvSelGroup::factory);


uiT2DConvSel::uiT2DConvSel( uiParent* p, const Setup& su )
    : uiGroup(p)
    , setup_(su)
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, setup_.fldtext_ );
    choicefld_ = lcb->box();
    choicefld_->setHSzPol( uiObject::SmallVar );
    if ( setup_.optional_ )
	choicefld_->addItem( "No" );

    const BufferStringSet& factnms( uiT2DConvSelGroup::factory().getNames() );
    for ( int idx=0; idx<factnms.size(); idx++ )
    {
	const char* nm = factnms.get( idx );
	choicefld_->addItem( nm );
	uiT2DConvSelGroup* grp = uiT2DConvSelGroup::factory().create(nm,this);
	grps_ += grp;
	if ( idx == 0 )
	    setHAlignObj( grp );
	grp->attach( rightOf, lcb );
    }

    if ( setup_.tiedto_ )
    {
	const CallBack cb( mCB(this,uiT2DConvSel,inpSel) );
	setup_.tiedto_->selectionDone.notify( cb );
	finaliseDone.notify( cb );
    }
    const CallBack cb( mCB(this,uiT2DConvSel,choiceSel) );
    choicefld_->selectionChanged.notify( cb );
    finaliseDone.notify( cb );
}


#define mGetGroupIdx \
    const int grpidx = choicefld_->currentItem() - (setup_.optional_ ? 1 : 0)


void uiT2DConvSel::choiceSel( CallBacker* cb )
{
    mGetGroupIdx;
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( idx == grpidx );
}


void uiT2DConvSel::inpSel( CallBacker* cb )
{
    if ( !setup_.tiedto_ ) return;
    const IOObj* ioobj = setup_.tiedto_->ioobj();
    if ( !ioobj ) return;

    choicefld_->setSensitive( ZDomain::isSIDomain(ioobj->pars()) );
}


void uiT2DConvSel::usePar( const IOPar& iop )
{
    const char* typ = iop.find( sKey::Type );
    if ( !typ || !*typ || !choicefld_->isPresent(typ) )
	return;

    int selidx = choicefld_->indexOf( typ );
    if ( selidx < 0 ) return;

    if ( setup_.optional_ ) selidx--;
    if ( selidx >= 0 )
	grps_[selidx]->usePar( iop );

    choicefld_->setCurrentItem( selidx );
}


void uiT2DConvSel::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type, choicefld_->text() );
    mGetGroupIdx;
    if ( grpidx >= 0 )
	grps_[grpidx]->fillPar( iop );
}


uiT2DLinConvSelGroup::uiT2DLinConvSelGroup( uiParent* p )
    : uiT2DConvSelGroup(p,"Linear T2D conv sel")
{
    //TODO handle Z unit properly
    const float dv = SI().zInFeet() ? 3000 : 1000;
    fld_ = new uiGenInput( this, "V0,dV/s", FloatInpSpec(0), FloatInpSpec(dv) );
}


#define mDefA0A1 float a0 = fld_->getfValue(0); float a1 = fld_->getfValue(1)


void uiT2DLinConvSelGroup::usePar( const IOPar& iop )
{
    //TODO handle Z unit properly
    mDefA0A1;
    if ( iop.get("V0,dV",a0,a1) )
	{ fld_->setValue( a0, 0 ); fld_->setValue( a1, 1 ); }
}


void uiT2DLinConvSelGroup::fillPar( IOPar& iop ) const
{
    //TODO handle Z unit properly
    mDefA0A1;
    iop.set( "V0,dV", a0, a1 );
}


void uiT2DLinConvSelGroup::initClass()
{
    uiT2DConvSelGroup::factory().addCreator( create, "Linear" );
}
