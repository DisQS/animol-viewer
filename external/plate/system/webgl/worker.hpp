#pragma once

#include <span>
#include <vector>

class worker
{

public:

  static void start()
  {
    if (!decoders_.empty())
      return;

    auto cores = EM_ASM_INT( { return window.navigator.hardwareConcurrency; }) - 1;

    log_debug(FMT_COMPILE("found: {} cores"), cores + 1);

    if (cores < 1)
      cores = 1;

    if (cores > 6)
      cores = 6;

    for (int i = 0; i < cores; ++i)
    {
      work w;
      w.handle = emscripten_create_worker("/animol-code/version/1/decoder_worker.js");//TODO: pass in path?
      w.slot   = i;

      decoders_.push_back(w);
    }
  }


  static void stop()
  {
    static char a = 'a';

    for (auto& d : decoders_)
    {
      emscripten_call_worker(d.handle, "end_worker", &a, 0, [] (char* d, int s, void* u) -> void
      {
        log_debug("shutdown webworker");

        auto handle = reinterpret_cast<int>(u);
        emscripten_destroy_worker(handle);
      }, reinterpret_cast<void*>(d.handle));
    }

    decoders_.clear();
  }


  static bool call(std::string api, std::string_view to_send, std::function< void (int, std::span<std::byte>)>&& cb)
  {
    if (decoders_.empty())
      start();

    for (auto& d : decoders_)
    {
      if (!d.cb) // this decoder is free
      {
        d.cb = std::move(cb);

        emscripten_call_worker(d.handle, api.c_str(), const_cast<char*>(to_send.data()), to_send.size(), callback, &d);

        return true;
      }
    }

    return false; // no spare workers
  }


  static bool call_slot(int slot, std::string api, std::string_view to_send, std::function< void (int, std::span<std::byte>)>&& cb)
  {
    if (slot < 0 || slot >= decoders_.size())
      return false;

    auto& d = decoders_[slot];

    if (d.cb)
      return false; // something already in this slot

    d.cb = std::move(cb);

    emscripten_call_worker(d.handle, api.c_str(), const_cast<char*>(to_send.data()), to_send.size(), callback, &d);

    return true;
  }


  static bool call_all(std::string api, std::string_view to_send, std::function< void (int, std::span<std::byte>)>&& cb)
  {
    if (decoders_.empty())
      start();

    for (auto& d : decoders_)
    {
      if (!d.cb) // this decoder is free
      {
        d.cb = cb;

        emscripten_call_worker(d.handle, api.c_str(), const_cast<char*>(to_send.data()), to_send.size(), callback, &d);
      }
      else
      {
        log_error("Not all decoder's were available!");
        return false;
      }
    }

    return true;
  }


private:


  static void callback(char* d, int s, void* u)
  {
    auto* work = reinterpret_cast<struct work*>(u);

    auto cb = std::move(work->cb);
    work->cb = nullptr;

    cb(work->slot, std::span<std::byte>(reinterpret_cast<std::byte*>(d), s));
  }


  struct work
  {
    worker_handle                               handle;
    int                                         slot;
    std::function< void (int, std::span<std::byte>)> cb;
  };

  inline static std::vector<work> decoders_;

}; // class worker
