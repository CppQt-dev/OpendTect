/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne and Kristofer
 Date:          December 2007
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "timedepthconv.h"
#include "seisseqio.h"
#include "segytr.h"
#include "seisblockstr.h"
#include "seiscbvs.h"
#include "seis2dlineio.h"
#include "seispscubetr.h"
#include "segydirecttr.h"
#include "segydirect2d.h"
#include "waveletio.h"
#include "seismulticubeps.h"
#include "seispacketinfo.h"
#include "seis2dto3d.h"
#include "seisposprovider.h"
#include "survgeommgr.h"
#include "taskrunner.h"

#include "uistrings.h"

uiString SeisTrcTranslatorGroup::sTypeName( int num )
{ return uiStrings::sSeisObjName(false,true,false,false,false); }

defineTranslatorGroup(SeisTrc,"Seismic Data");
defineTranslator(Blocks,SeisTrc,BlocksSeisTrcTranslator::sKeyTrName());
defineTranslator(CBVS,SeisTrc,"CBVS");
defineTranslator(SEGY,SeisTrc,"SEG-Y");
defineTranslator(TwoD,SeisTrc,"2D");
defineTranslator(TwoDData,SeisTrc,"TwoD DataSet");
defineTranslator(SEGYDirect,SeisTrc,mSEGYDirectTranslNm);
defineTranslator(SeisPSCube,SeisTrc,"PS Cube");

mDefSimpleTranslatorioContext(SeisTrc,Seis)
mDefSimpleTranslatorSelector(SeisTrc);

uiString SeisTrc2DTranslatorGroup::sTypeName( int num )
{ return uiStrings::sSeisObjName(true,false,false,false,false); }

defineTranslatorGroup(SeisTrc2D,"2D Seismic Data");
defineTranslator(CBVS,SeisTrc2D,"CBVS");
defineTranslator(SEGYDirect,SeisTrc2D,mSEGYDirectTranslNm);
defineTranslator(SEGYDirect,SurvGeom2D,mSEGYDirectTranslNm);
mDefSimpleTranslatorioContext(SeisTrc2D,Seis)
mDefSimpleTranslatorSelector(SeisTrc2D);

mDefModInitFn(Seis)
{
    mIfNotFirstTime( return );

    SeisPacketInfo::initClass();

    SeisTrcTranslatorGroup::initClass();
    SeisTrc2DTranslatorGroup::initClass();
    SeisPS3DTranslatorGroup::initClass();
    SeisPS2DTranslatorGroup::initClass();

    WaveletTranslatorGroup::initClass();
    dgbWaveletTranslator::initClass();

    // The order here is important!
    // The first one is the default unless explicitly changed.
    BlocksSeisTrcTranslator::initClass();
    CBVSSeisTrcTranslator::initClass();
    TwoDSeisTrcTranslator::initClass();
    TwoDDataSeisTrcTranslator::initClass();
    CBVSSeisTrc2DTranslator::initClass();
    SEGYSeisTrcTranslator::initClass();
    SEGYDirectSeisTrcTranslator::initClass();
    SEGYDirectSeisTrc2DTranslator::initClass();
    SEGYDirectSurvGeom2DTranslator::initClass();

    CBVSSeisPS3DTranslator::initClass();
    CBVSSeisPS2DTranslator::initClass();
    SEGYDirectSeisPS3DTranslator::initClass();
    SEGYDirectSeisPS2DTranslator::initClass();
    SeisPSCubeSeisTrcTranslator::initClass();
    MultiCubeSeisPS3DTranslator::initClass();

    LinearT2DTransform::initClass();
    LinearD2TTransform::initClass();
    Time2DepthStretcher::initClass();
    Depth2TimeStretcher::initClass();
    Survey::GMAdmin().updateGeometries( SilentTaskRunnerProvider() );
	    //Those using a transl from Seis.

    Seis2DTo3DImpl::initClass();
    Pos::SeisProvider3D::initClass();
}
