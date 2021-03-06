/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          July 2002
________________________________________________________________________

-*/

#include "vismarkerset.h"

#include "iopar.h"
#include "survinfo.h"
#include "vistransform.h"
#include "visosg.h"
#include "vismaterial.h"
#include "vispolygonoffset.h"

#include <osgGeo/MarkerSet>
#include <osgGeo/MarkerShape>

#include <math.h>

#define mOSGMarkerScaleFactor 2.5f


mCreateFactoryEntry( visBase::MarkerSet );

namespace visBase
{

MarkerSet::MarkerSet()
    : VisualObjectImpl(true)
    , markerset_( new osgGeo::MarkerSet )
    , displaytrans_( 0 )
    , coords_( Coordinates::create() )
    , onoffarr_( new osg::ByteArray() )
    , pixeldensity_( getDefaultPixelDensity() )
    , rotationvec_( 1.0, 0.0, 0.0 )
    , rotationangle_( mUdf(float) )
    , offset_( 0 )
{
    markerset_->ref();
    onoffarr_->ref();
    addChild( markerset_ );
    markerset_->setVertexArray( mGetOsgVec3Arr(coords_->osgArray()) );
    markerset_->setOnOffArray( (osg::ByteArray*)onoffarr_ );
    setMinimumScale( 1.0f );

    const Coord worksize = SI().maxCoord(OD::UsrWork)
			 - SI().minCoord(OD::UsrWork);
    const float maxscale = mMAX( worksize.x_, worksize.y_ ) / 1e3f;
    setMaximumScale( maxscale );

    setAutoRotateMode( NO_ROTATION );
    setType( OD::MarkerStyle3D::Cube );
    setScreenSize( cDefaultScreenSize() );
    setMaterial( 0 ); //Triggers update of markerset's color array
}


MarkerSet::~MarkerSet()
{
    removeChild( markerset_ );
    clearMarkers();
    onoffarr_->unref();
    markerset_->unref();
    removePolygonOffsetNodeState();
}


void MarkerSet::applyRotationToAllMarkers( bool yn )
{
    markerset_->applyRotationToAllMarkers( yn );
}


Normals* MarkerSet::getNormals()
{
    if ( !normals_ )
    {
	normals_ = Normals::create();
	markerset_->setNormalArray( mGetOsgVec3Arr( normals_->osgArray()) );
    }

    return normals_;
}


void MarkerSet::setMaterial( Material* mat )
{
   if ( mat && material_==mat ) return;

   if ( material_ )
       markerset_->setColorArray( 0 );

   VisualObjectImpl::setMaterial( mat );
   materialChangeCB( 0 );

}


void MarkerSet::materialChangeCB( CallBacker* cb)
{
     if ( material_ )
     {
	 const TypeSet<Color> colors = material_->getColors();
	 osg::ref_ptr<osg::Vec4Array> osgcolorarr =
	     new osg::Vec4Array( colors.size() );

	 for ( int idx = 0; idx< colors.size(); idx++ )
	     (*osgcolorarr)[idx]  = Conv::to<osg::Vec4>( colors[idx] );

	 markerset_->setColorArray( osgcolorarr.get() );
	 markerset_->useSingleColor( false );
     }
     else
     {
	 markerset_->setColorArray( 0 );
	 markerset_->useSingleColor( true );
     }
}


void MarkerSet::clearMarkers()
{
    if ( coords_ ) coords_->setEmpty();
    if ( normals_ ) normals_->clear();
    if ( material_ ) material_->clear();
    if ( onoffarr_ ) ((osg::ByteArray*)onoffarr_)->clear();
    markerset_->forceRedraw( true );
}


void MarkerSet::removeMarker( int idx )
{
    if ( normals_ ) normals_->removeNormal( idx );
    if ( material_ ) material_->removeColor( idx );
    if ( coords_ ) coords_->removePos( idx, false );
    if ( coords_ ) coords_->dirty();
    if ( onoffarr_ )
    {
	osg::ByteArray* onoffarr = (osg::ByteArray*)onoffarr_;
	onoffarr->erase( onoffarr->begin()+idx );
    }
    markerset_->forceRedraw( true );

}


void MarkerSet::setMarkerStyle( const OD::MarkerStyle3D& ms )
{
    setType( ms.type_ );
    setScreenSize( (float) ms.size_ );
}


void MarkerSet::setMarkersSingleColor( const Color& singlecolor )
{
    osg::Vec4f color = Conv::to<osg::Vec4>( singlecolor );
    markerset_->setSingleColor( color );
    setMaterial( 0 );
}


Color MarkerSet::getMarkersSingleColor() const
{
    return Conv::to<Color>(markerset_->getSingleColor() );
}


bool MarkerSet::usesSingleColor() const
{
    return markerset_->usesSingleColor();
}


void MarkerSet::getColorArray( TypeSet<Color>& colors ) const
{
    const osg::Vec4Array* clrarr = markerset_->getColorArray();
    if ( !clrarr )
	return;
    for ( int idx=0; idx<clrarr->size(); idx++ )
	colors += Conv::to<Color>((*clrarr)[idx]);
}


OD::MarkerStyle3D::Type MarkerSet::getType() const
{
    return markerstyle_.type_;
}


void MarkerSet::setType( OD::MarkerStyle3D::Type type )
{
    switch ( type )
    {
	case OD::MarkerStyle3D::Cube:
	    markerset_->setShape( osgGeo::MarkerShape::Box );
	    break;
	case OD::MarkerStyle3D::Cone:
	    markerset_->setShape( osgGeo::MarkerShape::Cone );
	    break;
	case OD::MarkerStyle3D::Cylinder:
	    markerset_->setShape( osgGeo::MarkerShape::Cylinder );
	    break;
	case OD::MarkerStyle3D::Sphere:
	    markerset_->setShape( osgGeo::MarkerShape::Sphere );
	    break;
	case OD::MarkerStyle3D::Cross:
	    markerset_->setShape( osgGeo::MarkerShape::Cross );
	    break;
	case OD::MarkerStyle3D::Arrow:
	    markerset_->setShape( osgGeo::MarkerShape::Arrow );
	    break;
	case OD::MarkerStyle3D::Plane:
	    markerset_->setShape( osgGeo::MarkerShape::Plane );
	    break;
	case OD::MarkerStyle3D::Point:
	    markerset_->setShape( osgGeo::MarkerShape::Point );
	    break;
	default:
	    pErrMsg("Shape not implemented");
	    markerset_->setShape( osgGeo::MarkerShape::None );
    }

    markerstyle_.type_ = type;
}


void MarkerSet::setSingleMarkerRotation( const Quaternion& rot, int idx )
{
    Coord3 axis(0,0,0);
    float angle = 0;
    rot.getRotation( axis, angle );
    const osg::Quat osgquat( angle,
			    osg::Vec3(axis.x_,axis.y_,axis.z_) );
    markerset_->setSingleMarkerRotation( osgquat, idx );
}


void MarkerSet::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );

    if ( dpi==pixeldensity_ )
	return;

    pixeldensity_ = dpi;
    setScreenSize( markerstyle_.size_ ); //Force update
}


