#ifndef netsocket_h
#define netsocket_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		August 2014
 RCS:		$Id$
________________________________________________________________________

-*/


#include "networkmod.h"
#include "callback.h"
#include "threadlock.h"
#include "uistring.h"

class BufferString;
template <class T> class DataInterpreter;
mFDQtclass(QTcpSocket)
mFDQtclass(QTcpSocketComm)


namespace Network
{

class RequestPacket;


/*!Enables the connection and sending of binary and text data through
   a socket both with and without event-loops.

   After construction, you need to connect to a host on a port. If you choose
   not to wait then (dis-)connecting only fails if it is already in progress.

   Strings are transferred without trailing '\0', but with a leading integer
   for the size.

 */

mExpClass(Network) Socket : public CallBacker
{ mODTextTranslationClass(Socket);

public:
		Socket(bool haveeventloop=true);
		Socket(mQtclass(QTcpSocket)*,bool haveeventloop=true);
		~Socket();

    void	setTimeout(int ms) { timeout_ = ms; }

    bool	connectToHost(const char* host,int port,
			      bool wait=true);
    bool	disconnectFromHost(bool wait=false);

    bool	isBad() const;
    bool	isConnected() const;
    od_int64	bytesAvailable() const;
    uiString	errMsg() const	{ return errmsg_; }
    void	abort();	//!<Just stops all pending operations.

    bool	writeChar(char);
    bool	writeShort(short);
    bool	writeInt32(od_int32);
    bool	writeInt64(od_int64);
    bool	writeFloat(float);
    bool	writeDouble(double);
    bool	write(const OD::String&);
    bool	write(const IOPar&);

    bool	write(const Network::RequestPacket&,bool wait=false);

    bool	writeArray(const void*,od_int64,bool wait=false);
    bool	writeShortArray(const short*,od_int64,bool wait=false);
    bool	writeInt32Array(const od_int32*,od_int64,bool wait=false);
    bool	writeInt64Array(const od_int64*,od_int64,bool wait=false);
    bool	writeFloatArray(const float*,od_int64,bool wait=false);
    bool	writeDoubleArray(const double*,od_int64,bool wait=false);

    enum ReadStatus { ReadOK, Timeout, ReadError };
    bool	readChar(char&) const;
    bool	readShort(short&) const;
    bool	readInt32(od_int32&) const;
    bool	readInt64(od_int64&) const;
    bool	readFloat(float&) const;
    bool	readDouble(double&) const;
    bool	read(BufferString&) const;
    bool	read(IOPar&) const;
    ReadStatus	read(Network::RequestPacket&) const;

    ReadStatus	readArray(void*,od_int64) const;
    bool	readShortArray(short*,od_int64) const;
    bool	readInt32Array(od_int32*,od_int64) const;
    bool	readInt64Array(od_int64*,od_int64) const;
    bool	readFloatArray(float*,od_int64) const;
    bool	readDoubleArray(double*,od_int64) const;

    Notifier<Socket> disconnected; //!< usually remote host terminates.
    Notifier<Socket> readyRead;    /*!<Note that object may or may not be
				      locked, so you may not be able to
				      read immediately */

    const void*			thread() const { return thread_; }
private:

    bool			waitForConnected() const;
				//!<\note Lock should be unlocked when calling
    bool			waitForNewData() const;
				//!<\note Lock should be locked when calling
    bool			waitForWrite(bool all) const;
				//!<\note Lock should be locked when calling

    mutable uiString		errmsg_;
    mutable Threads::Lock	lock_;

    int				timeout_;
    bool			noeventloop_;

    mQtclass(QTcpSocket)*	qtcpsocket_;
    bool			ownssocket_;

    mQtclass(QTcpSocketComm)*	socketcomm_;

    const void*			thread_;
};


} // namespace Network


#endif

