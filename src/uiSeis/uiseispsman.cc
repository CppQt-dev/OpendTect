/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2007
________________________________________________________________________

-*/


#include "uiseispsman.h"

#include "keystrs.h"
#include "posinfo.h"
#include "zdomain.h"
#include "seispsioprov.h"
#include "seisioobjinfo.h"

#include "uibutton.h"
#include "uiioobjmanip.h"
#include "uiioobjselgrp.h"
#include "uipixmap.h"
#include "uiprestkmergedlg.h"
#include "uiprestkcopy.h"
#include "uiseismulticubeps.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiSeisPreStackMan)

#define mHelpID is2d ? mODHelpKey(mSeisPrestackMan2DHelpID) : \
		       mODHelpKey(mSeisPrestackMan3DHelpID)
#define mMrgDataStoresStr() \
    uiStrings::phrMerge(uiStrings::sDataStore(mPlural))


uiSeisPreStackMan::uiSeisPreStackMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,uiDialog::Setup(createCaption(is2d),mNoDlgTitle,mHelpID)
		     .nrstatusflds(1).modal(false),
		   is2d ? SeisPS2DTranslatorGroup::ioContext()
			: SeisPS3DTranslatorGroup::ioContext())
    , is2d_(is2d)
    , copybut_(0)
    , mergebut_(0)
    , editbut_(0)
{
    createDefaultUI( true );
    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    if ( !is2d )
    {
	copybut_ = manipgrp->addButton( "copyobj", uiStrings::phrCopy(
					uiStrings::sDataStore()),
					mCB(this,uiSeisPreStackMan,copyPush) );
	mergebut_ = manipgrp->addButton( "mergeseis", mMrgDataStoresStr(),
			    mCB(this,uiSeisPreStackMan,mergePush) );
	manipgrp->addButton( "multicubeps",
			    tr("Create/Edit Multi-Cube Data Store"),
			    mCB(this,uiSeisPreStackMan,mkMultiPush) );
	editbut_ = manipgrp->addButton( "browseseis",
			tr("Change file/directory names in SEG-Y file"),
			mCB(this,uiSeisPreStackMan,editPush) );
    }

    mTriggerInstanceCreatedNotifier();
    selChg(0);
}


uiSeisPreStackMan::~uiSeisPreStackMan()
{
}



uiString uiSeisPreStackMan::createCaption( bool is2d )
{
    return is2d
	? uiStrings::phrManage( SeisPS2DTranslatorGroup::sTypeName() )
	: uiStrings::phrManage( SeisPS3DTranslatorGroup::sTypeName());
}


#define mSetButToolTip(but,str1,deftt) \
{ \
    if ( !but->isSensitive() ) \
	but->setToolTip( deftt ); \
    else \
	but->setToolTip( str1 ); \
}

void uiSeisPreStackMan::ownSelChg()
{
    if ( is2d_ )
	return;

    uiString tt,cursel;
    if ( curioobj_ )
	cursel = curioobj_->uiName();

    copybut_->setSensitive( curioobj_ );
    mergebut_->setSensitive( curioobj_ );
    mSetButToolTip( copybut_,tr("Make a copy of '%1'").arg(cursel),
		    tr("Copy data store"));
    BufferStringSet selnms;
    selgrp_->getChosen( selnms );
    mergebut_->setToolTip( mMrgDataStoresStr() );
    if ( selnms.size() > 1 )
	mSetButToolTip( mergebut_,
		    uiStrings::phrMerge(toUiString(selnms.getDispString(2))),
		    uiString::empty() );

    editbut_->display( getBrowserDef() );
}


