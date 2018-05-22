/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2004
-*/


#include "stratunitrepos.h"
#include "stratreftree.h"
#include "safefileio.h"
#include "dbman.h"
#include "uistrings.h"

const char* Strat::RepositoryAccess::fileNameBase()	{ return "StratUnits"; }

namespace Strat
{

class RefTreeMgr : public CallBacker
{
public:

RefTreeMgr()
{
    DBM().surveyChanged.notify( mCB(this,RefTreeMgr,doNull) );
}

~RefTreeMgr()
{
    doNull( 0 );
}

void doNull( CallBacker* )
{
    deepErase( rts_ );
}

void createTree()
{
    RepositoryAccess ra;
    RefTree* rt = ra.readTree();
    if ( !rt )
    {
	rt = new RefTree;
	rt->src_ = Repos::Survey;
    }
    rts_ += rt;
}

RefTree& curTree()
{
    if ( rts_.isEmpty() )
	createTree();
    return *rts_[rts_.size()-1];
}

    ObjectSet<RefTree> rts_;

};

} // namespace


static Strat::RefTreeMgr& refTreeMgr()
{ mDefineStaticLocalObject( Strat::RefTreeMgr, mgr, ); return mgr; }
const Strat::RefTree& Strat::RT()
{ return refTreeMgr().curTree(); }
void Strat::pushRefTree( Strat::RefTree* rt )
{ refTreeMgr().rts_ += rt; }
void Strat::popRefTree()
{ delete refTreeMgr().rts_.removeSingle( refTreeMgr().rts_.size()-1 ); }


void Strat::setRT( RefTree* rt )
{
    if ( !rt ) return;

    if ( refTreeMgr().rts_.isEmpty() )
	refTreeMgr().rts_ += rt;
    else
    {
	const int currentidx = refTreeMgr().rts_.indexOf( &Strat::RT() );
	delete refTreeMgr().rts_.replace( currentidx < 0 ? 0 : currentidx, rt );
    }
}


Strat::RefTree* Strat::RepositoryAccess::readTree()
{
    Repos::FileProvider rfp( fileNameBase(), true );
    while ( rfp.next() )
    {
	src_ = rfp.source();
	RefTree* rt = readTree( src_ );
	if ( !rt || !rt->hasChildren() )
	    delete rt;
	else
	    return rt;
    }

    msg_ = tr("New, empty stratigraphic tree created");
    src_ = Repos::Survey;
    return new RefTree;
}


#define mAddFilenameToMsg(fnm) msg_.arg(" '").arg(fnm).arg("'")

Strat::RefTree* Strat::RepositoryAccess::readTree( Repos::Source src )
{
    src_ = src;
    Repos::FileProvider rfp( fileNameBase() );
    const BufferString fnm( rfp.fileName(src) );
    SafeFileIO sfio( fnm );
    if ( !sfio.open(true) )
	{ msg_ = uiStrings::phrCannotOpenForRead(fnm); return 0; }

    RefTree* rt = new RefTree;
    if ( !rt->read(sfio.istrm()) )
    {
	delete rt;
	msg_ = uiStrings::phrErrDuringRead( fnm );
	sfio.closeFail(); return 0;
    }

    sfio.closeSuccess();
    msg_ = tr("Stratigraphic tree read from %1").arg(fnm);
    return rt;
}


bool Strat::RepositoryAccess::writeTree( const Strat::RefTree& rt,
					 Repos::Source src )
{
    src_ = src;
    Repos::FileProvider rfp( fileNameBase() );
    const BufferString fnm( rfp.fileName(src) );
    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(false) )
	{ msg_ = tr("Cannot write to %1").arg(fnm); return 0; }

    if ( !rt.write(sfio.ostrm()) )
    {
	msg_ = uiStrings::phrErrDuringWrite( fnm );
	sfio.closeFail(); return false;
    }

    sfio.closeSuccess();
    msg_ = tr("Stratigraphic tree written to %1").arg(fnm);
    return true;
}
