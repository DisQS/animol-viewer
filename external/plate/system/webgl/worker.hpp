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


  static int get_num_workers() noexcept
  {
    if (num_workers_ == 0)
    {
      auto num_threads = EM_ASM_INT( { return window.navigator.hardwareConcurrency; });

      num_workers_ = num_threads - 1;

      if (num_workers_ < 1)
        num_workers_ = 1;

      if (num_workers_ > 6)
        num_workers_ = 6;

      log_debug(FMT_COMPILE("found: {} hardware threads"), num_threads);
    }

    return num_workers_;
  }


  template<class DATA>
  bool call(std::string fname, DATA& data_to_send, std::function< void (std::span<char>)>&& cb) noexcept
  {
    if (workers_.empty())
      start();

    if (queue_.empty())
    {
      for (auto& w : workers_)
      {
        if (!w.cb) // this decoder is free
        {
          w.cb = std::move(cb);

          emscripten_call_worker(w.handle, fname.c_str(), data_to_send.data(), data_to_send.size(), callback, &w);

          return true;
        }
      }
    }

    std::vector<char> data_to_send_copy(data_to_send.data(), data_to_send.data() + data_to_send.size());

    queue_.emplace(fname, std::move(data_to_send_copy), std::move(cb));

    return false; // request has been queued
  }


private:


  static void start() noexcept
  {
    if (!workers_.empty())
      return;

    for (int i = 0; i < get_num_workers(); ++i)
    { 
      work w;
      w.handle = emscripten_create_worker(path_.c_str());
  
      workers_.push_back(w);
    }

    log_debug(FMT_COMPILE("started: {} workers"), num_workers_);
  } 
    
  
  static void stop() noexcept
  {
    for (auto& w : workers_)
      emscripten_destroy_worker(w.handle);
      
    workers_.clear();

    log_debug("workers all finished");
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
      for (auto& w : workers_)
      {
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
  inline static int num_users_{0};      // how many users/clients there are
 

}; // class worker
