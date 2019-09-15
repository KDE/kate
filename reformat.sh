# reformat the code
# no operator is implicit -a with higher precedence than -o
find . \( -name "*.cpp" -or -name "*.h" \) -exec clang-format -i {} \;

# check in the result
git commit -a -m "GIT_SILENT: application of coding style"
