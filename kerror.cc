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

auto kerror::Panic(char const *msg) -> void
{
  fputs(msg, stderr);
  fflush(stderr);
  abort();
}

auto kerror::Panicf(char const *fmt, ...) -> void
{
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, fmt, args);
  va_end(args);

  fflush(stderr);
  abort();
}
