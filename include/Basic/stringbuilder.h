#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		May 2019
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"

/*!\brief Builds a string by adding strings. Much faster than string
  manipulation. Only supports adding. */

mExpClass(Basic) StringBuilder
{
public:

    mUseType( BufferString,	size_type );

			StringBuilder()		    {}
			StringBuilder( const StringBuilder& oth )
						    { *this = oth; }
			StringBuilder(const char*);
    virtual		~StringBuilder()	    { delete [] buf_; }

    StringBuilder&	operator=(const StringBuilder&);
    StringBuilder&	operator=(const char*);

    const char*		result() const		    { return buf_; }

    bool		operator==(const StringBuilder&) const;
    bool		operator!=(const StringBuilder&) const;

    bool		isEmpty() const		    { return !buf_; }
    StringBuilder&	setEmpty();
    StringBuilder&	set(const char*);
    template <class T>
    StringBuilder&	set(const T&);

    StringBuilder&	add(const char*);
    StringBuilder&	add(char,size_type nr=1);
    StringBuilder&	add(const QString&);
    template <class T>	inline
    StringBuilder&	add( const T& t )	{ return add( toString(t) ); }

    StringBuilder&	addSpace( size_type nr=1 )	{ return add(' ',nr); }
    StringBuilder&	addTab( size_type nr=1 )	{ return add('\t',nr); }
    StringBuilder&	addNewLine( size_type nr=1 )	{ return add('\n',nr); }

protected:

    char*		buf_		= nullptr;
    size_type		bufsz_		= 0;
    size_type		curpos_		= 0;

};
