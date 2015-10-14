#ifndef uiisopachmaker_h
#define uiisopachmaker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2008
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"
#include "emposid.h"

class uiBatchJobDispatcherSel;
class uiIOObjSel;
class uiGenInput;
class CtxtIOObj;
class DataPointSet;
namespace EM { class EMObject; }


/*! \brief Create isochron as attribute of horizon */

mClass(uiHorizonAttrib) uiIsochronMakerGrp : public uiGroup
{ mODTextTranslationClass(uiIsochronMakerGrp);
public:
			uiIsochronMakerGrp(uiParent*,EM::ObjectID);
			~uiIsochronMakerGrp();

    bool		chkInputFlds();
    bool		fillPar(IOPar&);
    BufferString	getHorNm(EM::ObjectID);
    const char*		attrName() const;

protected:
    uiIOObjSel*		basesel_;
    uiIOObjSel*		horsel_;
    uiGenInput*		attrnmfld_;
    uiGenInput*		msecsfld_;
    CtxtIOObj&		basectio_;
    CtxtIOObj&		ctio_;
    EM::ObjectID	horid_;
    EM::EMObject*	baseemobj_;
    
    void		toHorSel(CallBacker*);
};


mClass(uiHorizonAttrib) uiIsochronMakerBatch : public uiDialog
{ mODTextTranslationClass(uiIsochronMakerBatch);
public:

			uiIsochronMakerBatch(uiParent*);
protected:
    bool		prepareProcessing();
    bool		fillPar();
    bool		acceptOK(CallBacker*);

    uiIsochronMakerGrp*		grp_;
    uiBatchJobDispatcherSel*	batchfld_;
    bool			isoverwrite_;
};


mClass(uiHorizonAttrib) uiIsochronMakerDlg : public uiDialog
{ mODTextTranslationClass(uiIsochronMakerDlg);
public:
			uiIsochronMakerDlg(uiParent*,EM::ObjectID);
			~uiIsochronMakerDlg();

    const char*		attrName() const	{ return grp_->attrName(); }
    const DataPointSet&	getDPS()		{ return *dps_; }

protected:    
    bool		acceptOK(CallBacker*);
    bool		doWork();

    uiIsochronMakerGrp*	grp_;
    DataPointSet*	dps_;
};

#endif

