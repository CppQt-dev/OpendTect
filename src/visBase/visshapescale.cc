/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: visshapescale.cc,v 1.7 2004-05-11 12:19:26 kristofer Exp $";

#include "visshapescale.h"
#include "iopar.h"
#include "visdataman.h"

#include "SoShapeScale.h"
#include "Inventor/nodes/SoSeparator.h"


mCreateFactoryEntry( visBase::ShapeScale );

const char* visBase::ShapeScale::shapeidstr = "Shape ID";


visBase::ShapeScale::ShapeScale()
    : root( new SoSeparator )
    , shapescalekit( new SoShapeScale )
    , shape( 0 )
    , node( 0 )
{
    root->ref();
    root->addChild(shapescalekit);
    shapescalekit->screenSize.setValue( 5 );
    shapescalekit->dorotate.setValue( false );
}


visBase::ShapeScale::~ShapeScale()
{
    if ( shape ) shape->unRef();
    root->unref();
}


void visBase::ShapeScale::setShape( DataObject* no )
{
    if ( shape )
    {
	shape->unRef();
    }

    if ( node )
    {
	root->removeChild(node);
	node = 0;
    }

    shape = no;
    if ( no )
    {
	no->ref();
	node = no->getInventorNode();
    }

    if ( node ) root->addChild( node );
}


void visBase::ShapeScale::setShape( SoNode* newnode )
{
    if ( shape )
    {
	shape->unRef();
    }

    if ( node )
	root->removeChild(node);

    shape = 0;
    node = newnode;
    root->addChild( node );
}


void visBase::ShapeScale::setSize( float nz )
{
    shapescalekit->screenSize.setValue( nz );
}


float visBase::ShapeScale::getSize() const
{
    return shapescalekit->screenSize.getValue();
}


void visBase::ShapeScale::freeze(bool yn)
{
    shapescalekit->doscale = yn;
}


bool visBase::ShapeScale::isFrozen() const
{
    return shapescalekit->doscale.getValue();
}


SoNode*  visBase::ShapeScale::getInventorNode() 
{
    return root;
}


int visBase::ShapeScale::usePar( const IOPar& iopar )
{
    int res = DataObject::usePar( iopar );
    if ( res!= 1 ) return res;

    int shapeid;
    if ( !iopar.get( shapeidstr, shapeid )) return -1;
    if ( shapeid==-1 ) return 1;

    DataObject* dataobj = DM().getObj( shapeid );
    if ( !dataobj ) { setShape( (SoNode*) 0 ); return 0; }
    mDynamicCastGet( DataObject*, sceneobj, dataobj );
    if ( !sceneobj ) return -1;

    setShape( sceneobj );
    return 1;
}


void visBase::ShapeScale::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    DataObject::fillPar( iopar, saveids );

    int shapeid = shape ? shape->id() : -1;
    iopar.set( shapeidstr, shapeid );
    if ( saveids.indexOf( shapeid )==-1 ) saveids += shapeid;
}
