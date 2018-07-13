/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uizaxistransform.h"

#include "datainpspec.h"
#include "refcount.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uidialog.h"
#include "zaxistransform.h"
#include "uimsg.h"

mImplClassFactory( uiZAxisTransform, factory );

bool uiZAxisTransform::isField( const uiParent* p )
{
    mDynamicCastGet( const uiZAxisTransformSel*, sel, p );
    return sel && sel->isField();
}


uiZAxisTransform::uiZAxisTransform( uiParent* p )
    : uiDlgGroup( p, uiString::empty() )
    , is2d_(false)
{}


void uiZAxisTransform::enableTargetSampling()
{}


bool uiZAxisTransform::getTargetSampling( StepInterval<float>& ) const
{ return false; }


void uiZAxisTransform::setIs2D( bool yn )
{
    is2d_ = yn;
}


bool uiZAxisTransform::is2D() const
{
    return is2d_;
}


// uiZAxisTransformSel
uiZAxisTransformSel::uiZAxisTransformSel( uiParent* p, bool withnone,
	const char* fromdomain, const char* todomain, bool withsampling,
	bool isfield, bool is2d )
    : uiDlgGroup( p, uiString::empty() )
    , selfld_( 0 )
    , isfield_( isfield )
{
    if ( isfield_ && withsampling )
	{ pErrMsg( "Field style cannot be used with sampling" ); return; }

    transflds_.setNullAllowed( true );
    uiStringSet names;

    const BufferStringSet& factorykeys =
	uiZAxisTransform::factory().getKeys();

    const uiStringSet& usernames =
	uiZAxisTransform::factory().getUserNames();

    for ( int idx=0; idx<factorykeys.size(); idx++ )
    {
	uiZAxisTransform* uizat = uiZAxisTransform::factory().create(
		factorykeys[idx]->buf(), this, fromdomain, todomain );
	if ( !uizat )
	    continue;

	if ( isfield_ && !uizat->canBeField() )
	{
	    delete uizat;
	    continue;
	}

	uizat->setIs2D( is2d );
	if ( withsampling )
	    uizat->enableTargetSampling();

	transflds_ += uizat;
	names += usernames[idx];
    }

    const bool hastransforms = names.size();

    const uiString nonestr = uiStrings::sNone();

    if ( hastransforms && withnone )
    {
	transflds_.insertAt( 0, 0 );
	names.insert( 0, nonestr );
    }

    if ( names.size()>1 )
    {
	selfld_ = new uiGenInput( this, tr("Z transform"),
		StringListInpSpec(names) );
	selfld_->valuechanged.notify( mCB(this, uiZAxisTransformSel,selCB) );

	setHAlignObj( selfld_ );


	for ( int idx=0; idx<transflds_.size(); idx++ )
	{
	    if ( !transflds_[idx] )
		continue;

	    transflds_[idx]->attach( isfield_ ? rightOf : alignedBelow,
				     selfld_ );
	}
    }
    else if ( hastransforms )
    {
	setHAlignObj( transflds_[0] );
    }

    selCB( 0 );
}


bool uiZAxisTransformSel::isField() const
{
    return isfield_;
}


void uiZAxisTransformSel::setLabel(const uiString& lbl )
{
    selfld_->setTitleText( lbl );
}


NotifierAccess* uiZAxisTransformSel::selectionDone()
{
    return &selfld_->valuechanged;
}

#define mGetSel		( selfld_ ? selfld_->getIntValue() : 0 )

bool uiZAxisTransformSel::getTargetSampling( StepInterval<float>& zrg ) const
{
    const int idx = mGetSel;
    if ( !transflds_[idx] )
	return false;

    return transflds_[idx]->getTargetSampling( zrg );

}


int uiZAxisTransformSel::nrTransforms() const
{
    int res = transflds_.size();
    if ( res && !transflds_[0] )
	res--;
    return res;
}


ZAxisTransform* uiZAxisTransformSel::getSelection()
{
    const int idx = mGetSel;
    return transflds_[idx] ? transflds_[idx]->getSelection() : 0;
}


FixedString uiZAxisTransformSel::selectedToDomain() const
{
    const int idx = mGetSel;
    if ( transflds_.validIdx(idx) )
	return transflds_[idx]->toDomain();

    return OD::EmptyString();
}


bool uiZAxisTransformSel::fillPar( IOPar& par )
{
    RefMan<ZAxisTransform> sel = getSelection();
    if ( !sel )
	return false;

    sel->fillPar( par );
    return true;
}


bool uiZAxisTransformSel::acceptOK()
{
    const int idx = mGetSel;
    if ( !transflds_[idx] )
	return true;

    if ( !transflds_[idx]->acceptOK() )
	return false;

    StepInterval<float> zrg;
    if ( !getTargetSampling( zrg ) )
	return true;

    if ( zrg.isUdf() )
    {
	uiMSG().error(tr("Sampling is not set"));
	return false;
    }

    if ( zrg.isRev() )
    {
	uiMSG().error(tr("Sampling is reversed."));
	return false;
    }

    if ( zrg.step<=0 )
    {
	uiMSG().error(tr("Sampling step is zero or negative"));
	return false;
    }

    return true;
}


void uiZAxisTransformSel::selCB( CallBacker* )
{
    const int selidx = mGetSel;
    for ( int idx=0; idx<transflds_.size(); idx++ )
    {
	if ( !transflds_[idx] )
	    continue;

	transflds_[idx]->display( idx==selidx );
    }
}
