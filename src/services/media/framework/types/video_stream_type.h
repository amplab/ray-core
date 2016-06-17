// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_MEDIA_FRAMEWORK_TYPES_VIDEO_STREAM_TYPE_H_
#define SERVICES_MEDIA_FRAMEWORK_TYPES_VIDEO_STREAM_TYPE_H_

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "base/logging.h"
#include "services/media/framework/types/stream_type.h"

namespace mojo {
namespace media {

// Describes the type of a video stream.
class VideoStreamType : public StreamType {
 public:
  enum class VideoProfile {
    kUnknown,
    kNotApplicable,
    kH264Baseline,
    kH264Main,
    kH264Extended,
    kH264High,
    kH264High10,
    kH264High422,
    kH264High444Predictive,
    kH264ScalableBaseline,
    kH264ScalableHigh,
    kH264StereoHigh,
    kH264MultiviewHigh
  };

  enum class PixelFormat {
    kUnknown,
    kI420,
    kYv12,
    kYv16,
    kYv12A,
    kYv24,
    kNv12,
    kNv21,
    kUyvy,
    kYuy2,
    kArgb,
    kXrgb,
    kRgb24,
    kRgb32,
    kMjpeg,
    kMt21
  };

  enum class ColorSpace {
    kUnknown,
    kNotApplicable,
    kJpeg,
    kHdRec709,
    kSdRec601
  };

  static const size_t kFrameSizeAlignment = 16;
  static const size_t kFrameSizePadding = 16;
  static const size_t kYPlaneIndex = 0;
  static const size_t kARGBPlaneIndex = 0;
  static const size_t kUPlaneIndex = 1;
  static const size_t kUVPlaneIndex = 1;
  static const size_t kVPlaneIndex = 2;
  static const size_t kAPlaneIndex = 3;
  static const size_t kMaxPlaneIndex = 3;

  // Width and height.
  struct Extent {
    Extent() : width_(0), height_(0) {}
    Extent(int width, int height) : width_(width), height_(height) {}
    size_t width() const { return width_; }
    size_t height() const { return height_; }

   private:
    size_t width_;
    size_t height_;
  };

  // Describes the layout of a frame of a particular extent.
  struct FrameLayout {
    size_t line_stride_for_plane(size_t plane) {
      DCHECK(plane < plane_count);
      return line_stride[plane];
    }

    size_t plane_offset_for_plane(size_t plane) {
      DCHECK(plane < plane_count);
      return plane_offset[plane];
    }

    size_t plane_count;
    size_t line_stride[kMaxPlaneIndex + 1];
    size_t plane_offset[kMaxPlaneIndex + 1];
    size_t size;
  };

  // Information regarding a pixel format.
  struct PixelFormatInfo {
    // Returns the number of bytes per element for the specified plane.
    size_t bytes_per_element_for_plane(size_t plane) const {
      DCHECK(plane < plane_count);
      return bytes_per_element[plane];
    }

    // Returns the sample size of the specified plane.
    const Extent& sample_size_for_plane(size_t plane) const {
      DCHECK(plane < plane_count);
      return sample_size[plane];
    }

    // Returns the row count for the specified plane.
    size_t RowCount(size_t plane, size_t height) const;

    // Returns the column count for the specified plane.
    size_t ColumnCount(size_t plane, size_t width) const;

    // Returns the number of bytes per row for the specified plane.
    size_t BytesPerRow(size_t plane, size_t width) const;

    // Calculates an aligned size from an unaligned size.
    Extent AlignedSize(const Extent& unaligned_size) const;

    // Determines a common alignment for all planes.
    Extent CommonAlignment() const;

    // Fills in a FrameLayout structure for the given coded size.
    void BuildFrameLayout(const Extent& coded_size,
                          FrameLayout* frame_layout) const;

    const size_t plane_count;
    const size_t bytes_per_element[kMaxPlaneIndex + 1];
    const Extent sample_size[kMaxPlaneIndex + 1];
  };

  // Gets information for the specified pixel format.
  static const PixelFormatInfo& InfoForPixelFormat(PixelFormat pixel_format);

  // Creates a VideoStreamType.
  static std::unique_ptr<StreamType> Create(
      const std::string& encoding,
      std::unique_ptr<Bytes> encoding_parameters,
      VideoProfile profile,
      PixelFormat pixel_format,
      ColorSpace color_space,
      uint32_t width,
      uint32_t height,
      uint32_t coded_width,
      uint32_t coded_height) {
    return std::unique_ptr<StreamType>(new VideoStreamType(
        encoding, std::move(encoding_parameters), profile, pixel_format,
        color_space, width, height, coded_width, coded_height));
  }

  VideoStreamType(const std::string& encoding,
                  std::unique_ptr<Bytes> encoding_parameters,
                  VideoProfile profile,
                  PixelFormat pixel_format,
                  ColorSpace color_space,
                  uint32_t width,
                  uint32_t height,
                  uint32_t coded_width,
                  uint32_t coded_height);

  ~VideoStreamType() override;

  const VideoStreamType* video() const override;

  VideoProfile profile() const { return profile_; }

  PixelFormat pixel_format() const { return pixel_format_; }

  ColorSpace color_space() const { return color_space_; }

  uint32_t width() const { return width_; }

  uint32_t height() const { return height_; }

  uint32_t coded_width() const { return coded_width_; }

  uint32_t coded_height() const { return coded_height_; }

  const PixelFormatInfo& GetPixelFormatInfo() const {
    return InfoForPixelFormat(pixel_format_);
  }

  std::unique_ptr<StreamType> Clone() const override;

 private:
  VideoProfile profile_;
  PixelFormat pixel_format_;
  ColorSpace color_space_;
  uint32_t width_;
  uint32_t height_;
  uint32_t coded_width_;
  uint32_t coded_height_;
};

// Describes a set of video stream types.
class VideoStreamTypeSet : public StreamTypeSet {
 public:
  static std::unique_ptr<StreamTypeSet> Create(
      const std::vector<std::string>& encodings,
      Range<uint32_t> width,
      Range<uint32_t> height) {
    return std::unique_ptr<StreamTypeSet>(
        new VideoStreamTypeSet(encodings, width, height));
  }

  VideoStreamTypeSet(const std::vector<std::string>& encodings,
                     Range<uint32_t> width,
                     Range<uint32_t> height);

  ~VideoStreamTypeSet() override;

  const VideoStreamTypeSet* video() const override;

  Range<uint32_t> width() const { return width_; }

  Range<uint32_t> height() const { return height_; }

  std::unique_ptr<StreamTypeSet> Clone() const override;

 private:
  Range<uint32_t> width_;
  Range<uint32_t> height_;
};

}  // namespace media
}  // namespace mojo

#endif  // SERVICES_MEDIA_FRAMEWORK_TYPES_VIDEO_STREAM_TYPE_H_
