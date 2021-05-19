add_library(rainbowparens MODULE "")
target_compile_definitions(rainbowparens PRIVATE TRANSLATION_DOMAIN="rainbowparens")
target_link_libraries(rainbowparens PRIVATE KF5::TextEditor)

target_sources(
  rainbowparens
  PRIVATE
    rainbowparens_plugin.cpp
)

