// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/moterm/moterm_view.h"

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <algorithm>
#include <string>
#include <utility>

#include "apps/moterm/key_util.h"
#include "base/bind.h"
#include "base/logging.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/services/files/interfaces/file.mojom.h"
#include "mojo/services/files/interfaces/types.mojom.h"
#include "mojo/services/terminal/interfaces/terminal_client.mojom.h"
#include "mojo/skia/ganesh_texture_surface.h"
#include "third_party/dejavu-fonts-ttf-2.34/kDejaVuSansMonoRegular.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/core/SkXfermode.h"

constexpr uint32_t kMotermImageResourceId = 1;
constexpr uint32_t kRootNodeId = mojo::gfx::composition::kSceneRootNodeId;

MotermView::MotermView(
    mojo::InterfaceHandle<mojo::ApplicationConnector> app_connector,
    mojo::InterfaceRequest<mojo::ui::ViewOwner> view_owner_request,
    mojo::InterfaceRequest<mojo::ServiceProvider> service_provider_request)
    : GaneshView(app_connector.Pass(), view_owner_request.Pass(), "Moterm"),
      input_handler_(GetViewServiceProvider(), this),
      model_(MotermModel::Size(240, 160), MotermModel::Size(24, 80), this),
      force_next_draw_(false),
      ascent_(0),
      line_height_(0),
      advance_width_(0),
      keypad_application_mode_(false) {
  // TODO(vtl): |service_provider_impl_|'s ctor doesn't like an invalid request,
  // so we have to conditionally, explicitly bind.
  if (service_provider_request.is_pending()) {
    // TODO(vtl): The connection context should probably be plumbed here, which
    // means that mojo::ui::ViewProviderApp::CreateView() should probably have a
    // connection context argument.
    service_provider_impl_.Bind(mojo::ConnectionContext(),
                                service_provider_request.Pass());
    service_provider_impl_.AddService<mojo::terminal::Terminal>([this](
        const mojo::ConnectionContext& connection_context,
        mojo::InterfaceRequest<mojo::terminal::Terminal> terminal_request) {
      terminal_bindings_.AddBinding(this, terminal_request.Pass());
    });
  }

  regular_typeface_ = SkTypeface::MakeFromStream(
      new SkMemoryStream(font_data::kDejaVuSansMonoRegular.data,
                         font_data::kDejaVuSansMonoRegular.size));

  // TODO(vtl): This duplicates some code.
  SkPaint fg_paint;
  fg_paint.setTypeface(regular_typeface_);
  fg_paint.setTextSize(16);
  // Figure out appropriate metrics.
  SkPaint::FontMetrics fm = {};
  fg_paint.getFontMetrics(&fm);
  ascent_ = static_cast<int>(ceilf(-fm.fAscent));
  line_height_ = ascent_ + static_cast<int>(ceilf(fm.fDescent + fm.fLeading));
  DCHECK_GT(line_height_, 0);
  // To figure out the advance width, measure an X. Better hope the font is
  // monospace.
  advance_width_ = static_cast<int>(ceilf(fg_paint.measureText("X", 1)));
  DCHECK_GT(advance_width_, 0);
}

MotermView::~MotermView() {
  if (driver_)
    driver_->Detach();
}

void MotermView::OnEvent(mojo::EventPtr event,
                         const OnEventCallback& callback) {
  if (event->action == mojo::EventType::KEY_PRESSED) {
    OnKeyPressed(event.Pass());
    callback.Run(true);
  } else {
    callback.Run(false);
  }
}

void MotermView::OnResponse(const void* buf, size_t size) {
  if (driver_)
    driver_->SendData(buf, size);
}

void MotermView::OnSetKeypadMode(bool application_mode) {
  keypad_application_mode_ = application_mode;
}

void MotermView::OnDataReceived(const void* bytes, size_t num_bytes) {
  model_.ProcessInput(bytes, num_bytes, &model_state_changes_);
  ScheduleDraw(false);
}

