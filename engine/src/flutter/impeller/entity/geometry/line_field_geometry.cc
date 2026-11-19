// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/entity/geometry/line_field_geometry.h"

#include <algorithm>
#include <vector>

#include "impeller/core/buffer_view.h"
#include "impeller/core/formats.h"
#include "impeller/core/vertex_buffer.h"
#include "impeller/entity/geometry/line_geometry.h"

namespace impeller {

LineFieldGeometry::LineFieldGeometry(const Point* points,
                                     size_t point_count,
                                     bool connected,
                                     const StrokeParameters& stroke)
    : points_(points),
      point_count_(point_count),
      connected_(connected),
      width_(stroke.width),
      cap_(stroke.cap) {
  FML_DCHECK(width_ >= 0);
}

LineFieldGeometry::~LineFieldGeometry() = default;

size_t LineFieldGeometry::GetSegmentCount() const {
  if (connected_) {
    return point_count_ > 0 ? point_count_ - 1 : 0;
  }
  return point_count_ / 2;
}

Scalar LineFieldGeometry::ComputeAlphaCoverage(const Matrix& entity) const {
  return Geometry::ComputeStrokeAlphaCoverage(entity, width_);
}

GeometryResult LineFieldGeometry::GetPositionBuffer(
    const ContentContext& renderer,
    const Entity& entity,
    RenderPass& pass) const {
  const size_t segment_count = GetSegmentCount();
  if (segment_count == 0) {
    return kEmptyResult;
  }

  const Matrix& transform = entity.GetTransform();
  const Scalar radius = LineGeometry::ComputePixelHalfWidth(transform, width_);
  const size_t step = connected_ ? 1 : 2;

  // Every segment is a triangle strip of its own. They are joined into one
  // strip by repeating the last vertex of one segment and the first vertex of
  // the next, which adds degenerate triangles that are never rasterized. This
  // could be optimized out if we switched to using primitive restart.
  std::vector<Point> vertices;
  vertices.reserve(segment_count * 6);
  auto begin_segment = [&vertices](const Point& first) {
    if (!vertices.empty()) {
      vertices.push_back(vertices.back());
      vertices.push_back(first);
    }
  };

  for (size_t i = 0; i < segment_count; i++) {
    const Point p0 = points_[i * step];
    const Point p1 = points_[i * step + 1];

    if (cap_ == Cap::kRound) {
      Tessellator::EllipticalVertexGenerator generator =
          renderer.GetTessellator().RoundCapLine(transform, p0, p1, radius);
      FML_DCHECK(generator.GetTriangleType() == PrimitiveType::kTriangleStrip);
      bool is_first = true;
      generator.GenerateVertices(
          [&vertices, &begin_segment, &is_first](const Point& p) {
            if (is_first) {
              begin_segment(p);
              is_first = false;
            }
            vertices.push_back(p);
          });
      continue;
    }

    Point corners[4];
    if (!LineGeometry::ComputeCorners(corners, transform, cap_ == Cap::kSquare,
                                      p0, p1, width_)) {
      // A degenerate segment, such as one of zero length with butt caps.
      continue;
    }
    begin_segment(corners[0]);
    vertices.insert(vertices.end(), std::begin(corners), std::end(corners));
  }

  if (vertices.empty()) {
    return kEmptyResult;
  }

  BufferView buffer_view = renderer.GetTransientsDataBuffer().Emplace(
      vertices.data(), vertices.size() * sizeof(Point), alignof(Point));

  return GeometryResult{
      .type = PrimitiveType::kTriangleStrip,
      .vertex_buffer =
          VertexBuffer{
              .vertex_buffer = std::move(buffer_view),
              .index_buffer = {},
              .vertex_count = vertices.size(),
              .index_type = IndexType::kNone,
          },
      .transform = entity.GetShaderTransform(pass),
  };
}

// |Geometry|
std::optional<Rect> LineFieldGeometry::GetCoverage(
    const Matrix& transform) const {
  if (GetSegmentCount() == 0) {
    return std::nullopt;
  }
  // Every point that takes part in a segment, padded by half of the stroke
  // width in all directions. This is conservative: it also covers the extent
  // of round and square caps, which reach half a stroke width past the end
  // points, and it is not tight for butt caps.
  const size_t used = connected_ ? point_count_ : GetSegmentCount() * 2;
  Scalar left = points_[0].x;
  Scalar top = points_[0].y;
  Scalar right = points_[0].x;
  Scalar bottom = points_[0].y;
  for (size_t i = 1; i < used; i++) {
    const Point point = points_[i];
    left = std::min(left, point.x);
    top = std::min(top, point.y);
    right = std::max(right, point.x);
    bottom = std::max(bottom, point.y);
  }
  const Scalar radius = LineGeometry::ComputePixelHalfWidth(transform, width_);
  Rect coverage = Rect::MakeLTRB(left - radius, top - radius, right + radius,
                                 bottom + radius);
  return coverage.TransformBounds(transform);
}

}  // namespace impeller
