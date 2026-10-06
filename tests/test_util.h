#pragma once
// tests/test_util.h — portable helpers for tests (CI runs Linux + Windows).
#include <cstdlib>

namespace mojave_test {

inline void SetEnv(const char* key, const char* value) {
#ifdef _WIN32
    _putenv_s(key, value);
#else
    setenv(key, value, 1);
#endif
}

} // namespace mojave_test
