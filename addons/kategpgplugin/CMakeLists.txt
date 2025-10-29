find_package(Gpgmepp QUIET)
find_library(GPGMEPP_LIBRARY gpgmepp QUIET)
# With this order we successfully skip the build when gpgmepp(.lib) was not found
if(NOT Gpgmepp_FOUND OR NOT GPGMEPP_LIBRARY)
    message(WARNING "Gpgmepp library not found. GPG plugin build will be skipped!")
    # Do not add target for this plugin
    set(SKIP_KATEGPGPLUGIN TRUE PARENT_SCOPE)
    return()
else()
    message(STATUS "GpgmeppLibrary found: ${GPGMEPP_LIBRARY}")
    kate_add_plugin(kategpgplugin)
    target_compile_definitions(kategpgplugin PRIVATE TRANSLATION_DOMAIN="kategpgplugin")

    target_link_libraries(
	    kategpgplugin
	    PRIVATE
	    kateprivate
	    KF6::CoreAddons
	    KF6::I18n
	    KF6::TextEditor
            ${GPGMEPP_LIBRARY}
    )

    target_sources(
        kategpgplugin
        PRIVATE
        kategpgplugin.hpp
        gpgkeydetails.hpp
        gpgmeppwrapper.hpp
        kategpgplugin.cpp
        gpgkeydetails.cpp
        gpgmeppwrapper.cpp
    )

    if (ENABLE_PCH)
        target_precompile_headers(kategpgplugin REUSE_FROM katepch)
    endif()
endif()
