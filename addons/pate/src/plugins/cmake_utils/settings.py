# -*- coding: utf-8 -*-
'''CMake helper plugin: configuration'''

import kate

# Configuration keys
CMAKE_BINARY = 'cmakeBinary'
PARENT_DIRS_LOOKUP_CNT = 'parentDirsLookupCnt'
PROJECT_DIR = 'projectBuildDir'
AUX_MODULE_DIRS = 'auxModuleDirs'

TOOLVIEW_ADVANCED_MODE = 'toolvewAdvancedMode'
TOOLVIEW_BEAUTIFY = 'toolviewBeautifyHelp'

CMAKE_BINARY_DEFAULT = '/usr/bin/cmake'
CMAKE_UTILS_SETTINGS_UI = 'settings.ui'
CMAKE_TOOLVIEW_CACHEVIEW_UI = 'toolview_cacheview_page.ui'
CMAKE_TOOLVIEW_HELP_UI = 'toolview_help_page.ui'
CMAKE_TOOLVIEW_SETTINGS_UI = 'toolview_settings_page.ui'


def get_aux_dirs():
    cmake_utils_conf = kate.sessionConfiguration.root.get('cmake_utils', {})
    if AUX_MODULE_DIRS in cmake_utils_conf and cmake_utils_conf[AUX_MODULE_DIRS]:
        return cmake_utils_conf[AUX_MODULE_DIRS]
    return []

# kate: indent-width 4;
