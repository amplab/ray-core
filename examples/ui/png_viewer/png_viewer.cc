// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/environment/scoped_chromium_init.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/run_application.h"
#include "mojo/ui/content_viewer_app.h"
#include "mojo/ui/ganesh_view.h"
#include "mojo/ui/view_provider_app.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "ui/gfx/codec/png_codec.h"

namespace examples {

namespace {
constexpr uint32_t kContentImageResourceId = 1;
constexpr uint32_t kRootNodeId = mojo::gfx::composition::kSceneRootNodeId;
}  // namespace

class PNGView : public mojo::ui::GaneshView {
 public:
  PNGView(mojo::InterfaceHandle<mojo::ApplicationConnector> app_connector,
          mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
          const sk_sp<SkImage>& image)
      : GaneshView(app_connector.Pass(),
                   view_owner_request.Pass(),
                   "PNGViewer"),
        image_(image) {
    DCHECK(image_);
  }

  ~PNGView() override {}

 private:
  // |GaneshView|:
  void OnDraw() override {
    DCHECK(properties());

    auto update = mojo::gfx::composition::SceneUpdate::New();

    const mojo::Size& size = *properties()->view_layout->size;
    if (size.width > 0 && size.height > 0) {
      mojo::RectF bounds;
      bounds.width = size.width;
      bounds.height = size.height;

      mojo::gfx::composition::ResourcePtr content_resource =
          ganesh_renderer()->DrawCanvas(
              size, base::Bind(&PNGView::DrawContent, base::Unretained(this)));
      DCHECK(content_resource);
      update->resources.insert(kContentImageResourceId,
                               content_resource.Pass());

      auto root_node = mojo::gfx::composition::Node::New();
      root_node->op = mojo::gfx::composition::NodeOp::New();
      root_node->op->set_image(mojo::gfx::composition::ImageNodeOp::New());
      root_node->op->get_image()->content_rect = bounds.Clone();
      root_node->op->get_image()->image_resource_id = kContentImageResourceId;
      update->nodes.insert(kRootNodeId, root_node.Pass());
    } else {
      auto root_node = mojo::gfx::composition::Node::New();
      update->nodes.insert(kRootNodeId, root_node.Pass());
    }

    scene()->Update(update.Pass());
    scene()->Publish(CreateSceneMetadata());
  }

  void DrawContent(const mojo::skia::GaneshContext::Scope& ganesh_scope,
                   const mojo::Size& size,
                   SkCanvas* canvas) {
    canvas->clear(SK_ColorBLACK);

    int32_t w, h;
    if (size.width * image_->height() < size.height * image_->width()) {
      w = size.width;
      h = image_->height() * size.width / image_->width();
    } else {
      w = image_->width() * size.height / image_->height();
      h = size.height;
    }
    canvas->drawImageRect(
        image_.get(), SkRect::MakeWH(image_->width(), image_->height()),
        SkRect::MakeXYWH((size.width - w) / 2, (size.height - h) / 2, w, h),
        nullptr);
  }

  sk_sp<SkImage> image_;

  DISALLOW_COPY_AND_ASSIGN(PNGView);
};

class PNGContentViewProviderApp : public mojo::ui::ViewProviderApp {
 public:
  PNGContentViewProviderApp(sk_sp<SkImage> image) : image_(image) {
    DCHECK(image_);
  }

  ~PNGContentViewProviderApp() override {}

  void CreateView(
      const std::string& connection_url,
      mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
      mojo::InterfaceRequest<mojo::ServiceProvider> services) override {
    new PNGView(mojo::CreateApplicationConnector(shell()),
                view_owner_request.Pass(), image_);
  }

 private:
  sk_sp<SkImage> image_;

  DISALLOW_COPY_AND_ASSIGN(PNGContentViewProviderApp);
};

class PNGContentViewerApp : public mojo::ui::ContentViewerApp {
 public:
  PNGContentViewerApp() {}

  ~PNGContentViewerApp() override {}

  mojo::ui::ViewProviderApp* LoadContent(
      const std::string& content_handler_url,
      mojo::URLResponsePtr response) override {
    std::string data;
    mojo::common::BlockingCopyToString(response->body.Pass(), &data);
    SkBitmap bitmap;
    if (!::gfx::PNGCodec::Decode(
            reinterpret_cast<const unsigned char*>(data.data()), data.length(),
            &bitmap) ||
        bitmap.empty()) {
      LOG(ERROR) << "Could not decode PNG.";
      return nullptr;
    }

    return new PNGContentViewProviderApp(SkImage::MakeFromBitmap(bitmap));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(PNGContentViewerApp);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  examples::PNGContentViewerApp png_content_viewer_app;
  return mojo::RunApplication(application_request, &png_content_viewer_app);
}
