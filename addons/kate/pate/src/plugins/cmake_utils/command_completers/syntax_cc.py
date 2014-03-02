# -*- coding: utf-8 -*-
'''CMake generic command completers registrar'''

#
# Copyright 2013 by Alex Turbov <i.zaufi@gmail.com>
#
# This software is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this software.  If not, see <http://www.gnu.org/licenses/>.

from cmake_utils import cmake_help_parser
from cmake_utils.param_types import *


def register_command_completer(completers):
    completers['add_compile_options'] = [
        Value([(ANY, ONE_OR_MORE)])
      ]

    add_custom_command_sig_1 = [
        Option('OUTPUT', 1, [(ANY, ONE_OR_MORE)])
      , Option('COMMAND', ONE_OR_MORE, [(ANY, ONE_OR_MORE)])
      , Option('MAIN_DEPENDENCY', ZERO_OR_ONE, [(ANY, 1)])
      , Option('DEPENDS', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('IMPLICIT_DEPENDS', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('WORKING_DIRECTORY', ZERO_OR_ONE, [(DIR, 1)])
      , Option('COMMENT', ZERO_OR_ONE, [(STRING, 1)])
      , Option('VERBATIM', ZERO_OR_ONE)
      , Option('APPEND', ZERO_OR_ONE)
      ]
    add_custom_command_sig_2 = [
        Option('TARGET', 1, [(ANY, 1)])
      , OneOf(Option('PRE_BUILD'), Option('PRE_LINK'), Option('POST_BUILD'))
      , Option('COMMAND', ONE_OR_MORE, [(ANY, ONE_OR_MORE)])
      , Option('WORKING_DIRECTORY', ZERO_OR_ONE, [(DIR, 1)])
      , Option('COMMENT', ZERO_OR_ONE, [(STRING, 1)])
      , Option('VERBATIM', ZERO_OR_ONE)
      ]

    def add_custom_command_sig_dispatch(comp_list):
        if 1 <= len(comp_list) and 'OUTPUT' == comp_list[0]:
            return 0
        if 1 <= len(comp_list) and 'TARGET' == comp_list[0]:
            return 1
        return ['TARGET', 'OUTPUT']

    completers['add_custom_command'] = MultiSignature(
        add_custom_command_sig_dispatch
      , add_custom_command_sig_1
      , add_custom_command_sig_2
      )

    completers['add_custom_target'] = [
        Value([(ANY, 1)])                                   # Name
      , Option('ALL', ZERO_OR_ONE)
      , Value([(ANY, ZERO_OR_MORE)])                        # 1st command
      , Option('COMMAND', ZERO_OR_MORE, [(ANY, ONE_OR_MORE)])
      , Option('DEPENDS', ZERO_OR_ONE, [(IDENTIFIER, ONE_OR_MORE)])
      , Option('WORKING_DIRECTORY', ZERO_OR_ONE, [(DIR, 1)])
      , Option('COMMENT', ZERO_OR_ONE, [(STRING, 1)])
      , Option('VERBATIM', ZERO_OR_ONE)
      , Option('SOURCES', ZERO_OR_MORE, [(ANY, ONE_OR_MORE)])
      ]

    completers['add_definitions'] = [
        Value([(ANY, ONE_OR_MORE)])
      ]

    completers['add_dependencies'] = [
        Value([(IDENTIFIER, ONE_OR_MORE)])
      ]

    # TODO Add ALIAS option
    completers['add_executable'] = [
        Value([(IDENTIFIER, 1)])
      , Option('WIN32', ZERO_OR_ONE, description='The property WIN32_EXECUTABLE will be set on the target')
      , Option('MACOSX_BUNDLE', ZERO_OR_ONE)
      , Option('EXCLUDE_FROM_ALL', ZERO_OR_ONE)
      , Value([(FILE, ONE_OR_MORE)])
      ]

    # TODO Add ALIAS option
    add_library_sig_1 = [
        Value([(IDENTIFIER, 1)])
      , OneOf(Option('STATIC'), Option('SHARED'), Option('MODULE'))
      , Option('EXCLUDE_FROM_ALL', ZERO_OR_ONE)
      , Value([(FILE, ONE_OR_MORE)])
      ]
    add_library_sig_2 = [
        Value([(IDENTIFIER, 1)])
      , OneOf(Option('STATIC'), Option('SHARED'), Option('MODULE'))
      , Option('IMPORTED')
      , Option('GLOBAL', ZERO_OR_ONE)
      ]
    add_library_sig_3 = [
        Value([(IDENTIFIER, 1)])
      , Option('OBJECT')
      , Value([(FILE, ONE_OR_MORE)])
      ]

    def add_library_sig_dispatch(comp_list):
        if 1 <= len(comp_list) and 'IMPORTED' in comp_list:
            return 1
        if 1 <= len(comp_list) and 'OBJECT' in comp_list:
            return 2
        return 0

    completers['add_library'] = MultiSignature(
        add_library_sig_dispatch
      , add_library_sig_1
      , add_library_sig_2
      , add_library_sig_3
      )

    completers['add_subdirectory'] = [
        Value([(DIR, ONE_OR_MORE)])
      , Option('EXCLUDE_FROM_ALL')
      ]

    # TODO add_test

    completers['aux_source_directory'] = [
        Value([(DIR, ONE_OR_MORE)])
      , Value([(ANY, 1)])
      ]

    completers['break'] = []

    completers['build_command'] = [
        Value([(ANY, 1)])
      , Option('CONFIGURATION', ZERO_OR_ONE, [(ANY, 1)])      # TODO Complete configuration names
      , Option('PROJECT_NAME', ZERO_OR_ONE, [(ANY, 1)])
      , Option('TARGET', ZERO_OR_ONE, [(ANY, 1)])
      ]

    completers['cmake_host_system_information'] = [
        Option('RESULT', 1, [(ANY, 1)])
      , Option('QUERY', ZERO_OR_ONE, [(ONE_OF, [
            'NUMBER_OF_LOGICAL_CORES'
          , 'NUMBER_OF_PHYSICAL_CORES'
          , 'HOSTNAME'
          , 'FQDN'
          , 'TOTAL_VIRTUAL_MEMORY'
          , 'AVAILABLE_VIRTUAL_MEMORY'
          , 'TOTAL_PHYSICAL_MEMORY'
          , 'AVAILABLE_PHYSICAL_MEMORY'
          ])])
      ]

    completers['cmake_minimum_required'] = [
        Option('VERSION', 1, [(VERSION, 1)])
      , Option('FATAL_ERROR', ZERO_OR_ONE)
      ]

    # ATTENTION `cmake_policy` completer in a separate module

    completers['configure_file'] = [
        Value([(FILE, 2)])
      , Option('COPY_ONLY', ZERO_OR_ONE)
      , Option('ESCAPE_QUOTES', ZERO_OR_ONE)
      , Option('@ONLY', ZERO_OR_ONE)
      , Option('NEWLINE_STYLE', ZERO_OR_ONE, [(ONE_OF, ['UNIX', 'DOS', 'WIN32', 'CRLF', 'LF'])])
      ]

    # TODO create_test_sourcelist

    completers['define_property'] = [
        OneOf(
            Option('GLOBAL')
          , Option('DIRECTORY')
          , Option('TARGET')
          , Option('SOURCE')
          , Option('TEST')
          , Option('VARIABLE')
          , Option('CACHED_VARIABLE')
          , exppos=0
          )
      , Option('PROPERTY', 1, [(PROPERTY, 1)])
      , Option('INHERITED', ZERO_OR_ONE)
      , Option('BRIEF_DOCS', 1, [(STRING, ONE_OR_MORE)])
      , Option('FULL_DOCS', 1, [(STRING, ONE_OR_MORE)])
      ]

    # NOTE Nothing to complete for else()
    # NOTE elseif() implemented as a separate module

    completers['enable_language'] = [
        OneOf(
            Option('C')
          , Option('CXX')
          , Option('Fortran')
          )
      , Option('OPTIONAL', ZERO_OR_ONE)
      ]

    # NOTE Nothing to complete for enable_testing(), endforeach(), endfunction(),
    # endif(), endmacro(), endwhile()

    completers['execute_process'] = [
        Option('COMMAND', ONE_OR_MORE, [(ANY, ONE_OR_MORE)], exppos=0)
      , Option('WORKING_DIRECTORY', ZERO_OR_ONE, [(DIR, 1)])
      , Option('TIMEOUT', ZERO_OR_ONE, [(ANY, 1)])
      , Option('RESULT_VARIABLE', ZERO_OR_ONE, [(ANY, 1)])
      , Option('OUTPUT_VARIABLE', ZERO_OR_ONE, [(ANY, 1)])
      , Option('ERROR_VARIABLE', ZERO_OR_ONE, [(ANY, 1)])
      , Option('INPUT_FILE', ZERO_OR_ONE, [(FILE, 1)])
      , Option('OUTPUT_FILE', ZERO_OR_ONE, [(FILE, 1)])
      , Option('ERROR_FILE', ZERO_OR_ONE, [(FILE, 1)])
      , Option('OUTPUT_QUIET', ZERO_OR_ONE)
      , Option('ERROR_QUIET', ZERO_OR_ONE)
      , Option('OUTPUT_STRIP_TRAILING_WHITESPACE', ZERO_OR_ONE)
      , Option('ERROR_STRIP_TRAILING_WHITESPACE', ZERO_OR_ONE)
      ]

    export_sig_1 = [
        Option('TARGETS', 1, [(ANY, ONE_OR_MORE)], exppos=0)
      , Option('NAMESPACE', ZERO_OR_ONE, [(ANY, 1)])
      , Option('APPEND', ZERO_OR_ONE)
      , Option('FILE', 1, [(FILE, 1)])
      , Option('EXPORT_LINK_INTERFACE_LIBRARIES', ZERO_OR_ONE)
      ]
    export_sig_2 = [
        Option('PACKAGE', 1, [(ANY, 1)], exppos=0)
      ]

    def export_sig_dispatch(comp_list):
        if 1 <= len(comp_list) and 'TARGETS' == comp_list[0]:
            return 0
        if 1 <= len(comp_list) and 'PACKAGE' == comp_list[0]:
            return 1
        return ['TARGETS', 'PACKAGE']

    completers['export'] = MultiSignature(export_sig_dispatch, export_sig_1, export_sig_2)

    file_sig_1 = [
        Option('WRITE', 1, [(FILE, 1), (STRING, ONE_OR_MORE)])
      ]
    file_sig_2 = [
        Option('APPEND', 1, [(FILE, 1), (STRING, ONE_OR_MORE)])
      ]
    file_sig_3 = [
        Option('READ', 1, [(FILE, 1), (ANY, 1)])
      , Option('LIMIT', ZERO_OR_ONE, [(ANY, 1)])
      , Option('OFFSET', ZERO_OR_ONE, [(ANY, 1)])
      , Option('HEX', ZERO_OR_ONE)
      ]
    file_sig_4 = [
        OneOf(
            Option('MD5', 1, [(FILE, 1), (ANY, 1)])
          , Option('SHA1', 1, [(FILE, 1), (ANY, 1)])
          , Option('SHA224', 1, [(FILE, 1), (ANY, 1)])
          , Option('SHA256', 1, [(FILE, 1), (ANY, 1)])
          , Option('SHA384', 1, [(FILE, 1), (ANY, 1)])
          , Option('SHA512', 1, [(FILE, 1), (ANY, 1)])
          )
      ]
    file_sig_5 = [
        Option('STRINGS', 1, [(FILE, 1), (ANY, 1)])
      , Option('LIMIT_COUNT', ZERO_OR_ONE, [(ANY, 1)])
      , Option('LIMIT_INPUT', ZERO_OR_ONE, [(ANY, 1)])
      , Option('LIMIT_OUTPUT', ZERO_OR_ONE, [(ANY, 1)])
      , Option('LENGTH_MINIMUM', ZERO_OR_ONE, [(ANY, 1)])
      , Option('LENGTH_MAXIMUM', ZERO_OR_ONE, [(ANY, 1)])
      , Option('NEWLINE_CONSUME', ZERO_OR_ONE)
      , Option('REGEX', ZERO_OR_ONE, [(ANY, 1)])
      , Option('NO_HEX_CONVERSION', ZERO_OR_ONE)
      ]
    file_sig_6 = [
        Option('GLOB', 1)
      , Option('RELATIVE', ZERO_OR_ONE, [(DIR, 1)])
      , Value([(ANY, ONE_OR_MORE)])
      ]
    file_sig_7 = [
        Option('GLOB_RECURSE', 1)
      , Option('RELATIVE', ZERO_OR_ONE, [(DIR, 1)])
      , Option('FOLLOW_SYMLINS', ZERO_OR_ONE)
      , Value([(ANY, ONE_OR_MORE)])
      ]
    file_sig_8 = [
        Option('RENAME', 1, [(ANY, 2)])
      ]
    file_sig_9 = [
        Option('REMOVE', 1, [(FILE, ONE_OR_MORE)])
      ]
    file_sig_10 = [
        Option('REMOVE_RECURSE', 1, [(FILE, ONE_OR_MORE)])
      ]
    file_sig_11 = [
        Option('MAKE_DIRECTORY', 1, [(DIR, ONE_OR_MORE)])
      ]
    file_sig_12 = [
        Option('RELATIVE_PATH', 1, [(ANY, 1), (DIR, 1), (FILE, 1)])
      ]
    file_sig_13 = [
        Option('TO_CMAKE_PATH', 1, [(DIR, 1), (ANY, 1)])
      ]
    file_sig_14 = [
        Option('TO_NATIVE_PATH', 1, [(DIR, 1), (ANY, 1)])
      ]
    file_sig_15 = [
        Option('DOWNLOAD', 1, [(ANY, 1), (FILE, 1)])
      , Option('INACTIVITY_TIMEOUT', ZERO_OR_ONE, [(ANY, 1)])
      , Option('TIMEOUT', ZERO_OR_ONE, [(ANY, 1)])
      , Option('STATUS', ZERO_OR_ONE, [(ANY, 1)])
      , Option('LOG', ZERO_OR_ONE, [(ANY, 1)])
      , Option('SHOW_PROGRESS', ZERO_OR_ONE)
      , Option('EXPECTED_HASH', ZERO_OR_ONE, [(ANY, 1)])
      , Option('EXPECTED_MD5', ZERO_OR_ONE, [(ANY, 1)])
      , Option('TLS_VERIFY', ZERO_OR_ONE, [(ONE_OF, ['on', 'off'])])
      , Option('TLS_CAINFO', ZERO_OR_ONE, [(FILE, 1)])
      ]
    file_sig_16 = [
        Option('UPLOAD', 1, [(FILE, 1), (ANY, 1)])
      , Option('INACTIVITY_TIMEOUT', ZERO_OR_ONE, [(ANY, 1)])
      , Option('TIMEOUT', ZERO_OR_ONE, [(ANY, 1)])
      , Option('STATUS', ZERO_OR_ONE, [(ANY, 1)])
      , Option('LOG', ZERO_OR_ONE, [(ANY, 1)])
      , Option('SHOW_PROGRESS', ZERO_OR_ONE)
      ]
    file_sig_17 = [
        Option('TIMESTAMP', 1, [(FILE, 1), (ANY, 1), (ANY, ZERO_OR_MORE)])
      , Option('UTC', ZERO_OR_ONE)
      ]

    _file_subcommands = [
        'WRITE', 'APPEND', 'READ', 'MD5', 'STRINGS', 'GLOB', 'GLOB_RECURSE', 'RENAME'
      , 'REMOVE', 'REMOVE_RECURSE', 'MAKE_DIRECTORY', 'RELATIVE_PATH', 'TO_CMAKE_PATH'
      , 'TO_NATIVE_PATH', 'DOWNLOAD', 'UPLOAD', 'TIMESTAMP'
      ]
    _file_digest_subcommands = ['MD5', 'SHA1', 'SHA224', 'SHA256', 'SHA384', 'SHA512']

    def file_sig_dispatch(comp_list):
        if comp_list and comp_list[0] in _file_subcommands:
            return _file_subcommands.index(comp_list[0])
        if comp_list and comp_list[0] in _file_digest_subcommands:
            return 3
        if not comp_list or len(comp_list) == 1:
            return _file_subcommands + _file_digest_subcommands
        return None

    completers['file'] = MultiSignature(
        file_sig_dispatch
      , file_sig_1,  file_sig_2,  file_sig_3,  file_sig_4
      , file_sig_5,  file_sig_6,  file_sig_7,  file_sig_8
      , file_sig_9,  file_sig_10, file_sig_11, file_sig_12
      , file_sig_13, file_sig_14, file_sig_15, file_sig_16
      , file_sig_17
      )

    completers['find_file'] = [
        Value([(ANY, 1)])                                   # Variable
      , Option('NAMES', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('HINTS', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('PATHS', ZERO_OR_ONE, [(DIR, ONE_OR_MORE)])
      , Option('PATH_SUFFIXES', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('DOC', ZERO_OR_ONE, [(STRING, 1)])
      , Option('NO_DEFAULT_PATH', ZERO_OR_ONE)
      , Option('NO_CMAKE_ENVIRONMENT_PATH', ZERO_OR_ONE)
      , Option('NO_CMAKE_PATH', ZERO_OR_ONE)
      , Option('NO_CMAKE_SYSTEM_PATH', ZERO_OR_ONE)
      , Option('NO_SYSTEM_ENVIRONMENT_PATH', ZERO_OR_ONE)
      , OneOf(
            Option('CMAKE_FIND_ROOT_PATH_BOTH', ZERO_OR_ONE)
          , Option('ONLY_CMAKE_FIND_ROOT_PATH', ZERO_OR_ONE)
          , Option('NO_CMAKE_FIND_ROOT_PATH', ZERO_OR_ONE)
          )
      ]

    # NOTE Syntax for find_library() same as for find_file() + one more option
    completers['find_library'] = completers['find_file'] + [Option('NAMES_PER_DIR', ZERO_OR_ONE)]

    find_package_common = [
        Value([(PACKAGE, 1), (VERSION, ZERO_OR_ONE)])       # Package name and version
      , Option('EXACT', ZERO_OR_ONE)
      , Option('QUIET', ZERO_OR_ONE)
      , Option('REQUIRED', ZERO_OR_ONE)
      , Option('COMPONENTS', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('NO_POLICY_SCOPE', ZERO_OR_ONE)
      ]
    find_package_sig_1 = find_package_common + [
        Option('MODULE', ZERO_OR_ONE)
      , Option('OPTIONAL_COMPONENTS', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      ]
    find_package_sig_2 = find_package_common + [
        OneOf(
            Option('CONFIG', ZERO_OR_ONE)
          , Option('NO_MODULE', ZERO_OR_ONE)
          )
      , Option('NAMES', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('CONFIGS', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('HINTS', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('PATHS', ZERO_OR_ONE, [(DIR, ONE_OR_MORE)])
      , Option('PATH_SUFFIXES', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('NO_DEFAULT_PATH', ZERO_OR_ONE)
      , Option('NO_CMAKE_BUILDS_PATH', ZERO_OR_ONE)
      , Option('NO_CMAKE_ENVIRONMENT_PATH', ZERO_OR_ONE)
      , Option('NO_CMAKE_PACKAGE_REGISTRY', ZERO_OR_ONE)
      , Option('NO_CMAKE_PATH', ZERO_OR_ONE)
      , Option('NO_CMAKE_SYSTEM_PACKAGE_REGISTRY', ZERO_OR_ONE)
      , Option('NO_CMAKE_SYSTEM_PATH', ZERO_OR_ONE)
      , Option('NO_SYSTEM_ENVIRONMENT_PATH', ZERO_OR_ONE)
      , OneOf(
            Option('CMAKE_FIND_ROOT_PATH_BOTH', ZERO_OR_ONE)
          , Option('ONLY_CMAKE_FIND_ROOT_PATH', ZERO_OR_ONE)
          , Option('NO_CMAKE_FIND_ROOT_PATH', ZERO_OR_ONE)
          )
      ]

    def find_package_sig_dispatch(comp_list):
        if 'CONFIG' in comp_list or 'NO_MODULE' in comp_list:
            return 1
        if 'MODULE' in comp_list:
            return 0
        return None

    completers['find_package'] = MultiSignature(
        find_package_sig_dispatch
      , find_package_sig_1
      , find_package_sig_2
      )

    # NOTE Syntax for find_path() and find_program() same as for find_file()
    completers['find_path'] = completers['find_file']
    completers['find_program'] = completers['find_file']

    # TODO fltk_wrap_ui

    foreach_sig_1 = [
        Value([(ANY, 1), (ANY, ONE_OR_MORE)])
      ]
    foreach_sig_2 = [
        Value([(ANY, 1)])
      , Option('RANGE', 1, [(ANY, ONE_OR_MORE)])
      ]
    foreach_sig_3 = [
        Value([(ANY, 1)])
      , Option('IN')
      , Option('LISTS', ZERO_OR_ONE, [(ANY, ZERO_OR_MORE)])
      , Option('ITEMS', ZERO_OR_ONE, [(ANY, ZERO_OR_MORE)])
      ]

    def foreach_sig_dispatch(comp_list):
        if comp_list and 2 <= len(comp_list):
            if comp_list[1] == 'RANGE':
                return 1
            if comp_list[1] == 'IN':
                return 2
            return 0
        return None

    completers['foreach'] = MultiSignature(
        foreach_sig_dispatch
      , foreach_sig_1
      , foreach_sig_2
      , foreach_sig_3
      )

    completers['function'] = [Value([(ANY, ONE_OR_MORE)])]

    completers['get_cmake_property'] = [Value([(ANY, 1), (PROPERTY, 1)])]

    get_directory_property_sig_1 = [
        Value([(ANY, 1)])                                   # Variable
      , Option('DIRECTORY', 1, [(DIR, 1)])
      , Value([(PROPERTY, 1)])
      ]
    get_directory_property_sig_2 = [
        Value([(ANY, 1)])                                   # Variable
      , Option('DIRECTORY', 1, [(DIR, 1)])
      , Option('DEFINITION', 1, [(ANY, 1)])
      ]

    def get_directory_property_sig_dispatch(comp_list):
        if 'DEFINITION' in comp_list:
            return 1
        # TODO Need a better way. Current syntax DSL too simple for this case...
        return 0

    completers['get_directory_property'] = MultiSignature(
        get_directory_property_sig_dispatch
      , get_directory_property_sig_1
      , get_directory_property_sig_2
      )

    completers['get_filename_component'] = [
        Value([(ANY, 1), (FILE, 1)])
      , OneOf(
            Option('PATH')
          , Option('ABSOLUTE')
          , Option('NAME')
          , Option('EXT')
          , Option('NAME_WE')
          , Option('RELPATH')
          , Option('PROGRAM', 1, [(ANY, ONE_OR_MORE)])
          )
      , Option('CACHE', ZERO_OR_ONE)
      ]

    completers['get_property'] = [
        Value([(ANY, 1)])
      , OneOf(
            Option('GLOBAL')
          , Option('DIRECTORY', 1, [(DIR, 1)])
          , Option('TARGET', 1, [(ANY, 1)])
          , Option('SOURCE', 1, [(ANY, 1)])
          , Option('TEST', 1, [(ANY, 1)])
          , Option('CACHE', 1, [(ANY, 1)])
          , Option('VARIABLE')
          )
      , Option('PROPERTY', 1, [(PROPERTY, 1)])
      , OneOf(
            Option('SET')
          , Option('DEFINED')
          , Option('BRIEF_DOCS')
          , Option('FULL_DOCS')
          )
      ]

    completers['get_source_file_property'] = [
        Value([(ANY, 1), (FILE, 1), (PROPERTY, 1)])
      ]

    completers['get_target_property'] = [
        Value([(ANY, 2), (PROPERTY, 1)])
      ]

    completers['get_test_property'] = [
        Value([(ANY, 2), (PROPERTY, 1)])
      ]

    # NOTE if() implemented as a separate module

    # TODO Better to implement include() as a separate module
    completers['include'] = [
        Value([(MODULE, 1)])
      , Option('OPTIONAL', ZERO_OR_ONE)
      , Option('RESULT_VARIABLE', ZERO_OR_ONE, [(ANY, 1)])
      , Option('NO_POLICY_SCOPE', ZERO_OR_ONE)
      ]

    completers['include_directories'] = [
        OneOf(Option('AFTER'), Option('BEFORE'))
      , Option('SYSTEM', ZERO_OR_ONE)
      , Value([(DIR, ONE_OR_MORE)])
      ]

    # TODO include_external_msproject

    completers['include_regular_expression'] = [Value([(ANY, 2)])]

    # TODO Need a better way. Current syntax DSL too simple for this case...
    install_sig_1 = [
        Option('TARGETS', 1, [(ANY, ONE_OR_MORE)])
      , Value([(ONE_OF, [
            'ARCHIVE'
          , 'LIBRARY'
          , 'RUNTIME'
          , 'FRAMEWORK'
          , 'BUNDLE'
          , 'PRIVATE_HEADER'
          , 'PUBLIC_HEADER'
          , 'RESOURCE'
          ])])
      , Option('DESTINATION', ZERO_OR_ONE, [(DIR, 1)])
      , Option('PERMISSIONS', ZERO_OR_ONE, [(ANY, 1)])
      , Option('CONFIGURATIONS', ZERO_OR_ONE, [(ANY, 1)])   # TODO Try to complete configurations?
      , Option('COMPONENT', ZERO_OR_ONE, [(ANY, 1)])
      , Option('OPTIONAL', ZERO_OR_ONE)
      , OneOf(Option('NAMELINK_ONLY', ZERO_OR_ONE), Option('NAMELINK_SKIP', ZERO_OR_ONE))
      ]
    install_sig_2 = [
        Option('FILES', 1, [(FILE, ONE_OR_MORE)])
      , Option('DESTINATION', 1, [(DIR, 1)])
      , Option('PERMISSIONS', ZERO_OR_ONE, [(ANY, 1)])
      , Option('CONFIGURATIONS', ZERO_OR_ONE, [(ANY, 1)])   # TODO Try to complete configurations?
      , Option('COMPONENT', ZERO_OR_ONE, [(ANY, 1)])
      , Option('RENAME', ZERO_OR_ONE, [(ANY, 1)])
      , Option('OPTIONAL', ZERO_OR_ONE)
      ]
    install_sig_3 = [
        Option('PROGRAMS', 1, [(FILE, ONE_OR_MORE)])
      , Option('DESTINATION', 1, [(DIR, 1)])
      , Option('PERMISSIONS', ZERO_OR_ONE, [(ANY, 1)])
      , Option('CONFIGURATIONS', ZERO_OR_ONE, [(ANY, 1)])   # TODO Try to complete configurations?
      , Option('COMPONENT', ZERO_OR_ONE, [(ANY, 1)])
      , Option('RENAME', ZERO_OR_ONE, [(ANY, 1)])
      , Option('OPTIONAL', ZERO_OR_ONE)
      ]
    install_sig_4 = [
        Option('DIRECTORY', 1, [(FILE, ONE_OR_MORE)])
      , Option('DESTINATION', 1, [(DIR, 1)])
      , Option('FILE_PERMISSIONS', ZERO_OR_ONE, [(ANY, 1)])
      , Option('DIRECTORY_PERMISSIONS', ZERO_OR_ONE, [(ANY, 1)])
      , Option('USE_SOURCE_PERMISSIONS', ZERO_OR_ONE)
      , Option('OPTIONAL', ZERO_OR_ONE)
      , Option('CONFIGURATIONS', ZERO_OR_ONE, [(ANY, 1)])   # TODO Try to complete configurations?
      , Option('COMPONENT', ZERO_OR_ONE, [(ANY, 1)])
      , Option('FILES_MATCHING', ZERO_OR_ONE)
      # TODO If 'PATTERN' option used, the rest options shouldn't appear,
      # but DSL nowadays is not flexible enough to express that ;-(
      # It is why it would be better to implement this completer as
      # a separate module.
      , Option('PATTERN', ZERO_OR_MORE, [(ANY, 1)])
      , Option('REGEX', ZERO_OR_ONE, [(ANY, 1)])
      , Option('EXCLUDE', ZERO_OR_ONE)
      , Option('PERMISSIONS', ZERO_OR_ONE, [(ANY, 1)])
      ]
    install_sig_5 = [
        Option('EXPORT', 1, [(FILE, 1)])
      , Option('DESTINATION', 1, [(DIR, 1)])
      , Option('NAMESPACE', ZERO_OR_ONE, [(ANY, 1)])
      , Option('FILE', ZERO_OR_ONE, [(FILE, 1)])
      , Option('PERMISSIONS', ZERO_OR_ONE, [(ANY, 1)])
      , Option('CONFIGURATIONS', ZERO_OR_ONE, [(ANY, 1)])   # TODO Try to complete configurations?
      , Option('COMPONENT', ZERO_OR_ONE, [(ANY, 1)])
      ]
    install_sig_6 = [
        Option('SCRIPT', ZERO_OR_MORE, [(FILE, 1)])
      , Option('CODE', ZERO_OR_MORE, [(ANY, 1)])
      ]

    _install_signatures = ['TARGETS', 'FILES', 'PROGRAMS', 'DIRECTORY', 'EXPORT']
    def install_sig_dispatch(comp_list):
        if comp_list:
            if comp_list[0] in _install_signatures:
                return _install_signatures.index(comp_list[0])
            if comp_list[0] == 'CODE' or comp_list[0] == 'SCRIPT':
                return 5
        return _install_signatures + ['CODE', 'SCRIPT']

    completers['install'] = MultiSignature(
        install_sig_dispatch
      , install_sig_1
      , install_sig_2
      , install_sig_3
      , install_sig_4
      , install_sig_5
      , install_sig_6
      )

    completers['link_directories'] = [Value([(DIR, ONE_OR_MORE)])]

    completers['list'] = [
        OneOf(
            Option('LENGTH', 1, [(ANY, 2)])
          , Option('GET', 1, [(ANY, ONE_OR_MORE)])
          , Option('APPEND', 1, [(ANY, ONE_OR_MORE)])
          , Option('FIND', 1, [(ANY, ONE_OR_MORE)])
          , Option('INSERT', 1, [(ANY, ONE_OR_MORE)])
          , Option('REMOVE_ITEM', 1, [(ANY, ONE_OR_MORE)])
          , Option('REMOVE_AT', 1, [(ANY, ONE_OR_MORE)])
          , Option('REMOVE_DUPLICATES', 1, [(ANY, ONE_OR_MORE)])
          , Option('REVERSE', 1, [(ANY, ONE_OR_MORE)])
          , Option('SORT', 1, [(ANY, ONE_OR_MORE)])
          )
      ]

    # TODO load_cache

    completers['load_command'] = [Value([(ANY, 1), (DIR, ONE_OR_MORE)])]

    completers['macro'] = [Value([(ANY, ONE_OR_MORE)])]

    completers['mark_as_advanced'] = [
        OneOf(Option('CLEAR'), Option('FORCE'))
      , Value([(ANY, ONE_OR_MORE)])
      ]

    completers['math'] = [
        Option('EXPR', 1, [(ANY, 1), (STRING, 1)])
      ]


    completers['message'] = [
        OneOf(
            Option('STATUS', ZERO_OR_ONE)
          , Option('WARNING', ZERO_OR_ONE)
          , Option('AUTHOR_WARNING', ZERO_OR_ONE)
          , Option('FATAL_ERROR', ZERO_OR_ONE)
          , Option('SEND_ERROR', ZERO_OR_ONE)
          )
      , Value([(STRING, ONE_OR_MORE)])
      ]

    completers['option'] = [Value([(ANY, 1), (STRING, 1), (ANY, 1)])]

    completers['project'] = [Value([(ANY, 1), (ANY, ONE_OR_MORE)])]

    completers['qt_wrap_cpp'] = [Value([(ANY, 1), (ANY, 1), (ANY, ONE_OR_MORE)])]

    completers['qt_wrap_ui'] = [Value([(ANY, 1), (ANY, 1), (ANY, ONE_OR_MORE)])]

    completers['remove_definitions'] = [Value([(ANY, ONE_OR_MORE)])]

    # NOTE Nothing to complete for return()

    completers['separate_arguments'] = [
        Value([(ANY, 1)])
      , OneOf(
            Option('UNIX_COMMAND', 1, [(STRING, 1)])
          , Option('WINDOWS_COMMAND', 1, [(STRING, 1)])
          )
      ]

    # TODO implement set() as a separate module

    # TODO implement set_directory_properties() as a separate module

    completers['set_property'] = [
        OneOf(
            Option('GLOBAL')
          , Option('DIRECTORY', 1, [(DIR, 1)])
          , Option('TARGET', 1, [(ANY, ONE_OR_MORE)])
          , Option('SOURCE', 1, [(ANY, ONE_OR_MORE)])
          , Option('TEST', 1, [(ANY, ONE_OR_MORE)])
          , Option('CACHE', 1, [(ANY, ONE_OR_MORE)])
          )
      , Option('APPEND', ZERO_OR_ONE)
      , Option('APPEND_STRING', ZERO_OR_ONE)
      , Option('PROPERTY', 1, [(ANY, ONE_OR_MORE)])
      ]

    # TODO implement set_source_file_properties() as a separate module

    # TODO implement set_target_properties() as a separate module

    # TODO implement set_test_properties() as a separate module

    completers['site_name'] = [Value([(ANY, 1)])]

    completers['source_group'] = [
        Value([(ANY, 1)])
      , Option('REGULAR_EXPRESSION', 1, [(ANY, 1)])
      , Option('FILES', 1, [(FILE, ONE_OR_MORE)])
      ]

    # TODO implement string() as a separate module due complexity
    string_sig_1 = [
        Option('REGEX')
      , OneOf(Option('MATCH'), Option('MATCHALL'), Option('REPLACE'))
      , Value([(ANY, 1), (ANY, 1), (ANY, ONE_OR_MORE)])
      ]
    string_sig_2 = [
        OneOf(
            Option('MD5')
          , Option('SHA1')
          , Option('SHA224')
          , Option('SHA256')
          , Option('SHA384')
          , Option('SHA512')
          )
      , Value([(ANY, 2)])
      ]
    string_sig_3 = [
        Option('COMPARE')
      , OneOf(Option('EQUAL'), Option('NOTEQUAL'), Option('LESS'), Option('GREATER'))
      , Value([(ANY, 3)])
      ]
    string_sig_4 = [
        Option('ASCII', 1, [(ANY, ONE_OR_MORE)])
      , Value([(ANY, 1)])
      ]
    string_sig_5 = [
        Option('CONFIGURE', 1, [(ANY, 2)])
      , Option('@ONLY', ZERO_OR_MORE)
      , Option('ESCAPE_QUOTES', ZERO_OR_MORE)
      ]
    string_sig_6 = [
        Option('TOUPPER', 1, [(ANY, 2)])
      ]
    string_sig_7 = [
        Option('TOLOWER', 1, [(ANY, 2)])
      ]
    string_sig_8 = [
        Option('LENGTH', 1, [(ANY, 2)])
      ]
    string_sig_9 = [
        Option('SUBSTRING', 1, [(ANY, 4)])
      ]
    string_sig_10 = [
        Option('STRIP', 1, [(ANY, 2)])
      ]
    string_sig_11 = [
        Option('RANDOM')
      , Option('LENGTH', ZERO_OR_ONE, [(ANY, 1)])
      , Option('ALPHABET', ZERO_OR_ONE, [(ANY, 1)])
      , Option('RANDOM_SEED', ZERO_OR_ONE, [(ANY, 1)])
      , Value([(ANY, 1)])
      ]
    string_sig_12 = [
        Option('FIND', 1, [(ANY, 3)])
      , Option('REVERSE', ZERO_OR_ONE)
      ]
    string_sig_13 = [
        Option('TIMESTAMP', 1, [(ANY, ONE_OR_MORE)])
      , Option('UTC', ZERO_OR_ONE)
      ]
    _string_subcommands = [
        'REGEX', 'REPLACE', 'MD5', 'COMPARE', 'ASCII', 'CONFIGURE', 'TOUPPER', 'TOLOWER', 'LENGTH'
      , 'SUBSTRING', 'STRIP', 'RANDOM', 'FIND', 'TIMESTAMP'
      ]
    _string_digest_subcommands = ['MD5', 'SHA1', 'SHA224', 'SHA256', 'SHA384', 'SHA512']

    def string_sig_dispatch(comp_list):
        if comp_list and comp_list[0] in _string_subcommands:
            return _string_subcommands.index(comp_list[0])
        if comp_list and comp_list[0] in _string_digest_subcommands:
            return 3
        if not comp_list or len(comp_list) == 1:
            return _string_subcommands + _string_digest_subcommands
        return None

    completers['string'] = MultiSignature(
        string_sig_dispatch
      , string_sig_1, string_sig_2,  string_sig_3,  string_sig_4
      , string_sig_5, string_sig_6,  string_sig_7,  string_sig_8
      , string_sig_9, string_sig_10, string_sig_11, string_sig_12
      , string_sig_13
      )

    # TODO implement target_compile_definitions() as a separate module

    # TODO implement target_compile_options() as a separate module

    # TODO implement target_include_directories() as a separate module

    # TODO implement target_link_libraries() as a separate module

    try_compile_sig_1 = [
        Value([(ANY, 1), (DIR, 2), (ANY, 1), (ANY, ZERO_OR_ONE)])
      , Option('CMAKE_FLAGS', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('OUTPUT_VARIABLE', ZERO_OR_ONE, [(ANY, 1)])
      ]
    try_compile_sig_2 = [
        Value([(ANY, 1), (DIR, 2)])
      , Option('CMAKE_FLAGS', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('COMPILE_DEFINITIONS', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('LINK_LIBRARIES', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('COPY_FILE', ZERO_OR_ONE, [(FILE, 1)])
      , Option('OUTPUT_VARIABLE', ZERO_OR_ONE, [(ANY, 1)])
      ]

    def try_compile_sig_dispatch(comp_list):
        if {'COMPILE_DEFINITIONS', 'LINK_LIBRARIES', 'COPY_FILE'}.intersection(set(comp_list)):
            return 1
        return None

    completers['try_compile'] = MultiSignature(
        try_compile_sig_dispatch
      , try_compile_sig_1
      , try_compile_sig_2
      )

    completers['try_run'] = [
        Value([(ANY, 2), (DIR, 1), (FILE, 1)])
      , Option('CMAKE_FLAGS', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('COMPILE_DEFINITIONS', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      , Option('COMPILE_OUTPUT_VARIABLE', ZERO_OR_ONE, [(ANY, 1)])
      , Option('RUN_OUTPUT_VARIABLE', ZERO_OR_ONE, [(ANY, 1)])
      , Option('OUTPUT_VARIABLE', ZERO_OR_ONE, [(ANY, 1)])
      , Option('ARGS', ZERO_OR_ONE, [(ANY, ONE_OR_MORE)])
      ]

    completers['unset'] = [
        Value([(ANY, 1)])
      , Option('CACHE', ZERO_OR_ONE)
      ]

    completers['variable_watch'] = [Value([(ANY, ONE_OR_MORE)])]

    # NOTE while() implemented as a separate module
