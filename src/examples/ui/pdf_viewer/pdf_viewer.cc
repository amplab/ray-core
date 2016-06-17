// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
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
#include "mojo/ui/input_handler.h"
#include "mojo/ui/view_provider_app.h"
#include "third_party/pdfium/fpdfsdk/include/fpdf_ext.h"
#include "third_party/pdfium/fpdfsdk/include/fpdfview.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "v8/include/v8.h"

namespace examples {

namespace {
constexpr uint32_t kContentImageResourceId = 1;
constexpr uint32_t kRootNodeId = mojo::gfx::composition::kSceneRootNodeId;
}  // namespace

class PDFLibrary;

class PDFDocument {
 public:
  PDFDocument(const std::shared_ptr<PDFLibrary>& pdf_library,
              FPDF_DOCUMENT doc,
              const std::string& data)
      : pdf_library_(pdf_library),
        doc_(doc),
        data_(data),
        page_count_(FPDF_GetPageCount(doc_)) {}

  ~PDFDocument() { FPDF_CloseDocument(doc_); }

  uint32_t page_count() { return page_count_; }

  sk_sp<SkImage> DrawPage(int page_index, const mojo::Size& size) {
    FPDF_PAGE page = FPDF_LoadPage(doc_, page_index);

    double width_pts = FPDF_GetPageWidth(page);
    double height_pts = FPDF_GetPageHeight(page);

    int width, height;
    if (size.width * height_pts < size.height * width_pts) {
      width = size.width;
      height = height_pts * size.width / width_pts;
    } else {
      width = width_pts * size.height / height_pts;
      height = size.height;
    }

    int stride = width * 4;
    sk_sp<SkData> pixels = SkData::MakeUninitialized(stride * height);
    DCHECK(pixels);

    FPDF_BITMAP bitmap = FPDFBitmap_CreateEx(width, height, FPDFBitmap_BGRA,
                                             pixels->writable_data(), stride);
    FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF);
    FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, 0);
    FPDFBitmap_Destroy(bitmap);

    FPDF_ClosePage(page);

    SkImageInfo info = SkImageInfo::Make(width, height, kBGRA_8888_SkColorType,
                                         kOpaque_SkAlphaType);
    return SkImage::MakeRasterData(info, pixels, stride);
  }

 private:
  std::shared_ptr<PDFLibrary> pdf_library_;
  FPDF_DOCUMENT doc_;
  std::string data_;
  uint32_t page_count_;
};

class PDFLibrary : public std::enable_shared_from_this<PDFLibrary> {
 public:
  PDFLibrary() {
    v8::V8::InitializeICU();
    FPDF_InitLibrary();
  }

  ~PDFLibrary() { FPDF_DestroyLibrary(); }

  std::shared_ptr<PDFDocument> Decode(mojo::URLResponsePtr response) {
    std::string data;
    mojo::common::BlockingCopyToString(response->body.Pass(), &data);
    if (data.length() >= static_cast<size_t>(std::numeric_limits<int>::max()))
      return nullptr;

    FPDF_DOCUMENT doc = FPDF_LoadMemDocument(
        data.data(), static_cast<int>(data.length()), nullptr);
    if (!doc)
      return nullptr;

    return std::make_shared<PDFDocument>(shared_from_this(), doc, data);
  }
};

