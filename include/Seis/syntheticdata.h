#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki/Bruno
 Date:		July 2013
________________________________________________________________________

-*/

#include "seismod.h"
#include "flatview.h"
#include "datapack.h"
#include "synthgenparams.h"
#include "reflectivitymodel.h"

class RayTracer1D;
class SeisTrc;
class TimeDepthModel;


mStruct(Seis) SynthFVSpecificDispPars
{
				SynthFVSpecificDispPars();

    BufferString		colseqname_;
    RefMan<ColTab::MapperSetup>	vdmapsetup_;
    RefMan<ColTab::MapperSetup>	wvamapsetup_;
    float			overlap_;

    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);
};


/*! brief the basic synthetic dataset. contains the data cubes*/

mExpClass(Seis) SyntheticData : public RefCount::Referenced
			      , public NamedObject
{
public:

    typedef RefMan<ReflectivityModelSet>	RfMdlSetRef;

    mStruct(Seis) RayModel
    {

				RayModel(const RayTracer1D& rt1d,
					 int nroffsets);
				~RayModel();
	void			forceReflTimes(const StepInterval<float>&);
	void			getTraces(ObjectSet<SeisTrc>&,bool steal);
	void			getD2T(ObjectSet<TimeDepthModel>&,bool steal);
	void			getZeroOffsetD2T(TimeDepthModel&);
	RfMdlSetRef&		getRefs(bool sampled=false);
	const SeisTrc*		stackedTrc() const;

	ObjectSet<SeisTrc>	outtrcs_;
	ObjectSet<TimeDepthModel> t2dmodels_;
	TimeDepthModel*		zerooffset2dmodel_;
	RfMdlSetRef		refmodels_;
	RfMdlSetRef		sampledrefmodels_;
    };


    void			setName(const char*);

    virtual const SeisTrc*	getTrace(int seqnr) const = 0;

    float			getTime(float dpt,int seqnr) const;
    float			getDepth(float time,int seqnr) const;

    const DataPack&		getPack() const { return *datapack_; }
    DataPack&			getPack()	{ return *datapack_; }

    ObjectSet<const TimeDepthModel> d2tmodels_;
    ObjectSet<const TimeDepthModel> zerooffsd2tmodels_;

    DataPack::FullID		datapackid_;
    ObjectSet<RayModel>*	raymodels_;
    RfMdlSetRef			reflectivitymodels_;

    int				id_;
    virtual bool		isPS() const		= 0;
    virtual bool		hasOffset() const	= 0;
    virtual bool		isAngleStack() const;
    virtual bool		isAVOGradient() const { return false; }
    virtual SynthGenParams::SynthType synthType() const	= 0;

    virtual void		useGenParams(const SynthGenParams&);
    virtual void		fillGenParams(SynthGenParams&) const;
    void			useDispPar(const IOPar&);
    void			fillDispPar(IOPar&) const;
    DBKey			waveletID() const { return wvltid_; }
    BufferString		waveletName() const { return nameOf(wvltid_); }
    void			setWavelet( const DBKey& id ) { wvltid_ = id; }
    SynthFVSpecificDispPars&	dispPars()	{ return disppars_; }
    const SynthFVSpecificDispPars& dispPars() const
							{ return disppars_; }
    RfMdlSetRef			getRefModels(int modelid,bool sampled);
    void			setRayModels(ObjectSet<RayModel>& rms );
    bool			haveSameRM(const IOPar& par1,
					   const IOPar& par2) const;
    const IOPar&		getRayPar() const	{ return raypars_; }
    void			updateD2TModels();
    void			adjustD2TModels(ObjectSet<TimeDepthModel>&);

protected:
				SyntheticData(const SynthGenParams&,DataPack&);
				~SyntheticData();

    DBKey			wvltid_;
    IOPar			raypars_;
    SynthFVSpecificDispPars	disppars_;

    void			removePack();

    RefMan<DataPack>		datapack_;

};
