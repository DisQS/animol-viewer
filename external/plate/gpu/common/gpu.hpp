#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <cstring>
#include <cstddef>
#include <cstdlib>


/*
    provides utility interfaces for:

    vectors:
      2 items: int_vec2, int_point, float_point, float_vec2
      3 items: int_vec3, float_vec3, scale, rotation, color_rgb
      4 items: int_vec4, float_vec4, coord, color
      boxes:   int_box, float_box

     math functions:
      matrix44_mult, matrix44_shift, matrix44_scale
      dist, mid

     projection matrix manipulation:
      transform
      set_projection
      set_x_offset
      set_y_offset
      get_x_offset
      get_y_offset

     do_draw();

  Along with main settings of:

    viewport_width_, viewport_height_
    pixel_width_, pixel_height_
    pixel_ratio_

    x_offset_, y_offset_

    to_draw_, in_draw_
*/


namespace plate {

namespace gpu {

  enum topology         // Vulkan                                 OpenGL
  {
    POINT_LIST     = 0, // VK_PRIMITIVE_TOPOLOGY_POINT_LIST       GL_LINES
    LINE_LIST      = 1, // VK_PRIMITIVE_TOPOLOGY_LINE_LIST        GL_LINE_LOOP
    LINE_STRIP     = 2, // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP       GL_LINE_STRIP
    TRIANGLE_LIST  = 3, // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST    GL_TRIANGLES
    TRIANGLE_STRIP = 4, // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP   GL_TRIANGLE_STRIP
    TRIANGLE_FAN   = 5  // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN     GL_TRIANGLE_FAN
  };

  enum align
  {
    EMPTY     = 0,
    LEFT      = 1<<0,
    RIGHT     = 1<<1,
    CENTER    = 1<<2,
    TOP       = 1<<3,
    BOTTOM    = 1<<4,
    OUTSIDE   = 1<<5    // used for eg: fonts when want the font height rather than the displayed height
  };

  // colors

  struct color_rgb { float r, g, b; };
  struct color { float r, g, b, a; };


  // 2 somethings

  template<class T>
  struct any_vec2 {
    T x, y;

    bool operator==(const any_vec2<T>& other) const { return x == other.x && y == other.y; };

    T& operator[](std::size_t i) const { return *(T *)((std::byte *)this + (i * sizeof(T))); };

    const any_vec2<T> operator+(const any_vec2<T>& other) const { return any_vec2<T>({x + other.x, y + other.y}); };
    const any_vec2<T> operator-(const any_vec2<T>& other) const { return any_vec2<T>({x - other.x, y - other.y}); };
  };

  typedef any_vec2<int>               int_vec2;
  typedef any_vec2<int>               int_point;
  typedef any_vec2<std::uint32_t>     uint_point;
  typedef any_vec2<float>             float_point;

  typedef any_vec2<float>             float_vec2;


  // 3 somethings

  template<class T>
  struct any_vec3 {
    union { T x; T r; };
    union { T y; T g; };
    union { T z; T b; };
    bool operator==(const any_vec3<T>& other) const { return x == other.x && y == other.y && z == other.z; };
    T& operator[](std::size_t i) const { return *(T *)((std::byte *)this + (i * sizeof(T))); };
  };

  typedef any_vec3<int>     int_vec3;

  typedef any_vec3<float> float_vec3;
  typedef any_vec3<float> scale;
  typedef any_vec3<float> rotation;

  // 4 somethings

  template<class T>
  struct any_vec4 {
    union { T x; T r; };
    union { T y; T g; };
    union { T z; T b; };
    union { T w; T a; };
    bool operator==(const any_vec4<T>& other) const { return x == other.x && y == other.y && z == other.z && w == other.w; };
    T& operator[](std::size_t i) const { return *(T *)((std::byte *)this + (i * sizeof(T))); };
  };

  typedef any_vec4<int>     int_vec4;

  typedef any_vec4<float> float_vec4;
  typedef any_vec4<float> coord;

  // compound structs

  struct int_box
  {
    int_point p1, p2;

    bool operator==(const int_box&   other) const { return p1 == other.p1 && p2 == other.p2; }

    int_box zoom(float z, const int_point about) const noexcept
    {
      // shift to 0,0
      // apply zoom
      // shift back

      int_point tp1{ p1.x - about.x, p1.y - about.y };
      int_point tp2{ p2.x - about.x, p2.y - about.y };

      tp1.x *= z;
      tp1.y *= z;
      tp2.x *= z;
      tp2.y *= z;

      int_box r{{ tp1.x + about.x, tp1.y + about.y }, {  tp2.x + about.x, tp2.y + about.y }};

      return r;
    }

    int width()  const noexcept { return p2.x - p1.x; }
    int height() const noexcept { return p2.y - p1.y; }

    int_point middle() const noexcept { return {p1.x + width()/2, p1.y + height()/2}; }

    void shift(int x, int y) noexcept
    {
      p1.x += x;
      p1.y += y;
      p2.x += x;
      p2.y += y;
    }
  };

  struct uint_box
  {
    uint_point  p1, p2;

    bool operator==(const uint_box&  other) const { return p1 == other.p1 && p2 == other.p2; }

  };

  struct float_box { float_point p1, p2; bool operator==(const float_box& other) const { return p1 == other.p1 && p2 == other.p2; } };


  // matrix operations

