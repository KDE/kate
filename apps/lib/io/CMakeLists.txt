kcoreaddons_add_plugin(kio_kateexec
    INSTALL_NAMESPACE kf6/kio
)

target_sources(kio_kateexec PRIVATE
    exec.cpp
)

target_link_libraries(kio_kateexec kateprivate KF6::KIOCore KF6::I18n KF6::ConfigCore)
