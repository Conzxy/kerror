# kerror

## Introduction
由于本人相比异常更偏向错误码（类似libc的errno），但是又因为错误码并不携带上下文信息并且可以被忽视，这些都导致错误码很难被方便地使用。  
受[LLVM错误处理](https://llvm.org/docs/ProgrammersManual.html#error-handling)的一些启发，编写这个单独的模块方便其他项目引入。

错误一般分为两种：

* `可恢复错误`（recoverable error）
* `严重错误`（fatal error）

这两种错误都会得到处理。

> 由于是单源文件，因此很容易会引入新项目，也因此并没有提供任何编译脚本。

## Usage
### Error
`Error`类是一个可携带错误上下文信息的类，如果没有携带任何信息，则视为没有错误。
```cpp
struct PathErrorInfo : IErrorInfo {
  std::string path;
  
  explicit PathErrorInfo(std::string path_) : path(std::move(path_)) {}

  std::string GetMessage() const noexcept {
    return std::string("Failed to open ") + path;
  }
};

Error f() {
  [..]
  if (...)
    return MakeError<PathErrorInfo>("xx");
  [..]

  return MakeSuccess();
}
```
当然，上面只是举个例子，你也可以通过 `MsgErrorInfo`以及一些方法来达到类似的效果：
```cpp
auto err = MakeMsgErrorf("Faield to open %s", "xx");
// 还有：
// MakeMsgError(message)
```
总之，给用户自定义的空间可以实现更复杂的上下文信息，比如backtrace等。

#### 强制检查
注意，`Error` 是要求用户进行检查的，主要有两个场合会认为你进行了检查：
* 转换为bool且为success。eg. if (err = ...) 
* 通过 `info()` 获取上下文信息类

> 注意：转化为bool时，`true` 表示有错误发生

如没进行检查，则终止当前的程序（类似异常没有被正确处理）。

```cpp
if (auto err = f()) {
  auto info = err.info();
  [...]
}
``` 当然，如果你认为忽视它也是OK的话，可以禁用强制检查： ```cpp
error.IgnoreCheck(); // Don't check it is OK.
```

### ErrorOr\<T>
`ErrorOr<T>`理念类似 `option<T>`，只不过检测信息是 `Error` 而不是 `bool`。  
你不用担心它会占用 `sizeof(T) + sizeof(Error)` 的空间，因为我是通过 `union` 实现的。

这个主要用于一些返回值是对象的同时可能出错，比如构造函数：
```cpp
// A.h
struct A {
  static ErrorOr<T> Create() {
    Error err;

    A a(&err);
    if (err) {
      return err;
    }
    return a;
  }

 private:
  A(Error *error)
  {
    if (...) {
      *error = MakeError<...>(...);
      return;
    }
  }
}

// main.cc
int main()
{
  auto aorerr = A::Create();
  if (aorerror) {
    error_handler();
    return -1; // or other
  }

  auto &a = *aorerr; // use this as A object
}
```

### Panic
`Panic` 是打印log和 `abort()` 的 wrapper，主要是为了方便。  
主要用于不可恢复错误。
```cpp
if (...) {
  Panicf("Can't recover from ...", ...);
}
```
