kate_add_plugin(rainbowparens)
target_compile_definitions(rainbowparens PRIVATE TRANSLATION_DOMAIN="rainbowparens")
target_link_libraries(rainbowparens PRIVATE KF6::TextEditor KF6::I18n)

target_sources(
  rainbowparens
  PRIVATE
    rainbowparens_plugin.cpp
)

if (ENABLE_PCH)
    target_precompile_headers(rainbowparens REUSE_FROM katepch)
endif()

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()
