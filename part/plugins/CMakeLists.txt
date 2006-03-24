kde4_header()

add_subdirectory( isearch ) 
add_subdirectory( insertfile ) 
add_subdirectory( kdatatool ) 
add_subdirectory( wordcompletion ) 

include_directories( ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = isearch insertfile kdatatool wordcompletion
#
