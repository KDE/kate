
find_package(Plasma)
if(PLASMA_FOUND)
	add_subdirectory( sessionmenu )
else(PLASMA_FOUND)
	        MESSAGE(STATUS "Plasma include files (plasma/kpanelapplet.h) was not found. compile kdebase before to compile applet")
endif(PLASMA_FOUND)
