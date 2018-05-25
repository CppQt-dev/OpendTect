#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2009
________________________________________________________________________


-*/

#include "seiscommon.h"
#include "datachar.h"
#include "datapackbase.h"
#include "dbkey.h"
#include "ranges.h"
#include "survgeom.h"
#include "taskrunner.h"

class IOObj;
class Scaler;
class TrcKeyZSampling;

namespace Seis
{

mExpClass(Seis) PreLoader
{ mODTextTranslationClass(PreLoader);
public:
			PreLoader(const DBKey&,
				Pos::GeomID=Survey::GM().default3DSurvID(),
				TaskRunner* =0);

    const DBKey&	id() const			{ return dbkey_; }
    Pos::GeomID		geomID() const			{ return geomid_; }
    void		setTaskRunner( TaskRunner& t )	{ tr_ = &t; }

    IOObj*		getIOObj() const;
    Interval<int>	inlRange() const;
			//!< PS 3D only. If nothing there: ret.start==mUdf(int)
    void		getLineNames(BufferStringSet&) const;
			//!< Line 2D only.

    bool		load(const TrcKeyZSampling&,
				DataCharacteristics::UserType=OD::AutoDataRep,
				const Scaler* =0) const;
    bool		load(const TypeSet<TrcKeyZSampling>&,
			     const TypeSet<Pos::GeomID>&,
				DataCharacteristics::UserType=OD::AutoDataRep,
				const Scaler* =0) const;
    bool		loadPS3D(const Interval<int>* inlrg=0) const;
    bool		loadPS2D(const char* lnm=0) const;	//!< null => all
    bool		loadPS2D(const BufferStringSet&) const;

    void		unLoad() const;
    uiString		errMsg() const			{ return errmsg_; }

    static void		load(const IOPar&,TaskRunner* tskr=0);
			//!< Seis.N.[loadObj_fmt]
    static void		loadObj(const IOPar&,TaskRunner* tskr=0);
			//!< sKey::ID() and optional subselections
    void		fillPar(IOPar&) const;

    static const char*	sKeyLines();
    static const char*	sKeyUserType();

protected:

    DBKey		dbkey_;
    Pos::GeomID		geomid_;
    TaskRunner*		tr_;
    SilentTaskRunner	deftr_;
    mutable uiString	errmsg_;

    TaskRunner&		getTr() const
			{ return *((TaskRunner*)(tr_ ? tr_ : &deftr_)); }
};


mExpClass(Seis) PreLoadDataEntry
{
public:
			PreLoadDataEntry(const DBKey&,Pos::GeomID,int dpid);

    bool		equals(const DBKey&,Pos::GeomID) const;

    DBKey		dbkey_;
    Pos::GeomID		geomid_;
    int			dpid_;
    bool		is2d_;
    BufferString	name_;
};


mExpClass(Seis) PreLoadDataManager
{
public:
    void			add(const DBKey&,DataPack*);
    void			add(const DBKey&,Pos::GeomID,DataPack*);
    void			remove(const DBKey&,Pos::GeomID =-1);
    void			remove(int dpid);
    void			removeAll();

    RefMan<DataPack>		get(const DBKey&,Pos::GeomID =-1);
    RefMan<DataPack>		get(int dpid);
    ConstRefMan<DataPack>	get(const DBKey&,Pos::GeomID =-1) const;
    ConstRefMan<DataPack>	get(int dpid) const;
    template<class T>
    inline RefMan<T>		getAndCast(const DBKey&,Pos::GeomID =-1);

    void			getInfo(const DBKey&,Pos::GeomID,
					BufferString&) const;

    void			getIDs(DBKeySet&) const;
    bool			isPresent(const DBKey&,Pos::GeomID =-1) const;

    const ObjectSet<PreLoadDataEntry>& getEntries() const;

protected:

    DataPackMgr&			dpmgr_;
    ManagedObjectSet<PreLoadDataEntry>	entries_;

public:
					PreLoadDataManager();
					~PreLoadDataManager();
};

mGlobal(Seis) PreLoadDataManager& PLDM();


template <class T> inline
RefMan<T> PreLoadDataManager::getAndCast( const DBKey& dbky, Pos::GeomID gid )
{
    RefMan<DataPack> dp = get( dbky, gid );
    mDynamicCastGet( T*, casted, dp.ptr() );
    return RefMan<T>( casted );
}

} // namespace Seis
