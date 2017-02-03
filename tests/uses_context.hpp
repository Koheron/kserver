#ifndef __USES_CONTEXT_HPP__
#define __USES_CONTEXT_HPP__

#include "context.hpp"

#include "tests.hpp"
#include "benchmarks.hpp"
#include "exception_tests.hpp"

class UsesContext
{
  public:
    UsesContext(Context& ctx_)
    : ctx(ctx_)
    , tests(ctx.get<Tests>())
    , benchmarks(ctx.get<Benchmarks>())
    , exception_tests(ctx.get<ExceptionTests>())
    {}

    bool set_float_from_tests(float f) {
        return tests.set_float(f);
    }

    bool is_myself() {
        return &ctx.get<UsesContext>() == this;
    }

    void log(const std::string& msg) {
        ctx.log<INFO>(msg.c_str());
    }

    void notify(const std::string& msg) {
        ctx.notify<UsesContext>(msg.c_str());
    }

  private:
    Context& ctx;

    Tests& tests;
    Benchmarks& benchmarks;
    ExceptionTests& exception_tests;
};

#endif // __USES_CONTEXT_HPP__