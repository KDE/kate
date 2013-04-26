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

from param_types import *

def register_command_completer(completers):
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

    completers['add_executable'] = [
        Value([(IDENTIFIER, 1)])
      , Option('WIN32', ZERO_OR_ONE, description='The property WIN32_EXECUTABLE will be set on the target')
      , Option('MACOSX_BUNDLE', ZERO_OR_ONE)
      , Option('EXCLUDE_FROM_ALL', ZERO_OR_ONE)
      , Value([(FILE, ONE_OR_MORE)])
      ]

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

    # NOTE Nothing to complete for else(), elseif()

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

    # TODO Make completer for file() as a separate module (too complex for this primitive syntax)

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

    # TODO implement for() as a separate module due complexity

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

    # TODO implement if() as a separate module due complexity

    # TODO Better to implement include() as a separate module
    completers['include'] = [
        Value([(FILE, 1)])
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

    # TODO implement if() as a separate module due complexity

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

    # TODO implement target_compile_definitions() as a separate module

    # TODO implement target_include_directories() as a separate module

    # TODO implement target_link_libraries() as a separate module

    # TODO implement target_link_libraries() as a separate module

    # TODO implement try_compile() as a separate module

    completers['try_run'] = [
        Option('RUN_RESULT_VAR', 1, [(ANY, 1)])
      , Value([(DIR, 1), (FILE, 1)])
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

    # NOTE Nothing to complete for while()

# kate: indent-width 4;
