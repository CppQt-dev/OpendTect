#ifndef vishorizon2ddisplay_h
#define vishorizon2ddisplay_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          May 2004
 RCS:           $Id: vishorizon2ddisplay.h,v 1.2 2007-01-16 14:28:21 cvsjaap Exp $
________________________________________________________________________


-*/


#include "emposid.h"
#include "multiid.h"
#include "visemobjdisplay.h"

namespace visBase { class DrawStyle; class IndexedPolyLine; class PointSet; }
namespace EM { class Horizon2D; }


namespace visSurvey
{

class Horizon2DDisplay :  public  EMObjectDisplay
{
public:
    static Horizon2DDisplay*	create()
				mCreateDataObj(Horizon2DDisplay);
    void			setDisplayTransformation(mVisTrans*);

    EM::SectionID		getSectionID(int visid) const;

protected:
    				~Horizon2DDisplay();
    void			removeSectionDisplay(const EM::SectionID&);
    bool			addSection(const EM::SectionID&);

    void			updateSection(int);
    void			emChangeCB(CallBacker*);

    void			fillPar(IOPar&,TypeSet<int>&) const;
    int				usePar(const IOPar&);

    ObjectSet<visBase::IndexedPolyLine>	lines_;
    ObjectSet<visBase::PointSet>	points_;
    TypeSet<EM::SectionID>		sids_;

    bool			burstalertison_;
    int				postponedcallbacks_;
};


};

#endif
