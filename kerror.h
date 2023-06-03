// SPDX-LICENSE-IDENTIFIER: MIT
//
// Reference some code about llvm Error
// \see https://llvm.org/doxygen/llvm_2Support_2Error_8h_source.html
// \see https://llvm.org/docs/ProgrammersManual.html#error-handling

#ifndef _KERROR_H__
#define _KERROR_H__

#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include "macro.h"

namespace kerror {

struct StringSlice {
 public:
  StringSlice(char const *data) noexcept
    : data_(data)
    , len_(strlen(data))
  {
  }

  StringSlice(std::string const &data) noexcept
    : data_(data.data())
    , len_(data.size())
  {
  }

  StringSlice(char const *data, size_t n) noexcept
    : data_(data)
    , len_(n)
  {
  }

  char const *data() const noexcept { return data_; }
  size_t size() const noexcept { return len_; }

 private:
  char const *data_;
  size_t len_;
};

class IErrorInfo {
 public:
  IErrorInfo() = default;
  virtual ~IErrorInfo() = default;

  virtual std::string GetMessage() const { return {}; }
};

enum class IsErrorFlag {
  OFF = 0,
  ON = 1,
};

/**
 * Error with error code and information(i.e. typed errno)
 */
class Error {
 public:
  /**
   * You should not call this function to create Error instead of calling
   * MakeError().
   *
   * \Param error The error code you defined or libc provide
   * \Param info Provide some information to debug or trace, etc.
   */
  explicit Error(std::unique_ptr<IErrorInfo> info)
    : info_(std::move(info))
    , checked_(false)
    , is_error_(true)
  {
  }

  /**
   * Used for implementing MakeNoInfoError() and MakeSuccess()
   *
   * \param is_error
   */
  Error(IsErrorFlag is_error = IsErrorFlag::OFF) noexcept
    : info_(nullptr)
    , checked_(false)
    , is_error_(IsErrorFlag::ON == is_error)
  {
  }

  ~Error() noexcept { AbortIsChecked(); }

  /**
   * Make the other be checked to avoid abort
   */
  Error(Error &&other) noexcept
    : checked_(true) // Satisfy the move assign precondition
    , is_error_(other.is_error_)
  {
    // invariant: this != &other
    *this = std::move(other);
  }

  Error &operator=(Error &&other) noexcept
  {
    if (&other != this) {
      // pre: The error has checked
      AbortIsChecked();
      checked_ = false;
      is_error_ = other.is_error_;
      info_ = std::move(other.info_);

      // avoid abort
      other.checked_ = true;
    }
    return *this;
  }

  /**
   * \brief Ignore the error check(ie. Disable the forced error check)
   */
  void IgnoreCheck() noexcept { checked_ = true; }

  operator bool() const noexcept
  {
    // If this is a success, user can don't extract the errno.
    // otherwise, caller must call error_no() to check and handle error
    // or call info() to get the information if caller don't handle it.
    checked_ = (is_success());
    return is_error();
  }

  IErrorInfo *info() const noexcept
  {
    checked_ = true;
    return info_.get();
  }

  bool is_success() const noexcept { return !is_error_; }
  bool is_error() const noexcept { return is_error_; }

 private:
  void AbortIsChecked() noexcept
  {
    if (KERROR_UNLIKELY(!checked_ && is_error())) {
      fprintf(stderr, "The error is don't checked by user");
      fflush(stderr);
      abort();
    }
  }

 protected:
  std::unique_ptr<IErrorInfo> info_;
  mutable bool checked_;
  bool is_error_;
};

template <typename T, typename... Args>
Error MakeError(Args &&...args)
{
  return Error(std::unique_ptr<T>(std::forward<Args>(args)...));
}

/**
 * In some cases, no info error as indicator is useful,
 * such error can don't bring information.
 */
KERROR_INLINE Error MakeNoInfoError() noexcept
{
  return Error(IsErrorFlag::ON);
}

KERROR_INLINE Error MakeSuccess() noexcept { return Error(IsErrorFlag::OFF); }

class MsgErrorInfo : public IErrorInfo {
 public:
  MsgErrorInfo(char const *str)
    : IErrorInfo()
    , msg_(str, strlen(str))
  {
  }

  MsgErrorInfo(std::string str)
    : IErrorInfo()
    , msg_(std::move(str))
  {
  }

  MsgErrorInfo(MsgErrorInfo &&) = default;
  MsgErrorInfo(MsgErrorInfo const &) = default;

  MsgErrorInfo &operator=(MsgErrorInfo &&) = default;
  MsgErrorInfo &operator=(MsgErrorInfo const &) = default;

