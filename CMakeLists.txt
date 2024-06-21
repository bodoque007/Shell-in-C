
# Minimum version of CMake required
cmake_minimum_required(VERSION 3.10)


project(shell C)

# Add the executable
add_executable(shell main.c builtins.c)
