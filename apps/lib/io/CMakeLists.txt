kcoreaddons_add_plugin(kio_exec
    INSTALL_NAMESPACE kf6/kio
)

target_sources(kio_exec PRIVATE
    exec.cpp
)

target_link_libraries(kio_exec executils KF6::KIOCore KF6::I18n KF6::ConfigCore)
