#pragma once

#include <cstdint>
#include <algorithm>
#include <math.h>

namespace plate {

class quaternion {

// quaternion maths reference: https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation
// interactive video lessons:  https://eater.net/quaternions

public:

  constexpr quaternion() noexcept
  { }


  constexpr quaternion(const float x, const float y, const float z, const float w) noexcept
  {
    x_ = x;
    y_ = y;
    z_ = z;
    w_ = w;

    normalized_ = false;
  }


  constexpr void set(const float x, const float y, const float z, const float w, const bool n = false) noexcept
  {
    x_ = x;
    y_ = y;
    z_ = z;
    w_ = w;

    normalized_ = n;
  }


  constexpr void set_to_default_angle() noexcept
  {
    set(0, 0, 0, 1);

    normalized_ = true;
  }


  constexpr void set_to_euler_angles(const float x, const float y, const float z) noexcept
  {
    quaternion qx, qy, qz;
    float t_x, t_y, t_z, t_w;

    t_x = (float)sin(x/2.0);
    t_y = 0;
    t_z = 0;
    t_w = (float)cos(x/2.0);
    qx.set(t_x, t_y, t_z, t_w);

    t_x = 0;
    t_y = (float)sin(y/2.0);
    t_z = 0;
    t_w = (float)cos(y/2.0);
    qy.set(t_x, t_y, t_z, t_w);

    t_x = 0;
    t_y = 0;
    t_z = (float)sin(z/2.0);
    t_w = (float)cos(z/2.0);
    qz.set(t_x, t_y, t_z, t_w);

    quaternion r = multiply(qy, qx);
    *this = multiply(qz, r);

    normalized_ = true;
  }


  constexpr void set_to_xy_angle(const float x, const float y) noexcept
  {

    auto a = std::sqrt(x*x + y*y); // magnitude of (x,y); angle of rotation

    if (a == 0.0) //if no movement
    {
      set_to_default_angle();

      return;
    }

    auto s = sin(a/2.0) / a; // ratio between sizes of quaternion and input vector 

    x_ = s * x;
    y_ = s * y;
    z_ = 0.0f;
    w_ = cos(a/2.0);

    return;
  }


  // rotate by a radians around the axis specified by the (x, y, z) unit vector
  // if it isn't a unit vector the result will not be normalized, so normalize() should be called manually
  constexpr void set_to_axis_angle(const float a, const float x, const float y, const float z) noexcept
  {
    double s = sin(a/2.0);

    x_ = x * s;
    y_ = y * s;
    z_ = z * s;
    w_ = cos(a/2.0);

    normalized_ = true;
  }


  // scale to unit quaternion (ie so x^2 + y^2 + z^2 + w^2 == 1)
  constexpr void normalize() noexcept
  {
    if (normalized_)
      return;

    auto mag = sqrt(x_ * x_ + y_ * y_ + z_ * z_ + w_ * w_);

    x_ /= mag;
    y_ /= mag;
    z_ /= mag;
    w_ /= mag;

    normalized_ = true;
  };


  // reverse angle
  constexpr void inverse() noexcept
  {
    w_ = -w_;
  }


  static constexpr quaternion multiply(quaternion const& lhs, quaternion const& rhs) noexcept
  {
    quaternion r;

    r.x_ = lhs.w_ * rhs.x_ + lhs.x_ * rhs.w_ + lhs.y_ * rhs.z_ - lhs.z_ * rhs.y_;      // w1x2 + x1w2 + y1z2 - z1y2
    r.y_ = lhs.w_ * rhs.y_ - lhs.x_ * rhs.z_ + lhs.y_ * rhs.w_ + lhs.z_ * rhs.x_;      // w1y2 - x1z2 + y1w2 + z1x2
    r.z_ = lhs.w_ * rhs.z_ + lhs.x_ * rhs.y_ - lhs.y_ * rhs.x_ + lhs.z_ * rhs.w_;      // w1z2 + x1y2 - y1x2 + z1w2
    r.w_ = lhs.w_ * rhs.w_ - lhs.x_ * rhs.x_ - lhs.y_ * rhs.y_ - lhs.z_ * rhs.z_;      // w1w2 - x1x2 - y1y2 - z1z2

    return r;
  }


  // angle between resulting orientations of applying the quaternions to the same orientation
  static constexpr float angle(const quaternion& lhs, const quaternion& rhs) noexcept
  {
    quaternion i = lhs;
    i.inverse();

    quaternion res = multiply(rhs, i);

    float r = (float)(acosf(res.w_) * 2.0);

    // Quaternions are a double-covering of orientations
    // This ensures the shortest of the two straight paths between them is taken
    if (r > (float)M_PI)
      r = (float)(2 * M_PI - r);

    return r;
  }


  constexpr void get_rotation_matrix(float* r) noexcept
  {
    normalize();

    double x2 = x_ * x_;
    double y2 = y_ * y_;
    double z2 = z_ * z_;
    double xy = x_ * y_;
    double xz = x_ * z_;
    double yz = y_ * z_;
    double wx = w_ * x_;
    double wy = w_ * y_;
    double wz = w_ * z_;

    r[0] = (float)(1.0 - 2.0 * (y2 + z2));
    r[1] = (float)(2.0 * (xy + wz));
    r[2] = (float)(2.0 * (xz - wy));
    r[3] = 0.0;

    r[4] = (float)(2.0 * (xy - wz));
    r[5] = (float)(1.0 - 2.0 * (x2 + z2));
    r[6] = (float)(2.0 * (yz + wx));
    r[7] = 0.0;

    r[8] = (float)(2.0 * (xz + wy));
    r[9] = (float)(2.0 * (yz - wx));
    r[10] = (float)(1.0 - 2.0 * (x2 + y2));
    r[11] = 0.0;

    r[12] = 0.0;
    r[13] = 0.0;
    r[14] = 0.0;
    r[15] = 1.0;
  }


  constexpr void get_euler_angles(float& x, float& y, float& z) noexcept
  {
    normalize();

    double t = 2.0 * ( w_ * y_ - z_ * x_ );

    x = (float)atan2( 2.0 * ( w_ * x_ + y_ * z_ ), 1.0 - 2.0 * ( (x_ * x_) + (y_ * y_) ) );

    if (t == 1.0)
      y = (float)M_PI/2.0;
    else
    {
      if (t == -1.0)
        y = -(float)M_PI/2.0;
      else
        y = (float)asin(t);
    }

    z = (float)atan2( 2.0 * ( w_ * z_ + x_ * y_ ), 1.0 - 2.0 * ( (y_ * y_) + (z_ * z_) ) );
  }

private:

  double x_, y_, z_, w_; // i, j, k, 1
  bool normalized_ = false;

};


} // namespace plate
