/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2003
________________________________________________________________________

-*/

#include "uimain.h"
#include "uibatchhostsdlg.h"

#include "prog.h"

int main( int argc, char** argv )
{
    OD::SetRunContext( OD::UiProgCtxt );
    SetProgramArgs( argc, argv );
    uiMain app( argc, argv );

    uiBatchHostsDlg* dlg = new uiBatchHostsDlg( 0 );
    dlg->showAlwaysOnTop();
    app.setTopLevel( dlg );
    dlg->show();

    return ExitProgram( app.exec() );
}
