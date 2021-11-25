#pragma once

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/bind.h>

#include <cstdlib>
#include <span>

#include <GLES3/gl3.h>


#include "../common/ui_event_destination.hpp"
#include "../log.hpp"
#include "../../gpu/gpu.hpp"

/* An ui_event implementation using emscripten

  To install emscripten see: https://emscripten.org/docs/getting_started/downloads.html

  see the emscripten guide: https://emscripten.org/docs/api_reference/html5.h.html#focus

*/


namespace plate {
namespace ui_event {

// public


bool initialised_{false};
std::vector<std::pair<plate::state*, std::string>> states_;

// private vars

std::set<int> touch_avail_{0,1,2,3,4,5,6,7,8,9};

std::map<int, int> touch_mapping_; // maps touch identifiers to the touch number...
                                   // ie.. touch x comes in first that is number 0...


EM_JS(int, js_get_device_type, (),
{
  const ua = navigator.userAgent;

  if (/(tablet|ipad|playbook|silk)|(android(?!.*mobi))/i.test(ua))
  {
    return 2; // tablet
  }
  if (/Mobile|iP(hone|od)|Android|BlackBerry|IEMobile|Kindle|Silk-Accelerated|(hpw|web)OS|Opera M(obi|ini)/.test(ua))
  {
    return 1; // mobile;
  }
  
  return 3; // desktop
});


EM_JS(int, js_is_touch_device, (),
{
  var match = window.matchMedia || window.msMatchMedia;
  if (match)
  {
    var mq = match("(pointer:coarse)");
    if (mq.matches) return 1;
  }

  return 0;
});


EM_JS(void, js_vibrate, (int t),
{
  if (!window || !window.navigator || !window.navigator.vibrate)
    return;

  window.navigator.vibrate(t);
});


EM_JS(void, js_copy, (const char* t, int t_len),
{
  var text = UTF8ToString(t, t_len);

  var a = window["navigator"]["clipboard"];

  if (a)
    a["writeText"](text);
});


EM_JS(void, js_paste, (const char* name, int name_len), // must have focus before requesting a paste
{
  var w_name = UTF8ToString(name, name_len);
  
  document.getElementById(w_name).focus();
  
  var a = window["navigator"]["clipboard"];

  if (a)
    a["readText"]().then(t => window["Plate"]["f_paste"](t));
});


void paste(const char* name, int name_len) noexcept
{
  js_paste(name, name_len);
}

double now()
{
  return emscripten_get_now();
}


EM_JS(char*, js_get_tz, (),
{
  var tz = Intl.DateTimeFormat().resolvedOptions().timeZone;

  var sz               = lengthBytesUTF8(tz) + 1;
  var str_in_wasm_heap = _malloc(sz);

  stringToUTF8(tz, str_in_wasm_heap, sz);

  return str_in_wasm_heap;
});


std::string get_tz() noexcept
{
  char* str = js_get_tz();

  std::string s(str);

  free(str);

  return s;
}


EM_JS(void, js_close_window, (),
{
  window.close("","_parent","");
});

void close_window() noexcept
{
  js_close_window();
}


EM_JS(void, js_open_url, (const char* path, int path_len),
{
  var p_name = UTF8ToString(path, path_len);
  
  window.open(p_name);
});


inline void open_url(std::string_view path) noexcept
{
  js_open_url(path.data(), path.size());
};


EM_JS(void, js_set_fragment, (const char* frag, int frag_len),
{
  var p_frag = UTF8ToString(frag, frag_len);

  window.location.hash = p_frag;
});


inline void set_fragment(std::string_view frag) noexcept
{
  js_set_fragment(frag.data(), frag.size());
}


EM_JS(char*, js_get_fragment, (),
{
  var sz               = lengthBytesUTF8(window.location.hash) + 1;
  var str_in_wasm_heap = _malloc(sz);

  stringToUTF8(window.location.hash, str_in_wasm_heap, sz);

  return str_in_wasm_heap;
});


inline std::string get_fragment() noexcept
{
  char* str = js_get_fragment();

  std::string s(str);

  free(str);

  return s;
}


EM_JS(int, js_file_system_access_support, (),
{
  if ('showDirectoryPicker' in self)
    return 1;
  else
    return 0;
});


bool have_file_system_access_support()
{
  return js_file_system_access_support() == 1;
}


EM_JS(void, js_directory_show_picker, (const char* pattern, int pattern_len, std::uint32_t cb),
{
  var w_pattern = UTF8ToString(pattern, pattern_len);
  var w_regexp  = new RegExp(w_pattern, "i"); // case insensitive regexp

  if (typeof window["Plate"]["dir_handles"] === 'undefined')
    window["Plate"]["dir_handles"] = new Map();

  var dir_name = "";
  var dir_list = ""; // string of files seperated by '\r'

  async function process(it, x, cb)
  {
    if (!x.done)
    {
      if (w_pattern == "" || x.value.name.match(w_regexp))
      {
        dir_list += x.value.name;
        dir_list += "\n";
      }

      it.next().then (x => process(it, x, cb));
    }
    else  // all done
    {
      window["Plate"]["f_directory_show_picker_cb"](cb, dir_name, dir_list);
    }
  }


  window.showDirectoryPicker().then (handle =>
  {
    dir_name = handle.name;

    window["Plate"]["dir_handles"].set(dir_name, handle);

    let it = handle.values();

    it.next().then (x => process(it, x, cb));
  }).catch (e =>
  {
    window["Plate"]["f_directory_show_picker_cb"](cb, "", "");
  });

});


void directory_show_picker(std::string_view file_pattern, std::function<void (std::string, std::vector<std::string>&)> cb) noexcept
{
  auto f = new std::function(cb);

  js_directory_show_picker(file_pattern.data(), file_pattern.size(), reinterpret_cast<std::uint32_t>(f));
}


EM_JS(void, js_directory_load_file, (const char* dir, int dir_len, const char* fname, int fname_len, std::uint32_t cb, std::uint32_t store),
{
  var j_dir   = UTF8ToString(dir,   dir_len);
  var j_fname = UTF8ToString(fname, fname_len);

  var dir_handle = window["Plate"]["dir_handles"].get(j_dir);

  if (dir_handle === 'undefined')
  {
    window["Plate"]["f_directory_load_file_cb"](cb, store);
    return;
  }

  dir_handle.getFileHandle(j_fname, { create: false }).then (file_handle =>
  {
    file_handle.getFile().then (file =>
    {
      file.arrayBuffer().then (buff =>
      {
        var arr = window["Plate"]["f_get_memory"](store, buff.byteLength);

        var u = new Uint8Array(buff);
        arr.set(u);

        window["Plate"]["f_directory_load_file_cb"](cb, store);
      });
    });
  }).catch (e =>
  {
    window["Plate"]["f_directory_load_file_cb"](cb, store);
  });

});


void directory_load_file(std::string_view dir, std::string_view name, std::function<void (std::string&&)> cb) noexcept
{
  auto f = new std::function(cb);
  auto s = new std::string();

  js_directory_load_file(dir.data(), dir.size(), name.data(), name.size(), reinterpret_cast<std::uint32_t>(f),
                                                                           reinterpret_cast<std::uint32_t>(s));
}


void stop()
{
  auto ss = states_;

  for (auto& [s, name] : ss) rm_dest(s, name);

  // event: window_size

  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);