void MotermView::OnClosed() {
  DCHECK(driver_);
  driver_->Detach();
  driver_.reset();

  OnDestroyed();
}

void MotermView::OnDestroyed() {
  DCHECK(!driver_);
  if (!on_closed_callback_.is_null()) {
    mojo::Closure callback;
    std::swap(callback, on_closed_callback_);
    callback.Run();
  }
}

void MotermView::Connect(
    mojo::InterfaceRequest<mojo::files::File> terminal_file,
    bool force,
    const ConnectCallback& callback) {
  if (driver_) {
    // We already have a connection.
    if (force) {
      OnClosed();
    } else {
      // TODO(vtl): Is this error code right?
      callback.Run(mojo::files::Error::UNAVAILABLE);
      return;
    }
  }

  driver_ = MotermDriver::Create(this, terminal_file.Pass());
  DCHECK(on_closed_callback_.is_null());
  on_closed_callback_ = [callback] { callback.Run(mojo::files::Error::OK); };
}

void MotermView::ConnectToClient(
    mojo::InterfaceHandle<mojo::terminal::TerminalClient> terminal_client,
    bool force,
    const ConnectToClientCallback& callback) {
  if (driver_) {
    // We already have a connection.
    if (force) {
      OnClosed();
    } else {
      // TODO(vtl): Is this error code right?
      callback.Run(mojo::files::Error::UNAVAILABLE);
      return;
    }
  }

  mojo::InterfaceHandle<mojo::files::File> file;
  driver_ = MotermDriver::Create(this, GetProxy(&file));
  mojo::terminal::TerminalClientPtr::Create(std::move(terminal_client))
      ->ConnectToTerminal(std::move(file));
  DCHECK(on_closed_callback_.is_null());
  on_closed_callback_ = [callback] { callback.Run(mojo::files::Error::OK); };
}

void MotermView::GetSize(const GetSizeCallback& callback) {
  MotermModel::Size size = model_.GetSize();
  callback.Run(mojo::files::Error::OK, size.rows, size.columns);
}

void MotermView::SetSize(uint32_t rows,
                         uint32_t columns,
                         bool reset,
                         const SetSizeCallback& callback) {
  if (!rows) {
    rows =
        properties()
            ? static_cast<uint32_t>(properties()->view_layout->size->height) /
                  line_height_
            : 0u;
    rows = std::max(1u, std::min(MotermModel::kMaxRows, rows));
  }
  if (!columns) {
    columns =
        properties()
            ? static_cast<uint32_t>(properties()->view_layout->size->width) /
                  advance_width_
            : 0u;
    columns = std::max(1u, std::min(MotermModel::kMaxColumns, columns));
  }

  model_.SetSize(MotermModel::Size(rows, columns), reset);
  callback.Run(mojo::files::Error::OK, rows, columns);

  ScheduleDraw(false);
}

void MotermView::ScheduleDraw(bool force) {
  if (!properties() ||
      (!model_state_changes_.IsDirty() && !force && !force_next_draw_)) {
    force_next_draw_ |= force;
    return;
  }
  force_next_draw_ = false;
  Invalidate();
}

void MotermView::OnDraw() {
  DCHECK(properties());

  // TODO(vtl): Draw only the dirty region(s)?
  model_state_changes_.Reset();

  auto update = mojo::gfx::composition::SceneUpdate::New();

  const mojo::Size& size = *properties()->view_layout->size;
  if (size.width > 0 && size.height > 0) {
    mojo::RectF bounds;
    bounds.width = size.width;
    bounds.height = size.height;

    mojo::gfx::composition::ResourcePtr moterm_resource =
        ganesh_renderer()->DrawCanvas(
            size, base::Bind(&MotermView::DrawContent, base::Unretained(this)));
    DCHECK(moterm_resource);
    update->resources.insert(kMotermImageResourceId, moterm_resource.Pass());

    auto root_node = mojo::gfx::composition::Node::New();
    root_node->hit_test_behavior =
        mojo::gfx::composition::HitTestBehavior::New();
    root_node->op = mojo::gfx::composition::NodeOp::New();
    root_node->op->set_image(mojo::gfx::composition::ImageNodeOp::New());
    root_node->op->get_image()->content_rect = bounds.Clone();
    root_node->op->get_image()->image_resource_id = kMotermImageResourceId;
    update->nodes.insert(kRootNodeId, root_node.Pass());
  } else {
    auto root_node = mojo::gfx::composition::Node::New();
    update->nodes.insert(kRootNodeId, root_node.Pass());
  }

  scene()->Update(update.Pass());
  scene()->Publish(CreateSceneMetadata());
}

