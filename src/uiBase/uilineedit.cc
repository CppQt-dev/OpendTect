/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          25/05/2000
________________________________________________________________________

-*/

#include "uilineedit.h"
#include "i_qlineedit.h"

#include "datainpspec.h"
#include "mouseevent.h"
#include "uibody.h"
#include "uiicon.h"
#include "uiobjbody.h"
#include "uivirtualkeyboard.h"

#include <QSize>
#include <QCompleter>
#include <QContextMenuEvent>
#include <QIntValidator>
#include <QDoubleValidator>

mUseQtnamespace

class uiLineEditBody : public uiObjBodyImpl<uiLineEdit,QLineEdit>
{
public:

                        uiLineEditBody( uiLineEdit& hndle,
				   uiParent*, const char* nm="Line Edit body");

    virtual		~uiLineEditBody()		{ delete &messenger_; }

    virtual int	nrTxtLines() const		{ return 1; }

protected:

    virtual void	contextMenuEvent(QContextMenuEvent*);

private:

    i_lineEditMessenger& messenger_;

};


uiLineEditBody::uiLineEditBody( uiLineEdit& hndle,uiParent* parnt,
				const char* nm )
    : uiObjBodyImpl<uiLineEdit,QLineEdit>(hndle,parnt,nm)
    , messenger_ ( *new i_lineEditMessenger(this,&hndle) )
{
    setStretch( 1, 0 );
    setHSzPol( uiObject::Medium );
}


void uiLineEditBody::contextMenuEvent( QContextMenuEvent* ev )
{
    if ( uiVirtualKeyboard::isVirtualKeyboardEnabled() )
	handle().popupVirtualKeyboard( ev->globalX(), ev->globalY() );
    else
	QLineEdit::contextMenuEvent( ev );
}


void uiLineEdit::setIcon( const char* icid )
{
    uiIcon icon( icid );
    body_->addAction( icon.qicon(), QLineEdit::LeadingPosition );
}


//------------------------------------------------------------------------------


uiLineEdit::uiLineEdit( uiParent* parnt, const DataInpSpec& spec,
			const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , editingFinished(this), returnPressed(this)
    , focusChanged(this), textChanged(this)
    , UserInputObjImpl<const char*>()
{
    setText( spec.text() );
    if ( spec.type().rep() == InpDataType::floatTp ||
	 spec.type().rep() == InpDataType::doubleTp )
	body_->setValidator( new QDoubleValidator );
    else if ( spec.type().rep() == InpDataType::intTp )
	body_->setValidator( new QIntValidator );
}


uiLineEdit::uiLineEdit( uiParent* parnt, const char* nm )
    : uiObject( parnt, nm, mkbody(parnt,nm) )
    , editingFinished(this), returnPressed(this)
    , focusChanged(this), textChanged(this)
    , UserInputObjImpl<const char*>()
{
    setText( "" );
}


uiLineEditBody& uiLineEdit::mkbody( uiParent* parnt, const char* nm )
{
    body_ = new uiLineEditBody(*this,parnt,nm);
    return *body_;
}


const char* uiLineEdit::getvalue_() const
{
    result_.set( body_->text() ).trimBlanks();
    return result_.buf();
}


void uiLineEdit::setvalue_( const char* t )
{
    mBlockCmdRec;
    body_->setText( mIsUdf(t) ? QString() : QString(t) );
    body_->setCursorPosition( 0 );
    setEdited( false );
}


void uiLineEdit::setPasswordMode()
{
    body_->setEchoMode( QLineEdit::Password );
}


void uiLineEdit::setValidator( const uiIntValidator& val )
{
    body_->setValidator( new QIntValidator(val.bottom_, val.top_,body_) );
}


void uiLineEdit::setValidator( const uiFloatValidator& val )
{
    QDoubleValidator* qdval =
	new QDoubleValidator( val.bottom_, val.top_, val.nrdecimals_, body_ );
    if ( !val.scnotation_ )
	qdval->setNotation( QDoubleValidator::StandardNotation );
    body_->setValidator( qdval );
}


void uiLineEdit::setMaxLength( int maxtxtlength )
{ body_->setMaxLength( maxtxtlength ); }

int uiLineEdit::maxLength() const
{ return body_->maxLength(); }

void uiLineEdit::setEdited( bool yn )
{ body_->setModified( yn ); }

bool uiLineEdit::isEdited() const
{ return body_->isModified(); }

void uiLineEdit::setCompleter( const BufferStringSet& bs, bool cs )
{
    QStringList qsl;
    for ( int idx=0; idx<bs.size(); idx++ )
	qsl << QString( bs.get(idx) );

    QCompleter* qc = new QCompleter( qsl, 0 );
    qc->setCaseSensitivity( cs ? Qt::CaseSensitive
			       : Qt::CaseInsensitive );
    body_->setCompleter( qc );
}


void uiLineEdit::setPlaceholderText( const uiString& txt )
{
    body_->setPlaceholderText( toQString(txt) );
}


void uiLineEdit::setReadOnly( bool yn )
{ body_->setReadOnly( yn ); }

bool uiLineEdit::isReadOnly() const
{ return body_->isReadOnly(); }

bool uiLineEdit::update_( const DataInpSpec& spec )
{ setText( spec.text() ); return true; }

void uiLineEdit::home()
{ body_->home( false ); }

void uiLineEdit::end()
{ body_->end( false ); }

void uiLineEdit::backspace()
{ body_->backspace(); }

void uiLineEdit::del()
{ body_->del(); }

void uiLineEdit::cursorBackward( bool mark, int steps )
{ body_->cursorBackward( mark, steps ); }

void uiLineEdit::cursorForward( bool mark, int steps )
{ body_->cursorForward( mark, steps ); }

int uiLineEdit::cursorPosition() const
{ return body_->cursorPosition(); }

void uiLineEdit::insert( const char* txt )
{
    mBlockCmdRec;
    body_->insert( txt );
}

int uiLineEdit::selectionStart() const
{ return body_->selectionStart(); }

void uiLineEdit::setSelection( int start, int length )
{ body_->setSelection( start, length ); }


const char* uiLineEdit::selectedText() const
{
    result_ = body_->selectedText();
    return result_.buf();
}


bool uiLineEdit::handleLongTabletPress()
{
    const Geom::Point2D<int> pos = TabletInfo::currentState()->globalpos_;
    popupVirtualKeyboard( pos.x_, pos.y_ );
    return true;
}


void uiLineEdit::popupVirtualKeyboard( int globalx, int globaly )
{
    mDynamicCastGet( uiVirtualKeyboard*, virkeyboardparent, parent() );

    if ( virkeyboardparent || isReadOnly() || !hasFocus() )
	return;

    uiVirtualKeyboard virkeyboard( *this, globalx, globaly );
    virkeyboard.show();

    if ( virkeyboard.enterPressed() )
	returnPressed.trigger();

    editingFinished.trigger();
}
