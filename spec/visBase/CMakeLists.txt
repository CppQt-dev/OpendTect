#_______________________Pmake___________________________________________________
#
#	Makefile : 	visBase
# 	May 2012 :	Nageswara
#_______________________________________________________________________________

find_package( OpenGL REQUIRED )
if ( UNIX )
    if ( APPLE )
    else()
	find_package( X11 )
	set( EXTRA_LIBS ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${X11_LIBRARIES} )
	set( OD_MODULE_PROGS od_glxinfo.c )
    endif( APPLE )
else()
	set( EXTRA_LIBS ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} )
	set( OD_MODULE_PROGS od_glxinfo.c ) 
endif( UNIX )

OD_INIT_MODULE()
