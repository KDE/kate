include_directories( ${KDE4_KIO_INCLUDES} )

add_subdirectory( insertfile )
add_subdirectory( kdatatool )
add_subdirectory( exporter )
add_subdirectory( autobrace )
add_subdirectory( kte_iconinserter )
add_subdirectory( kte_insanehtml_le )
add_subdirectory( pythonencoding )
# This plugin is for howto-write-a-plugin purposes. It is not meant to be
# installed. If you want to do so, just uncomment the add_subdirectory line.
#
# add_subdirectory( timedate )
