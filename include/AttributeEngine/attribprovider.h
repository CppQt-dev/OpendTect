#ifndef attribprovider_h
#define attribprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribprovider.h,v 1.8 2005-05-09 14:40:01 cvshelene Exp $
________________________________________________________________________

-*/

#include "refcount.h"
#include "position.h"
#include "ranges.h"
#include "sets.h"
#include "survinfo.h"

class BasicTask;
class CubeSampling;
class SeisRequester;
namespace Threads { class ThreadWorkManager; };

namespace Attrib
{

class DataHolder;
class DataHolderLineBuffer;
class Desc;
class ProviderBasicTask;


/*!\brief provides the actual output to ... */

class Provider
{				mRefCountImpl(Provider);

    friend class		ProviderBasicTask;

public:

    static Provider*		create(Desc&);
				/*!< Also creates all inputs, the input's
				     inputs, and so on */
    virtual bool		isOK() const;

    const Desc&			getDesc() const;
    Desc&			getDesc();
    virtual const DataHolder*	getData(const BinID& relpos=BinID(0,0));
    virtual const DataHolder*	getDataDontCompute(const BinID& relpos) const;

    void			enableOutput(int output,bool yn=true);
    bool			isOutputEnabled(int output) const;

    void			setBufferStepout(const BinID&);
    const BinID&		getBufferStepout() const;
    void			setDesiredVolume( const CubeSampling& );
    virtual bool		getPossibleVolume(int outp,CubeSampling&);
    int				getTotalNrPos(bool);

    virtual int			moveToNextTrace();
    				/*!<\retval -1	something went wrong
				    \retval  0  finished, no more positions
				    \retval  1	arrived at new position
				*/
				    
    BinID			getCurrentPosition() const;
    virtual bool		setCurrentPosition( const BinID& );
    void			addLocalCompZInterval(const Interval<int>&);
    const Interval<int>&	localCompZInterval() const;

    void               		updateInputReqs(int input=-1);
    int				getTraceNr () { return trcnr_; }
    float                       getRefStep() const { return refstep; }

protected:

			Provider( Desc& );
    virtual bool	init();
    				/*!< Should be run _after_ inputs are set */

    virtual SeisRequester* getSeisRequester();
    static Provider*	internalCreate(Desc&,ObjectSet<Provider>&);

    virtual bool	getInputOutput( int input, TypeSet<int>& ) const;
    virtual bool	getInputData( const BinID& relpos );
    virtual bool	computeData( const DataHolder& output,
	    			     const BinID& relpos,
	    			     int t1, int nrsamples ) const
    			{ return false; }

    virtual bool	allowParallelComputation() const { return true; }

    			//DataBuffer stuff
    DataHolder*		getDataHolder( const BinID& relpos );
    void		removeDataHolder( const BinID& relpos );
    void		setInput( int input, Provider* );
    bool		computeDesInputCube( int inp, int out,
					     CubeSampling& ) const;

    virtual const BinID*	desStepout(int input, int output) const;
    virtual const BinID*	reqStepout(int input, int output) const;
    virtual const Interval<float>* desZMargin(int input, int output) const;
    virtual const Interval<float>* reqZMargin(int input, int output) const;
    virtual bool		getZStepStoredData(float& step) const
				{return false;}

    void			computeRefZStep(const ObjectSet<Provider>&);
    void			propagateZRefStep( const ObjectSet<Provider>& );
    
    bool                        zIsTime() const {return SI().zIsTime();}
    float			zFactor() const { return zIsTime() ? 1000 : 1;}
    float			dipFactor() const {return zIsTime() ? 1e6: 1e3;}
    int				inlStep(bool work=true) const 
    					{return SI().inlStep(work);}
    int				crlStep(bool work=true) const
					{return SI().crlStep(work);}
    float				inldist() const 
					{return SI().transform(BinID(0,0)).
					 distance(SI().transform(BinID(1,0)));}
    float			crldist() const
	                                {return SI().transform(BinID(0,0)).
					 distance(SI().transform(BinID(1,0)));}

    ObjectSet<Provider>		inputs;
    Desc&			desc;
    TypeSet<int>		outputinterest;
    BinID			bufferstepout;
    CubeSampling*		desiredvolume;
    CubeSampling*               possiblevolume;
    Interval<int>		localcomputezinterval;

    Threads::ThreadWorkManager*	threadmanager;
    ObjectSet<BasicTask>	computetasks;
    DataHolderLineBuffer*	linebuffer;
    BinID			currentbid;

    float                       refstep;
    int				trcnr_;

};


int getSteeringIndex( const BinID& );
//!< For every position there is a single steering index ...?


}; //namespace


#endif

