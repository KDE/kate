# Copyright 2011 Alex Turbov <i.zaufi@gmail.com>

# check if autogen is even installed
find_program(AUTOGEN_EXECUTABLE autogen)
if(AUTOGEN_EXECUTABLE)

    # prepare autogen wrapper for new class skeleton creation
    configure_file(
        ${PROJECT_SOURCE_DIR}/cmake/support/new_class_wrapper.sh.in
        ${PROJECT_BINARY_DIR}/cmake/support/new_class_wrapper.sh
        @ONLY
      )
    # prepare autogen wrapper for unit tests
    configure_file(
        ${PROJECT_SOURCE_DIR}/cmake/support/new_class_tester_wrapper.sh.in
        ${PROJECT_BINARY_DIR}/cmake/support/new_class_tester_wrapper.sh
        @ONLY
      )

    # add new-class as target
    add_custom_target(new-class /bin/sh ${PROJECT_BINARY_DIR}/cmake/support/new_class_wrapper.sh)
    add_dependencies(new-class ${PROJECT_BINARY_DIR}/cmake/support/new_class_wrapper.sh)
    add_dependencies(new-class ${PROJECT_SOURCE_DIR}/cmake/support/class.tpl)

    # add new-class-tester as target
    add_custom_target(new-class-tester /bin/sh ${PROJECT_BINARY_DIR}/cmake/support/new_class_tester_wrapper.sh)
    add_dependencies(new-class-tester ${PROJECT_BINARY_DIR}/cmake/support/new_class_tester_wrapper.sh)
    add_dependencies(new-class-tester ${PROJECT_SOURCE_DIR}/cmake/support/class_tester.tpl)

endif(AUTOGEN_EXECUTABLE)
