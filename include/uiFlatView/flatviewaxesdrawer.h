#ifndef flatviewaxesdrawer_h
#define flatviewaxesdrawer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicssceneaxismgr.h"

class uiFlatViewer;

/*!
\brief Axis drawer for flat viewers.
*/

mExpClass(uiFlatView) AxesDrawer : public ::uiGraphicsSceneAxisMgr
{
public:
			AxesDrawer(uiFlatViewer&);
			~AxesDrawer();

    int			altdim0_;
    void		updateScene();
    void		setZValue(int z);
    void		updateViewRect();
    uiRect		getViewRect(bool withextraborders=true) const;
    void		setWorldCoords(const uiWorldRect&);
    void		setExtraBorder(const uiBorder&);
    uiBorder		getAnnotBorder(bool withextraborders=true) const;

protected:

    uiFlatViewer&	vwr_;
    uiRectItem*         rectitem_;
    uiTextItem*         axis1nm_;
    uiTextItem*         axis2nm_;
    uiTextItem*		titletxt_;
    uiArrowItem*        arrowitem1_;
    uiArrowItem*        arrowitem2_;
    uiBorder		extraborder_;
};

#endif


