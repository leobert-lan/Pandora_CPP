# Pandora-CPP

Pandora-CPP is a C++ library for efficient management of complex data structures and multi-view type support.

The Java/Kotlin version of Pandora can be found [here](https://github.com/leobert-lan/Pandora), provide for Android.

## Features
- Complex data structure management
- Multi-view type support
- Type visitor pattern
- Exception handling
- Logging

## Getting Started

### Prerequisites
- C++17 or later
- CMake 3.10+
- GoogleTest (for unit tests)

### Build
```sh
mkdir build
cd build
cmake ..
cmake --build .
```

### Run Tests
```sh
ctest
```
Or run the test executable directly:
```sh
./pandora_tests.exe
```

## Directory Structure
- `pandora/include/pandora/` : Core library headers
- `pandora/tests/` : Unit tests
- `sample/` : Sample usage (if available)

## Example Usage
```cpp
#include <pandora/real_data_set.h>
#include <pandora/wrapper_data_set.h>
// ...
```

## License
This project is licensed under the MIT License.

