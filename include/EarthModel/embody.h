#ifndef embody_h
#define embody_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: embody.h,v 1.5 2009-02-13 21:18:37 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "arraynd.h"
#include "emobject.h"
#include "samplingdata.h"

class TaskRunner;

namespace EM
{

/*!Implicit representation of a body. */

mStruct ImplicitBody
{
    				ImplicitBody();
    virtual			~ImplicitBody();

    Array3D<float>*		arr_;
    float 			threshold_;
    SamplingData<int>		inlsampling_;
    SamplingData<int>		crlsampling_;
    SamplingData<float>		zsampling_;

    ImplicitBody		operator =(const ImplicitBody&);
};


/*!A body that can deliver an implicit body. */

mClass Body
{ 
public:

    virtual ImplicitBody*	createImplicitBody(TaskRunner*) const;
    const IOObjContext&		getBodyContext() const;

    virtual void		refBody()	= 0;
    				//!<Should be mapped to EMObject::ref()
    virtual void		unRefBody()	= 0;
    				//!<Should be mapped to EMObject::unRef()
    virtual bool		useBodyPar(const IOPar&);
    				//!<Should be mapped to EMObject::usePar;
    virtual void                fillBodyPar(IOPar&) const;
    				//!<Should be mapped to EMObject::fillPar;
protected:
    				~Body()		{}
};


}; // Namespace

#endif
