#pragma once

#pragma GCC diagnostic ignored "-Wformat-security"

#include <string>
#include <mutex>

#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>

#define FMT_HEADER_ONLY 1
#include "fmt/format.h"
#include "fmt/compile.h"



namespace plate
{

//#define log_user(args...) logx::user_fmt(args)
#define log_user(...)

#define log_error(args...) plate::logx::error_fmt(args)
//#define log_error(...)

//#define log_info(args...) logx::info_fmt(args)
#define log_info(...)

#define log_debug(args...) plate::logx::debug_fmt(args)
//#define log_debug(...) 

//#define log_trace(args...) logx::trace_fmt(args);
#define log_trace(...)


class logx
{

public:

  static const bool ENABLE_ERROR_LOGGING = true;
  static const bool ENABLE_DEBUG_LOGGING = true;


  static void log(const char* c, const char* s)
  {
    std::lock_guard<std::mutex> lock(mutex_);

    timeval t;
    gettimeofday(&t, NULL);

    char datetime[32];
    std::strftime(datetime, sizeof(datetime), "%y-%m-%d %H:%M:%S", std::localtime(&t.tv_sec));

    std::printf("%s.%ld [%s] (%lu) %s\n", datetime, t.tv_usec, c, pthread_self(), s);
  };


  template<class C, typename... Args> static void debug_fmt(C&& c, const Args... args) noexcept
  {
    if constexpr (ENABLE_DEBUG_LOGGING)
    {
      if constexpr (sizeof...(args) == 0)
        log("debug", c);
      else
        log("debug", fmt::format(std::move(c), args...).c_str());
    }
  };

  template<class C, typename... Args> static void error_fmt(C&& c, const Args... args) noexcept
  {
    if constexpr (ENABLE_ERROR_LOGGING)
    {
      if constexpr (sizeof...(args) == 0)
        log("error", c);
      else
        log("error", fmt::format(std::move(c), args...).c_str());
    }
  };


private:


  inline static std::mutex mutex_;

}; // class log

} // namespace plate