void MotermView::DrawContent(
    const mojo::skia::GaneshContext::Scope& ganesh_scope,
    const mojo::Size& texture_size,
    SkCanvas* canvas) {
  canvas->clear(SK_ColorBLACK);

  SkPaint bg_paint;
  bg_paint.setStyle(SkPaint::kFill_Style);

  SkPaint fg_paint;
  fg_paint.setTypeface(regular_typeface_);
  fg_paint.setTextSize(16);
  fg_paint.setTextEncoding(SkPaint::kUTF32_TextEncoding);

  MotermModel::Size size = model_.GetSize();
  int y = 0;
  for (unsigned i = 0; i < size.rows; i++, y += line_height_) {
    int x = 0;
    for (unsigned j = 0; j < size.columns; j++, x += advance_width_) {
      MotermModel::CharacterInfo ch =
          model_.GetCharacterInfoAt(MotermModel::Position(i, j));

      // Paint the background.
      bg_paint.setColor(SkColorSetRGB(ch.background_color.red,
                                      ch.background_color.green,
                                      ch.background_color.blue));
      canvas->drawRect(SkRect::MakeXYWH(x, y, advance_width_, line_height_),
                       bg_paint);

      // Paint the foreground.
      if (ch.code_point) {
        uint32_t flags = SkPaint::kAntiAlias_Flag;
        // TODO(vtl): Use real bold font?
        if ((ch.attributes & MotermModel::kAttributesBold))
          flags |= SkPaint::kFakeBoldText_Flag;
        if ((ch.attributes & MotermModel::kAttributesUnderline))
          flags |= SkPaint::kUnderlineText_Flag;
        // TODO(vtl): Handle blink, because that's awesome.
        fg_paint.setFlags(flags);
        fg_paint.setColor(SkColorSetRGB(ch.foreground_color.red,
                                        ch.foreground_color.green,
                                        ch.foreground_color.blue));

        canvas->drawText(&ch.code_point, sizeof(ch.code_point), x, y + ascent_,
                         fg_paint);
      }
    }
  }

  if (model_.GetCursorVisibility()) {
    // Draw the cursor.
    MotermModel::Position cursor_pos = model_.GetCursorPosition();
    // Reuse the background paint, but don't just paint over.
    // TODO(vtl): Consider doing other things. Maybe make it blink, to be extra
    // annoying.
    // TODO(vtl): Maybe vary how we draw the cursor, depending on if we're
    // focused and/or active.
    bg_paint.setColor(SK_ColorWHITE);
    bg_paint.setXfermodeMode(SkXfermode::kDifference_Mode);
    canvas->drawRect(SkRect::MakeXYWH(cursor_pos.column * advance_width_,
                                      cursor_pos.row * line_height_,
                                      advance_width_, line_height_),
                     bg_paint);
  }
  canvas->flush();
}

void MotermView::OnKeyPressed(mojo::EventPtr key_event) {
  std::string input_sequence =
      GetInputSequenceForKeyPressedEvent(*key_event, keypad_application_mode_);
  if (input_sequence.empty())
    return;

  if (driver_)
    driver_->SendData(input_sequence.data(), input_sequence.size());
}
