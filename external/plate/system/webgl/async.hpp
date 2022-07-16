#pragma once

#pragma GCC diagnostic ignored "-Wmacro-redefined" // M_PI macro redefined

#include <string.h>

#include <string>
#include <functional>
#include <map>
#include <queue>
#include <thread>
#include <condition_variable>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/fetch.h>
#include <SDL/SDL_image.h>

#include "../log.hpp"
#include "../common/data_store.hpp"


/*

  retrieves data for an app - identified by an id

*/

namespace plate {

class async {

public:


  // NB: header in weird format: head1 \0 value1 \0 \0

  static std::uint32_t fetch_post(const std::string url, std::span<std::byte> post_data,
                         const char* const * headers,
                         std::function<void (std::size_t, data_store&& d, std::uint16_t)> on_load,
                         std::function<void (std::size_t, int)> on_error)
  {
    std::size_t c;
    {
      std::unique_lock<std::mutex> lock(fetch_mutex_);

      c = fetch_counter_++;

      fetch_cbs_[c] = { on_load, on_error };
    }
      
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);

    strcpy(attr.requestMethod, "POST");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.userData   = reinterpret_cast<void*>(c);
    attr.requestHeaders = headers;
    attr.requestData = reinterpret_cast<char*>(post_data.data());
    attr.requestDataSize = post_data.size();

    attr.onsuccess  = [] (emscripten_fetch_t *fetch) -> void
    {
      auto c = reinterpret_cast<std::size_t>(fetch->userData);

      //log_debug(FMT_COMPILE("Finished job: {} downloading {} bytes from URL {}"), c, fetch->numBytes, fetch->url);

      std::function<void (std::size_t, data_store&&, std::uint16_t)> on_load;

      {
        std::unique_lock<std::mutex> lock(fetch_mutex_);

        auto it = fetch_cbs_.find(c);

        if (it != fetch_cbs_.end()) on_load = it->second.on_load;
      }

      if (on_load)
      {
        data_store d(reinterpret_cast<std::byte*>(const_cast<char*>(fetch->data)),
                                         static_cast<std::uint32_t>(fetch->numBytes),
                                         [fetch] () -> void { emscripten_fetch_close(fetch); });

        on_load(c, std::move(d), fetch->status);

        std::unique_lock<std::mutex> lock(fetch_mutex_);

        fetch_cbs_.erase(c);
      }
      else
        emscripten_fetch_close(fetch);
    };

    attr.onerror    = [] (emscripten_fetch_t *fetch) -> void
    {
      auto c = reinterpret_cast<std::size_t>(fetch->userData);

      log_debug(FMT_COMPILE("Failed job: {} to download from URL {} code: {}"), c, fetch->url, fetch->status);

      std::function<void (std::size_t, int)> on_error;

      {
        std::unique_lock<std::mutex> lock(fetch_mutex_);

        auto it = fetch_cbs_.find(c);

        if (it != fetch_cbs_.end()) on_error = it->second.on_error;
      }

      if (on_error)
      {
        on_error(c, fetch->status);

        std::unique_lock<std::mutex> lock(fetch_mutex_);

        fetch_cbs_.erase(c);
      }

      emscripten_fetch_close(fetch);
    };

    emscripten_fetch(&attr, url.c_str());

    return c;
  };


  static std::uint32_t fetch_get(const std::string url,
                         const char* const * headers,
                         std::function<void (std::size_t, data_store&&, std::uint16_t)> on_load,
                         std::function<void (std::size_t, int)> on_error)
  {
    std::size_t c;
    {
      std::unique_lock<std::mutex> lock(fetch_mutex_);

      c = fetch_counter_++;

      fetch_cbs_[c] = { on_load, on_error };
    }
      
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);

    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.userData   = reinterpret_cast<void*>(c);
    attr.requestHeaders = headers;
    attr.withCredentials = true;

