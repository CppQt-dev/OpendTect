#ifndef visaxes_h
#define visaxes_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          January 2013
 RCS:           $Id$
________________________________________________________________________

-*/


#include "visdata.h"

class FontData;
namespace osgGeo{ class AxesNode; }

namespace visBase
{

class Camera;
    
mExpClass(visBase) Axes : public DataObject
{
public:
    static Axes*		create()
    				mCreateDataObj(Axes);
    void			setRadius(float);
    float			getRadius() const;
    void			setLength(float);
    float			getLength() const;
    void			setPosition(float,float);
    void			setSize(float rad, float len);
    void			setAnnotationColor(const Color&);
    void			setAnnotationTextSize(int);
    void			setAnnotationFont(const FontData&);
    void			setMasterCamera(visBase::Camera*);

    virtual void		setPixelDensity(float dpi);

protected:
    				~Axes();

    osgGeo::AxesNode*		axesnode_;
    Camera*			mastercamera_;
    bool			ison_;
    float			pixeldensity_;
    int				annottextsize_;
};

} // namespace visBase
#endif

