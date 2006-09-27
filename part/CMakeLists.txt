
project(kate)

include_directories( ${KDE4_KIO_INCLUDES} )

add_subdirectory( part )
add_subdirectory( data ) 
add_subdirectory( indent ) 
add_subdirectory( scripts ) 
add_subdirectory( plugins ) 
add_subdirectory( tests ) 

########### install files ###############