bool uiSeisPreStackMan::gtItemInfo( const IOObj& ioobj, uiPhraseSet& inf ) const
{
    SeisIOObjInfo objinf( ioobj );
    if ( !objinf.isOK() )
	{ inf.add( uiStrings::sNoInfoAvailable() ); return false; }

    if ( is2d_ )
    {
	BufferStringSet nms;
	SPSIOPF().getLineNames( ioobj, nms );
	addObjInfo( inf, uiStrings::sLine(mPlural),
						nms.getDispString(3,false) );
    }
    else
    {
	PtrMan<SeisPS3DReader> rdr = SPSIOPF().get3DReader( ioobj );
	if ( rdr )
	{
	    const PosInfo::CubeData& cd = rdr->posData();
	    addObjInfo( inf, tr("Total number of gathers"), cd.totalSize() );

	    StepInterval<int> rg; cd.getInlRange( rg );

	    uiString inlrngtxt = toUiString("%1 - %2").arg(rg.start)
								.arg(rg.stop);
	    if ( cd.haveInlStepInfo() )
		inlrngtxt.appendPlainText( "[%1]" ).arg( rg.step );

	    addObjInfo( inf, uiStrings::sInlineRange(), inlrngtxt );

	    cd.getCrlRange( rg );
	    uiString crlrngtxt = tr("%1 - %2").arg(rg.start)
							    .arg(rg.stop);
	    if ( cd.haveCrlStepInfo() )
		crlrngtxt.appendPlainText( "[%1]" ).arg( rg.step );

	    addObjInfo( inf, uiStrings::sCrosslineRange(), crlrngtxt );
	}

    }


    TrcKeyZSampling cs;
    if ( objinf.getRanges(cs) )
    {
	const bool zistm = objinf.isTime();
	const ZDomain::Def& zddef = objinf.zDomainDef();
#	    define mAddZValTxt(memb) .arg(zistm ? mNINT32(1000*memb) : memb)
	inf.add( toUiString("%1 %2 (%3): %4 - %5 [%6]")
				    .arg(zddef.userName())
				    .arg(uiStrings::sRange().toLower())
				    .arg(toString(zddef.unitStr()))
				    mAddZValTxt(cs.zsamp_.start)
				    mAddZValTxt(cs.zsamp_.stop)
				    mAddZValTxt(cs.zsamp_.step) );
    }

    return true;
}


void uiSeisPreStackMan::copyPush( CallBacker* )
{
    if ( !curioobj_ )
	return;

    const DBKey key( curioobj_->key() );
    uiPreStackCopyDlg dlg( this, key );
    dlg.go();
    selgrp_->fullUpdate( key );
}


void uiSeisPreStackMan::mergePush( CallBacker* )
{
    if ( !curioobj_ )
	return;

    const DBKey key( curioobj_->key() );
    uiPreStackMergeDlg dlg( this );
    BufferStringSet selnms;
    selgrp_->getChosen( selnms );
    dlg.setInputIds( selnms );
    dlg.go();
    selgrp_->fullUpdate( key );
}


void uiSeisPreStackMan::mkMultiPush( CallBacker* )
{
    DBKey key; BufferString toedit;
    if ( curioobj_ )
    {
	key = curioobj_->key();
	if ( curioobj_->translator() == "MultiCube" )
	    toedit = key.toString();
    }
    uiSeisMultiCubePS dlg( this, toedit.str() );
    dlg.go();
    selgrp_->fullUpdate( key );
}


static ObjectSet<uiSeisPreStackMan::BrowserDef> browserdefs_;

int uiSeisPreStackMan::addBrowser( uiSeisPreStackMan::BrowserDef* bd )
{
    browserdefs_ += bd;
    return browserdefs_.size() - 1;
}


const uiSeisPreStackMan::BrowserDef* uiSeisPreStackMan::getBrowserDef() const
{
    if ( curioobj_ )
    {
	for ( int idx=0; idx<browserdefs_.size(); idx++ )
	{
	    const BrowserDef* bdef = browserdefs_[idx];
	    if ( bdef->name_ == curioobj_->translator() )
		return bdef;
	}
    }

    return 0;
}


void uiSeisPreStackMan::editPush( CallBacker* )
{
    const uiSeisPreStackMan::BrowserDef* bdef = getBrowserDef();
    if ( bdef )
    {
	CallBack cb( bdef->cb_ );
	cb.doCall( this );
    }
}

