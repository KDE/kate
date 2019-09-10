exec find . -name "*.cpp" -or -name "*.h" -exec clang-format -i {} \;
