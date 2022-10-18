# sudo apt install clang-format
find ../application/sources/ ../dust3d/ -iname *.h -o -iname *.cc | xargs clang-format -style=webkit -i