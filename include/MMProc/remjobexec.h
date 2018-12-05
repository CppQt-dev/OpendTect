#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/

#include "mmprocmod.h"
#include "callback.h"
#include "gendefs.h"

namespace Network { class Socket; }


/*!
\brief Remote job executor
*/

mExpClass(MMProc) RemoteJobExec : public CallBacker
{
public:
			RemoteJobExec(const char*,const int);
			~RemoteJobExec();

    bool		launchProc() const;
    void		addPar(const IOPar&);

protected:

    void		checkConnection();
    void		uiErrorMsg(const char*);

    Network::Socket&	socket_;
    IOPar&		par_;
    bool		isconnected_;
    const char*		host_;

};
