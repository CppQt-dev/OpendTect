/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiodapplmgr.cc,v 1.189 2007-07-06 14:11:05 cvskris Exp $
________________________________________________________________________

-*/

#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiodmenumgr.h"

#include "uipickpartserv.h"
#include "uivispartserv.h"
#include "uimpepartserv.h"
#include "uiattribpartserv.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uinlapartserv.h"
#include "uiseispartserv.h"
#include "uiwellpartserv.h"
#include "uiwellattribpartserv.h"
#include "vispicksetdisplay.h"
#include "vispolylinedisplay.h"
#include "visrandomtrackdisplay.h"

#include "emseedpicker.h"
#include "emsurfacetr.h"
#include "emtracker.h"
#include "mpeengine.h"
#include "externalattrib.h"
#include "attribdescset.h"
#include "attribdatacubes.h"
#include "attribsel.h"
#include "seisbuf.h"
#include "posvecdataset.h"
#include "pickset.h"
#include "survinfo.h"
#include "errh.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "linekey.h"
#include "oddirs.h"
#include "odsession.h"
#include "helpview.h"
#include "filegen.h"
#include "ptrman.h"

#include "uimsg.h"
#include "uifontsel.h"
#include "uitoolbar.h"
#include "uibatchlaunch.h"
#include "uipluginman.h"
#include "uibatchprogs.h"
#include "uiviscoltabed.h"
#include "uifiledlg.h"
#include "uisurvey.h"
#include "uistereodlg.h"
#include "uishortcuts.h"

static BufferString retstr;


class uiODApplService : public uiApplService
{
public:
    			uiODApplService( uiParent* p, uiODApplMgr& am )
			: par_(p), applman_(am)	{}
    uiParent*		parent() const		{ return par_; }
    bool		eventOccurred( const uiApplPartServer* ps, int evid )
			{ return applman_.handleEvent( ps, evid ); }
    void*		getObject( const uiApplPartServer* ps, int evid )
			{ return applman_.deliverObject( ps, evid ); }

    uiODApplMgr&	applman_;
    uiParent*		par_;

};


uiODApplMgr::uiODApplMgr( uiODMain& a )
	: appl_(a)
	, applservice_(*new uiODApplService(&a,*this))
    	, nlaserv_(0)
    	, getOtherFormatData(this)
	, otherformatvisid_(-1)
	, otherformatattrib_(-1)
{
    pickserv_ = new uiPickPartServer( applservice_ );
    visserv_ = new uiVisPartServer( applservice_ );
    attrserv_ = new uiAttribPartServer( applservice_ );
    seisserv_ = new uiSeisPartServer( applservice_ );
    emserv_ = new uiEMPartServer( applservice_ );
    emattrserv_ = new uiEMAttribPartServer( applservice_ );
    wellserv_ = new uiWellPartServer( applservice_ );
    wellattrserv_ = new uiWellAttribPartServer( applservice_ );
    mpeserv_ = new uiMPEPartServer( applservice_ );

    IOM().surveyToBeChanged.notify( mCB(this,uiODApplMgr,surveyToBeChanged) );
    IOM().surveyChanged.notify( mCB(this,uiODApplMgr,surveyChanged) );
}


uiODApplMgr::~uiODApplMgr()
{
    IOM().surveyToBeChanged.remove( mCB(this,uiODApplMgr,surveyToBeChanged) );
    delete mpeserv_;
    delete pickserv_;
    delete nlaserv_;
    delete attrserv_;
    delete seisserv_;
    delete visserv_;

    delete emserv_;
    delete emattrserv_;
    delete wellserv_;
    delete wellattrserv_;
    delete &applservice_;
}


void uiODApplMgr::resetServers()
{
    if ( nlaserv_ ) nlaserv_->reset();
    delete attrserv_; delete mpeserv_;
    attrserv_ = new uiAttribPartServer( applservice_ );
    mpeserv_ = new uiMPEPartServer( applservice_ );
    visserv_->deleteAllObjects();
    emserv_->removeUndo();
}


int uiODApplMgr::manageSurvey()
{
    BufferString prevnm = GetDataDir();
    uiSurvey dlg( &appl_ );
    if ( !dlg.go() )
	return 0;
    else if ( prevnm == GetDataDir() )
	return 1;
    else
	return 2;
}


void uiODApplMgr::surveyToBeChanged( CallBacker* )
{
    bool anythingasked = false;
    appl_.askStore( anythingasked );

    if ( nlaserv_ ) nlaserv_->reset();
    delete attrserv_; attrserv_ = 0;
    delete mpeserv_; mpeserv_ = 0;
    if ( appl_.sceneMgrAvailable() )
	sceneMgr().cleanUp( false );
}