  // event: window_focus

  emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);

  // these aren't working...?
  //emscripten_set_focusin_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);
  //emscripten_set_focusout_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);


  // event: orientation

  emscripten_set_orientationchange_callback(nullptr, 1, nullptr);

  // event: key

  emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);
  emscripten_set_keydown_callback (EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);
  emscripten_set_keyup_callback   (EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);


  // event: mouse_position

  emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);

  // event: mouse_over

  emscripten_set_mouseover_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);
  emscripten_set_mouseout_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);

  // event: mouse_button

  emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);
  emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);
  emscripten_set_click_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);
  emscripten_set_dblclick_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);


  // event: scroll_wheel

  emscripten_set_wheel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);

  // event: touch

  emscripten_set_touchstart_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);
  emscripten_set_touchend_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);
  emscripten_set_touchmove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);
  emscripten_set_touchcancel_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, nullptr);
};


void init();


//void start(std::string c)
//{
//  canvas_ = c;
//
//  emscripten_run_script("document.getElementById('canvas').focus()");
//
//  init();
//};


void to_fullscreen()
{
/*
  EmscriptenFullscreenChangeEvent e;
  emscripten_get_fullscreen_status(&e);

  if (e.isFullscreen == EM_TRUE)
  {
    emscripten_exit_fullscreen();
  }
  else
  {
    emscripten_request_fullscreen(canvas_.c_str(), true);
  }
  */
};