class PDFDocumentView : public mojo::ui::GaneshView,
                        public mojo::ui::InputListener {
 public:
  PDFDocumentView(
      mojo::InterfaceHandle<mojo::ApplicationConnector> app_connector,
      mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
      const std::shared_ptr<PDFDocument>& pdf_document)
      : GaneshView(app_connector.Pass(),
                   view_owner_request.Pass(),
                   "PDFDocumentViewer"),
        pdf_document_(pdf_document),
        input_handler_(GetViewServiceProvider(), this) {
    DCHECK(pdf_document_);
  }

  ~PDFDocumentView() override {}

 private:
  // |InputListener|:
  void OnEvent(mojo::EventPtr event, const OnEventCallback& callback) override {
    if (event->key_data && (event->action != mojo::EventType::KEY_PRESSED ||
                            event->key_data->is_char)) {
      callback.Run(false);
      return;
    }

    if ((event->key_data &&
         event->key_data->windows_key_code == mojo::KeyboardCode::DOWN) ||
        (event->pointer_data && event->pointer_data->vertical_wheel < 0)) {
      GotoNextPage();
      callback.Run(true);
      return;
    }

    if ((event->key_data &&
         event->key_data->windows_key_code == mojo::KeyboardCode::UP) ||
        (event->pointer_data && event->pointer_data->vertical_wheel > 0)) {
      GotoPreviousPage();
      callback.Run(true);
      return;
    }

    callback.Run(false);
  }

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
              size, base::Bind(&PDFDocumentView::DrawContent,
                               base::Unretained(this)));
      DCHECK(content_resource);
      update->resources.insert(kContentImageResourceId,
                               content_resource.Pass());

      auto root_node = mojo::gfx::composition::Node::New();
      root_node->hit_test_behavior =
          mojo::gfx::composition::HitTestBehavior::New();
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
    if (!cached_image_ || !cached_size_.Equals(size)) {
      cached_image_ = pdf_document_->DrawPage(page_, size);
      cached_size_ = size;
    }

    if (cached_image_) {
      canvas->clear(SK_ColorBLACK);
      canvas->drawImageRect(
          cached_image_.get(),
          SkRect::MakeWH(cached_image_->width(), cached_image_->height()),
          SkRect::MakeXYWH((size.width - cached_image_->width()) / 2,
                           (size.height - cached_image_->height()) / 2,
                           cached_image_->width(), cached_image_->height()),
          nullptr);
    } else {
      canvas->clear(SK_ColorBLUE);
    }
  }

  void GotoNextPage() {
    if (page_ + 1 < pdf_document_->page_count()) {
      page_++;
      Redraw();
    }
  }

  void GotoPreviousPage() {
    if (page_ > 0) {
      page_--;
      Redraw();
    }
  }

  void Redraw() {
    cached_image_.reset();
    Invalidate();
  }

  std::shared_ptr<PDFDocument> pdf_document_;
  uint32_t page_ = 0u;
  sk_sp<SkImage> cached_image_;
  mojo::Size cached_size_;

  mojo::ui::InputHandler input_handler_;

  DISALLOW_COPY_AND_ASSIGN(PDFDocumentView);
};

class PDFContentViewProviderApp : public mojo::ui::ViewProviderApp {
 public:
  PDFContentViewProviderApp(const std::shared_ptr<PDFLibrary>& pdf_library,
                            const std::shared_ptr<PDFDocument>& pdf_document)
      : pdf_library_(pdf_library), pdf_document_(pdf_document) {
    DCHECK(pdf_library_);
    DCHECK(pdf_document_);
  }

  ~PDFContentViewProviderApp() override {}

  void CreateView(
      const std::string& connection_url,
      mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
      mojo::InterfaceRequest<mojo::ServiceProvider> services) override {
    new PDFDocumentView(mojo::CreateApplicationConnector(shell()),
                        view_owner_request.Pass(), pdf_document_);
  }

 private:
  std::shared_ptr<PDFLibrary> pdf_library_;
  std::shared_ptr<PDFDocument> pdf_document_;

  DISALLOW_COPY_AND_ASSIGN(PDFContentViewProviderApp);
};

class PDFContentViewerApp : public mojo::ui::ContentViewerApp {
 public:
  PDFContentViewerApp() : pdf_library_(std::make_shared<PDFLibrary>()) {}

  ~PDFContentViewerApp() override {}

  mojo::ui::ViewProviderApp* LoadContent(
      const std::string& content_handler_url,
      mojo::URLResponsePtr response) override {
    std::shared_ptr<PDFDocument> pdf_document =
        pdf_library_->Decode(response.Pass());
    if (!pdf_document) {
      LOG(ERROR) << "Could not decode PDFDocument.";
      return nullptr;
    }

    return new PDFContentViewProviderApp(pdf_library_, pdf_document);
  }

 private:
  std::shared_ptr<PDFLibrary> pdf_library_;

  DISALLOW_COPY_AND_ASSIGN(PDFContentViewerApp);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ScopedChromiumInit init;
  examples::PDFContentViewerApp pdf_content_viewer_app;
  return mojo::RunApplication(application_request, &pdf_content_viewer_app);
}