void MarkerSet::setScreenSize( float sz )
{
    markerstyle_.size_ = (int)sz;

    const float screenfactor = pixeldensity_/getDefaultPixelDensity();
    markerset_->setMarkerSize( mOSGMarkerScaleFactor*sz*screenfactor );
}


float MarkerSet::getScreenSize() const
{
    return markerset_->getMarkerSize() / mOSGMarkerScaleFactor;
}


void MarkerSet::setMarkerResolution( float res )
{
    markerset_->setDetail( res );
}


void MarkerSet::doFaceCamera(bool yn)
{
    setAutoRotateMode( yn ? ROTATE_TO_CAMERA : NO_ROTATION );
}


void MarkerSet::setMarkerHeightRatio( float heightratio )
{
    markerset_->setMarkerHeightRatio( heightratio );
}


float MarkerSet::getMarkerHeightRatio() const
{
    return markerset_->getMarkerHeightRatio();
}


void MarkerSet::setMaximumScale( float maxscale )
{
    markerset_->setMaxScale( maxscale );
}


float MarkerSet::getMaximumScale() const
{
    return markerset_->getMaxScale();
}


void MarkerSet::setMinimumScale( float minscale )
{
    markerset_->setMinScale( minscale );
}


float MarkerSet::getMinimumScale() const
{
    return markerset_->getMinScale();
}


