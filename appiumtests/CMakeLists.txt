# SPDX-License-Identifier: MIT
# SPDX-FileCopyrightText: 2024 Antoine Herlicq <antoine.herlicq@enioka.com>

if(NOT BUILD_TESTING OR NOT CMAKE_SYSTEM_NAME MATCHES "Linux")
    return()
endif()

find_package(SeleniumWebDriverATSPI)
set_package_properties(SeleniumWebDriverATSPI PROPERTIES
        DESCRIPTION "Server component for selenium tests using Linux accessibility infrastructure"
        PURPOSE "Needed for GUI tests"
        URL "https://invent.kde.org/sdk/selenium-webdriver-at-spi"
        TYPE OPTIONAL
)
if(NOT SeleniumWebDriverATSPI_FOUND AND NOT DEFINED ENV{KDECI_BUILD})
    return()
endif()

add_test(
        NAME kwritetest
        COMMAND selenium-webdriver-at-spi-run ${CMAKE_CURRENT_SOURCE_DIR}/kwritetest.py
)
set_tests_properties(kwritetest PROPERTIES TIMEOUT 300 ENVIRONMENT "CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}/bin")

