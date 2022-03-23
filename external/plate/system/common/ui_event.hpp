#pragma once

#include <string>
#include <string_view>

/*
  ui event interface. Abstracted so that underneath can support any event model/system, eg: webgl, vulkan, metal..

  All input events are handled by this:

    Keyboard - key presses
    Mouse    - movement, buttons and wheel
    Touch    - on screen
    Window   - rotate, focus, full screen, iconify

  All window creation is handled by this:
*/


namespace plate {
namespace ui_event {


// For keyboard events

enum KeyMod
{
  KeyModNone       = 0,
  KeyModShift      = 1<<0,
  KeyModControl    = 1<<1,
  KeyModAlt        = 1<<2,
  KeyModSuper      = 1<<3,
  KeyModCaps_Lock  = 1<<4,
  KeyModNum_lock   = 1<<5
};

enum KeyEvent
{
  KeyEventDown     = 1<<0,
  KeyEventUp       = 1<<1,
  KeyEventRepeat   = 1<<2,
  KeyEventPress    = 1<<3
};

// For mouse events

enum MouseButton
{
  MouseButtonLeft     = 1<<0,
  MouseButtonMiddle   = 1<<1,
  MouseButtonRight    = 1<<2,
  MouseButtonBack     = 1<<3,
  MouseButtonForward  = 1<<4
};

enum MouseButtonEvent
{
  MouseButtonEventDown        = 1<<0,
  MouseButtonEventUp          = 1<<1,
  MouseButtonEventClick       = 1<<2,
  MouseButtonEventDoubleClick = 1<<3
};

// For touch events

enum TouchEvent
{
  TouchEventDown   = 1<<0,
  TouchEventUp     = 1<<1,
  TouchEventMove   = 1<<2,
  TouchEventCancel = 1<<3
};

// For device orientation

enum Orientation
{
  PortraitPrimary    = 1<<0,
  PortraitSecondary  = 1<<1,
  LandscapePrimary   = 1<<2,
  LandscapeSecondary = 1<<3
};

// For gpu context version

enum Context
{
  ContextNone        = 0,

  ContextWebGL       = 1<<0,    // web
  ContextWebGPU      = 1<<1,

  ContextOpenGL      = 1<<2,    // linux
  ContextVulkan      = 1<<3,

  ContextMetal       = 1<<4,    // apple

  ContextDirectx     = 1<<5     // microsoft
};

// For device type

enum DeviceType
{
  DeviceMobile   = 1,
  DeviceTablet   = 2,
  DeviceDesktop  = 3
};

// For device touch capability

enum DeviceHasTouch
{
  DeviceNoTouch  = 0,
  DeviceTouch    = 1
};



// for printing

std::string KeyMod_string(const KeyMod km)
{
  std::string s;
  if (km & KeyModShift)     s += "KeyModShift ";
  if (km & KeyModControl)   s += "KeyModControl ";
  if (km & KeyModAlt)       s += "KeyModAlt ";
  if (km & KeyModSuper)     s += "KeyModSuper ";
  if (km & KeyModCaps_Lock) s += "KeyModCaps_Lock ";
  if (km & KeyModNum_lock)  s += "KeyModNum_lock ";

  if (s.size() > 0) s.resize(s.size() - 1); // remove last space

  return s;
};


std::string KeyEvent_string(const KeyEvent ke)
{
  if (ke & KeyEventDown)   return "KeyEventDown";
  if (ke & KeyEventUp)     return "KeyEventUp";
  if (ke & KeyEventRepeat) return "KeyEventRepeat";
  if (ke & KeyEventPress)  return "KeyEventPress";

  return "";
};


std::string DeviceType_string(const DeviceType t)
{
  if (t == DeviceMobile)  return "Mobile";
  if (t == DeviceTablet)  return "Tablet";
  if (t == DeviceDesktop) return "Desktop";

  return "UnknwonDevice";
};


std::string DeviceHasTouch_string(const DeviceHasTouch t)
{
  if (t == DeviceNoTouch) return "NoTouch";
  if (t == DeviceTouch)   return "Touch";

  return "UnknownTouch";
};


// stored running context and context's version

enum Context context_ = ContextNone;
float version_ = 0;


void set_context(enum Context context, float version)
{
  context_ = context;
  version_ = version;
};


void get_context(enum Context& context, float& version)
{
  context = context_;
  version = version_;
};


// called by client

extern void paste(const char* name, int name_len) noexcept;
extern inline void open_url(std::string_view name) noexcept;
extern inline void set_fragment(std::string_view name) noexcept;
extern inline std::string get_fragment() noexcept;
extern std::string get_tz() noexcept;
extern void set_focus(const std::string& name) noexcept;
extern float get_font_size() noexcept;

extern double now();

template<class S>
extern void to_fullscreen(std::shared_ptr<S>);

template<class S>
extern void add_dest(S*, std::string);
  
template<class S>
extern void rm_dest(S*, std::string);

}; // namespace ui_event
}; // namespace plate

