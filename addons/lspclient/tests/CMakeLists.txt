add_executable(lsptestapp lsptestapp.cpp ../lspclientserver.cpp
    ${lspclientplugin_debug_SRCS})
target_link_libraries(lsptestapp Qt5::Core)