  /**
   * \warning
   *   This function should be called only once.
   *   The message is moved!
   * \return
   */
  std::string GetMessage() const override
  {
    return std::move(const_cast<MsgErrorInfo *>(this)->msg_);
  }

 private:
  std::string msg_;
};

Error MakeMsgErrorf(char const *fmt, ...);

KERROR_INLINE Error MakeMsgError(char const *msg)
{
  return Error(std::unique_ptr<IErrorInfo>(new MsgErrorInfo(msg)));
}
KERROR_INLINE Error MakeMsgError(std::string msg)
{
  return Error(std::unique_ptr<IErrorInfo>(new MsgErrorInfo(std::move(msg))));
}

namespace detail {

template <typename T,
          typename std::enable_if<!std::is_trivially_destructible<T>::value,
                                  int>::type = 0>
void destroy_(T &obj)
{
  obj.~T();
}

template <typename T, typename = typename std::enable_if<
                          std::is_trivially_destructible<T>::value>::type>
void destroy_(T &obj) noexcept
{
}

} // namespace detail

template <typename T>
struct ErrorOr {
 public:
  ErrorOr(Error error)
    : error_(std::move(error))
  {
    is_error_ = true;
  }

  ErrorOr(T obj)
    : obj_(std::move(obj))
  {
    is_error_ = false;
  }

  ~ErrorOr()
  {
    if (is_error_) {
      detail::destroy_(error_);
    } else {
      detail::destroy_(obj_);
    }
  }

  ErrorOr(ErrorOr const &other)
    : is_error_(other.is_error_)
  {
    if (other.is_error) {
      new (&obj_) T(other.obj_);
    } else {
      new (&error_) Error(other.error_);
    }
  }

  ErrorOr(ErrorOr &&other) noexcept(noexcept(std::move(other.obj_)))
    : is_error_(other.is_error_)
  {
    if (other.is_error_) {
      new (&obj_) T(std::move(other.obj_));
    } else {
      new (&error_) Error(std::move(other.error_));
    }
  }

  ErrorOr &operator=(ErrorOr const &other)
  {
    if (&other == this) return *this;

    if (other.is_error_ == is_error_) {
      if (is_error_) {
        error_ = other.error_;
      } else {
        obj_ = other.obj_;
      }
    } else {
      is_error_ = !other.is_error_;
      if (other.is_error_) {
        error_ = other.error_;
      } else {
        obj_ = other.obj;
      }
    }
    return *this;
  }

  ErrorOr &operator=(ErrorOr &&other)
  {
    if (&other == this) return *this;

    if (other.is_error_ == is_error_) {
      if (is_error_) {
        error_ = std::move(other.error_);
      } else {
        obj_ = std::move(other.obj_);
      }
    } else {
      is_error_ = !other.is_error_;
      if (other.is_error_) {
        error_ = std::move(other.error_);
      } else {
        obj_ = std::move(other.obj);
      }
    }
    return *this;
  }

  operator bool() const noexcept { return is_error_ && !error_.is_success(); }

  IErrorInfo *info() const noexcept { return error_.info(); }

  T &operator*() noexcept { return obj_; }

  T const &operator*() const noexcept
  {
    return *const_cast<ErrorOr<T> *>(this);
  }

  T *operator->() noexcept { return &obj_; }
  T const *operator->() const noexcept { return &obj_; }

  Error const &error() const noexcept { return error_; }
  Error &error() noexcept { return error_; }

 private:
  union {
    Error error_;
    T obj_;
  };

  bool is_error_;
};

/**
 * \brief Print a message to stderr and abort program
 *
 * Used for handle Fatal error that Cannot be recoverd.
 *
 * \Param msg A descrition for fatal error
 */
void Panic(char const *msg) noexcept;

/**
 * \brief Like Panic() but support C style format string
 *
 * \Param fmt A string contains formatted sign
 * \Param ... Arguments to fill the @p fmt
 */
void Panicf(char const *fmt, ...) noexcept;

void PError(char const *prefix, Error const &err) noexcept;

KERROR_INLINE void PError(Error const &err) noexcept { PError("Reason", err); }

void PErrorSys(char const *prefix, char const *sys_prefix,
               Error const &err) noexcept;

KERROR_INLINE void PErrorSys(Error const &err) noexcept
{
  PErrorSys("Reason", "SysReason", err);
}

void PSysError(char const *msg) noexcept;
void PSysErrorf(char const *fmt, ...) noexcept;

} // namespace kerror

#endif