void uiODApplMgr::surveyChanged( CallBacker* )
{
    bool douse = false; MultiID id;
    ODSession::getStartupData( douse, id );
    if ( !douse || id == "" )
	sceneMgr().addScene( true );

    attrserv_ = new uiAttribPartServer( applservice_ );
    mpeserv_ = new uiMPEPartServer( applservice_ );
    MPE::engine().init();
}


bool uiODApplMgr::editNLA( bool is2d )
{
    if ( !nlaserv_ ) return false;

    nlaserv_->set2DEvent( is2d );
    bool res = nlaserv_->go();
    if ( !res ) attrserv_->setNLAName( nlaserv_->modelName() );
    return res;
}


void uiODApplMgr::doOperation( ObjType ot, ActType at, int opt )
{
    switch ( ot )
    {
    case Seis:
	switch ( at )
	{
	case Imp:	seisserv_->importSeis( opt );	break;
	case Exp:	seisserv_->exportSeis( opt );	break;
	case Man:	seisserv_->manageSeismics();	break;
	}
    break;
    case Hor:
	switch ( at )
	{
	case Imp:
	    if ( opt == 0 )
		emserv_->importHorizon();
	    else if ( opt == 1 )
		emserv_->importHorizonAttribute();
	    break;
	case Exp:	emserv_->exportHorizon();			break;
	case Man:
	    if ( opt == 0 )
		emserv_->manageSurfaces(
				 EMHorizon3DTranslatorGroup::keyword );
	    else if ( opt == 1 )
		emserv_->manageSurfaces( EMHorizon2DTranslatorGroup::keyword );
	    break;
	}
    break;
    case Flt:
        switch( at )
	{
	case Man:	emserv_->manageSurfaces(
				 EMFaultTranslatorGroup::keyword );	break;
	}
    break;
    case Wll:
	switch ( at )
	{
	case Imp:
	    if ( opt == 0 )
		wellserv_->importTrack();
	    else if ( opt == 1 )
		wellserv_->importLogs();
	    else if ( opt == 2 )
		wellserv_->importMarkers();

	break;
	case Man:	wellserv_->manageWells();	break;
	}
    break;
    case Attr:
	if ( at == Man ) attrserv_->manageAttribSets();
    break;
    case Pick:
	switch ( at )
	{
	case Imp:	pickserv_->impexpSet( true );	break;
	case Exp:	pickserv_->impexpSet( false );	break;
	case Man:	pickserv_->managePickSets();	break;
	}
    break;
    case Wvlt:
	switch ( at )
	{
	case Imp:	seisserv_->importWavelets();	break;
	default:	seisserv_->manageWavelets();	break;
	}
    break;
    }
}

void uiODApplMgr::importLMKFault()
{ emserv_->importLMKFault(); }


void uiODApplMgr::enableMenusAndToolBars( bool yn )
{
    sceneMgr().disabRightClick( !yn );
    visServer()->disabMenus( !yn );
    visServer()->disabToolBars( !yn );
    menuMgr().dtectTB()->setSensitive( yn );
    menuMgr().manTB()->setSensitive( yn );
    menuMgr().enableMenuBar( yn );
}


void uiODApplMgr::enableTree( bool yn )
{
    sceneMgr().disabTrees( !yn );
    visServer()->blockMouseSelection( !yn );
}


void uiODApplMgr::enableSceneManipulation( bool yn )
{
    if ( !yn ) sceneMgr().setToViewMode();
    menuMgr().enableActButton( yn );
}


void uiODApplMgr::editAttribSet()
{
    editAttribSet( SI().has2D() );
}


void uiODApplMgr::editAttribSet( bool is2d )
{
    enableMenusAndToolBars( false );
    enableSceneManipulation( false );

    attrserv_->editSet( is2d ); 
}


void uiODApplMgr::createHorOutput( int tp, bool is2d )
{
    emattrserv_->setDescSet( attrserv_->curDescSet(is2d) );
    MultiID nlaid; const NLAModel* nlamdl = 0;
    if ( nlaserv_ )
    {
	nlaserv_->set2DEvent( is2d );
	nlaid = nlaserv_->modelId();
	nlamdl = &nlaserv_->getModel();
    }
    emattrserv_->setNLA( nlamdl, nlaid );

    uiEMAttribPartServer::HorOutType type =
	  tp==0 ? uiEMAttribPartServer::OnHor :
	( tp==1 ? uiEMAttribPartServer::AroundHor : 
	  	  uiEMAttribPartServer::BetweenHors );
    emattrserv_->createHorizonOutput( type );
}


void uiODApplMgr::createVol( bool is2d )
{
    MultiID nlaid;
    if ( nlaserv_ )
    {
	nlaserv_->set2DEvent( is2d );
	nlaid = nlaserv_->modelId();
    }
    attrserv_->outputVol( nlaid, is2d );
}


