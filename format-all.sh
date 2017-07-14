#!/bin/bash
find . -maxdepth 10 -name "*.c" | xargs clang-format -i -sort-includes
find . -maxdepth 10 -name "*.h" | xargs clang-format -i -sort-includes
