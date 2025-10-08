#include "main.h"
#include "set.h"

// This is a weak implementation of setCredentials.
// It will be overridden by the one in set.cpp if it exists.
// This allows the project to be compiled without having a set.cpp file with secrets.
__attribute__((weak)) void setCredentials() {
    // Intentionally left empty.
    // The user should create a src/set.cpp file with the actual credentials.
    // The setup_env.sh script can be used to create a template.
}