void uiODApplMgr::reStartProc()
{ uiRestartBatchDialog dlg( &appl_ ); dlg.go(); }
void uiODApplMgr::batchProgs() { uiBatchProgLaunch dlg( &appl_ ); dlg.go(); }
void uiODApplMgr::pluginMan() { uiPluginMan dlg( &appl_ ); dlg.go(); }
void uiODApplMgr::manageShortcuts()
{ uiShortcutsDlg dlg(&appl_,"ODScene"); dlg.go(); }


void uiODApplMgr::setFonts()
{
    uiSetFonts dlg( &appl_, "Set font types" );
    dlg.go();
}


void uiODApplMgr::setStereoOffset()
{
    ObjectSet<uiSoViewer> vwrs;
    sceneMgr().getSoViewers( vwrs );
    uiStereoDlg dlg( &appl_, vwrs );
    dlg.go();
}


void uiODApplMgr::setWorkingArea()
{
    if ( visserv_->setWorkingArea() )
	sceneMgr().viewAll(0);
}


void uiODApplMgr::setZScale()
{
    visserv_->setZScale();
}


bool uiODApplMgr::selectAttrib( int id, int attrib )
{
    if ( id < 0 ) return false;
    const Attrib::SelSpec* as = visserv_->getSelSpec( id, attrib );
    if ( !as ) return false;

    const char* key = visserv_->getDepthDomainKey( visserv_->getSceneID(id) );
    Attrib::SelSpec myas( *as );
    const bool selok = attrserv_->selectAttrib( myas, key, myas.is2D() );
    if ( selok )
	visserv_->setSelSpec( id, attrib, myas );

    return selok;
}


void uiODApplMgr::selectWells( ObjectSet<MultiID>& wellids )
{ wellserv_->selectWells( wellids ); }


bool uiODApplMgr::storePickSets()
{ return pickserv_->storeSets(); }
bool uiODApplMgr::storePickSet( const Pick::Set& ps )
{ return pickserv_->storeSet( ps ); }
bool uiODApplMgr::storePickSetAs( const Pick::Set& ps )
{ return pickserv_->storeSetAs( ps ); }
bool uiODApplMgr::setPickSetDirs( Pick::Set& ps )
{ return attrserv_->setPickSetDirs( ps, nlaserv_ ? &nlaserv_->getModel() : 0 ); }
bool uiODApplMgr::pickSetsStored() const
{ return pickserv_->pickSetsStored(); }


bool uiODApplMgr::getNewData( int visid, int attrib )
{
    if ( visid<0 ) return false;

    const Attrib::SelSpec* as = visserv_->getSelSpec( visid, attrib );
    if ( !as )
    {
	uiMSG().error( "Cannot calculate attribute on this object" );
	return false;
    }

    Attrib::SelSpec myas( *as );
    if ( myas.id()!=Attrib::DescID::undef() ) attrserv_->updateSelSpec( myas );
    if ( myas.id()<-1 )
    {
	uiMSG().error( "Cannot find selected attribute" );
	return false;
    } 

    bool res = false;
    switch ( visserv_->getAttributeFormat(visid) )
    {
	case uiVisPartServer::Cube :
	{
	    const Attrib::DataCubes* cache =
				visserv_->getCachedData( visid, attrib );

	    CubeSampling cs = visserv_->getCubeSampling( visid, attrib );
	    if ( !cs.isDefined() )
		return false;

	    if ( myas.id()==Attrib::SelSpec::cOtherAttrib() )
	    {
		PtrMan<Attrib::ExtAttribCalc> calc = 
				Attrib::ExtAttrFact().createCalculator( myas );

		if ( !calc )
		{
		    BufferString errstr = "Selected attribute is not present ";
		    errstr += "in the set\n and cannot be created";
		    uiMSG().error( errstr );
		    return false;
		}

		RefMan<const Attrib::DataCubes> newdata =
				calc->createAttrib( cs, cache );
		res = newdata;
		visserv_->setCubeData( visid, attrib, newdata );
		break;
	    }

	    const DataPack::ID cacheid =
				visserv_->getDataPackID( visid, attrib );
	    attrserv_->setTargetSelSpec( myas );
	    const DataPack::ID newid = attrserv_->createOutput( cs, cacheid );
	    if ( newid == -1 )
	    {
		visserv_->setCubeData( visid, attrib, 0 );
		return false;
	    }

	    visserv_->setDataPackID( visid, attrib, newid );

	    res = true;
	    break;
	}
	case uiVisPartServer::Traces :
	{
	    const Interval<float> zrg = visserv_->getDataTraceRange( visid );
	    TypeSet<BinID> bids;
	    visserv_->getDataTraceBids( visid, bids );
	    BinIDValueSet bidset(2,false);
	    for ( int idx=0; idx<bids.size(); idx++ )
		bidset.add( bids[idx], zrg.start, zrg.stop );
	    SeisTrcBuf data( true );
	    attrserv_->setTargetSelSpec( myas );
	    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rdmtdisp,
		    	    visserv_->getObject(visid) );
	    TypeSet<BinID>* trcspath = rdmtdisp ? rdmtdisp->getPath() : 0;
	    const DataPack::ID newid = 
		    attrserv_->createRdmTrcsOutput( bidset, data, trcspath );
	    if ( newid == -1 )
	    {
		visserv_->setTraceData( visid, attrib, data );
		return false;
	    }

	    visserv_->setDataPackID( visid, attrib, newid );
	    return true;
	}
	case uiVisPartServer::RandomPos :
	{
	    if ( myas.id()==Attrib::SelSpec::cOtherAttrib() )
	    {
		const MultiID surfmid = visserv_->getMultiID(visid);
		const EM::ObjectID emid = emserv_->getObjectID(surfmid);
		const int auxdatanr = emserv_->loadAuxData(emid,myas.userRef());
		if ( auxdatanr<0 )
		    uiMSG().error( "Cannot find stored data" );
		else
		{
		    BufferString attrnm;
		    ObjectSet<BinIDValueSet> vals;
		    emserv_->getAuxData( emid, auxdatanr, attrnm, vals );
		    visserv_->setRandomPosData( visid, attrib, &vals );
		}
		
		return  auxdatanr>=0;
	    }

	    ObjectSet<BinIDValueSet> data;
	    visserv_->getRandomPos( visid, data );
	    attrserv_->setTargetSelSpec( myas );
	    if ( !attrserv_->createOutput(data) )
	    {
		deepErase( data );
		return false;
	    }

	    visserv_->setRandomPosData( visid, attrib, &data );

	    deepErase( data );
	    return true;
	}
	case uiVisPartServer::OtherFormat :
	{
	    otherformatvisid_ = visid;
	    otherformatattrib_ = attrib;
	    getOtherFormatData.trigger();
	    otherformatvisid_ = -1;
	    otherformatattrib_ = -1;
	    return true;
	}
	default :
	{
	    pErrMsg("Invalid format");
	    return false;
	}
    }

    setHistogram( visid, attrib );
    return res;
}


