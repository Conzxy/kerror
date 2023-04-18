#include "kerror.h"
#include <cerrno>
#include <cstdio>

using namespace kerror;

struct A {
  static ErrorOr<A> Create(int x)
  {
    Error err;
    A a(x, &err);
    if (err) {
      fflush(stdout);
      return std::move(err);
    }
    return std::move(a);
  }

  A(int x, Error *error)
  {
    if (x == 0) {
      *error = MakeMsgError("error");
      return;
    }

    x_ = x;
  }

  int x_;
};

Error f() { return MakeMsgError("out of range"); }

int main()
{
  auto err = MakeSuccess();
  if (err) {
  }

  auto err2 = f();
  if (err2) {
    printf("Error msg: %s\n", err2.info()->GetMessage().data());
  }

  auto err3 = A::Create(0);

  assert(err3);
  if (err3) {
    fprintf(stderr, "%s\n", err3.info()->GetMessage().data());
  }

  auto obj = A::Create(1);
  assert(!obj);

  printf("x = %d\n", obj->x_);
}