// private


EM_BOOL f_context_lost(int event_type, const void* reserved, void* user_data)
{
  log_debug(FMT_COMPILE("Got context lost type: {}"), event_type);

  return EM_TRUE;
}


EM_BOOL f_window_size(int event_type, const EmscriptenUiEvent* e, [[maybe_unused]] void* user_data)
{
  log_debug(FMT_COMPILE("Got window_size type: {} user_data: {}"), event_type, user_data);

//  if (event_type != EMSCRIPTEN_EVENT_RESIZE) return EM_FALSE;
//
//  window_size(e->windowInnerWidth, e->windowInnerHeight);
//
  return EM_TRUE;
};




EM_BOOL f_window_focus(int event_type, [[maybe_unused]] const EmscriptenFocusEvent *e, void* user_data)
{
  state* s = reinterpret_cast<plate::state*>(user_data);

  emscripten_webgl_make_context_current(s->ctx_);

  if (event_type == EMSCRIPTEN_EVENT_FOCUS)
  {
    state* s = reinterpret_cast<plate::state*>(user_data);
    s->incoming_focus(true);

    return EM_TRUE;
  }

  return EM_FALSE;
};


EM_BOOL f_orientation([[maybe_unused]] int event_type, const EmscriptenOrientationChangeEvent *e, [[maybe_unused]] void *user_data)
{
  log_debug(FMT_COMPILE("Got window orientation: {}"), e->orientationIndex);

//  orientation((enum Orientation)e->orientationIndex);

  return EM_TRUE;
};


EM_BOOL f_key(int event_type, const EmscriptenKeyboardEvent *e, void *user_data)
{
  //log_debug(FMT_COMPILE("got key event_type: {}"), event_type);

  enum KeyEvent event;

  switch (event_type)
  {
    case EMSCRIPTEN_EVENT_KEYPRESS: event = KeyEventPress; break;
    case EMSCRIPTEN_EVENT_KEYDOWN:  event = KeyEventDown;  break;
    case EMSCRIPTEN_EVENT_KEYUP:    event = KeyEventUp;    break;

    default: return EM_FALSE;
  }

  int mods = (e->shiftKey ? KeyModShift   : 0) |
             (e->ctrlKey  ? KeyModControl : 0) |
             (e->altKey   ? KeyModAlt     : 0) |
             (e->metaKey  ? KeyModSuper   : 0);

  state* s = reinterpret_cast<plate::state*>(user_data);

  emscripten_webgl_make_context_current(s->ctx_);

  /*
  int k = *(e->key);
  if (k >= 97 && k <= 122) k -= 'a' - 'A';
  if (strlen(e->key) != 1) k = 0;

  ui->key(k, event, (enum KeyMod)mods);
  */


  //if (!strcmp(e->key, "Shift") || !strcmp(e->key, "Control") || !strcmp(e->key, "Alt") || !strcmp(e->key, "Meta") || !strcmp(e->key, "CapsLock") || !strcmp(e->key, "NumLock") || !strcmp(e->key, "Backspace"))

  if ((strlen(e->key) > 4) || !strcmp(e->key, "Alt") || !strcmp(e->key, "CapsLock") ||
                              !strcmp(e->key, "Home") || !strcmp(e->key, "End"))
  {
    const char empty = 0;

    s->incoming_key_event(event, &empty, e->code, static_cast<KeyMod>(mods));
  }
  else
  {
    s->incoming_key_event(event, e->key, e->code, static_cast<KeyMod>(mods));
  }

  return EM_TRUE;
};


