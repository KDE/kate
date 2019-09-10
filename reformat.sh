# reformat the code
find . -name "*.cpp" -or -name "*.h" -exec clang-format -i {} \;

# check in the result
git commit -a -m "GIT_SILENT: application of coding style"
