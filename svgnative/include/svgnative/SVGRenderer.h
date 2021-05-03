/*
Copyright 2019 Adobe. All rights reserved.
This file is licensed to you under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License. You may obtain a copy
of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under
the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
OF ANY KIND, either express or implied. See the License for the specific language
governing permissions and limitations under the License.
*/

#ifndef SVGViewer_SVGRenderer_h
#define SVGViewer_SVGRenderer_h

#include "Config.h"

#include <array>
#include <boost/variant.hpp>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace SVGNative
{
/**
 * Supported image encoding formats are PNG and JPEG.
 * The assumed encoding format based on the base64 string.
 */
enum class ImageEncoding
{
    kPNG,
    kJPEG
};

/**
 * Line caps as described in:
 * https://www.w3.org/TR/SVG2/painting.html#LineCaps
 */
enum class LineCap
{
    kButt,
    kRound,
    kSquare
};

/**
 * Line joins as described in:
 * https://www.w3.org/TR/SVG2/painting.html#LineJoin
 */
enum class LineJoin
{
    kMiter,
    kRound,
    kBevel
};

/**
 * Winding rules as described in:
 * https://www.w3.org/TR/SVG2/painting.html#WindingRule
 */
enum class WindingRule
{
    kNonZero,
    kEvenOdd
};

/**
 * Gradient type. SVG Native supports the 2 gradient types
 * * linear gradient and
 * * radial gradient.
 */
enum class GradientType
{
    kLinearGradient,
    kRadialGradient
};

/**
 * Gradient spread method.
 * * pad
 * * reflect
 * * repeat
 *
 * @note See https://www.w3.org/TR/SVG11/pservers.html#LinearGradientElementSpreadMethodAttribute
 */
enum class SpreadMethod
{
    kPad,
    kReflect,
    kRepeat
};

struct Gradient;
class Transform;
class Path;
class Shape;

using Color = std::array<float, 4>;
using Paint = boost::variant<Color, Gradient>;
using ColorStop = std::pair<float, Color>;
using ColorMap = std::map<std::string, Color>;

class Interval
{
  public:
    Interval(): isEmpty(true) {}
    Interval(float u): a(u), b(u), isEmpty(false) {}
    Interval(float u, float v)
    {
      isEmpty = false;
      if (u <= v)
      {
        a = u;
        b = v;
      }
      else
      {
        a = v;
        b = u;
      }
    }
    float Min() { return a; }
    float Max() { return b; }
    operator bool()
    {
      return !isEmpty;
    }
    Interval operator&(Interval other)
    {
      if (!(*this) || !(other))
        return Interval();
      else
      {
        float a = (std::max)(this->Min(), other.Min());
        float b = (std::min)(this->Max(), other.Max());
        if (a <= b)
          return Interval(a, b);
        else
          return Interval();
      }
    }
    bool contains(Interval other)
    {
        return this->Min() <= other.Min() && this->Max() >= other.Max();
    }
  private:
    float a = std::numeric_limits<float>::quiet_NaN();
    float b = std::numeric_limits<float>::quiet_NaN();
    bool isEmpty = true;
};

struct Rect
{
    Rect() = default;
    Rect(float aX, float aY, float aWidth, float aHeight)
    {
      x = aX;
      y = aY;
      width = aWidth;
      height = aHeight;
      if (width != 0 && height != 0)
        isEmpty = false;
    }
    operator bool(){ return !isEmpty; }
    std::tuple<Interval, Interval> intervals()
    {
      double x0, y0, x1, y1;
      x0 = x;
      y0 = y;
      x1 = x0 + width - 1;
      y1 = y0 + height - 1;
      return std::tuple<Interval, Interval>(Interval(x0, x1), Interval(y0, y1));
    }
    Rect operator&(Rect other){
      std::tuple<Interval, Interval> intervals_a = intervals();
      std::tuple<Interval, Interval> intervals_b = other.intervals();
      Interval resultant_x = std::get<0>(intervals_a) & std::get<0>(intervals_b);
      Interval resultant_y = std::get<1>(intervals_a) & std::get<1>(intervals_b);
      if (!resultant_x || !resultant_y)
        return Rect{};
      float x0, y0, x1, y1;
      x0 = resultant_x.Min();
      y0 = resultant_y.Min();
      x1 = resultant_x.Max();
      y1 = resultant_y.Max();
      return Rect{x0, y0, (x1 - x0 + 1), (y1 - y0 + 1)};
    }
    bool contains(Rect other)
    {
      std::tuple<Interval, Interval> intervals_a = intervals();
      std::tuple<Interval, Interval> intervals_b = other.intervals();
      Interval a_x = std::get<0>(intervals_a);
      Interval a_y = std::get<1>(intervals_a);
      Interval b_x = std::get<0>(intervals_b);
      Interval b_y = std::get<1>(intervals_b);
      return (a_x.contains(b_x) && a_y.contains(b_y));
    }
    float x = std::numeric_limits<float>::quiet_NaN();
    float y = std::numeric_limits<float>::quiet_NaN();
    float width = std::numeric_limits<float>::quiet_NaN();
    float height = std::numeric_limits<float>::quiet_NaN();
    bool isEmpty = true;
};

/**
 * Representation of a linear gradient paint server.
 */
struct Gradient
{
    GradientType type = GradientType::kLinearGradient;
    SpreadMethod method = SpreadMethod::kPad;
    std::vector<ColorStop> colorStops; /** Color stops with offset-color pairs **/
    float x1 = std::numeric_limits<float>::quiet_NaN(); /** x1 for linearGradient **/
    float y1 = std::numeric_limits<float>::quiet_NaN(); /** y1 for linearGradient **/
    float x2 = std::numeric_limits<float>::quiet_NaN(); /** x2 for linearGradient **/
    float y2 = std::numeric_limits<float>::quiet_NaN(); /** y2 for linearGradient **/
    float cx = std::numeric_limits<float>::quiet_NaN(); /** cx for radialGradient **/
    float cy = std::numeric_limits<float>::quiet_NaN(); /** cy for radialGradient **/
    float fx = std::numeric_limits<float>::quiet_NaN(); /** fx for radialGradient **/
    float fy = std::numeric_limits<float>::quiet_NaN(); /** fy for radialGradient **/
    float r = std::numeric_limits<float>::quiet_NaN(); /** r for radialGradient **/
    std::shared_ptr<Transform> transform; /** Joined transformation matrix based to the "transform" attribute. **/
};

/**
 * Stroke style information.
 */
struct StrokeStyle
{
    bool hasStroke = false;
    float strokeOpacity = 1.0;
    float lineWidth = 1.0;
    LineCap lineCap = LineCap::kButt;
    LineJoin lineJoin = LineJoin::kMiter;
    float miterLimit = 4.0;
    std::vector<float> dashArray;
    float dashOffset = 0.0;
    Paint paint = Color{{0, 0, 0, 1.0}};
};

/**
 * Fill style information.
 */
struct FillStyle
{
    bool hasFill = true;
    WindingRule fillRule = WindingRule::kNonZero;
    float fillOpacity = 1.0;
    Paint paint = Color{{0, 0, 0, 1.0}};
};

/**
 * Representation of a 2D affine transform with 6 values.
 */
class Transform
{
public:
    virtual ~Transform() = default;

    virtual void Set(float a, float b, float c, float d, float tx, float ty) = 0;
    virtual void Rotate(float r) = 0;
    virtual void Translate(float tx, float ty) = 0;
    virtual void Scale(float sx, float sy) = 0;
    virtual void Concat(float a, float b, float c, float d, float tx, float ty) = 0;
};

struct ClippingPath
{
    ClippingPath(bool aHasClipContent, WindingRule aClipRule, std::shared_ptr<Path> aPath, std::shared_ptr<Transform> aTransform)
        : hasClipContent{aHasClipContent}
        , clipRule{aClipRule}
        , path{aPath}
        , transform{aTransform}
    {}

    bool hasClipContent = false;
    WindingRule clipRule = WindingRule::kNonZero;
    std::shared_ptr<Path> path; /** Clipping path. **/
    std::shared_ptr<Transform> transform; /** Joined transformation matrix based to the "transform" attribute. **/
};

/**
 * All compositing related properties. With the exception of the
 */
struct GraphicStyle
{
    // Add blend modes and other graphic style options here.
    float opacity = 1.0; /** Corresponds to the "opacity" CSS property. **/
    std::shared_ptr<Transform> transform; /** Joined transformation matrix based to the "transform" attribute. **/
    std::shared_ptr<ClippingPath> clippingPath;
};

/**
 * A presentation of a path.
 */
class Path
{
public:
    virtual ~Path() = default;

    virtual void Rect(float x, float y, float width, float height) = 0;
    virtual void RoundedRect(float x, float y, float width, float height, float cornerRadiusX, float cornerRadiusY) = 0;
    virtual void Ellipse(float cx, float cy, float rx, float ry) = 0;

    virtual void MoveTo(float x, float y) = 0;
    virtual void LineTo(float x, float y) = 0;
    virtual void CurveTo(float x1, float y1, float x2, float y2, float x3, float y3) = 0;
    virtual void CurveToV(float x2, float y2, float x3, float y3) = 0;
    virtual void ClosePath() = 0;
};

/**
 * An image object generated from a base64 string.
 * The port needs to decode the Base64 string and provide
 * information about the dimensions of the image.
 **/
class ImageData
{
public:
    virtual ~ImageData() = default;

    virtual float Width() const = 0;
    virtual float Height() const = 0;
};

/**
 * Base class for deriving, platform dependent renderer classes with immediate
 * graphic library calls.
 */
class SVGRenderer
{
public:
    virtual ~SVGRenderer() = default;

    virtual std::unique_ptr<ImageData> CreateImageData(const std::string& base64, ImageEncoding) = 0;
    virtual std::unique_ptr<Path> CreatePath() = 0;
    virtual std::unique_ptr<Transform> CreateTransform(
        float a = 1.0, float b = 0.0, float c = 0.0, float d = 1.0, float tx = 0.0, float ty = 0.0) = 0;

    virtual void Save(const GraphicStyle& graphicStyle) = 0;
    virtual void Restore() = 0;
    virtual void Reset() {}

    virtual void DrawPath(
        const Path& path, const GraphicStyle& graphicStyle, const FillStyle& fillStyle, const StrokeStyle& strokeStyle) = 0;
    virtual void DrawImage(const ImageData& image, const GraphicStyle& graphicStyle, const Rect& clipArea, const Rect& fillArea) = 0;

    virtual Rect PathBounds(const Path& path, const GraphicStyle& graphicStyle, const FillStyle& fillStyle, const StrokeStyle& strokeStyle)
    {
      return Rect{};
    }
};

class SaveRestoreHelper
{
public:
    SaveRestoreHelper(std::weak_ptr<SVGRenderer> renderer, const GraphicStyle& graphicStyle)
        : mRenderer{renderer}
    {
        if (auto renderer = mRenderer.lock())
            renderer->Save(graphicStyle);
    }

    ~SaveRestoreHelper()
    {
        if (auto renderer = mRenderer.lock())
            renderer->Restore();
    }
private:
    std::weak_ptr<SVGRenderer> mRenderer{};
};

} // namespace SVGNative

#endif // SVGViewer_SVGRenderer_h
