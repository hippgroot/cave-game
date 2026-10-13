#include "client/ui/layout.hpp"

#include <algorithm>

namespace ui {

namespace {

vec2 anchorFactor(Anchor anchor) {
  switch (anchor) {
  case Anchor::TopLeft:     return {0.0f, 0.0f};
  case Anchor::Top:         return {0.5f, 0.0f};
  case Anchor::TopRight:    return {1.0f, 0.0f};
  case Anchor::Left:        return {0.0f, 0.5f};
  case Anchor::Center:      return {0.5f, 0.5f};
  case Anchor::Right:       return {1.0f, 0.5f};
  case Anchor::BottomLeft:  return {0.0f, 1.0f};
  case Anchor::Bottom:      return {0.5f, 1.0f};
  case Anchor::BottomRight: return {1.0f, 1.0f};
  }
  return {0.0f, 0.0f};
}

f32 alignOffset(Align align, f32 available, f32 extent) {
  switch (align) {
  case Align::Start:   return 0.0f;
  case Align::Center:  return (available - extent) * 0.5f;
  case Align::End:     return available - extent;
  case Align::Stretch: return 0.0f;
  }
  return 0.0f;
}

vec2 stackExtent(std::span<const vec2> extents, f32 spacing, u32 axis) {
  if (extents.empty()) return vec2(0.0f);

  const u32 cross = 1u - axis;
  vec2      total(0.0f);
  for (const vec2& extent : extents) {
    total[axis] += extent[axis];
    total[cross] = std::max(total[cross], extent[cross]);
  }
  total[axis] += spacing * static_cast<f32>(extents.size() - 1);
  return total;
}

vec2 uniformStackExtent(vec2 itemExtent, u32 count, f32 spacing, u32 axis) {
  if (count == 0) return vec2(0.0f);

  vec2 total  = itemExtent;
  total[axis] = itemExtent[axis] * static_cast<f32>(count) + spacing * static_cast<f32>(count - 1);
  return total;
}

std::vector<Rect> stack(const Rect& area, std::span<const vec2> extents, f32 spacing, Align align, u32 axis) {
  const u32 cross = 1u - axis;

  std::vector<Rect> rects;
  rects.reserve(extents.size());

  f32 cursor = area.min[axis];
  for (const vec2& extent : extents) {
    Rect rect;
    rect.min[axis] = cursor;
    rect.max[axis] = cursor + extent[axis];

    if (align == Align::Stretch) {
      rect.min[cross] = area.min[cross];
      rect.max[cross] = area.max[cross];
    } else {
      rect.min[cross] = area.min[cross] + alignOffset(align, area.size()[cross], extent[cross]);
      rect.max[cross] = rect.min[cross] + extent[cross];
    }

    rects.push_back(rect);
    cursor += extent[axis] + spacing;
  }

  return rects;
}

std::vector<Rect> uniformStack(const Rect& area, u32 count, f32 spacing, u32 axis) {
  if (count == 0) return {};

  vec2 extent  = area.size();
  extent[axis] = std::max(0.0f, (extent[axis] - spacing * static_cast<f32>(count - 1)) / static_cast<f32>(count));

  std::vector<vec2> extents(count, extent);
  return stack(area, extents, spacing, Align::Stretch, axis);
}

}

vec2 anchorPoint(const Rect& rect, Anchor anchor) {
  return rect.min + rect.size() * anchorFactor(anchor);
}

Rect anchor(const Rect& parent, Anchor anchor, vec2 extent, vec2 offset) {
  const vec2 position = anchorPoint(parent, anchor) - extent * anchorFactor(anchor) + offset;
  return Rect::fromSize(position, extent);
}

vec2 vstackExtent(std::span<const vec2> extents, f32 spacing) {
  return stackExtent(extents, spacing, 1u);
}

vec2 hstackExtent(std::span<const vec2> extents, f32 spacing) {
  return stackExtent(extents, spacing, 0u);
}

vec2 vstackExtent(vec2 itemExtent, u32 count, f32 spacing) {
  return uniformStackExtent(itemExtent, count, spacing, 1u);
}

vec2 hstackExtent(vec2 itemExtent, u32 count, f32 spacing) {
  return uniformStackExtent(itemExtent, count, spacing, 0u);
}

std::vector<Rect> vstack(const Rect& area, std::span<const vec2> extents, f32 spacing, Align align) {
  return stack(area, extents, spacing, align, 1u);
}

std::vector<Rect> hstack(const Rect& area, std::span<const vec2> extents, f32 spacing, Align align) {
  return stack(area, extents, spacing, align, 0u);
}

std::vector<Rect> vstack(const Rect& area, u32 count, f32 spacing) {
  return uniformStack(area, count, spacing, 1u);
}

std::vector<Rect> hstack(const Rect& area, u32 count, f32 spacing) {
  return uniformStack(area, count, spacing, 0u);
}

}
