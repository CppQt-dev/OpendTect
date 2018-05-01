/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/

#include "uipicksetman.h"
#include "uipicksettools.h"

#include "uibutton.h"
#include "uiioobjselgrp.h"
#include "uiioobjmanip.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "ioobjctxt.h"
#include "draw.h"
#include "picksetmanager.h"
#include "picksettr.h"
#include "keystrs.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiPickSetMan)

static IOObjContext getIOObjContext( const char* fixedtrkey )
{
    IOObjContext ctxt = mIOObjContext( PickSet );
    if ( fixedtrkey )
	ctxt.fixTranslator( fixedtrkey );

    BufferString types = sKey::PickSet();
    types.add( "`" ).add( sKey::Polygon() ).add( "`" );
    ctxt.toselect_.require_.set( sKey::Type(), types.buf() );
    return ctxt;
}

uiPickSetMan::uiPickSetMan( uiParent* p, const char* fixedtrkey )
    : uiObjFileMan(p,uiDialog::Setup(
             uiStrings::phrManage( toUiString("%1/%2")
                .arg( uiStrings::sPickSet(mPlural) )
                .arg( uiStrings::sPolygon(mPlural))),
             mNoDlgTitle,
             mODHelpKey(mPickSetManHelpID) )
                 .nrstatusflds(1).modal(false),
			getIOObjContext(fixedtrkey))
{
    createDefaultUI();
    mergebut_ = selgrp_->getManipGroup()->addButton( "mergepicksets",
				    uiStrings::phrMerge(uiStrings::sPickSet()),
				    mCB(this,uiPickSetMan,mergeSets) );
    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiPickSetMan::~uiPickSetMan()
{
}


void uiPickSetMan::ownSelChg()
{
    BufferStringSet chsnnms;
    selgrp_->getChosen( chsnnms );
    uiString tt;
    if ( chsnnms.size() > 1 )
	tt = uiStrings::phrMerge( toUiString(chsnnms.getDispString(2)) );
    else
	tt = uiStrings::phrMerge(uiStrings::sPickSet());
    mergebut_->setToolTip( tt );
}


bool uiPickSetMan::gtItemInfo( const IOObj& ioobj, uiPhraseSet& inf ) const
{
    uiRetVal uirv;
    ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( ioobj.key(), uirv );
    if ( !ps )
	{ inf.add( uirv ); return false; }

    mImplTODOGtItemInfo();

    /*
    const bool ispoly = ps->isPolygon();
    const BufferString cat = ps->category();
    txt = tr("Type:");
    if ( ispoly )
	txt.appendPhrase( uiStrings::sPolygon(), uiString::Space,
						uiString::OnSameLine );
    else if ( !cat.isEmpty() )
	txt.appendPlainText( cat );
    else
	txt.appendPhrase(uiStrings::sPickSet(), uiString::Space,
						uiString::OnSameLine );

    MonitorLock ml( *ps );
    const int sz = ps->size();
    if ( sz < 1 )
	txt.appendPhrase( tr("Empty Pick Set") );
    else
    {
	txt.appendPhrase(toUiString(" < %1 %2").arg(sz).arg(ispoly ?
				tr("vertices") : uiStrings::sPick(mPlural)));
	if ( !ispoly && ps->first().hasDir() )
	{
	    txt.addSpace();
	    txt.appendPhrase(tr("(with directions)"));
	}
	if ( ispoly && sz > 2 )
	{
	    const float area = ps->getXYArea();
	    if ( !mIsUdf(area) )
		txt.appendPhrase( tr(", area=%1").arg( area ));
	}
	txt.appendPlainText( ">" );
    }

    const Pick::Set::Disp disp = ps->getDisp();
    Color col( disp.mkstyle_.color_ ); col.setTransparency( 0 );
    txt.appendPhrase(uiStrings::sColor(), uiString::NoSep)
	.appendPlainText(": ").appendPlainText(col.largeUserInfoString());
    txt.appendPhrase(tr("Marker size (pixels): %1"), uiString::NoSep)
						.arg(disp.mkstyle_.size_);
    txt.appendPhrase(tr("Marker type: %1")
		    .arg(OD::MarkerStyle3D::TypeDef()
	     .getUiStringForIndex(disp.mkstyle_.type_)), uiString::NoSep);

    txt.appendPhrase( getFileInfo(), uiString::NoSep );
    */
}


void uiPickSetMan::mergeSets( CallBacker* )
{
    DBKey curid;
    if ( curioobj_ )
	curid = curioobj_->key();

    uiMergePickSets dlg( this, curid );
    if ( dlg.go() )
	selgrp_->fullUpdate( curid );
}
