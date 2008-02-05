#ifndef seisselection_h
#define seisselection_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Nov 2007
 RCS:		$Id: seisselection.h,v 1.3 2008-02-05 14:25:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "ranges.h"
class IOPar;
class BinID;
class LineKey;
namespace Pos { class Provider; }


namespace Seis
{

/*!\brief setup for subselection of seismic data */

class SelSetup
{
public:

		SelSetup( Seis::GeomType gt )
		    : is2d_(Seis::is2D(gt))
		    , isps_(Seis::isPS(gt))
		    , allowtable_(false)		//!< 3D only
		    , allowpoly_(false)			//!< 3D only
		    , fornewentry_(false)		//!< 2D only
		    , multiline_(false)			//!< 2D only
		    , withoutz_(false)
		    , withstep_(true)			{}
		SelSetup( bool is_2d, bool is_ps=false )
		    : is2d_(is_2d)
		    , isps_(is_ps)
		    , allowtable_(false)		//!< 3D only
		    , allowpoly_(false)			//!< 3D only
		    , fornewentry_(false)		//!< 2D only
		    , multiline_(false)			//!< 2D only
		    , withoutz_(false)
		    , withstep_(true)			{}

    mDefSetupClssMemb(SelSetup,bool,is2d)
    mDefSetupClssMemb(SelSetup,bool,isps)
    mDefSetupClssMemb(SelSetup,bool,allowtable)
    mDefSetupClssMemb(SelSetup,bool,allowpoly)
    mDefSetupClssMemb(SelSetup,bool,fornewentry)
    mDefSetupClssMemb(SelSetup,bool,multiline)
    mDefSetupClssMemb(SelSetup,bool,withoutz)
    mDefSetupClssMemb(SelSetup,bool,withstep)

    Seis::GeomType geomType() const	{ return geomTypeOf(is2d_,isps_); }

};


/*!\brief contains input (sub-)selection data from a cube or lineset

  This class exists so that without knowing the form of the subselection,
  other classes can find out whether a trace is included or not.
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

class SelData
{
public:

    virtual		~SelData();

    typedef SelType	Type;
    virtual Type	type() const		= 0;
    static SelData*	get(Type);		//!< empty
    static SelData*	get(const IOPar&);	//!< fully filled
    static SelData*	get(const Pos::Provider&); //!< filled; some defaults
    virtual SelData*	clone() const		= 0;
    virtual void	copyFrom(const SelData&) = 0;	

    bool		isAll() const		{ return isall_; }
    void		setIsAll( bool yn=true ) { isall_ = yn; }
    inline bool		isOK( const BinID& b ) const	{ return !selRes(b); }
    virtual int		selRes(const BinID&) const	= 0; //!< see class doc

    virtual Interval<float> zRange() const;
    virtual bool	setZRange(Interval<float>) { return false; }
    virtual Interval<int> inlRange() const;
    virtual bool	setInlRange(Interval<int>) { return false; }
    virtual Interval<int> crlRange() const;
    virtual bool	setCrlRange(Interval<int>) { return false; }
    virtual int		expectedNrTraces(bool for2d=false,
	    				 const BinID* step=0) const = 0;

    virtual void	fillPar(IOPar&) const		= 0;
    virtual void	usePar(const IOPar&)		= 0;
    static void		removeFromPar(IOPar&);

    virtual void	extendZ(const Interval<float>&)	= 0;
    virtual void 	extendH(const BinID& stepout,
	    			const BinID* stepoutstep=0);
    virtual void	include(const SelData&)		= 0;

    			// Interesting in some 2D situations:
    inline LineKey&	lineKey()		{ return linekey_; }
    inline const LineKey& lineKey() const	{ return linekey_; }

protected:

    			SelData();

    bool		isall_;
    LineKey&		linekey_;	//!< 2D only

    int			tracesInSI() const;
    virtual void 	doExtendH(BinID stepout,BinID stepoutstep) = 0;
};


inline bool isEmpty( const SelData* sd )
{
    return !sd || sd->isAll();
}


} // namespace

#endif
