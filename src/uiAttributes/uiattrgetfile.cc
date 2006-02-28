/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: uiattrgetfile.cc,v 1.1 2006-02-28 15:58:52 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrgetfile.h"
#include "uiattrsrchprocfiles.h"
#include "uifileinput.h"
#include "uibutton.h"
#include "uitextedit.h"
#include "uimsg.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "iopar.h"
#include "oddirs.h"
#include "filegen.h"


using namespace Attrib;


uiGetFileForAttrSet::uiGetFileForAttrSet( uiParent* p, bool isads )
    : uiDialog(p,uiDialog::Setup(
		isads ? "Get Attribute Set" : "Get attributes from job file",
		isads ? "Select file containing an attribute set"
		      : "Select job specification file",
		 "101.1.3"))
    , attrset_(*new DescSet)
    , isattrset_(isads)
{
    fileinpfld = new uiFileInput( this, "File name" );
    fileinpfld->setFilter( isattrset_ ? "AttributeSet files (*.attr)"
				    : "Job specifications (*.par)" );
    fileinpfld->setDefaultSelectionDir( isattrset_ ? GetBaseDataDir()
						 : GetProcFileName(0) );
    fileinpfld->valuechanged.notify( mCB(this,uiGetFileForAttrSet,selChg) );
    if ( !isattrset_ )
    {
	uiPushButton* but = new uiPushButton( this,
				"Find from created cube ...",
	       			mCB(this,uiGetFileForAttrSet,srchDir) );
	but->attach( rightOf, fileinpfld );
    }
    infofld = new uiTextEdit( this, "Attribute info", true );
    infofld->attach( ensureBelow, fileinpfld );
    infofld->attach( widthSameAs, fileinpfld );
    infofld->setPrefHeightInChar( 4 );
}


uiGetFileForAttrSet::~uiGetFileForAttrSet()
{
    delete &attrset_;
}


void uiGetFileForAttrSet::srchDir( CallBacker* )
{
    uiAttrSrchProcFiles dlg( this );
    if ( dlg.go() )
    {
	fileinpfld->setFileName( dlg.fileName() );
	selChg();
    }
}


void uiGetFileForAttrSet::selChg( CallBacker* )
{
    fname_ = fileinpfld->fileName();
    IOPar iop; iop.read( fname_, sKey::Pars );
    if ( !isattrset_ )
    {
	PtrMan<IOPar> subpar = iop.subselect( "Attributes" );
	iop.clear();
	if ( subpar ) iop = *subpar;
    }

    attrset_.removeAll(); attrset_.usePar( iop );
    const int nrgood = attrset_.nrDescs( false, false );
    BufferString txt( nrgood == 1  ? "Attribute: "
			: (nrgood ? "Attributes:\n"
				  : "No valid attributes present") );
    int nrdone = 0;
    const int totalnrdescs = attrset_.nrDescs();
    for ( int idx=0; idx<totalnrdescs; idx++ )
    {
	Desc* desc = attrset_.getDesc( attrset_.getID(idx) );
	if ( desc->isHidden() || desc->isStored() ) continue;

	nrdone++;
	txt += desc->userRef();
	txt += " ("; txt += desc->attribName(); txt += ")";
	if ( nrdone != nrgood )
	    txt += "\n";
    }

    infofld->setText( txt );
}


bool uiGetFileForAttrSet::acceptOK( CallBacker* )
{
    fname_ = fileinpfld->fileName();
    if ( fname_ == "" || !File_exists(fname_) )
    {
	uiMSG().error( "Please enter the filename" );
	return false;
    }
    selChg(0);
    return true;
}
