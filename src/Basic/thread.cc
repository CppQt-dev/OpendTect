/*
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Mar 2000
-*/

static const char* rcsID = "$Id: thread.cc,v 1.6 2002-08-26 13:03:37 bert Exp $";

#include "thread.h"
#include "callback.h"

Threads::Mutex::Mutex()
{
#ifdef __pthread__
    pthread_mutexattr_init( &attr );
    pthread_mutex_init( &mutex, &attr );
#endif
}

Threads::Mutex::~Mutex()
{
#ifdef __pthread__
    pthread_mutex_destroy( &mutex );
    pthread_mutexattr_destroy( &attr );
#endif
}

int Threads::Mutex::lock()
{ 
#ifdef __pthread__
    return pthread_mutex_lock( &mutex ); 
#endif
    return 0;
}

int Threads::Mutex::unlock()
{
#ifdef __pthread__
    return pthread_mutex_unlock( &mutex );
#endif
    return 0;
}


Threads::ConditionVar::ConditionVar()
{
#ifdef __pthread__
    pthread_condattr_init( &condattr );
    pthread_cond_init( &cond, &condattr );
#endif
}

Threads::ConditionVar::~ConditionVar()
{
#ifdef __pthread__
    pthread_cond_destroy( &cond );
    pthread_condattr_destroy( &condattr );
#endif
}


int Threads::ConditionVar::wait()
{
#ifdef __pthread__
    return pthread_cond_wait( &cond, &mutex );
#endif
    return 0;
}


int Threads::ConditionVar::signal(bool all)
{
#ifdef __pthread__
    return all 	? pthread_cond_broadcast( &cond )
		: pthread_cond_signal( &cond );
#endif
    return 0;
}


Threads::Thread::Thread( void (func)(void*), void* data )
    	: id(0)
{
#ifdef __pthread__
    pthread_create( &id, 0, (void* (*)(void*)) func, data );
#endif
}


static void* thread_exec_fn( void* obj )
{
    CallBack* cbptr = reinterpret_cast<CallBack*>( obj );
    cbptr->doCall( 0 );
    return 0;
}


Threads::Thread::Thread( const CallBack& cb )
    	: id(0)
{
    if ( !cb.willCall() ) return;
    static CallBack lcb( cb );
#ifdef __pthread__
    pthread_create( &id, 0, thread_exec_fn, &lcb );
#endif
}


void Threads::Thread::destroy(bool wait, void** ret_val)
{
#ifdef __pthread__
    if ( wait )
	pthread_join( id, ret_val );
    else
    {
	pthread_detach( id );
    }
    delete this;
#endif
}


void Threads::Thread::threadExit( void* rv )
{
#ifdef __pthread__
    pthread_exit( rv );
#endif
}
