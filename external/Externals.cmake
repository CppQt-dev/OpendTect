include ( "CMakeModules/ODExternals.cmake" )

DEFINE_GIT_EXTERNAL( osgGeo https://github.com/OpendTect/osgGeo.git master )
DEFINE_GIT_EXTERNAL( proj4 https://github.com/OSGeo/proj.4.git 5.1 )
DEFINE_SVN_EXTERNAL( doc_csh https://github.com/OpendTect/opendtectdoc.git/trunk/od_userdoc/Project/Advanced/CSH ${CMAKE_SOURCE_DIR}/external HEAD )