    attr.onsuccess  = [] (emscripten_fetch_t *fetch) -> void
    {
      auto c = reinterpret_cast<std::size_t>(fetch->userData);

      //log_debug(FMT_COMPILE("Finished downloading {} bytes from URL {} counter: {}"), fetch->numBytes, fetch->url, c);

      std::function<void (std::size_t, data_store&&, std::uint16_t)> on_load;

      {
        std::unique_lock<std::mutex> lock(fetch_mutex_);

        auto it = fetch_cbs_.find(c);

        if (it != fetch_cbs_.end())
          on_load = it->second.on_load;
      }

      if (on_load)
      {
        data_store d(reinterpret_cast<std::byte*>(const_cast<char*>(fetch->data)),
                                         static_cast<std::uint32_t>(fetch->numBytes),
                                         [fetch] () -> void { emscripten_fetch_close(fetch); });

        on_load(c, std::move(d), fetch->status);

        std::unique_lock<std::mutex> lock(fetch_mutex_);

        fetch_cbs_.erase((std::size_t)(fetch->userData));
      }
      else
        emscripten_fetch_close(fetch);
    };

    attr.onerror    = [] (emscripten_fetch_t *fetch) -> void
    {
      auto c = reinterpret_cast<std::size_t>(fetch->userData);

      log_debug(FMT_COMPILE("Failed to download from URL {} code: {} counter: {}"),
                                                                fetch->url, fetch->status, c);

      std::function<void (std::size_t, int)> on_error;

      {
        std::unique_lock<std::mutex> lock(fetch_mutex_);

        auto it = fetch_cbs_.find((std::size_t)(fetch->userData));
        if (it != fetch_cbs_.end()) on_error = it->second.on_error;
      }

      if (on_error)
      {
        on_error(c, fetch->status);

        std::unique_lock<std::mutex> lock(fetch_mutex_);

        fetch_cbs_.erase((std::size_t)(fetch->userData));
      }

      emscripten_fetch_close(fetch);
    };

    emscripten_fetch(&attr, url.c_str());

    return c;
  };


  // request allows the downloaded data to be decoded by the browser - so useful for images..
  //
  // url - url to issue request to
  // request_type - whether a GET or POST
  // param - args eg: key=value&key2=value2

  static std::uint32_t request(const std::string url, std::string request_type, std::string param,
                     std::function<void (std::uint32_t, data_store&&)> on_load,
                     std::function<void (std::uint32_t, int, std::string)> on_error,
                     std::function<void (std::uint32_t, int, int)> on_progress)
  {
    auto handle = emscripten_async_wget2_data(url.c_str(), request_type.c_str(), param.c_str(), nullptr, EM_FALSE,

      [] (unsigned int handle, [[maybe_unused]] void* user_data, void* data, unsigned int data_size) -> void
      {
        auto it = cbs_.find(handle);
        if (it != cbs_.end())
        {
          data_store d(reinterpret_cast<std::byte*>(data), data_size, data_store::FREE::Yes);

          it->second.on_load(handle, std::move(d));
          cbs_.erase(it);
        }
        else { log_error("Not found async::on_load"); }
      },

      [] (unsigned int handle, [[maybe_unused]] void* user_data, int error_code, const char* error_msg)
      {
        auto it = cbs_.find(handle);
        if (it != cbs_.end())
        {
          if (it->second.on_error) it->second.on_error(handle, error_code, error_msg);
          cbs_.erase(it);
        }
        else { log_error("Not found async::on_error"); }
      },

      [] (unsigned int handle, [[maybe_unused]] void* user_data, int number_of_bytes, int total_bytes)
      {
        auto it = cbs_.find(handle);
        if (it != cbs_.end())
        {
          if (it->second.on_progress)
            it->second.on_progress(handle, number_of_bytes, total_bytes);
        }
        else { log_error("Not found async::on_progress"); }
      }
    );

    cbs_[handle] = { on_load, on_error, on_progress };

    return handle;
  };


  static void abort_request(std::uint32_t id)
  {
    emscripten_async_wget2_abort(id);
    cbs_.erase(id);
  };


  static void decode(data_store& d, std::string ftype, std::function<void (int, int, int, std::byte*)> decoded)
  {
    auto c = decode_counter_++;

    auto [ it, ok ] = decode_cbs_.emplace(c, std::make_pair(std::move(decoded), d ));

    log_debug(FMT_COMPILE("about to decode: {} bytes"), it->second.second.size());

    emscripten_run_preload_plugins_data(reinterpret_cast<char*>(it->second.second.data()), it->second.second.size(), ftype.c_str(), reinterpret_cast<void*>(c),

      [] (void* user_data, const char* file) -> void
      {
        log_debug(FMT_COMPILE("Got fake filename: {} data: {}"), file, user_data ? "yes" : "no");

        SDL_Surface* image = IMG_Load(file);

        log_debug("img_load done");

        auto c = reinterpret_cast<std::size_t>(user_data);
        auto it = decode_cbs_.find(c);

        if (it == decode_cbs_.end())
        {
          log_error("Unable to find decode callback");
        }
        else
        {
          if (!image)
          {
            log_error(FMT_COMPILE("loading of decoded data failed with: {}"), IMG_GetError());

            it->second.first(0,0,0, nullptr);
          }
          else
          {
            // fixme - really need this copy!?

            std::byte* data = (std::byte*)malloc(image->w * image->h * 4);
            std::memcpy(data, image->pixels, image->w * image->h * 4);

            it->second.first(image->w, image->h, 4, data);
          }

          decode_cbs_.erase(it);
        }

        SDL_FreeSurface(image);
      },

      [] (void* user_data) -> void
      {
        log_error(FMT_COMPILE("failed to decode: {}"), IMG_GetError());

        auto c = reinterpret_cast<std::size_t>(user_data);
        auto it = decode_cbs_.find(c);

        if (it == decode_cbs_.end())
        {
          log_error("Unable to find decode callback");
        }
        else
        {
          it->second.first(0,0,0, nullptr);
          decode_cbs_.erase(it);
        }
      }
    );
  };


  static void start_threads(int c)
  {
    #ifdef __EMSCRIPTEN_PTHREADS__

    log_debug(FMT_COMPILE("starting threads: {}"), c);

    if (running_threadpool_)
      return;

    running_threadpool_ = true;

    for (int i = 0; i < c; ++i)
      threadpool_.push_back(std::thread([] () { listener(); }));

    #else

    log_debug("running in non threaded mode");

    #endif
  };


  static void run(std::function< void ()> f)
  {
    #ifdef __EMSCRIPTEN_PTHREADS__

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      queue_.push(f);
    }

    cond_.notify_one();

    #else

    f(); // run inline if we are only single threaded

    #endif
  };


  static void stop_threads()
  {
    #ifdef __EMSCRIPTEN_PTHREADS__

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);

      stop_threadpool_ = true;
    }

    cond_.notify_all();

    for(auto& t : threadpool_) t.join();

    threadpool_.clear();  

    #endif
  };