EM_BOOL f_mouse_position(int event_type, const EmscriptenMouseEvent *e, [[maybe_unused]] void *user_data)
{
  int button = 0;
  switch (e->button)
  {
    case 0: button = MouseButtonLeft;    break;
    case 1: button = MouseButtonMiddle;  break;
    case 2: button = MouseButtonRight;   break;
    case 3: button = MouseButtonBack;    break;
    case 4: button = MouseButtonForward; break;
  }

  int mods = (e->shiftKey ? KeyModShift   : 0) |
             (e->ctrlKey  ? KeyModControl : 0) |
             (e->altKey   ? KeyModAlt     : 0) |
             (e->metaKey  ? KeyModSuper   : 0);

  state* s = reinterpret_cast<plate::state*>(user_data);

  emscripten_webgl_make_context_current(s->ctx_);

  switch (event_type)
  {
    case EMSCRIPTEN_EVENT_CLICK:

      s->incoming_mouse_button(MouseButtonEventClick, static_cast<enum MouseButton>(button), static_cast<enum KeyMod>(mods));
      break;

    case EMSCRIPTEN_EVENT_MOUSEDOWN:

      s->incoming_mouse_button(MouseButtonEventDown, static_cast<enum MouseButton>(button), static_cast<enum KeyMod>(mods));
      break;

    case EMSCRIPTEN_EVENT_MOUSEUP:

      s->incoming_mouse_button(MouseButtonEventUp, static_cast<enum MouseButton>(button), static_cast<enum KeyMod>(mods));
      break;

    case EMSCRIPTEN_EVENT_DBLCLICK:

      s->incoming_mouse_button(MouseButtonEventDoubleClick, static_cast<enum MouseButton>(button), static_cast<enum KeyMod>(mods));
      break;

    case EMSCRIPTEN_EVENT_MOUSEMOVE:

      s->incoming_mouse_position(e->targetX * s->pixel_ratio_, (double)(s->pixel_height_ - (e->targetY * s->pixel_ratio_)));
      break;

    case EMSCRIPTEN_EVENT_MOUSEENTER: break;
    case EMSCRIPTEN_EVENT_MOUSELEAVE: break;
    case EMSCRIPTEN_EVENT_MOUSEOVER:  s->incoming_mouse_over(true);  break;
    case EMSCRIPTEN_EVENT_MOUSEOUT:   s->incoming_mouse_over(false); break;

    default: return EM_TRUE; // ??
  }

  return EM_TRUE;
};


EM_BOOL f_scroll_wheel([[maybe_unused]] int event_type, const EmscriptenWheelEvent *e, void *user_data)
{
  state* s = reinterpret_cast<plate::state*>(user_data);

  emscripten_webgl_make_context_current(s->ctx_);

  return s->incoming_scroll_wheel(e->deltaX, e->deltaY);
};


EM_BOOL f_touch(int event_type, const EmscriptenTouchEvent *e, void *user_data)
{
  state* s = reinterpret_cast<plate::state*>(user_data);

  emscripten_webgl_make_context_current(s->ctx_);

  //log_debug(FMT_COMPILE("got: {} touches"), e->numTouches);

  for (int i = 0; i < e->numTouches; i++)
  {
    const auto& t = e->touches[i];

    if (!t.isChanged)
      continue;

    auto x = t.targetX * s->pixel_ratio_;
    auto y = s->pixel_height_ - (t.targetY * s->pixel_ratio_);


    int id;

    if (auto it = touch_mapping_.find(t.identifier); it != touch_mapping_.end())
      id = it->second;
    else
    {
      auto it2 = touch_avail_.begin();
      id = *it2;
      touch_mapping_[t.identifier] = id;
      touch_avail_.erase(it2);
    }

    if (event_type == EMSCRIPTEN_EVENT_TOUCHEND)
    {
      touch_avail_.insert(id);
      touch_mapping_.erase(t.identifier);
    }

    if (event_type == EMSCRIPTEN_EVENT_TOUCHCANCEL)
    {
      touch_avail_.insert(id);
      touch_mapping_.erase(t.identifier);
    }

    switch (event_type)
    {
      case EMSCRIPTEN_EVENT_TOUCHSTART:  s->incoming_touch(TouchEventDown,   id, x, y); break;
      case EMSCRIPTEN_EVENT_TOUCHEND:    s->incoming_touch(TouchEventUp,     id, x, y); break;
      case EMSCRIPTEN_EVENT_TOUCHMOVE:   s->incoming_touch(TouchEventMove,   id, x, y); break;
      case EMSCRIPTEN_EVENT_TOUCHCANCEL: s->incoming_touch(TouchEventCancel, id, x, y); break;
    }
  }

  return EM_TRUE;
};



// EMSCRIPTEN_EVENT_TARGET_WINDOW

void init()
{
  // event: window_resize

  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, f_window_size);

  // event: orientation

  emscripten_set_orientationchange_callback(nullptr, 1, f_orientation);
};


