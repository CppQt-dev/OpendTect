#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2007
________________________________________________________________________

*/


#include "seisdatapack.h"
#include "trckeyzsampling.h"
#include "perthreadrepos.h"
#include "uistring.h"

template <class T> class Array2D;
class SeisTrc;
class IOObj;
class TaskRunner;


mExpClass(Seis) SeisFixedCubeProvider
{ mODTextTranslationClass(SeisFixedCubeProvider);
public:
			SeisFixedCubeProvider(const DBKey&);
			~SeisFixedCubeProvider();

    void		clear();
    bool		isEmpty() const;
    bool		readData(const TrcKeyZSampling&,TaskRunner* tskr=0);
    bool		readData(const TrcKeyZSampling&,
				const Pos::GeomID geomid, TaskRunner* tskr = 0);

    const SeisTrc*	getTrace(const BinID&) const;
    const SeisTrc*	getTrace(int trcnr) const;
    float		getTrcDist() const		{ return trcdist_; }
    uiString		errMsg() const;

protected:

    Array2D<SeisTrc*>*	data_;
    RefMan<SeisVolumeDataPack> seisdp_;
    Pos::GeomID		geomid_;
    mutable PerThreadObjectRepository<SeisTrc> dptrc_;

    TrcKeyZSampling	tkzs_;
    IOObj*		ioobj_;
    uiString		errmsg_;
    float		trcdist_;

    bool		calcTrcDist(const Pos::GeomID);

};
