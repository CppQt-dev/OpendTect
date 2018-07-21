#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno / Bert
 Date:          Dec 2010 / Oct 2016
________________________________________________________________________

-*/

/*!\brief select IOObj(s), possibly from an other survey. */


#include "uiiocommon.h"
#include "uigroup.h"
#include "fulldbkey.h"

class IOObjContext;
class uiSurveySelect;
class uiListBox;


mExpClass(uiIo) uiSurvIOObjSelGroup : public uiGroup
{
public:

			uiSurvIOObjSelGroup(uiParent*,const IOObjContext&,
						bool selmulti=false);
			~uiSurvIOObjSelGroup();

    SurveyDiskLocation	surveyDiskLocation() const;
    void		setSurvey(const SurveyDiskLocation&);
    void		addExclude(const SurveyDiskLocation&);
    void		excludeCurrentSurvey()
			{ addExclude(SurveyDiskLocation::currentSurvey()); }

    void		setSelected(const DBKey&);
    void		setSelected(const DBKeySet&);

    bool		evaluateInput();

    int			nrSelected() const	{ return chosenidxs_.size(); }
    const IOObj*	ioObj(int idx=0) const;
    FullDBKey		key(int idx=0) const;
    BufferString	mainFileName(int idx=0) const;

    const ObjectSet<IOObj>& objsInSurvey() const { return ioobjs_; }

    Notifier<uiSurvIOObjSelGroup>	dClicked;
    Notifier<uiSurvIOObjSelGroup>	survChange;
    Notifier<uiSurvIOObjSelGroup>	selChange;

protected:

    IOObjContext&	ctxt_;
    ObjectSet<IOObj>	ioobjs_;
    TypeSet<int>	chosenidxs_;
    DBKeySet		seldbkys_;
    const bool		ismultisel_;

    uiSurveySelect*	survsel_;
    uiListBox*		objfld_;

    void		initGrp(CallBacker*);
    void		survSelCB(CallBacker*);
    void		dClickCB(CallBacker*);
    void		selChgCB(CallBacker*);

    void		updGrp(bool withsurvsel);
    void		selSurvFromSelection();
    void		updateObjs();
    void		setSelection();

};