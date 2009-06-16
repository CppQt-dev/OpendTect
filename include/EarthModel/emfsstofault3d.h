#ifndef emfsstofault3d_h
#define emfsstofault3d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	J.C. Glas
 Date:		March 2009
 RCS:		$Id: emfsstofault3d.h,v 1.3 2009-06-16 07:31:01 cvsjaap Exp $
________________________________________________________________________


-*/

#include "emposid.h"
#include "position.h"


namespace Geometry { class FaultStickSet; }

namespace EM
{

class FaultStickSet;
class Fault3D;

mClass FSStoFault3DConverter
{
public:

    mClass Setup
    {
    public:
				Setup();

	enum			DirSpec { Auto, Horizontal, Vertical };

	mDefSetupMemb(DirSpec,pickplanedir);		// Default Auto
	mDefSetupMemb(bool,sortsticks);			// Default true
	mDefSetupMemb(double,zscale);			// Default SI().zScale()

	mDefSetupMemb(bool,useinlcrlslopesep);		// Default false
	mDefSetupMemb(double,stickslopethres);		// Default mUdf(double)
	mDefSetupMemb(DirSpec,stickseldir);		// Default Auto
    };

				FSStoFault3DConverter(const Setup&,
						      const FaultStickSet&,
						      Fault3D&);  
    bool			convert();

protected:

    mClass FaultStick
    {
    public:
				FaultStick(int sticknr);

	int			sticknr_;
	TypeSet<Coord3>		crds_;
	Coord3			normal_;
	bool			pickedonplane_;

	Coord3			findPlaneNormal() const;
	double			slope(double zscale) const;
	bool			pickedOnInl() const;
	bool			pickedOnCrl() const;
	bool			pickedOnTimeSlice() const;
    };

    const FaultStickSet&	fss_;
    Fault3D&			fault3d_;
    Setup			setup_;
    ObjectSet<FaultStick>	sticks_;

    const Geometry::FaultStickSet* curfssg_;

    bool			readSection(const SectionID&);
    bool			preferHorPicked() const;
    void			selectSticks( bool selhorpicked );
    void			geometricSort(double zscale);
    void			untwistSticks(double zscale);
    void			resolveUdfNormals();
    bool			writeSection(const SectionID&) const;
};


} // namespace EM

#endif
