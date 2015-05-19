#ifndef datapackbase_h
#define datapackbase_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra and Helene Huck
 Date:		January 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"

#include "bindatadesc.h"
#include "bufstringset.h"
#include "datapack.h"
#include "position.h"
#include "trckeyzsampling.h"
#include "valseries.h"

template <class T> class Array2D;
template <class T> class Array3D;
template <class T> class Array3DImpl;

class FlatPosData;
class TaskRunner;
namespace ZDomain { class Info; }


/*!\brief DataPack for point data. */

mExpClass(General) PointDataPack : public DataPack
{
public:

    virtual int			size() const			= 0;
    virtual BinID		binID(int) const		= 0;
    virtual float		z(int) const			= 0;
    virtual Coord		coord(int) const;
    virtual int			trcNr(int) const		{ return 0; }

    virtual bool		simpleCoords() const		{ return true; }
				//!< If true, coords are always SI().tranform(b)
    virtual bool		isOrdered() const		{ return false;}
				//!< If yes, one can draw a line between the pts

protected:

				PointDataPack( const char* categry )
				    : DataPack( categry )	{}

};

/*!\brief DataPack for flat data.

  FlatPosData is initialized to ranges of 0 to sz-1 step 1.

  */

mExpClass(General) FlatDataPack : public DataPack
{
public:
				FlatDataPack(const char* categry,
					     Array2D<float>*);
				//!< Array2D become mine (of course)
				FlatDataPack(const FlatDataPack&);
				~FlatDataPack();

    virtual Array2D<float>&	data()			{ return *arr2d_; }
    const Array2D<float>&	data() const
				{ return const_cast<FlatDataPack*>(this)
							->data(); }

    virtual FlatPosData&	posData()		{ return posdata_; }
    const FlatPosData&		posData() const
				{ return const_cast<FlatDataPack*>(this)
							->posData(); }
    virtual const char*		dimName( bool dim0 ) const
				{ return dim0 ? "X1" : "X2"; }

    virtual Coord3		getCoord(int,int) const;
				//!< int,int = Array2D position
				//!< if not overloaded, returns posData() (z=0)

    virtual bool		posDataIsCoord() const	{ return true; }
				// Alternative positions for dim0
    virtual void		getAltDim0Keys(BufferStringSet&) const	{}
				//!< First one is 'default'
    virtual double		getAltDim0Value(int ikey,int idim0) const;
    virtual bool		isAltDim0InInt(const char* key) const
				{ return false; }

    virtual void		getAuxInfo(int idim0,int idim1,IOPar&) const {}

    virtual float		nrKBytes() const;
    virtual void		dumpInfo(IOPar&) const;

    virtual int			size(bool dim0) const;

protected:

				FlatDataPack(const char* category);
				//!< For this you have to overload data()
				//!< and the destructor

    Array2D<float>*		arr2d_;
    FlatPosData&		posdata_;

private:

    void			init();

};


/*!\brief DataPack for 2D data to be plotted on a Map. */

mExpClass(General) MapDataPack : public FlatDataPack
{
public:
				MapDataPack(const char* cat,
					    Array2D<float>*);
				~MapDataPack();

    Array2D<float>&		data();
    const Array2D<float>&	rawData() const	{ return *arr2d_; }
    FlatPosData&		posData();
    void			setDimNames(const char*,const char*,bool forxy);
    const char*			dimName( bool dim0 ) const;

				//!< Alternatively, it can be in Inl/Crl
    bool			posDataIsCoord() const	{ return isposcoord_; }
    void			setPosCoord(bool yn);
				//!< int,int = Array2D position
    virtual void		getAuxInfo(int idim0,int idim1,IOPar&) const;
    void			setProps(StepInterval<double> inlrg,
					 StepInterval<double> crlrg,
					 bool,BufferStringSet*);
    void			initXYRotArray(TaskRunner* = 0 );

    void			setRange( StepInterval<double> dim0rg,
					  StepInterval<double> dim1rg,
					  bool forxy );

protected:

    float			getValAtIdx(int,int) const;
    friend class		MapDataPackXYRotater;

    Array2D<float>*		xyrotarr2d_;
    FlatPosData&		xyrotposdata_;
    bool			isposcoord_;
    TypeSet<BufferString>	axeslbls_;
    Threads::Lock		initlock_;
};



/*!\brief DataPack for volume data, where the dims correspond to
          inl/crl/z . */

mExpClass(General) VolumeDataPack : public DataPack
{
public:

    virtual Array3D<float>&	data();
    const Array3D<float>&	data() const;

    virtual const char*		dimName(char dim) const;
    virtual double		getPos(char dim,int idx) const;
    int				size(char dim) const;
    virtual float		nrKBytes() const;
    virtual void		dumpInfo(IOPar&) const;


protected:
				VolumeDataPack(const char* categry,
					     Array3D<float>*);
				//!< Array3D become mine (of course)
				~VolumeDataPack();

				VolumeDataPack(const char* category);
				//!< For this you have to overload data()
				//!< and the destructor

    Array3D<float>*		arr3d_;
};



/*!\brief DataPack for volume data. Should be renamed to VolumeDataPack later*/

mExpClass(General) SeisDataPack : public DataPack
{
public:
				~SeisDataPack();

    virtual bool		is2D() const				= 0;
    virtual int			nrTrcs() const				= 0;
    virtual TrcKey		getTrcKey(int globaltrcidx) const	= 0;
    virtual int			getGlobalIdx(const TrcKey&) const	= 0;
    virtual int			getNearestGlobalIdx(const TrcKey&) const;

    virtual bool		addComponent(const char* nm)		= 0;

    virtual const StepInterval<float>&	getZRange() const		= 0;

    const OffsetValueSeries<float> getTrcStorage(
					int comp,int globaltrcidx) const;
    const float*		getTrcData(int comp,int globaltrcidx)const;

    int				nrComponents() const
				{ return arrays_.size(); }
    bool			isEmpty() const
				{ return arrays_.isEmpty(); }
    bool			validComp( int comp ) const
				{ return arrays_.validIdx( comp ); }
    const char*			getComponentName(int comp=0) const;

    const Array3DImpl<float>&	data(int component=0) const;
    Array3DImpl<float>&		data(int component=0);

    void			setZDomain(const ZDomain::Info&);
    const ZDomain::Info&	zDomain() const
				{ return *zdomaininfo_; }

    void			setScale(int comp,const SamplingData<float>&);
    const SamplingData<float>&	getScale(int comp) const;

    const BinDataDesc&		getDataDesc() const	{ return desc_; }

    float			nrKBytes() const;

protected:
				SeisDataPack(const char*,const BinDataDesc*);

    bool			addArray(int sz0,int sz1,int sz2);

    BufferStringSet			componentnames_;
    ObjectSet<Array3DImpl<float> >	arrays_;
    TypeSet<SamplingData<float> >	scales_;
    ZDomain::Info*			zdomaininfo_;
    BinDataDesc				desc_;
};

#endif
