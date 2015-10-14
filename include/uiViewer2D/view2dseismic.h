#ifndef view2dseismic_h
#define view2dseismic_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiviewer2dmod.h"
#include "view2ddata.h"


mExpClass(uiViewer2D) VW2DSeis : public Vw2DDataObject
{
public:
			VW2DSeis();
			~VW2DSeis(){}

    NotifierAccess*	deSelection()	{ return &deselted_; }

protected:

    void		triggerDeSel();

    Notifier<VW2DSeis>	deselted_;
};


#endif