bool uiODApplMgr::evaluateAttribute( int visid, int attrib )
{
    /* Perhaps better to merge this with uiODApplMgr::getNewData(), 
       for now it works */
    uiVisPartServer::AttribFormat format = visserv_->getAttributeFormat( visid);
    if ( format == uiVisPartServer::Cube )
    {
	const CubeSampling cs = visserv_->getCubeSampling( visid );
	RefMan<const Attrib::DataCubes> newdata = attrserv_->createOutput( cs );
	visserv_->setCubeData( visid, attrib, newdata );
    }
    else if ( format==uiVisPartServer::RandomPos )
    {
	ObjectSet<BinIDValueSet> data;
	visserv_->getRandomPos( visid, data );

	attrserv_->createOutput( data );
	visserv_->setRandomPosData( visid, attrib, &data );
	deepErase( data );
    }
    else
    {
	uiMSG().error( "Cannot evaluate attributes on this object" );
	return false;
    }

    return true;
}


bool uiODApplMgr::handleEvent( const uiApplPartServer* aps, int evid )
{
    if ( !aps ) return true;

    if ( aps == pickserv_ )
	return handlePickServEv(evid);
    else if ( aps == visserv_ )
	return handleVisServEv(evid);
    else if ( aps == nlaserv_ )
	return handleNLAServEv(evid);
    else if ( aps == attrserv_ )
	return handleAttribServEv(evid);
    else if ( aps == emserv_ )
	return handleEMServEv(evid);
    else if ( aps == wellserv_ )
	return handleWellServEv(evid);
    else if ( aps == mpeserv_ )
	return handleMPEServEv(evid);

    return false;
}


void* uiODApplMgr::deliverObject( const uiApplPartServer* aps, int id )
{
    if ( aps == attrserv_ )
    {
	bool isnlamod2d = id == uiAttribPartServer::objNLAModel2D;
	bool isnlamod3d = id == uiAttribPartServer::objNLAModel3D;
	if ( isnlamod2d || isnlamod3d  )
	{
	    if ( nlaserv_ )
		nlaserv_->set2DEvent( isnlamod2d );
	    return nlaserv_ ? (void*)(&nlaserv_->getModel()) : 0;
	}
    }
    else
	pErrMsg("deliverObject for unsupported part server");

    return 0;
}


