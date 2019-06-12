#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
________________________________________________________________________

-*/

#include "attribprovider.h"
#include "bindatadesc.h"
#include "datapack.h"
#include "trckeyzsampling.h"

class BufferStringSet;
class RegularSeisDataPack;
class SeisTrc;
namespace Seis { class MSCProvider; }

namespace PosInfo{ class LineSet2DData; }

namespace Attrib
{

class DataHolder;

/*!\brief Attribute storage provider. */

mExpClass(AttributeEngine) StorageProvider : public Provider
{ mODTextTranslationClass(StorageProvider)
public:

    static void		initClass();
    static const char*  attribName()		{ return "Storage"; }
    static const char*  keyStr()		{ return "id"; }

    int			moveToNextTrace(BinID startpos=BinID(-1,-1),
					bool firstcheck=false);
    bool		calcPossibleSubsel(int outp,FullSubSel&) override;
    BinID		getStepoutStep() const;
    void		updateStorageReqs(bool all=true);
    GeomID		getGeomID() const;

    void		fillDataPackWithTrc(RegularSeisDataPack*) const;
    bool		needStoredInput() const	{ return true; }
    virtual void	getCompNames(BufferStringSet&) const;
    float		getDistBetwTrcs(bool,GeomID g=GeomID()) const override;
    virtual bool	compDistBetwTrcsStats(bool force=false);

protected:

			StorageProvider(Desc&);
			~StorageProvider();

    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);
    static void		updateDescAndGetCompNms(Desc&,BufferStringSet*);

    bool		getLine2DStoredSubSel();
    bool		checkInpAndParsAtStart();
    bool		allowParallelComputation() const { return false; }

    //From disc
    Seis::MSCProvider*	getMSCProvider(bool&) const;
    bool		initMSCProvider();
    bool		setMSCProvSelData();

    //From memory (in Attrib::DataPacks)
    SeisTrc*		getTrcFromPack(const BinID&,int) const;
    DataPack::FullID	getDPID() const;

    void		setReqBufStepout(const BinID&,bool wait=false);
    void		setDesBufStepout(const BinID&,bool wait=false);
    bool		computeData(const DataHolder& output,
				    const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;

    bool		fillDataHolderWithTrc(const SeisTrc*,
					      const DataHolder&) const;
    bool		getZStepStoredData(float& step) const
			{ step = storedvolume_.zsamp_.step; return true; }
    bool		getZ0StoredData(float& z0) const
			{ z0 = storedvolume_.zsamp_.start; return true; }

    BinDataDesc		getOutputFormat(int output) const;

    bool		checkDesiredTrcRgOK(StepInterval<int>,
					    StepInterval<float>);
    bool		checkDesiredSubSelOK();
    void		checkClassType(const SeisTrc*,BoolTypeSet&) const;
    bool		setTableSelData();
    bool		set2DRangeSelData();

    void		registerNewPosInfo(SeisTrc*,const BinID&,bool,bool&);
    bool                useInterTrcDist() const;

    enum Status		{ None, StorageOpened, Ready };

    Status		status_;
    TypeSet<BinDataDesc> datachar_;
    Seis::MSCProvider*	mscprov_;
    BinID		stepoutstep_;
    FullSubSel		storedsubsel_;
    bool		isondisk_;
    bool		useintertrcdist_;
    PosInfo::LineSet2DData*  ls2ddata_;

    friend class	Provider;
};

}; // namespace Attrib
