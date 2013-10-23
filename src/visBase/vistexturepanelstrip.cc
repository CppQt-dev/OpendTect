/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          April 2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "vistexturepanelstrip.h"
#include "vistexturechannels.h"


#include <osgGeo/TexturePanelStrip>

mCreateFactoryEntry( visBase::TexturePanelStrip );

using namespace visBase;


TexturePanelStrip::TexturePanelStrip()
    : VisualObjectImpl( false )
    , osgpanelstrip_( new osgGeo::TexturePanelStripNode )
    , pathcoords_( new TypeSet<Coord> )
    , pathtexoffsets_( new TypeSet<float> )
{
    osgpanelstrip_->ref();
    addChild( osgpanelstrip_ );
    // osgpanelstrip_->toggleTilingMode();	// for testing purposes only
    osgpanelstrip_->setTextureBrickSize( 64, false );
    // TODO: around release time, tile size should be increased
}


TexturePanelStrip::~TexturePanelStrip()
{
    osgpanelstrip_->unref();
}


void TexturePanelStrip::setTextureChannels( visBase::TextureChannels* channels )
{
    channels_ = channels;
    osgpanelstrip_->setTexture( channels_->getOsgTexture() );
}


visBase::TextureChannels* TexturePanelStrip::getTextureChannels()
{ return channels_; }


void TexturePanelStrip::freezeDisplay( bool yn )
{ osgpanelstrip_->freezeDisplay( yn ); }


bool TexturePanelStrip::isDisplayFrozen() const
{ return osgpanelstrip_->isDisplayFrozen(); }


void TexturePanelStrip::updatePath()
{
    if ( !pathcoords_ )
	return;

    osg::ref_ptr<osg::Vec2Array> osgpath =
				    new osg::Vec2Array( pathcoords_->size() );

    for ( int idx=0; idx<osgpath->size(); idx++ )
    {
	Coord3 dummy( (*pathcoords_)[idx], 0.0 );
	mVisTrans::transform( displaytrans_, dummy );
	(*osgpath)[idx] = osg::Vec2( dummy.x, dummy.y );
    }

    osgpanelstrip_->setPath( *osgpath );
}


void TexturePanelStrip::setPath( const TypeSet<Coord>& coords )
{
    *pathcoords_ = coords;
    updatePath();
}


const TypeSet<Coord>& TexturePanelStrip::getPath() const
{ return *pathcoords_; }


void TexturePanelStrip::setPath2TextureMapping( const TypeSet<float>& offsets )
{
    *pathtexoffsets_ = offsets;

    osg::ref_ptr<osg::FloatArray> osgmapping =
					new osg::FloatArray( offsets.size() );

    for ( int idx=0; idx<osgmapping->size(); idx++ )
	(*osgmapping)[idx] = offsets[idx];

    osgpanelstrip_->setPath2TextureMapping( *osgmapping );
}


const TypeSet<float>& TexturePanelStrip::getPath2TextureMapping() const
{ return *pathtexoffsets_; }


void TexturePanelStrip::setPathTextureShift( float shift, int startidx )
{ osgpanelstrip_->setPathTextureShift( shift, startidx ); }


float TexturePanelStrip::getPathTextureShift() const
{ return osgpanelstrip_->getPathTextureShift(); }


float TexturePanelStrip::getPathTextureShiftStartIdx() const
{ return osgpanelstrip_->getPathTextureShiftStartIdx(); }


void TexturePanelStrip::setZRange( const Interval<float>& zrange )
{
    Coord3 topdummy( Coord(), zrange.start );
    mVisTrans::transform( displaytrans_, topdummy );
    Coord3 bottomdummy( Coord(), zrange.stop );
    mVisTrans::transform( displaytrans_, bottomdummy );

    osgpanelstrip_->setZRange( topdummy.z, bottomdummy.z );
}


Interval<float> TexturePanelStrip::getZRange() const
{
    Coord3 topdummy( Coord(), osgpanelstrip_->getTop() );
    mVisTrans::transformBack( displaytrans_, topdummy );
    Coord3 bottomdummy( Coord(), osgpanelstrip_->getBottom() );
    mVisTrans::transformBack( displaytrans_, bottomdummy );

    return Interval<float>(topdummy.z,bottomdummy.z);
}


void TexturePanelStrip::setZRange2TextureMapping(
					    const Interval<float>& offsets )
{
    osgpanelstrip_->setZRange2TextureMapping(true, offsets.start, offsets.stop);
} 


void TexturePanelStrip::unsetZRange2TextureMapping()
{ osgpanelstrip_->setZRange2TextureMapping( false ); }


bool TexturePanelStrip::isZRange2TextureMappingSet() const
{ return osgpanelstrip_->isZRange2TextureMappingSet(); }


Interval<float> TexturePanelStrip::getZRange2TextureMapping() const
{
    return Interval<float>( osgpanelstrip_->getTopTextureMapping(),
			    osgpanelstrip_->getBottomTextureMapping() );
}


void TexturePanelStrip::setZTextureShift( float shift )
{ osgpanelstrip_->setZTextureShift( shift ); }


float TexturePanelStrip::getZTextureShift() const
{ return osgpanelstrip_->getZTextureShift(); }


void TexturePanelStrip::swapTextureAxes( bool yn )
{ osgpanelstrip_->swapTextureAxes( yn ); }


bool TexturePanelStrip::areTextureAxesSwapped() const
{ return osgpanelstrip_->areTextureAxesSwapped(); }


void TexturePanelStrip::smoothNormals( bool yn )
{ osgpanelstrip_->smoothNormals( yn ); }


bool TexturePanelStrip::areNormalsSmoothed() const
{ return osgpanelstrip_->areNormalsSmoothed(); }


void TexturePanelStrip::setDisplayTransformation( const mVisTrans* tr )
{
    Coord3 dummy( 0.0, 0.0, 1.0 );
    mVisTrans::transformDir( displaytrans_, dummy );
    if ( fabs(dummy.normalize().z) < 1.0 )
    {
	pErrMsg( "Display transformation violates assumed orthogonality "
		 "between xy and z." );
    }
    
    Interval<float> zrange = getZRange();
    displaytrans_ = tr;
    setZRange( zrange );
    updatePath();
}


const mVisTrans* TexturePanelStrip::getDisplayTransformation() const
{ return displaytrans_; }
