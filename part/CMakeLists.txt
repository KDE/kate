kde4_header()

add_subdirectory( part ) 
add_subdirectory( data ) 
add_subdirectory( scripts ) 
add_subdirectory( plugins ) 
add_subdirectory( tests ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############


kde4_create_doxygen_docs( REFERENCES kdecore dcop kio kdeui kparts  )

kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = part data scripts plugins tests
#EXTRA_DIST = AUTHORS COPYING.LIB ChangeLog NEWS README TODO
#
#messages:
#	$(EXTRACTRC) ./*/*.rc > rc.cpp
#	$(EXTRACTATTR) --attr=language,name,Language --attr="language,section,Language Section" data/*.xml >> rc.cpp
#	$(XGETTEXT) `find . -name "*.cpp"` part/*.h -o $(podir)/katepart.pot
#
#DOXYGEN_REFERENCES = kdecore dcop kio kdeui kparts
#include ../admin/Doxyfile.am
#
