#ifndef uiproxydlg_h
#define uiproxydlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2012
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

class uiCheckBox;
class uiGenInput;
class uiLabel;
class uiLabeledSpinBox;
class uiLineEdit;

mExpClass(uiTools) uiProxyDlg : public uiDialog
{ mODTextTranslationClass(uiProxyDlg);
public:
			uiProxyDlg(uiParent*);
			~uiProxyDlg();

    void		setHost(const char*);
    const char*		getHost() const;
    void		setPort(int);
    int			getPort() const;

protected:

    void		initFromSettings();
    bool		saveInSettings();
    void		useProxyCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    uiCheckBox*		authenticationfld_;
    uiGenInput*		usernamefld_;
    uiLineEdit*		pwdfld_;
    uiLabel*		pwdlabel_;
    uiGenInput*		useproxyfld_;
    uiGenInput*		hostfld_;
    uiLabeledSpinBox*	portfld_;
};

#endif