template<class S>
void add_dest(S* s, std::string canvas)
{
  if (!initialised_)
  {
    initialised_ = true;
//    init();
  }

  states_.emplace_back(s, canvas);

  // webgl context
    
  emscripten_set_webglcontextlost_callback(canvas.c_str(), s, 1, f_context_lost);

  // event: window_focus

  emscripten_set_focus_callback(canvas.c_str(), s, EM_TRUE, f_window_focus);

  // event: key
  
  emscripten_set_keypress_callback(canvas.c_str(), s, EM_TRUE, f_key);
  emscripten_set_keydown_callback (canvas.c_str(), s, EM_TRUE, f_key);
  emscripten_set_keyup_callback   (canvas.c_str(), s, EM_TRUE, f_key);
  
  
  // event: mouse_position
  
  emscripten_set_mousemove_callback(canvas.c_str(), s, 1, f_mouse_position);
  //emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, s, 1, f_mouse_position);
  
  // event: mouse_over
  
  emscripten_set_mouseover_callback(canvas.c_str(), s, 1, f_mouse_position);
  emscripten_set_mouseout_callback(canvas.c_str(), s, 1, f_mouse_position);
  //emscripten_set_mouseover_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, s, 1, f_mouse_position);
  //emscripten_set_mouseout_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, s, 1, f_mouse_position);
  
  // event: mouse_button
  
  emscripten_set_mousedown_callback(canvas.c_str(), s, 1, f_mouse_position);
  //emscripten_set_mouseup_callback(canvas.c_str(), s, 1, f_mouse_position);
  emscripten_set_click_callback(canvas.c_str(), s, 1, f_mouse_position);
  emscripten_set_dblclick_callback(canvas.c_str(), s, 1, f_mouse_position);
  //emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, s, 1, f_mouse_position);
  emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, s, 1, f_mouse_position);
  //emscripten_set_click_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, s, 1, f_mouse_position);
  //emscripten_set_dblclick_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, s, 1, f_mouse_position);
  
  
  // event: scroll_wheel
  
  emscripten_set_wheel_callback(canvas.c_str(), s, 1, f_scroll_wheel);

  
  // event: touch
  
  emscripten_set_touchstart_callback(canvas.c_str(), s, 0, f_touch);
  emscripten_set_touchend_callback(canvas.c_str(), s, 0, f_touch);
  emscripten_set_touchmove_callback(canvas.c_str(), s, 0, f_touch);
  emscripten_set_touchcancel_callback(canvas.c_str(), s, 0, f_touch);
};


template<class S> // plate::state
void rm_dest(S* s, std::string canvas)
{
  for (std::size_t i = 0; i < states_.size(); ++i)
  {
    if (states_[i].first == s)
    {
      states_.erase(states_.begin() + i);
    }
  }
  
  // webgl context
    
  emscripten_set_webglcontextlost_callback(canvas.c_str(), nullptr, 1, nullptr);

  // event: window_focus

  emscripten_set_focus_callback(canvas.c_str(), nullptr, EM_TRUE, nullptr);

  // event: key
  
  emscripten_set_keypress_callback(canvas.c_str(), nullptr, EM_TRUE, nullptr);
  emscripten_set_keydown_callback (canvas.c_str(), nullptr, EM_TRUE, nullptr);
  emscripten_set_keyup_callback   (canvas.c_str(), nullptr, EM_TRUE, nullptr);
  
  
  // event: mouse_position
  
  //emscripten_set_mousemove_callback(canvas.c_str(), nullptr, 1, nullptr);
  emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_SCREEN, nullptr, 1, nullptr);
  
  // event: mouse_over
  
  //emscripten_set_mouseover_callback(canvas.c_str(), nullptr, 1, nullptr);
  //emscripten_set_mouseout_callback(canvas.c_str(), nullptr, 1, nullptr);
  emscripten_set_mouseover_callback(EMSCRIPTEN_EVENT_TARGET_SCREEN, nullptr, 1, nullptr);
  emscripten_set_mouseout_callback(EMSCRIPTEN_EVENT_TARGET_SCREEN, nullptr, 1, nullptr);
  
  // event: mouse_button
  
  //emscripten_set_mousedown_callback(canvas.c_str(), nullptr, 1, nullptr);
  //emscripten_set_mouseup_callback(canvas.c_str(), nullptr, 1, nullptr);
  //emscripten_set_click_callback(canvas.c_str(), nullptr, 1, nullptr);
  //emscripten_set_dblclick_callback(canvas.c_str(), nullptr, 1, nullptr);
  emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_SCREEN, nullptr, 1, nullptr);
  emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_SCREEN, nullptr, 1, nullptr);
  emscripten_set_click_callback(EMSCRIPTEN_EVENT_TARGET_SCREEN, nullptr, 1, nullptr);
  emscripten_set_dblclick_callback(EMSCRIPTEN_EVENT_TARGET_SCREEN, nullptr, 1, nullptr);
  
  
  // event: scroll_wheel
  
  emscripten_set_wheel_callback(canvas.c_str(), nullptr, 1, nullptr);
  
  
  // event: touch
  
  emscripten_set_touchstart_callback(canvas.c_str(), nullptr, 0, nullptr);
  emscripten_set_touchend_callback(canvas.c_str(), nullptr, 0, nullptr);
  emscripten_set_touchmove_callback(canvas.c_str(), nullptr, 0, nullptr);
  emscripten_set_touchcancel_callback(canvas.c_str(), nullptr, 0, nullptr);
};



}; // namespace ui_event

