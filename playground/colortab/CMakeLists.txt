project(colortab)
find_package(KDE4 REQUIRED)
include_directories( ${KDE4_INCLUDES} . )
set(colortab_SRCS main.cpp katecolortreewidget.cpp)
kde4_add_executable(colortab ${colortab_SRCS} )
target_link_libraries(colortab ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS} )