bool uiODApplMgr::handleMPEServEv( int evid )
{
    if ( evid == uiMPEPartServer::evAddTreeObject )
    {
	const int trackerid = mpeserv_->activeTrackerID();
	const EM::ObjectID emid = mpeserv_->getEMObjectID(trackerid);
	const int sceneid = mpeserv_->getCurSceneID();
	const int sdid = sceneMgr().addEMItem( emid, sceneid );
	if ( sdid==-1 )
	    return false;

	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiMPEPartServer::evRemoveTreeObject )
    {
	const int trackerid = mpeserv_->activeTrackerID();
	const EM::ObjectID emid = mpeserv_->getEMObjectID(trackerid);
	const MultiID mid = emserv_->getStorageID(emid);

	TypeSet<int> sceneids;
	visserv_->getChildIds( -1, sceneids );

	TypeSet<int> ids;
	visserv_->findObject( mid, ids );

	for ( int idx=0; idx<ids.size(); idx++ )
	{
	    for ( int idy=0; idy<sceneids.size(); idy++ )
		visserv_->removeObject( ids[idx], sceneids[idy] );
	    sceneMgr().removeTreeItem(ids[idx] );
	}


	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiMPEPartServer::evStartSeedPick )
    {
	//Turn off everything

	visserv_->turnSeedPickingOn( true );
	sceneMgr().setToViewMode( false );
    }
    else if ( evid==uiMPEPartServer::evEndSeedPick )
    {
	visserv_->turnSeedPickingOn( false );
    }
    else if ( evid==uiMPEPartServer::evWizardClosed )
    {
	enableMenusAndToolBars( true );
	enableTree( true );
    }
    else if ( evid==uiMPEPartServer::evGetAttribData )
    {
	const Attrib::SelSpec* as = mpeserv_->getAttribSelSpec();
	if ( !as ) return false;
	const CubeSampling cs = mpeserv_->getAttribVolume(*as);
	const Attrib::DataCubes* cache = mpeserv_->getAttribCache(*as);
	attrserv_->setTargetSelSpec( *as );
	RefMan<const Attrib::DataCubes> newdata =
	    				attrserv_->createOutput( cs, cache );
	mpeserv_->setAttribData(*as, newdata );
    }
    else if ( evid==uiMPEPartServer::evCreate2DSelSpec )
    {
	LineKey lk( mpeserv_->get2DLineSet(), mpeserv_->get2DAttribName() );
	const Attrib::DescID attribid = attrServer()->getStoredID( lk, true );
	if ( attribid<0 ) return false;

	const Attrib::SelSpec as( mpeserv_->get2DAttribName(), attribid );
	mpeserv_->set2DSelSpec( as );
    }
    else if ( evid==uiMPEPartServer::evShowToolbar )
	visserv_->showMPEToolbar();
    else if ( evid==uiMPEPartServer::evMPEDispIntro )
	visserv_->introduceMPEDisplay();
    else if ( evid==uiMPEPartServer::evInitFromSession )
	visserv_->initMPEStuff();
    else if ( evid==uiMPEPartServer::evUpdateTrees )
	sceneMgr().updateTrees();
    else
	pErrMsg("Unknown event from mpeserv");

    return true;
}


bool uiODApplMgr::handleWellServEv( int evid )
{
    if ( evid == uiWellPartServer::evPreviewRdmLine )
    {
	TypeSet<Coord> coords;
	wellserv_->getRdmLineCoordinates( coords );
	setupRdmLinePreview( coords );
	enableTree( false );
	enableMenusAndToolBars( false );
    }
    if ( evid == uiWellPartServer::evCreateRdmLine )
    {
	TypeSet<Coord> coords;
	wellserv_->getRdmLineCoordinates( coords );
	cleanPreview();

	TypeSet<BinID> bidset;
	for ( int idx=0; idx<coords.size(); idx++ )
	{
	    BinID bid = SI().transform( coords[idx] );
	    if ( bidset.indexOf(bid) < 0 )
		bidset += bid;
	}

	const int rdmlineid = visserv_->getSelObjectId();
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
			visserv_->getObject(rdmlineid));
	rtd->setKnotPositions( bidset );
	enableTree( true );
	enableMenusAndToolBars( true );
    }
    if ( evid == uiWellPartServer::evCleanPreview )
    {
	cleanPreview();
	enableTree( true );
	enableMenusAndToolBars( true );
    }
    
    return true;
}


bool uiODApplMgr::handleEMServEv( int evid )
{
    if ( evid == uiEMPartServer::evDisplayHorizon )
    {
	TypeSet<int> sceneids;
	visserv_->getChildIds( -1, sceneids );
	if ( sceneids.isEmpty() ) return false;

	const EM::ObjectID emid = emserv_->selEMID();
	sceneMgr().addEMItem( emid, sceneids[0] );
	sceneMgr().updateTrees();
	return true;
    }
    else if ( evid == uiEMPartServer::evRemoveTreeObject )
    {
	const EM::ObjectID emid = emserv_->selEMID();
	const MultiID mid = emserv_->getStorageID(emid);

	TypeSet<int> sceneids;
	visserv_->getChildIds( -1, sceneids );

	TypeSet<int> ids;
	visserv_->findObject( mid, ids );

	for ( int idx=0; idx<ids.size(); idx++ )
	{
	    for ( int idy=0; idy<sceneids.size(); idy++ )
		visserv_->removeObject( ids[idx], sceneids[idy] );
	    sceneMgr().removeTreeItem(ids[idx] );
	}


	sceneMgr().updateTrees();
	return true;
    }
    else
	pErrMsg("Unknown event from emserv");

    return true;
}


