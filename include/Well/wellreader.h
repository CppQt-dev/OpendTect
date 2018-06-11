#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellcommon.h"
#include "ranges.h"
#include "bufstring.h"
#include "uistring.h"
class IOObj;
class BufferStringSet;


namespace Well
{
class ReadAccess;

/*!\brief Reads Well::Data from any data store */

mExpClass(Well) Reader
{ mODTextTranslationClass(Reader);
public:

			Reader(const DBKey&,Data&);
			Reader(const IOObj&,Data&);
			~Reader();
    bool		isUsable() const	{ return ra_; }

    bool		getInfo() const;	//!< Read Info only
    bool		getTrack() const;	//!< Read Track only
    bool		getLogs() const;	//!< Read logs only
    bool		getMarkers() const;	//!< Read Markers only
    bool		getD2T() const;		//!< Read D2T model parts
    bool		getCSMdl() const;	//!< Read Checkshot model parts
    bool		getDispProps() const;	//!< Read display props only
    bool		getLog(const char* lognm) const; //!< Read this one only
    void		getLogNames(BufferStringSet&) const;
    void		getLogInfo(ObjectSet<IOPar>&) const;

    const uiString&	errMsg() const		{ return errmsg_; }
    Well::Data*		data();
    const Well::Data*	data() const
			{ return const_cast<Reader*>(this)->data(); }

    bool		getMapLocation(Coord&) const;

protected:

    ReadAccess*		ra_;
    mutable uiString	errmsg_;

private:

    void		init(const IOObj&,Data&);

public:

    mDeprecated bool	get() const { return getAll(); /* use Well::MGR() */ }
    bool		getAll() const;
			    //!< probably you want to use Well::MGR().get()

};


}; // namespace Well