private:


  #ifdef __EMSCRIPTEN_PTHREADS__

  static void listener()
  {
    log_debug("in multithreaded listener");

    while(true)
    {
      std::function<void ()> f;

      {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        cond_.wait(lock, [] { return !queue_.empty() || stop_threadpool_; });

        if (stop_threadpool_) return;

        if (!queue_.empty())
        {
          f = queue_.front();
          queue_.pop();
        }
      }

      if (f) f();
    }

    log_debug("quit multithreaded listener");
  };

  #endif


  // callbacks from async::request

  struct cb {
    std::function<void (std::uint32_t, data_store&&)>      on_load;
    std::function<void (std::uint32_t, int, std::string)> on_error;
    std::function<void (std::uint32_t, int, int)>         on_progress;
  };

  inline static std::map<std::uint32_t, cb> cbs_;

  // callbacks from async::fetch

  struct fetch_cb {
    std::function<void (std::size_t, data_store&&, std::uint16_t)> on_load;
    std::function<void (std::size_t, int)>                         on_error;
  };

  inline static std::mutex fetch_mutex_;
  inline static std::size_t fetch_counter_{1};
  inline static std::map<std::size_t, fetch_cb> fetch_cbs_;


  // callbacks from async::decode

  inline static std::size_t decode_counter_{1};

  inline static std::map<std::size_t, std::pair<std::function<void (int, int, int, std::byte*)>, data_store>> decode_cbs_;


  #ifdef __EMSCRIPTEN_PTHREADS__

  inline static std::vector<std::thread> threadpool_;
  inline static std::queue<std::function<void ()>> queue_;
  inline static std::mutex queue_mutex_;
  inline static std::condition_variable cond_;
  inline static bool stop_threadpool_{false};
  inline static bool running_threadpool_{false};

  #endif

}; // class request

} // namespace plate
