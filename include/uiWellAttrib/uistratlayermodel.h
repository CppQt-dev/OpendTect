#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uimainwin.h"
#include "uistring.h"
class CtxtIOObj;
class Wavelet;
class SeisTrcBuf;
class StratSynth;
class PropertyRef;
class SyntheticData;
class TimeDepthModel;
class PropertyRefSelection;
class ElasticPropSelection;
class uiToolBar;
class uiIOObjSelDlg;
class uiStratSynthDisp;
class uiStratLayerModelDisp;
class uiLayerSequenceGenDesc;
class uiStratGenDescTools;
class uiStratLayModEditTools;
class uiStratLayerModelLMProvider;
namespace Strat { class LayerModel; class LayerSequenceGenDesc; }


mExpClass(uiWellAttrib) uiStratLayerModel : public uiMainWin
{ mODTextTranslationClass(uiStratLayerModel);
public:
    friend class		uiStratLayerModelManager;

				uiStratLayerModel(uiParent*,
					const char* disptype=0,int opt=0);
				~uiStratLayerModel();

    void			go()		{ show(); }

    static const char*		sKeyModeler2Use();

    mDeclInstanceCreatedNotifierAccess(uiStratLayerModel);
    uiToolBar*			analysisToolBar()	   { return analtb_; }

    void				setNrModels(int);
    int					nrModels() const;
    const Strat::LayerSequenceGenDesc&	genDesc() const	   { return desc_; }
    const Strat::LayerModel&		layerModelOriginal() const;
    Strat::LayerModel&			layerModelOriginal();
    const Strat::LayerModel&		layerModelEdited() const;
    Strat::LayerModel&			layerModelEdited();
    const Strat::LayerModel&            layerModel() const;
    Strat::LayerModel&                  layerModel();
    BufferString			levelName() const;
    const StratSynth&			currentStratSynth() const;
    StratSynth&				currentStratSynth();
    const StratSynth&			normalStratSynth() const;
    const StratSynth&			editStratSynth() const;
    bool				isEditUsed() const;
    const PropertyRefSelection&		modelProperties() const;
    const ObjectSet<const TimeDepthModel>& d2TModels() const;
    const Wavelet*			wavelet() const;
    DBKey				genDescID() const;

    Notifier<uiStratLayerModel>	newModels;
    Notifier<uiStratLayerModel>	waveletChanged;
    Notifier<uiStratLayerModel> saveRequired;   // CallBacker: CBCapsule<IOPar>
    Notifier<uiStratLayerModel> retrieveRequired;// CallBacker: CBCapsule<IOPar>

    bool			checkUnscaledWavelet();

    static void			doBasicLayerModel();

    void			displayFRText(bool yn,bool isbrine=true);
    uiStratLayerModelDisp*      getLayModelDisp() const	{ return moddisp_; }
    void			displayFRResult(bool usefr,bool parschanged,
						bool isbrine);
    void			prepareFluidRepl();
    void			resetFluidRepl();

    //Utility
    //SyntheticData*		getCurrentSyntheticData() const;

    void			setSynthView(const uiWorldRect& wr);
    const uiWorldRect&		curSynthView() const;
    uiStratSynthDisp*		getSynthDisp() const	{ return synthdisp_; }

protected:

    uiLayerSequenceGenDesc*	seqdisp_;
    uiStratLayerModelDisp*	moddisp_;
    uiStratSynthDisp*		synthdisp_;
    uiStratGenDescTools*	gentools_;
    uiStratLayModEditTools*	modtools_;
    uiToolBar*			analtb_;
    uiIOObjSelDlg*		modeldlg_;
    Strat::LayerSequenceGenDesc& desc_;
    uiStratLayerModelLMProvider& lmp_;
    CtxtIOObj&			descctio_;
    ElasticPropSelection*	elpropsel_;
    bool			needtoretrievefrpars_;
    bool			automksynth_;
    int				nrmodels_;


    void			setWinTitle();
    void			handleNewModel();
    void			setModelProps();
    void			setElasticProps();
    bool			selElasticProps(ElasticPropSelection&);
    bool			openGenDesc();
    bool			saveGenDesc() const;
    bool			saveGenDescIfNecessary(
					bool allowcancel=true) const;
    void			doGenModels(bool forceupdsynth,
					    bool overridedispeach=false);
    void			calcAndSetDisplayEach(bool overridepar);
    bool			closeOK();

    void			fillDisplayPars(IOPar&) const;
    void			fillWorkBenchPars(IOPar&) const;
    void			fillSyntheticsPars(IOPar&) const;
    bool			useDisplayPars(const IOPar&);
    bool			useSyntheticsPars(const IOPar&);


    void			openGenDescCB(CallBacker*) { openGenDesc(); }
    void			saveGenDescCB(CallBacker*) { saveGenDesc(); }
    void			manPropsCB(CallBacker*);
    void			snapshotCB(CallBacker*);
    void			synthDispParsChangedCB(CallBacker*);
    void			lmDispParsChangedCB(CallBacker*);
    void			selPropChgCB(CallBacker*);
    void			infoChanged(CallBacker*);
    void			selElasticPropsCB(CallBacker*);

    void			initWin(CallBacker*);
    void			dispEachChg(CallBacker*);
    void			mkSynthChg(CallBacker*);
    void			levelChg(CallBacker*);
    void			flattenChg(CallBacker*);
    void			lmViewChangedCB(CallBacker*);
    void			seqSel(CallBacker*);
    void			modEd(CallBacker*);
    void			modDispRangeChanged(CallBacker*);
    void			syntheticsChangedCB(CallBacker*);
    void			viewChgedCB(CallBacker*);
    void			wvltChg(CallBacker*);
    void			modSelChg(CallBacker*);
    void			genModels(CallBacker*);
    void			xPlotReq(CallBacker*);
    void			nrModelsChangedCB(CallBacker*);
    void			helpCB(CallBacker*);
};