bool uiODApplMgr::handlePickServEv( int evid )
{
    if ( evid == uiPickPartServer::evGetHorInfo )
    {
	emserv_->getSurfaceInfo( pickserv_->horInfos() );
    }
    else if ( evid == uiPickPartServer::evGetHorDef )
    {
	TypeSet<EM::ObjectID> horids;
	const ObjectSet<MultiID>& storids = pickserv_->selHorIDs();
	for ( int idx=0; idx<storids.size(); idx++ )
	    horids += emserv_->getObjectID(*storids[idx]);
	
	emserv_->getSurfaceDef( horids, pickserv_->genDef(),
			       pickserv_->selBinIDRange() );
    }
    else

	pErrMsg("Unknown event from pickserv");

    return true;
}


bool uiODApplMgr::handleVisServEv( int evid )
{
    int visid = visserv_->getEventObjId();
    visserv_->unlockEvent();

    if ( evid == uiVisPartServer::evUpdateTree )
	sceneMgr().updateTrees();
    else if ( evid == uiVisPartServer::evDeSelection
	   || evid == uiVisPartServer::evSelection )
	sceneMgr().updateSelectedTreeItem();
    else if ( evid == uiVisPartServer::evGetNewData )
	return getNewData( visid, visserv_->getEventAttrib() );
    else if ( evid == uiVisPartServer::evInteraction )
	sceneMgr().setItemInfo( visid );
    else if ( evid == uiVisPartServer::evMouseMove ||
	      evid==uiVisPartServer::evPickingStatusChange )
	sceneMgr().updateStatusBar();
    else if ( evid == uiVisPartServer::evViewModeChange )
	sceneMgr().setToViewMode( visserv_->isViewMode() );
    else if ( evid == uiVisPartServer::evSelectAttrib )
	return selectAttrib( visid, visserv_->getEventAttrib() );
    else if ( evid == uiVisPartServer::evViewAll )
	sceneMgr().viewAll(0);
    else if ( evid == uiVisPartServer::evToHomePos )
	sceneMgr().toHomePos(0);
    else if ( evid == uiVisPartServer::evShowSetupDlg )
    {
	const int selobjvisid = visserv_->getSelObjectId();
	const MultiID selobjmid = visserv_->getMultiID(selobjvisid);
	const EM::ObjectID& emid = emserv_->getObjectID(selobjmid);
	const int trackerid = mpeserv_->getTrackerID(emid);
	MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
	const MPE::EMSeedPicker* seedpicker = tracker ? 
					      tracker->getSeedPicker(false) : 0;
	const EM::SectionID sid = seedpicker ? seedpicker->getSectionID() : -1;
	mpeserv_->showSetupDlg( emid, sid, true );
	visserv_->updateMPEToolbar();
    }
    else if ( evid == uiVisPartServer::evLoadPostponedData )
	mpeserv_->loadPostponedVolume();
    else if ( evid == uiVisPartServer::evToggleBlockDataLoad )
	mpeserv_->blockDataLoading( !mpeserv_->isDataLoadingBlocked() );
    else
	pErrMsg("Unknown event from visserv");

    return true;
}


