ecm_set_disabled_deprecation_versions(
    QT 5.15
    KF 5.90
)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.cpp "/*empty file*/")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.c "/*empty file*/")


add_library(katepch STATIC ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.cpp ${CMAKE_CURRENT_BINARY_DIR}/empty_pch.c)
target_precompile_headers(katepch PRIVATE pch.h)
target_link_libraries(katepch PRIVATE Qt::Core Qt::Gui Qt::Widgets)
