// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_GFX_COMPOSITOR_GRAPH_RESOURCES_H_
#define SERVICES_GFX_COMPOSITOR_GRAPH_RESOURCES_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "mojo/services/gfx/composition/interfaces/resources.mojom.h"
#include "services/gfx/compositor/render/render_image.h"

namespace compositor {

// Base class for resources in a scene graph.
//
// Instances of this class are immutable and reference counted so they may
// be shared by multiple versions of the same scene.
class Resource : public base::RefCounted<Resource> {
 public:
  enum class Type { kScene, kImage };

  Resource();

  // Gets the resource type.
  virtual Type type() const = 0;

 protected:
  friend class base::RefCounted<Resource>;
  virtual ~Resource();

 private:
  DISALLOW_COPY_AND_ASSIGN(Resource);
};

// A resource which represents a reference to a specified scene.
class SceneResource : public Resource {
 public:
  explicit SceneResource(const mojo::gfx::composition::SceneToken& scene_token);

  Type type() const override;

  const mojo::gfx::composition::SceneToken& scene_token() const {
    return scene_token_;
  }

 protected:
  ~SceneResource() override;

 private:
  mojo::gfx::composition::SceneToken scene_token_;

  DISALLOW_COPY_AND_ASSIGN(SceneResource);
};

// A resource which represents a refrence to a specified image.
class ImageResource : public Resource {
 public:
  explicit ImageResource(const scoped_refptr<RenderImage>& image);

  Type type() const override;

  // The referenced image, never null.
  const scoped_refptr<RenderImage>& image() const { return image_; }

 protected:
  ~ImageResource() override;

 private:
  scoped_refptr<RenderImage> image_;

  DISALLOW_COPY_AND_ASSIGN(ImageResource);
};

}  // namespace compositor

#endif  // SERVICES_GFX_COMPOSITOR_GRAPH_RESOURCES_H_