extern void arch_resize(plate::state* s);
extern void arch_resize(plate::state* s, int w, int h);

}; // namespace plate



void f_fragment_change(std::string name)
{
  using namespace plate;

  for (auto& [s, n] : ui_event::states_)
    s->fragment_change(name);
}


void f_canvas_resize_exact(std::string name, int width, int height)
{
  using namespace plate;

  auto name_s = "#" + name.substr(0, name.length()-3);

  //LOG_DEBUG("Got c++ resize for: %s %s", name.c_str(), name_s.c_str());

  for (auto& [s, n] : ui_event::states_)
    if (name_s == n)
    {
      arch_resize(s, width, height);
      break;
    }
}


void f_canvas_resize(std::string name)
{
  using namespace plate;

  auto name_s = "#" + name.substr(0, name.length()-3);

  //LOG_DEBUG("Got c++ resize for: %s %s", name.c_str(), name_s.c_str());

  for (auto& [s, n] : ui_event::states_)
    if (name_s == n)
    {
      arch_resize(s);
      break;
    }
}


void f_color_mode_change(int mode)
{
  using namespace plate;

  log_debug(FMT_COMPILE("Got c++ color mode change to: {}"), mode);

  for (auto& [s, n] : ui_event::states_)
    s->set_darkmode(mode == 0 ? false : true);
}

void f_paste(std::string t)
{
  using namespace plate;

  for (auto& [s, n] : ui_event::states_)
    s->incoming_paste_event(t);
}

void f_directory_show_picker_cb(std::uint32_t cb, std::string dir_name, std::string filelist)
{
  using namespace plate;

  auto f = reinterpret_cast<std::function<void (std::string, std::vector<std::string>&)>*>(cb);

  std::vector<std::string> files;

  int start;
  int end = 0;

  while ((start = filelist.find_first_not_of('\n', end)) != std::string::npos)
  {
    end = filelist.find('\n', start);

    if (end - start > 0)
      files.push_back(filelist.substr(start, end - start));
  }

  std::sort(files.begin(), files.end());

  if (f)
  {
    (*f)(dir_name, files);
    delete f;
  }
}


void f_directory_load_file_cb(std::uint32_t cb, std::uint32_t store)
{
  using namespace plate;

  auto f = reinterpret_cast<std::function<void (std::string&&)>*>(cb);
  auto s = reinterpret_cast<std::string*>(store);

  if (f)
  { 
    (*f)(std::move(*s));
    delete f;
  }

  if (s)
    delete s;
}

emscripten::val f_get_memory(std::uint32_t store, int sz) // allocate amount of bytes to our string and return it as a javaxript typed array
{
  auto s = reinterpret_cast<std::string*>(store);

  s->resize(sz, 'x');
  return emscripten::val(emscripten::typed_memory_view(sz, s->data()));
}


EMSCRIPTEN_BINDINGS(plate)
{
  emscripten::function("f_fragment_change", &f_fragment_change);
  emscripten::function("f_canvas_resize", &f_canvas_resize);
  emscripten::function("f_canvas_resize_exact", &f_canvas_resize_exact);
  emscripten::function("f_color_mode_change", &f_color_mode_change);
  emscripten::function("f_paste", &f_paste);
  emscripten::function("f_directory_show_picker_cb", &f_directory_show_picker_cb);
  emscripten::function("f_directory_load_file_cb", &f_directory_load_file_cb);

  emscripten::function("f_get_memory", &f_get_memory);
}
