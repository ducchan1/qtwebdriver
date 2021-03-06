// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBDRIVER_WEBDRIVER_BASIC_TYPES_H_
#define WEBDRIVER_WEBDRIVER_BASIC_TYPES_H_

#include "value_conversion_traits.h"

namespace base {
class Value;
}

namespace webdriver {

/// @enum MouseButton button codes
enum MouseButton {
    kLeftButton = 0,
    kMiddleButton = 1,
    kRightButton = 2
};    

/// @enum StorageType storage types
enum StorageType {
    kLocalStorageType = 0,
    kSessionStorageType
};

///@enum PlayerState possible states
enum PlayerState{
    Stopped = 0,
    Playing = 1,
    Paused = 2
};

class Point {
public:
    Point();
    Point(double x, double y);
    ~Point();

    void Offset(double x, double y);
    void setX(double x);
    void setY(double y);

    double x() const;
    double y() const;
    int rounded_x() const;
    int rounded_y() const;

private:
    double x_, y_;
};

class Size {
public:
    Size();
    Size(double width, double height);
    ~Size();

    double width() const;
    double height() const;

private:
    double width_, height_;
};

class Rect {
public:
    Rect();
    Rect(double x, double y, double width, double height);
    Rect(const Point& origin, const Size& size);
    ~Rect();

    const Point& origin() const;
    const Size& size() const;
    double x() const;
    double y() const;
    double width() const;
    double height() const;

private:
    Point origin_;
    Size size_;
};

}  // namespace webdriver

template <>
struct ValueConversionTraits<webdriver::Point> {
    static base::Value* CreateValueFrom(const webdriver::Point& t);
    static bool SetFromValue(const base::Value* value, webdriver::Point* t);
    static bool CanConvert(const base::Value* value);
};

template <>
struct ValueConversionTraits<webdriver::Size> {
    static base::Value* CreateValueFrom(const webdriver::Size& t);
    static bool SetFromValue(const base::Value* value, webdriver::Size* t);
    static bool CanConvert(const base::Value* value);
};

template <>
struct ValueConversionTraits<webdriver::Rect> {
    static base::Value* CreateValueFrom(const webdriver::Rect& t);
    static bool SetFromValue(const base::Value* value, webdriver::Rect* t);
    static bool CanConvert(const base::Value* value);
};

#endif  // WEBDRIVER_WEBDRIVER_BASIC_TYPES_H_
