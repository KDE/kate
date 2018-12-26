# small test for the tabswitches
add_executable(tstestapp tstestapp.cpp ../tabswitcherfilesmodel.cpp)
target_link_libraries(tstestapp KF5::TextEditor)
