#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2004
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "dbkey.h"
#include "geomid.h"
#include "ranges.h"
#include "sets.h"
#include "uistring.h"

class IOObj;
class TrcKeySampling;
class TrcKeyZSampling;

class uiCompoundParSel;
class uiCheckBox;
class uiLineSel;
class uiPosSubSel;
class uiSeis2DSubSel;
class uiSelSubline;
class uiSeis2DMultiLineSel;
class uiSeis2DLineNameSel;
namespace Seis { class SelSetup; }


mExpClass(uiSeis) uiSeisSubSel : public uiGroup
{ mODTextTranslationClass(uiSeisSubSel);
public:

    static uiSeisSubSel* get(uiParent*,const Seis::SelSetup&);
    virtual		~uiSeisSubSel();

    bool		isAll() const;
    void		getSampling(TrcKeyZSampling&) const;
    void		getSampling(TrcKeySampling&) const;
    void		getZRange(StepInterval<float>&) const;

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    virtual void	clear();
    virtual void	setInput(const IOObj&)				= 0;
    void		setInput(const TrcKeySampling&);
    void		setInput(const DBKey&);
    void		setInput(const StepInterval<float>& zrg);
    void		setInput(const TrcKeyZSampling&);

    virtual int		expectedNrSamples() const;
    virtual int		expectedNrTraces() const;

    virtual uiCompoundParSel*	compoundParSel();
    Notifier<uiSeisSubSel>	selChange;

protected:

			uiSeisSubSel(uiParent*,const Seis::SelSetup&);

    void		selChangeCB(CallBacker*);
    void		afterSurveyChangedCB(CallBacker*);
    uiPosSubSel*	selfld_;

};


mExpClass(uiSeis) uiSeis3DSubSel : public uiSeisSubSel
{ mODTextTranslationClass(uiSeis3DSubSel);
public:

			uiSeis3DSubSel( uiParent* p, const Seis::SelSetup& ss )
			    : uiSeisSubSel(p,ss)		{}

    void		setInput(const IOObj&);

};


mExpClass(uiSeis) uiSeis2DSubSel : public uiSeisSubSel
{ mODTextTranslationClass(uiSeis2DSubSel);
public:

    mUseType( Pos,	GeomID );

			uiSeis2DSubSel(uiParent*,const Seis::SelSetup&);
			~uiSeis2DSubSel();

    virtual void	clear();
    bool		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    void		setInput(const IOObj&);
    void		setInputLines(const GeomIDSet&);

    bool		isSingLine() const;
    const char*		selectedLine() const;
    Pos::GeomID		selectedGeomID() const;
    void		setSelectedLine(const char*);

    void		selectedGeomIDs(GeomIDSet&) const;
    void		selectedLines(BufferStringSet&) const;
    void		setSelectedLines(const BufferStringSet&);

    int			expectedNrSamples() const;
    int			expectedNrTraces() const;

    void		getSampling(TrcKeyZSampling&,
				    Pos::GeomID gid=Pos::GeomID::get3D()) const;
    StepInterval<int>	getTrcRange(Pos::GeomID gid=Pos::GeomID::get3D()) const;
    StepInterval<float> getZRange(Pos::GeomID gid=Pos::GeomID::get3D()) const;

protected:

    uiSeis2DMultiLineSel*	multilnmsel_;
    uiSeis2DLineNameSel*	singlelnmsel_;

    bool		multiln_;
    DBKey		inpkey_;

    void		lineChg(CallBacker*);
};
