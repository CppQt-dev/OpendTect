#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2007 / Feb 2019
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "ranges.h"
#include "binid.h"
#include "bufstring.h"
#include "geomid.h"


namespace Seis
{

class PolySelData;
class RangeSelData;
class SelDataPosIter;
class TableSelData;


/*!\brief encapsulates subselection from a cube or lines.

  This class exists so that without knowing the form of the subselection,
  other classes can find out whether a trace is included or not.
  Use one of the isOK() functions for this.

  The function selRes() returns an integer which gives more information than
  just yes/no. If 0 is returned, the position is included. If non-zero,
  the inline or crossline number can be one of:

  0 - this number is OK by itself, but not the combination
  1 - this number is the 'party-pooper' but there are selected posns with it
  2 - No selected position has this number

  Especially (2) is very useful: an entire inl or crl can be skipped from input.
  The return value of selRes is inl_result + 256 * crl_result.

  If you're not interested in all that, just use isOK().

 */

mExpClass(Seis) SelData
{
public:

    mUseType( Pos::IdxPair,	pos_type );
    mUseType( Pos,		GeomID );
    typedef int			idx_type;
    typedef idx_type		size_type;
    typedef float		z_type;
    typedef SelType		Type;
    typedef SelDataPosIter	PosIter;
    typedef Interval<pos_type>	pos_rg_type;
    typedef Interval<z_type>	z_rg_type;
    typedef pos_type		trcnr_type;

    virtual		~SelData()			{}
    virtual SelData*	clone() const			= 0;
    void		copyFrom(const SelData&);
    static SelData*	get(Type);			//!< empty
    static SelData*	get(const IOPar&);		//!< fully filled
    static SelData*	get(const DBKey&);		//!< fully filled

    virtual Type	type() const			= 0;
    virtual bool	is2D() const			{ return false; }
    virtual bool	isAll() const			{ return false; }
    bool		isOK(const TrcKey&) const;
    inline bool		isOK( const BinID& b ) const	{ return !selRes(b); }
    inline bool		isOK( GeomID gid, trcnr_type trcnr ) const
			{ return !selRes(gid,trcnr); }

    virtual PosIter*	posIter() const			= 0;
    virtual pos_rg_type inlRange() const		= 0;
    virtual pos_rg_type crlRange() const		= 0;
    virtual pos_rg_type trcNrRange(idx_type i=0) const	{ return crlRange(); }
    virtual z_rg_type	zRange(idx_type i=0) const;

    uiString		usrSummary() const;
    virtual size_type	expectedNrTraces() const	= 0;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
    static void		removeFromPar(IOPar&,const char* subky=0);

    void		extendZ( const z_rg_type&);
    void		extendH(const BinID& stepout,
				const BinID* stepoutstep=0);
    void		include(const SelData&);
    virtual void	setZRange(const z_rg_type&,idx_type i=0) = 0;

    virtual size_type	nrGeomIDs() const		{ return 1; }
    virtual GeomID	geomID( idx_type i=0 ) const	{ return gtGeomID(i); }
    bool		hasGeomID(GeomID) const;

    bool		isRange() const		{ return type() == Range; }
    bool		isTable() const		{ return type() == Table; }
    bool		isPoly() const		{ return type() == Polygon; }
    RangeSelData*	asRange();
    const RangeSelData*	asRange() const;
    TableSelData*	asTable();
    const TableSelData*	asTable() const;
    PolySelData*	asPoly();
    const PolySelData*	asPoly() const;

protected:

			SelData()			{}

    size_type		nrTrcsInSI() const;
    virtual GeomID	gtGeomID( idx_type ) const { return GeomID::get3D(); }

    virtual void	doCopyFrom(const SelData&)	= 0;
    virtual void	doExtendH(BinID so,BinID sostp)	= 0;
    virtual void	doExtendZ(const z_rg_type&)	= 0;
    virtual void	doFillPar(IOPar&) const		= 0;
    virtual void	doUsePar(const IOPar&)		= 0;
    virtual uiString	gtUsrSummary() const		= 0;
    virtual int		selRes3D(const BinID&) const	= 0;
    virtual int		selRes2D(GeomID,trcnr_type) const;

    static const char*	sNrLinesKey();

public:

    inline int		selRes( const BinID& bid ) const
			{ return selRes3D(bid); }
    inline int		selRes( GeomID gid, trcnr_type tnr ) const
			{ return selRes2D(gid,tnr); }

};


inline bool isAll( const SelData* sd )
{
    return !sd || sd->isAll();
}


/*!\brief needs a next() before a valid position is reached. */

mExpClass(Seis) SelDataPosIter
{
public:

    mUseType( SelData,	trcnr_type );
    mUseType( Pos,	GeomID );

    virtual		~SelDataPosIter()	{}
    virtual SelDataPosIter* clone() const	= 0;

    virtual bool	next()			= 0;
    virtual void	reset()			= 0;

    const SelData&	selData() const		{ return sd_; }
    virtual bool	is2D() const		{ return false; }

    virtual GeomID	geomID() const		{ return GeomID::get3D(); }
    virtual trcnr_type	trcNr() const;
    virtual BinID	binID() const		= 0;
    void		getTrcKey(TrcKey&) const;

protected:

			SelDataPosIter(const SelData&);
			SelDataPosIter(const SelDataPosIter&);

    const SelData&	sd_;

};



} // namespace
