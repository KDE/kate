find_package(KF5DocTools QUIET)

if(NOT KF5DocTools_FOUND)
  return()
endif()

ecm_optional_add_subdirectory(kate)
ecm_optional_add_subdirectory(katepart)
ecm_optional_add_subdirectory(kwrite)