bool uiODApplMgr::handleNLAServEv( int evid )
{
    if ( evid == uiNLAPartServer::evPrepareWrite )
    {
	// Before NLA model can be written, the AttribSet's IOPar must be
	// made available as it almost certainly needs to be stored there.
	const Attrib::DescSet* ads = attrserv_->curDescSet(nlaserv_->is2DEvent());
	if ( !ads ) return false;
	IOPar& iopar = nlaserv_->modelPars();
	iopar.clear();
	BufferStringSet inputs = nlaserv_->modelInputs();
	const Attrib::DescSet* cleanads = ads->optimizeClone( inputs );
	cleanads->fillPar( iopar );
	delete cleanads;
    }
    else if ( evid == uiNLAPartServer::evPrepareRead )
    {
	bool saved = attrserv_->setSaved(nlaserv_->is2DEvent());
        const char* msg = "Current attribute set is not saved.\nSave now?";
        if ( !saved && uiMSG().askGoOn( msg ) )
	    attrserv_->saveSet(nlaserv_->is2DEvent());
    }
    else if ( evid == uiNLAPartServer::evReadFinished )
    {
	// New NLA Model available: replace the attribute set!
	// Create new attrib set from NLA model's IOPar

	attrserv_->replaceSet( nlaserv_->modelPars(), nlaserv_->is2DEvent() );
	wellattrserv_->setNLAModel( &nlaserv_->getModel() );
    }
    else if ( evid == uiNLAPartServer::evGetInputNames )
    {
	// Construct the choices for input nodes.
	// Should be:
	// * All attributes (stored ones filtered out)
	// * All cubes - between []
	attrserv_->getPossibleOutputs( nlaserv_->is2DEvent(),
				      nlaserv_->inputNames() );
	if ( nlaserv_->inputNames().size() == 0 )
	    { uiMSG().error( "No usable input" ); return false; }
    }
    else if ( evid == uiNLAPartServer::evGetStoredInput )
    {
	BufferStringSet linekeys;
	nlaserv_->getNeededStoredInputs( linekeys );
	for ( int idx=0; idx<linekeys.size(); idx++ )
            attrserv_->addToDescSet( linekeys.get(idx), nlaserv_->is2DEvent() );
    }
    else if ( evid == uiNLAPartServer::evGetData )
    {
	// OK, the input and output nodes are known.
	// Query the server and make sure the relevant data is extracted
	// Put data in the training and test posvec data sets

	if ( !attrserv_->curDescSet(nlaserv_->is2DEvent()) ) 
	{ pErrMsg("Huh"); return false; }
	ObjectSet<BinIDValueSet> bivss;
	const bool dataextraction = nlaserv_->willDoExtraction();
	if ( dataextraction )
	{
	    nlaserv_->getBinIDValueSets( bivss );
	    if ( bivss.isEmpty() )
		{ uiMSG().error("No valid data locations found"); return false;}
	}
	ObjectSet<PosVecDataSet> vdss;
	bool extrres = attrserv_->extractData( nlaserv_->creationDesc(),
					      bivss, vdss, 
					      nlaserv_->is2DEvent() );
	deepErase( bivss );
	if ( extrres )
	{
	    if ( dataextraction )
	    {
		IOPar& iop = nlaserv_->storePars();
		attrserv_->curDescSet(nlaserv_->is2DEvent())->fillPar( iop );
		if ( iop.name().isEmpty() )
		    iop.setName( "Attributes" );
	    }
	    const char* res = nlaserv_->prepareInputData( vdss );
	    if ( res && *res && strcmp(res,uiNLAPartServer::sKeyUsrCancel) )
		uiMSG().warning( res );
	    if ( !dataextraction ) // i.e. if we have just read a PosVecDataSet
		attrserv_->replaceSet( vdss[0]->pars(), nlaserv_->is2DEvent() );
	}
	deepErase(vdss);
    }
    else if ( evid == uiNLAPartServer::evSaveMisclass )
    {
	const BinIDValueSet& bvsmc = nlaserv_->vdsMCA().data();
	BinIDValueSet mcpicks( 2, true );
	BinID bid; float vals[bvsmc.nrVals()];
	BinIDValueSet::Pos pos;
	while ( bvsmc.next(pos) )
	{
	    bvsmc.get( pos, bid, vals );
	    const float conf = vals[3];
	    if ( mIsUdf(conf) )
		continue;

	    const int actualclass = mNINT(vals[1]);
	    const int predclass = mNINT(vals[2]);
	    if ( actualclass != predclass )
		mcpicks.add( bid, vals[0], conf );
	}
	pickserv_->setMisclassSet( mcpicks );
    }
    else if ( evid == uiNLAPartServer::evCreateAttrSet )
    {
	Attrib::DescSet attrset(nlaserv_->is2DEvent());
	if ( !attrserv_->createAttributeSet(nlaserv_->modelInputs(),&attrset) )
	    return false;
	attrset.fillPar( nlaserv_->modelPars() );
	attrserv_->replaceSet( nlaserv_->modelPars(), nlaserv_->is2DEvent() );
    }
    else

	pErrMsg("Unknown event from nlaserv");

    return true;
}


