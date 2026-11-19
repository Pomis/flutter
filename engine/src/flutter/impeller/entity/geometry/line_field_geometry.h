// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_IMPELLER_ENTITY_GEOMETRY_LINE_FIELD_GEOMETRY_H_
#define FLUTTER_IMPELLER_ENTITY_GEOMETRY_LINE_FIELD_GEOMETRY_H_

#include "impeller/entity/geometry/geometry.h"

namespace impeller {

/// @brief A geometry that draws many line segments which share one set of
///        stroke parameters as a single vertex buffer, in the same way
///        that |PointFieldGeometry| draws many points.
///
///        The segments are independent of each other. Each one is
///        tessellated exactly as a |LineGeometry| would tessellate it, with
///        its own caps, and segments that overlap are blended on top of each
///        other just as they would be if they were drawn one at a time.
class LineFieldGeometry final : public Geometry {
 public:
  /// @param points       The end points of the segments. Must outlive this
  ///                     geometry.
  /// @param point_count  The number of entries in `points`.
  /// @param connected    If false, each pair of points `(0, 1), (2, 3), ...`
  ///                     is a segment and a trailing unpaired point is
  ///                     ignored. If true, each point is connected to the
  ///                     next one: `(0, 1), (1, 2), ...`.
  /// @param stroke       The stroke parameters shared by all segments. Only
  ///                     the width and the cap are used.
  LineFieldGeometry(const Point* points,
                    size_t point_count,
                    bool connected,
                    const StrokeParameters& stroke);

  ~LineFieldGeometry() override;

  /// @brief The number of segments described by the points.
  size_t GetSegmentCount() const;

  // |Geometry|
  Scalar ComputeAlphaCoverage(const Matrix& transform) const override;

  // |Geometry|
  std::optional<Rect> GetCoverage(const Matrix& transform) const override;

 private:
  // |Geometry|
  GeometryResult GetPositionBuffer(const ContentContext& renderer,
                                   const Entity& entity,
                                   RenderPass& pass) const override;

  const Point* points_;
  size_t point_count_;
  bool connected_;
  Scalar width_;
  Cap cap_;

  LineFieldGeometry(const LineFieldGeometry&) = delete;

  LineFieldGeometry& operator=(const LineFieldGeometry&) = delete;
};

}  // namespace impeller

#endif  // FLUTTER_IMPELLER_ENTITY_GEOMETRY_LINE_FIELD_GEOMETRY_H_