void MarkerSet::setAutoRotateMode( AutoRotateMode rotatemode )
{
    markerset_->applyRotationToAllMarkers( true );
    markerset_->setRotateMode( (osg::AutoTransform::AutoRotateMode)rotatemode );
    if ( rotatemode != NO_ROTATION )
	setRotationForAllMarkers( rotationvec_, rotationangle_ );
}


void MarkerSet::setRotationForAllMarkers( const Coord3& vec, float angle )
{
    rotationvec_ = vec;
    rotationangle_ = angle;

    osg::Quat quat;
    if ( !mIsUdf(angle) )
	quat.makeRotate( angle, Conv::to<osg::Vec3>(vec) );
    else if ( markerset_->getRotateMode() != osg::AutoTransform::NO_ROTATION )
	quat.makeRotate( M_PI_2, osg::Vec3(-1.0,0.0,0.0) );

    markerset_->setRotationForAllMarkers( quat );
}


bool MarkerSet::facesCamera() const
{
    return markerset_->getRotateMode() == osg::AutoTransform::ROTATE_TO_CAMERA;
}


void MarkerSet::setDisplayTransformation( const mVisTrans* nt )
{
    coords_->setDisplayTransformation( nt );
    if ( normals_ ) normals_->setDisplayTransformation( nt );

    displaytrans_ = nt;

    forceRedraw( true );

}


const mVisTrans* MarkerSet::getDisplayTransformation() const
{ return displaytrans_; }


void MarkerSet::turnMarkerOn( unsigned int idx,bool yn )
{
    markerset_->turnMarkerOn( idx, yn );
}


bool MarkerSet::markerOn( unsigned int idx )
{
    return markerset_->markerOn( idx );
}


void MarkerSet::turnAllMarkersOn( bool yn )
{
    markerset_->turnAllMarkersOn( yn );
}


int MarkerSet::findClosestMarker( const Coord3& tofindpos,
				 const bool scenespace )
{
    double minsqdist = mUdf(double);
    int minidx = -1;
    for ( int idx=0; idx<coords_->size(); idx++ )
    {
	const double sqdist = tofindpos.sqDistTo(
	    coords_->getPos( idx,scenespace ) );
	if ( sqdist<minsqdist )
	{
	    minsqdist = sqdist;
	    minidx = idx;
	}
    }
    return minidx;
}


int MarkerSet::findMarker( const Coord3& tofindpos, const Coord3& eps,
				 const bool scenespace )
{
    int minidx = findClosestMarker( tofindpos, scenespace );
    if( minidx == -1 )
	return minidx;

    const Coord3 findedpos = coords_->getPos( minidx, scenespace );

    if ( findedpos.isSameAs( tofindpos,eps ) )
	return minidx;
    return -1;
}


int MarkerSet::addPos( const Coord3& crd, bool draw )
{
    int index ( 0 );
    if ( coords_ )
	index = coords_->addPos( crd );

    markerset_->turnMarkerOn( index, draw );
    if ( draw && markerset_ )
        markerset_->forceRedraw( true );

    return index;
}


void MarkerSet::setPos( int idx, const Coord3& crd, bool draw )
{
    if ( coords_ )
	coords_->setPos( idx, crd );
    markerset_->turnMarkerOn( idx, draw );
    if ( draw && markerset_ )
	markerset_->forceRedraw( true );
}


void MarkerSet::insertPos( int idx, const Coord3& crd, bool draw )
{
    if ( coords_ )
	coords_->insertPos( idx, crd );
    markerset_->turnMarkerOn( idx, draw );
    if ( draw && markerset_ )
	markerset_->forceRedraw( true );
}


void MarkerSet::forceRedraw( bool yn )
{
    if ( markerset_ )
	markerset_->forceRedraw( yn );
}


int MarkerSet::size() const
{
    if ( coords_ )
	return coords_->size();

    return 0;
}


void MarkerSet::addPolygonOffsetNodeState()
{
    if ( !offset_ )
    {
        offset_ = new PolygonOffset;
	offset_->setFactor( -1.0f );
	offset_->setUnits( 1.0f );
	offset_->setMode( PolygonOffset::Protected | PolygonOffset::On );
	addNodeState( offset_ );
    }
}


void MarkerSet::removePolygonOffsetNodeState()
{
    if ( offset_ )
	removeNodeState( offset_ );
    offset_ = 0;

}

} // namespace visBase
