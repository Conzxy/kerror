// SPDX-LICENSE-IDENTIFIER: MIT
#include "kerror.h"

#include <cstring>

using namespace kerror;

auto kerror::MakeMsgErrorf(error_t error, char const *fmt, ...) -> Error
{
  char buf[4096];

  va_list args;
  vsnprintf(buf, sizeof buf, fmt, args);
  va_start(args, fmt);
  va_end(args);

  return MakeMsgError(error, std::string(buf));
}
