#ifndef uiiosurfacedlg_h
#define uiiosurfacedlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.h,v 1.5 2003-08-15 13:19:49 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class IOObj;
class uiIOSurface;
class uiSurfaceOutSel;
class MultiID;

namespace EM { class Horizon; class SurfaceIODataSelection; };


/*! \brief Dialog for horizon export */

class uiWriteSurfaceDlg : public uiDialog
{
public:
			uiWriteSurfaceDlg(uiParent*,const EM::Horizon&);

    bool		auxDataOnly() const;
    bool		surfaceOnly() const;
    bool		surfaceAndData() const;
    void		getSelection(EM::SurfaceIODataSelection&);
    IOObj*		ioObj() const;

protected:

    int			auxdataidx;

    uiSurfaceOutSel*	iogrp;
    const EM::Horizon&	hor;

    bool		checkIfAlreadyPresent(const char*);
    bool		acceptOK(CallBacker*);
};


class uiReadSurfaceDlg : public uiDialog
{
public:
                        uiReadSurfaceDlg(uiParent*);

    IOObj*		ioObj() const;
    void		getSelection(EM::SurfaceIODataSelection&);

protected:

    uiIOSurface*        iogrp;

    bool                acceptOK(CallBacker*);
};


#endif