bool uiODApplMgr::handleAttribServEv( int evid )
{
    if ( evid==uiAttribPartServer::evDirectShowAttr )
    {
	Attrib::SelSpec as;
	attrserv_->getDirectShowAttrSpec( as );
	const int visid = visserv_->getEventObjId();
	const int attrib = visserv_->getSelAttribNr();
	if ( attrib<0 || attrib>=visserv_->getNrAttribs(visid) )
	    return false;
	visserv_->setSelSpec( visid, attrib, as );
	getNewData( visid, attrib );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evNewAttrSet )
	mpeserv_->setCurrentAttribDescSet(
				attrserv_->curDescSet(attrserv_->is2DEvent()) );
    else if ( evid==uiAttribPartServer::evAttrSetDlgClosed )
    {
	enableMenusAndToolBars( true );
	enableSceneManipulation( true );
    }
    else if ( evid==uiAttribPartServer::evEvalAttrInit )
    {
	const uiVisPartServer::AttribFormat format = 
	    	visserv_->getAttributeFormat( visserv_->getEventObjId() );
	const bool alloweval = format==uiVisPartServer::Cube || 
	    		       format==uiVisPartServer::RandomPos;
	const bool allowstorage = format==uiVisPartServer::RandomPos;
	attrserv_->setEvaluateInfo( alloweval, allowstorage );
    }
    else if ( evid==uiAttribPartServer::evEvalCalcAttr )
    {
	const int visid = visserv_->getEventObjId();
	Attrib::SelSpec as( "Evaluation", Attrib::SelSpec::cOtherAttrib() );
	const int attrib = visserv_->getSelAttribNr();
	if ( attrib<0 || attrib>=visserv_->getNrAttribs(visid) )
	    return false;
	visserv_->setSelSpec( visid, attrib, as );
	if ( !evaluateAttribute(visid,attrib) )
	    return false;
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evEvalShowSlice )
    {
	const int visid = visserv_->getEventObjId();
	const int sliceidx = attrserv_->getSliceIdx();
	const int attrnr =
	    visserv_->getSelAttribNr()==-1 ? 0 : visserv_->getSelAttribNr();
	visserv_->selectTexture( visid, attrnr, sliceidx );
	modifyColorTable( visid, attrnr );
	sceneMgr().updateTrees();
    }
    else if ( evid==uiAttribPartServer::evEvalStoreSlices )
    {
	const int visid = visserv_->getEventObjId();
	const int attrnr =
	    visserv_->getSelAttribNr()==-1 ? 0 : visserv_->getSelAttribNr();
	const uiVisPartServer::AttribFormat format = 
	    				visserv_->getAttributeFormat( visid );
	if ( format!=uiVisPartServer::RandomPos ) return false;

	ObjectSet<const BinIDValueSet> data;
	visserv_->getRandomPosCache( visid, attrnr, data );
	if ( data.isEmpty() ) return false;

	const MultiID mid = visserv_->getMultiID( visid );
	const EM::ObjectID emid = emserv_->getObjectID( mid );
	const TypeSet<Attrib::SelSpec>& specs = attrserv_->getTargetSelSpecs();
	const int nrvals = data[0]->nrVals()-1;
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    emserv_->setAuxData( emid, data, specs[idx].userRef(), idx+1 );
	    emserv_->storeAuxData( emid );
	}
    }
    else
	pErrMsg("Unknown event from attrserv");

    return true;
}


void uiODApplMgr::pageUpDownPressed( bool up )
{
    const int visid = visserv_->getEventObjId();
    const int attrib = visserv_->getSelAttribNr();
    if ( attrib<0 || attrib>=visserv_->getNrAttribs(visid) )
	return;

    int texture = visserv_->selectedTexture( visid, attrib );
    if ( texture<visserv_->nrTextures(visid,attrib)-1 && up )
	texture++;
    else if ( texture && !up )
	texture--;

    visserv_->selectTexture( visid, attrib, texture );
    modifyColorTable( visid, attrib );
    sceneMgr().updateTrees();
}


void uiODApplMgr::modifyColorTable( int visid, int attrib )
{
    if ( attrib<0 || attrib>=visserv_->getNrAttribs(visid) )
	return;

    appl_.colTabEd().setColTab( visserv_->getColTabId(visid,attrib) );
    setHistogram( visid, attrib );
}


void uiODApplMgr::coltabChg( CallBacker* )
{
    const int visid = visserv_->getEventObjId();
    int attrib = visserv_->getSelAttribNr();
    if ( attrib == -1 ) attrib = 0;
    setHistogram( visid, attrib );
    sceneMgr().updateSelectedTreeItem();
}


NotifierAccess* uiODApplMgr::colorTableSeqChange()
{
    return &appl_.colTabEd().sequenceChange;
}


void uiODApplMgr::setHistogram( int visid, int attrib )
{ appl_.colTabEd().setHistogram( visserv_->getHistogram(visid,attrib) ); }


void uiODApplMgr::setupRdmLinePreview(const TypeSet<Coord>& coords)
{
    if ( wellserv_->getPreviewIds().size()>0 )
	cleanPreview();

    TypeSet<int> plids;
    TypeSet<int> sceneids;
    visSurvey::PolyLineDisplay* pl = visSurvey::PolyLineDisplay::create();
    pl->fillPolyLine( coords );
    mDynamicCastGet(visBase::DataObject*,doobj,pl);
    visserv_->getChildIds( -1, sceneids );
    
    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	visserv_->addObject( doobj, sceneids[idx], true );
	plids.addIfNew( doobj->id() );
    }
    wellserv_->setPreviewIds(plids);
}


void uiODApplMgr::cleanPreview()
{
    TypeSet<int> sceneids;
    visserv_->getChildIds( -1, sceneids );
    TypeSet<int>& previds = wellserv_->getPreviewIds();
    if ( previds.size() == 0 ) return;
    for ( int idx=0; idx<sceneids.size(); idx++ )
	visserv_->removeObject( previds[0],sceneids[idx] );

    previds.erase();
}
