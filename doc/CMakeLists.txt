find_package(KF6DocTools ${KF6_DEP_VERSION} QUIET)

if(NOT KF5DocTools_FOUND AND NOT KF6DocTools_FOUND)
  return()
endif()

ecm_optional_add_subdirectory(kate)
ecm_optional_add_subdirectory(katepart)
ecm_optional_add_subdirectory(kwrite)
