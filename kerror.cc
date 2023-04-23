// SPDX-LICENSE-IDENTIFIER: MIT
#include "kerror.h"

#include <cstring>

using namespace kerror;

auto kerror::MakeMsgErrorf(char const *fmt, ...) -> Error
{
  char buf[4096];

  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof buf, fmt, args);
  va_end(args);

  return MakeMsgError(std::string(buf));
}

auto kerror::Panic(char const *msg) noexcept -> void
{
  fputs(msg, stderr);
  fflush(stderr);
  abort();
}

auto kerror::Panicf(char const *fmt, ...) noexcept -> void
{
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, fmt, args);
  va_end(args);

  fflush(stderr);
  abort();
}

void kerror::PError(char const *prefix, Error const &err) noexcept
{
  fprintf(stderr, "%s%s\n", prefix, err.info()->GetMessage().c_str());
}

void kerror::PErrorSys(char const *prefix, char const *sys_prefix,
                       Error const &err) noexcept
{
  auto saved_errno = errno;
  PError(prefix, err);

  char error_buf[4096];
  error_buf[0] = 0;
  const auto error_msg = strerror_r(saved_errno, error_buf, sizeof error_buf);
  fprintf(stderr, "%s: %s(%d)\n", sys_prefix, error_msg, saved_errno);
  errno = 0;
}

void kerror::PSysError(char const *msg) noexcept
{
  auto saved_errno = errno;
  char error_buf[4096];
  error_buf[0] = 0;
  const auto error_msg = strerror_r(saved_errno, error_buf, sizeof error_buf);
  fprintf(stderr, "%s\nSysError: %s(%d)\n", msg, error_msg, saved_errno);
}

void kerror::PSysErrorf(char const *fmt, ...) noexcept
{
  char buf[4096];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof buf, fmt, args);
  va_end(args);

  PSysError(buf);
}
