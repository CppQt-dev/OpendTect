/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/

#include "uiattrsetman.h"

#include "uibutton.h"
#include "uiioobjsel.h"
#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uimsg.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "ioobjctxt.h"
#include "keystrs.h"
#include "survinfo.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiAttrSetMan)


uiAttrSetMan::uiAttrSetMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,uiDialog::Setup(uiStrings::phrManage(tr("Attribute Sets")),
				     mNoDlgTitle,
				     mODHelpKey(mAttrSetManHelpID) )
			       .nrstatusflds(1).modal(false),
		   AttribDescSetTranslatorGroup::ioContext())
{
    ctxt_.toselect_.dontallow_.set( sKey::Type(), is2d ? "3D" : "2D" );
    createDefaultUI();
    setPrefWidth( 50 );

    uiListBox::Setup su( OD::ChooseNone, uiStrings::sAttribute(2),
			 uiListBox::AboveMid );
    attribfld_ = new uiListBox( listgrp_, su );
    attribfld_->attach( rightOf, selgrp_ );
    attribfld_->setHSzPol( uiObject::Wide );

    mTriggerInstanceCreatedNotifier();
    selChg( this );
}


uiAttrSetMan::~uiAttrSetMan()
{
}


static void addStoredNms( const Attrib::DescSet& attrset, uiString& txt )
{
    BufferStringSet nms;
    attrset.getStoredNames( nms );
    txt.appendPlainText( nms.getDispString() );
}


static void fillAttribList( uiListBox* attribfld,
			    const Attrib::DescSet& attrset )
{
    BufferStringSet nms;
    attrset.getAttribNames( nms, false );
    attribfld->addItems( nms.getUiStringSet() );
}


void uiAttrSetMan::mkFileInfo()
{
    attribfld_->setEmpty();
    if ( !curioobj_ )
	{ setInfo( uiString::empty() ); return; }

    uiPhrase txt;
    Attrib::DescSet attrset( SI().has2D() );
    uiRetVal warns;
    uiRetVal errs = attrset.load( curioobj_->key(), &warns );
    if ( !errs.isOK() )
	uiMSG().error( errs );
    else
    {
	if ( !warns.isOK() )
	    uiMSG().warning( warns );

	txt = tr("Type: %1").arg(attrset.is2D() ? uiStrings::s2D() :
							    uiStrings::s3D());
	txt.appendPhrase(tr("Input"), uiString::Empty);
	txt.appendPlainText( ": " );
	addStoredNms( attrset, txt );

	fillAttribList( attribfld_, attrset );
    }

    txt.appendPhrase( mToUiStringTodo(getFileInfo()), uiString::Empty,
						    uiString::LeaveALine );
    setInfo(txt);
}
