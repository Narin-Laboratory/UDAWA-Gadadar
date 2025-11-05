#!/bin/bash
set -e

# This script sets up the development environment.

# Copy platformio.ini from sample
if [ ! -f platformio.ini ]; then
    cp platformio.ini.sample platformio.ini
    echo "Created platformio.ini"
else
    echo "platformio.ini already exists."
fi

# Copy secret.h from sample
if [ ! -f include/secret.h ]; then
    cp include/secret.h-example include/secret.h
    echo "Created include/secret.h"
else
    echo "include/secret.h already exists."
fi

# Copy set.cpp from sample
if [ ! -f src/set.cpp ]; then
    cp src/set.cpp-example src/set.cpp
    echo "Created src/set.cpp"
else
    echo "src/set.cpp already exists."
fi

echo "Environment setup complete."