set(PROJECT_SOURCES
    plugin.qrc
    assistant.cpp
    assistant.h
    assistantview.cpp
    assistantview.h
)

kate_add_plugin(assistant)
target_compile_definitions(assistant PRIVATE TRANSLATION_DOMAIN="assistant")
target_link_libraries(assistant PRIVATE kateprivate KF6::I18n KF6::TextEditor)
target_sources(assistant PRIVATE ${PROJECT_SOURCES})

if (ENABLE_PCH)
    target_precompile_headers(assistant REUSE_FROM katepch)
endif()
