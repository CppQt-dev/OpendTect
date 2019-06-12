#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H.Payraudeau
 Date:          04/2005
________________________________________________________________________

-*/

#include "attribdescid.h"
#include "attribsel.h"
#include "sets.h"
#include "ranges.h"
#include "bufstring.h"
#include "uistring.h"

class BinIDValueSet;
class BufferStringSet;
class DataPackMgr;
class CubeSubSel;
class DataPointSet;
class Executor;
class NLAModel;
class RegularSeisDataPack;
class SeisTrcBuf;
class SeisTrcInfo;

namespace Attrib
{
class SeisTrcStorOutput;
class Processor;
class Data2DHolder;

/*!\brief The attribute engine manager. */

mExpClass(AttributeEngine) EngineMan
{ mODTextTranslationClass(Attrib::EngineMan);
public:
			EngineMan();
    virtual		~EngineMan();

    Processor*		usePar(const IOPar&,DescSet&,
			       const char* linename,uiString&,int outputidx);

    static Processor*	createProcessor(const DescSet&,const char*,
					const DescID&,uiString& errmsg);
    static void		getPossibleExtents(DescSet&,CubeSubSel&,const DescID&);
    static void		getPossibleExtents(DescSet&,LineSubSel&,
					  const char* linename,const DescID&);
    static void		addNLADesc(const char*,DescID&,DescSet&,int,
				   const NLAModel*,uiString&);

    SeisTrcStorOutput*	createOutput(const IOPar&,Pos::GeomID,uiString&,
				     int outidx);

    const DescSet*	attribSet() const	{ return attrset_; }
    const NLAModel*	nlaModel() const	{ return nlamodel_; }
    const GeomSubSel&	getSubSel() const	{ return *subsel_; }
    Pos::GeomID		getGeomID() const	{ return geomid_; }
    float		undefValue() const	{ return udfval_; }

    void		setAttribSet(const DescSet*);
    void		setNLAModel(const NLAModel*);
    void		setAttribSpec(const SelSpec&);
    void		setAttribSpecs(const SelSpecList&);
    void		setTrcKeyZSampling(const TrcKeyZSampling&);
    void		setGeomID( const Pos::GeomID geomid )
			{ geomid_ = geomid; }
    void		setUndefValue( float v )	{ udfval_ = v; }
    DescSet*		createNLAADS(DescID& outid,uiString& errmsg,
				     const DescSet* addtoset=0);
    static DescID	createEvaluateADS(DescSet&, const TypeSet<DescID>&,
					  uiString&);

    Processor*		createDataPackOutput(uiString& errmsg,
				      const RegularSeisDataPack* cached_data=0);
			//!< Give the previous calculated data in cached data
			//!< and some parts may not be recalculated.
			//!< is used to create stuff for many regular visualization elements.

    RefMan<RegularSeisDataPack> getDataPackOutput(const Processor&);
    RefMan<RegularSeisDataPack> getDataPackOutput(
				   const ObjectSet<const RegularSeisDataPack>&);

    Executor*		createFeatureOutput(const BufferStringSet& inputs,
					    const ObjectSet<BinIDValueSet>&);

    Processor*		createScreenOutput2D(uiString& errmsg,
					     Data2DHolder&);
    Processor*		createLocationOutput(uiString& errmsg,
					     ObjectSet<BinIDValueSet>&);

    Processor*		createTrcSelOutput(uiString& errmsg,
					   const BinIDValueSet& bidvalset,
					   SeisTrcBuf&, float outval=0,
					   const Interval<float>* cubezbounds=0,
					   const TypeSet<BinID>* trueknotspos=0,
					   const TypeSet<BinID>* path=0);
    Processor*		create2DVarZOutput(uiString& errmsg,
					   const IOPar& pars,
					   DataPointSet* bidvalset,
					   float outval=0,
					   Interval<float>* cubezbounds = 0);
    Processor*		getTableOutExecutor(DataPointSet& datapointset,
					    uiString& errmsg,
					    int firstcol);
    Executor*		getTableExtractor(DataPointSet&,const Attrib::DescSet&,
					  uiString& errmsg,int firstcol =0,
					  bool needprep=true);
    static bool		ensureDPSAndADSPrepared(DataPointSet&,
						const Attrib::DescSet&,
						uiString& errmsg);
    int			getNrOutputsToBeProcessed(const Processor&) const;

    const char*		getCurUserRef() const;
    void		computeIntersect2D(ObjectSet<BinIDValueSet>&) const;

    bool		hasCache() const		{ return cache_; }

protected:

    const DescSet*	attrset_;
    const NLAModel*	nlamodel_;
    GeomSubSel*		subsel_;
    float		udfval_;
    Pos::GeomID		geomid_;
    DataPackMgr&	dpm_;

    const RegularSeisDataPack*	cache_;

    DescSet*		procattrset_;
    int			curattridx_;
    SelSpecList		attrspecs_;

    Processor*		getProcessor(uiString& err);
    void		setExecutorName(Executor*);

private:

    friend class		AEMFeatureExtracter;//TODO will soon be removed
    friend class		AEMTableExtractor;
};

} // namespace Attrib