  void matrix44_mult(std::array<float, 16>& r, const std::array<float, 16>& a, const std::array<float, 16>& b)
  {
    r[0]  = a[0] * b[0] +  a[4] * b[1]  + a[8]  * b[2]  + a[12] * b[3];
    r[1]  = a[1] * b[0] +  a[5] * b[1]  + a[9]  * b[2]  + a[13] * b[3];
    r[2]  = a[2] * b[0] +  a[6] * b[1]  + a[10] * b[2]  + a[14] * b[3];
    r[3]  = a[3] * b[0] +  a[7] * b[1]  + a[11] * b[2]  + a[15] * b[3];

    r[4]  = a[0] * b[4] +  a[4] * b[5]  + a[8]  * b[6]  + a[12] * b[7];
    r[5]  = a[1] * b[4] +  a[5] * b[5]  + a[9]  * b[6]  + a[13] * b[7];
    r[6]  = a[2] * b[4] +  a[6] * b[5]  + a[10] * b[6]  + a[14] * b[7];
    r[7]  = a[3] * b[4] +  a[7] * b[5]  + a[11] * b[6]  + a[15] * b[7];

    r[8]  = a[0] * b[8]  + a[4] * b[9]  + a[8]  * b[10] + a[12] * b[11];
    r[9]  = a[1] * b[8]  + a[5] * b[9]  + a[9]  * b[10] + a[13] * b[11];
    r[10] = a[2] * b[8]  + a[6] * b[9]  + a[10] * b[10] + a[14] * b[11];
    r[11] = a[3] * b[8]  + a[7] * b[9]  + a[11] * b[10] + a[15] * b[11];

    r[12] = a[0] * b[12] + a[4] * b[13] + a[8]  * b[14] + a[12] * b[15];
    r[13] = a[1] * b[12] + a[5] * b[13] + a[9]  * b[14] + a[13] * b[15];
    r[14] = a[2] * b[12] + a[6] * b[13] + a[10] * b[14] + a[14] * b[15];
    r[15] = a[3] * b[12] + a[7] * b[13] + a[11] * b[14] + a[15] * b[15];
  }


  void matrix44_shift(std::array<float, 16>& r, float x, float y, float z)
  {
    r[0]  = 1; r[1]  = 0; r[2]  = 0; r[3]  = 0;
    r[4]  = 0; r[5]  = 1; r[6]  = 0; r[7]  = 0;
    r[8]  = 0; r[9]  = 0; r[10] = 1; r[11] = 0;
    r[12] = x; r[13] = y; r[14] = z; r[15] = 1;
  }


  void matrix44_scale(std::array<float, 16>& r, float x, float y, float z)
  {
    r[0]  = x; r[1]  = 0; r[2]  = 0; r[3]  = 0;
    r[4]  = 0; r[5]  = y; r[6]  = 0; r[7]  = 0;
    r[8]  = 0; r[9]  = 0; r[10] = z; r[11] = 0;
    r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;
  }


  template<class T>
  float dist(T p1, T p2)
  {
    return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
  }


  template<class T>
  float_point mid(T p1, T p2)
  {
    return { (p1.x + p2.x)/2.0f, (p1.y + p2.y)/2.0f };
  }


  inline float_vec3 operator+ (const float_vec3& a, const float_vec3& b)
  {
    float_vec3 r = { {a.x + b.x}, {a.y + b.y}, {a.z + b.z} };

    return r;
  }


  inline float_vec3 operator- (const float_vec3& a, const float_vec3& b)
  {
    float_vec3 r = { {a.x - b.x}, {a.y - b.y}, {a.z - b.z} };

    return r;
  }


  inline float_vec3 operator / (const float_vec3& a, const float b)
  {
    float_vec3 r = { {a.x / b}, {a.y / b}, {a.z / b} };

    return r;
  }


  float_vec3 cross(const float_vec3& a, const float_vec3& b)
  {
    float_vec3 r;

    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;

    return r;
  }


  double dot(const float_vec3& a, const float_vec3& b)
  {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
  }


  void normalize(float_vec3& p)
  {
    /*
    double square = sqrt(p.x * p.x + p.y * p.y + p.z * p.z);

    if (square == 0) {
      p.x = 0;
      p.y = 0;
      p.z = 0;
    } else {
      p.x /= square;
      p.y /= square;
      p.z /= square;
    }
    */

    float b, c, xd;
    union {
      float   f;
      int     i;
    } a;

    xd = p.x * p.x;
    xd += p.y * p.y;
    xd += p.z * p.z;

    // fast invsqrt approx

    a.f = xd;
    a.i = 0x5F3759DF - (a.i >> 1);          //VRSQRTE
    c = xd * a.f;
    b = (float)((3.0f - c * a.f) * 0.5);    //VRSQRTS
    a.f = a.f * b;
    c = xd * a.f;
    b = (float)((3.0f - c * a.f) * 0.5);
    a.f = a.f * b;

    p.x = p.x * a.f;
    p.y = p.y * a.f;
    p.z = p.z * a.f;
  }


  float_vec3 triangle_normal(const float_vec3 &a, const float_vec3 &b, const float_vec3 &c)
  {
    float_vec3 lv1 = b - a;
    float_vec3 lv2 = c - a;

    float_vec3 r = cross(lv1, lv2);
    normalize(r);

    return r;
  }


} // namespace gpu

} // namespace plate
