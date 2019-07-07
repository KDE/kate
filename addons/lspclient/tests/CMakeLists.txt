add_executable(lsptestapp lsptestapp.cpp ../lspclientserver.cpp
    ${lspclientplugin_debug_SRCS})
target_link_libraries(lsptestapp KF5::TextEditor Qt5::Core)
