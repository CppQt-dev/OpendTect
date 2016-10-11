#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2016
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uigroup.h"

class uiComboBox;


mExpClass(uiIo) uiDataRootSel : public uiGroup
{ mODTextTranslationClass(uiDataRootSel);
public:

			uiDataRootSel(uiParent*,const char* defdir=0);

    BufferString	getDir() const;
			//!< non-empty is a valid dir
			//!< if invalid, errors have been presented to the user

    static const char*	sKeyRootDirs()	    { return "Known DATA directories"; }
    static const char*	sKeyDefRootDir()    { return "Default DATA directory"; }
    static uiString	userDataRootString();

protected:

    uiComboBox*		dirfld_;

    void		selButCB(CallBacker*);
    bool		handleDirName(BufferString&) const;
    bool		isValidFolder(const char*) const;
    void		addDirNameToSettingsIfNew(const char*) const;

};
