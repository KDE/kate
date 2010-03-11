# test executable for encoding
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
kde4_add_executable(kateencodingtest NOGUI kateencodingtest.cpp)
target_link_libraries(kateencodingtest ${KDE4_KDECORE_LIBS} ${QT_QTSCRIPT_LIBRARY} ${QT_QTTEST_LIBRARY} kutils ktexteditor katepartinterfaces )

# test macro for encoding tests
MACRO(KATE_ENCODING_TEST _encoding _testname)
  ADD_TEST (${_testname}_create kateencodingtest ${_encoding} ${CMAKE_CURRENT_SOURCE_DIR}/${_testname} ${CMAKE_CURRENT_BINARY_DIR}/${_testname} )
  ADD_TEST (${_testname}_diff ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/${_testname} ${CMAKE_CURRENT_BINARY_DIR}/${_testname} )
ENDMACRO(KATE_ENCODING_TEST)

# add tests

# this file is utf-8, simple
KATE_ENCODING_TEST ("utf-8" "utf8.txt")

# this file is latin15, but fallback should work!
KATE_ENCODING_TEST ("utf-8" "latin15.txt")

# this file is utf32, little endian, but fallback should work!
KATE_ENCODING_TEST ("utf-8" "utf32.txt")

# this file is utf16, little endian, but fallback should work!
KATE_ENCODING_TEST ("utf-8" "utf16.txt")

# this file is utf32, big endian, but fallback should work!
KATE_ENCODING_TEST ("utf-8" "utf32be.txt")

# this file is utf16, big endian, but fallback should work!
KATE_ENCODING_TEST ("utf-8" "utf16be.txt")
