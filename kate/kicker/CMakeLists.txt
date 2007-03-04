FIND_PATH( KICKER_INCLUDE_DIR plasma/kpanelapplet.h
	  ${KDE4_INCLUDE_DIR}	  
	  )
if(KICKER_INCLUDE_DIR)
	add_subdirectory( menuext )
else(KICKER_INCLUDE_DIR)
	        MESSAGE(STATUS "Plasma include files (plasma/kpanelapplet.h) was not found. compile kdebase before to compile applet")
endif(KICKER_INCLUDE_DIR)
