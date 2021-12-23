#pragma once

#include <span>
#include <vector>
#include <queue>

/*
    client interface to call user worker code.

    set_path(path)

      sets the webworker url

    get_num_workers()

      returns the number of workers. The code limits this to a maximum of 6 workers even if the client has more.

    call(fname, data_to_send, cb)

      call the fname function on any available worker, with data_to_send input and issue a callback cb with the results

      if all the workers are busy, the action is automatically queued until one is available.
*/


class worker
{

public:

  worker() noexcept
  {
    ++num_users_;
  }


  ~worker() noexcept
  {
    --num_users_;

    if (num_users_ == 0)
      stop();
  }


  static void set_path(std::string_view path) noexcept
  {
    path_ = path;
  }


  static int get_max_workers() noexcept
  {
    if (max_workers_ == 0)
    {
      auto num_threads = EM_ASM_INT( { return window.navigator.hardwareConcurrency; });

      max_workers_ = num_threads - 1;

      if (max_workers_ < 1)
        max_workers_ = 1;

      if (max_workers_ > 6)
        max_workers_ = 6;

      workers_.resize(max_workers_);

      log_debug(FMT_COMPILE("found: {} hardware threads"), num_threads);
    }

    return max_workers_;
  }


  template<class DATA>
  bool call(std::string fname, DATA& data_to_send, std::function< void (std::span<char>)>&& cb) noexcept
  {
    if (queue_.empty())
    {
      for (int i = 0; i < num_workers_; ++i)
      {
        auto& w = workers_[i];

        if (!w.cb) // this decoder is free
        {
          w.cb = std::move(cb);

          emscripten_call_worker(w.handle, fname.c_str(), data_to_send.data(), data_to_send.size(), callback, &w);

          return true;
        }
      }

      if (create_worker())
      {
        auto& w = workers_[num_workers_-1];

        w.cb = std::move(cb);

        emscripten_call_worker(w.handle, fname.c_str(), data_to_send.data(), data_to_send.size(), callback, &w);

        return true;
      }
    }

    std::vector<char> data_to_send_copy(data_to_send.data(), data_to_send.data() + data_to_send.size());

    queue_.emplace(fname, std::move(data_to_send_copy), std::move(cb));

    return false; // request has been queued
  }


private:


  static bool create_worker() noexcept
  {
    if (num_workers_ >= get_max_workers())
      return false;

    workers_[num_workers_++].handle = emscripten_create_worker(path_.c_str());
  
    log_debug("started a worker");

    return true;
  } 
    
  
  static void stop() noexcept
  {
    if (num_workers_ > 1)
    {
      for (int i = 1; i < num_workers_; ++i)
        emscripten_destroy_worker(workers_[i].handle);
      
      log_debug(FMT_COMPILE("destroyed {} workers"), num_workers_ - 1);

      num_workers_ = 1;
    }
  }


  static void callback(char* d, int s, void* u)
  {
    auto work = reinterpret_cast<struct work*>(u);

    auto cb = std::move(work->cb);
    work->cb = nullptr;

    cb(std::span<char>(d, s));

    // can we process anything in the queue?

    if (!queue_.empty())
    {
      for (int i = 0; i < num_workers_; ++i)
      {
        auto& w = workers_[i];

        if (!w.cb) // this decoder is free
        {
          auto& req = queue_.front();

          w.cb = std::move(req.cb);

          emscripten_call_worker(w.handle, req.fname.c_str(), req.data_to_send.data(), req.data_to_send.size(), callback, &w);

          queue_.pop();

          break; // can be a maximum of 1 queue entry invoked as we've only freed 1 worker
        }
      }
    }
  }


  // each worker has a work structure which stores it's handle/id and if there is work in progress, the callback function
  // to call once the work is complete

  struct work
  {
    worker_handle                          handle;
    std::function< void (std::span<char>)> cb;
  };

  inline static std::vector<work> workers_;


  // If all the workers are busy, we copy the work request for submission later

  struct work_request
  {
    work_request(std::string fname, std::vector<char>&& data_to_send, std::function< void (std::span<char>)>&& cb) :
      fname(fname),
      data_to_send(std::move(data_to_send)),
      cb(std::move(cb))
    {
    }

    std::string                            fname;
    std::vector<char>                      data_to_send;
    std::function< void (std::span<char>)> cb;
  };

  inline static std::queue<work_request> queue_;

  inline static std::string path_;

  inline static int num_workers_{0};    // how many workers we have
  inline static int max_workers_{0};    // maximum number of workers we can have
  inline static int num_users_{0};      // how many users/clients there are
 

}; // class worker
