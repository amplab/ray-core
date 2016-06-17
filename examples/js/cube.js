#!mojo mojo:js_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

define("main", [
  "console",
  "mojo/public/js/bindings",
  "mojo/public/js/core",
  "mojo/services/geometry/interfaces/geometry.mojom",
  "mojo/services/native_viewport/interfaces/native_viewport.mojom",
  "mojo/services/public/js/application",
  "services/js/modules/gl",
  "services/js/modules/clock",
  "timer",
], function(console,
            bindings,
            core,
            geometry,
            nv,
            application,
            gl,
            clock,
            timer) {

  const Application = application.Application;
  const Context = gl.Context;
  const NativeViewport = nv.NativeViewport;
  const Size = geometry.Size;
  const StubBindings = bindings.StubBindings;

  const VERTEX_SHADER_SOURCE = [
    'uniform mat4 u_mvpMatrix;',
    'attribute vec4 a_position;',
    'attribute vec4 a_normal;',
    'varying vec4 v_color;',
    'void main()',
    '{',
    '  gl_Position = u_mvpMatrix * a_position;',
    '  vec4 rotated_normal = u_mvpMatrix * a_normal;',
    '  vec4 light_direction = normalize(vec4(0.0, 1.0, -1.0, 0.0));',
    '  float directional_capture = ',
    '      clamp(dot(rotated_normal, light_direction), 0.0, 1.0);',
    '  float light_intensity = 0.6 * directional_capture + 0.4;',
    '  vec3 base_color = a_position.xyz + 0.5;',
    '  v_color = vec4(base_color * light_intensity, 1.0);',
    '}'
  ].join('\n');

  const FRAGMENT_SHADER_SOURCE = [
    'precision mediump float;',
    'varying vec4 v_color;',
    'void main()',
    '{',
    '  gl_FragColor = v_color;',
    '}'
  ].join('\n');

  // The number of vertices in the cube.
  const NUM_VERTICES = 24;

  class ESMatrix {
    constructor() {
      this.m = new Float32Array(16);
    }

    getIndex(x, y) {
      return x * 4 + y;
    }

    set(x, y, v) {
      this.m[this.getIndex(x, y)] = v;
    }

    get(x, y) {
      return this.m[this.getIndex(x, y)];
    }

    loadZero() {
      for (var i = 0; i < this.m.length; i++) {
        this.m[i] = 0;
      }
    }

    loadIdentity() {
      this.loadZero();
      for (var i = 0; i < 4; i++) {
        this.set(i, i, 1);
      }
    }

    multiply(a, b) {
      var result = new ESMatrix();
      for (var i = 0; i < 4; i++) {
        result.set(i, 0,
                   (a.get(i, 0) * b.get(0, 0)) +
                   (a.get(i, 1) * b.get(1, 0)) +
                   (a.get(i, 2) * b.get(2, 0)) +
                   (a.get(i, 3) * b.get(3, 0)));

        result.set(i, 1,
                   (a.get(i, 0) * b.get(0, 1)) +
                   (a.get(i, 1) * b.get(1, 1)) +
                   (a.get(i, 2) * b.get(2, 1)) +
                   (a.get(i, 3) * b.get(3, 1)));

        result.set(i, 2,
                   (a.get(i, 0) * b.get(0, 2)) +
                   (a.get(i, 1) * b.get(1, 2)) +
                   (a.get(i, 2) * b.get(2, 2)) +
                   (a.get(i, 3) * b.get(3, 2)));

        result.set(i, 3,
                   (a.get(i, 0) * b.get(0, 3)) +
                   (a.get(i, 1) * b.get(1, 3)) +
                   (a.get(i, 2) * b.get(2, 3)) +
                   (a.get(i, 3) * b.get(3, 3)));
      }
      for (var i = 0; i < result.m.length; i++) {
        this.m[i] = result.m[i];
      }
    }

    frustrum(left, right, bottom, top, nearZ, farZ) {
      var deltaX = right - left;
      var deltaY = top - bottom;
      var deltaZ = farZ - nearZ;

      if (nearZ < 0 || farZ < 0 || deltaZ < 0 || deltaY < 0 || deltaX < 0) {
        return;
      }

      var frust = new ESMatrix();
      frust.set(0, 0, 2 * nearZ / deltaX);

      frust.set(1, 1, 2 * nearZ / deltaY);

      frust.set(2, 0, (right + left) / deltaX);
      frust.set(2, 1, (top + bottom) / deltaY);
      frust.set(2, 2, -(nearZ + farZ) / deltaZ);
      frust.set(2, 3, -1);

      frust.set(3, 2, -2 * nearZ * farZ / deltaZ);

      this.multiply(frust, this);
    }

    perspective(fovY, aspect, nearZ, farZ) {
      var frustrumH = Math.tan(fovY / 360 * Math.PI) * nearZ;
      var frustrumW = frustrumH * aspect;
      this.frustrum(-frustrumW, frustrumW, -frustrumH, frustrumH, nearZ, farZ);
    }

    translate(tx, ty, tz) {
      this.set(3, 0, this.get(3, 0) + this.get(0, 0) *
               tx + this.get(1, 0) * ty + this.get(2, 0) * tz);
      this.set(3, 1, this.get(3, 1) + this.get(0, 1) *
               tx + this.get(1, 1) * ty + this.get(2, 1) * tz);
      this.set(3, 2, this.get(3, 2) + this.get(0, 2) *
               tx + this.get(1, 2) * ty + this.get(2, 2) * tz);
      this.set(3, 3, this.get(3, 3) + this.get(0, 3) *
               tx + this.get(1, 3) * ty + this.get(2, 3) * tz);
    }

    rotate(angle, x, y, z) {
      var mag = Math.sqrt(x * x + y * y + z * z);
      var sinAngle = Math.sin(angle * Math.PI / 180);
      var cosAngle = Math.cos(angle * Math.PI / 180);
      if (mag <= 0) {
        return;
      }

      var xx, yy, zz, xy, yz, zx, xs, ys, zs, oneMinusCos;
      var rotation = new ESMatrix();

      x /= mag;
      y /= mag;
      z /= mag;

      xx = x * x;
      yy = y * y;
      zz = z * z;
      xy = x * y;
      yz = y * z;
      zx = z * x;
      xs = x * sinAngle;
      ys = y * sinAngle;
      zs = z * sinAngle;
      oneMinusCos = 1 - cosAngle;

      rotation.set(0, 0, (oneMinusCos * xx) + cosAngle);
      rotation.set(0, 1, (oneMinusCos * xy) - zs);
      rotation.set(0, 2, (oneMinusCos * zx) + ys);
      rotation.set(0, 3, 0);

      rotation.set(1, 0, (oneMinusCos * xy) + zs);
      rotation.set(1, 1, (oneMinusCos * yy) + cosAngle);
      rotation.set(1, 2, (oneMinusCos * yz) - xs);
      rotation.set(1, 3, 0);

      rotation.set(2, 0, (oneMinusCos * zx) - ys);
      rotation.set(2, 1, (oneMinusCos * yz) + xs);
      rotation.set(2, 2, (oneMinusCos * zz) + cosAngle);
      rotation.set(2, 3, 0);

      rotation.set(3, 0, 0);
      rotation.set(3, 1, 0);
      rotation.set(3, 2, 0);
      rotation.set(3, 3, 1);

      this.multiply(rotation, this);
    }
  }

  function loadProgram(gl) {
    var vertexShader = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(vertexShader, VERTEX_SHADER_SOURCE);
    gl.compileShader(vertexShader);

    var fragmentShader = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(fragmentShader, FRAGMENT_SHADER_SOURCE);
    gl.compileShader(fragmentShader);

    var program = gl.createProgram();
    gl.attachShader(program, vertexShader);
    gl.attachShader(program, fragmentShader);

    gl.linkProgram(program);
    // TODO(aa): Check for errors using getProgramiv and LINK_STATUS.

    gl.deleteShader(vertexShader);
    gl.deleteShader(fragmentShader);

    return program;
  }

  var vboVertices;
  var vboIndices;
  function generateCube(gl) {
    var cubeVertices = [
      // -Y side.
      -0.5, -0.5, -0.5,
      -0.5, -0.5,  0.5,
      0.5, -0.5,  0.5,
      0.5, -0.5, -0.5,

      // +Y side.
      -0.5,  0.5, -0.5,
      -0.5,  0.5,  0.5,
      0.5,  0.5,  0.5,
      0.5,  0.5, -0.5,

      // -Z side.
      -0.5, -0.5, -0.5,
      -0.5,  0.5, -0.5,
      0.5,  0.5, -0.5,
      0.5, -0.5, -0.5,

      // +Z side.
      -0.5, -0.5, 0.5,
      -0.5,  0.5, 0.5,
      0.5,  0.5, 0.5,
      0.5, -0.5, 0.5,

      // -X side.
      -0.5, -0.5, -0.5,
      -0.5, -0.5,  0.5,
      -0.5,  0.5,  0.5,
      -0.5,  0.5, -0.5,

      // +X side.
      0.5, -0.5, -0.5,
      0.5, -0.5,  0.5,
      0.5,  0.5,  0.5,
      0.5,  0.5, -0.5
    ];

    var vertexNormals = [
      // -Y side.
      0.0, -1.0, 0.0,
      0.0, -1.0, 0.0,
      0.0, -1.0, 0.0,
      0.0, -1.0, 0.0,

      // +Y side.
      0.0, 1.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 1.0, 0.0,

      // -Z side.
      0.0, 0.0, -1.0,
      0.0, 0.0, -1.0,
      0.0, 0.0, -1.0,
      0.0, 0.0, -1.0,

      // +Z side.
      0.0, 0.0, 1.0,
      0.0, 0.0, 1.0,
      0.0, 0.0, 1.0,
      0.0, 0.0, 1.0,

      // -X side.
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,
      -1.0, 0.0, 0.0,

      // +X side.
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
      1.0, 0.0, 0.0,
    ];

    var cubeIndices = [
      // -Y side.
      0, 2, 1,
      0, 3, 2,

      // +Y side.
      4, 5, 6,
      4, 6, 7,

      // -Z side.
      8, 9, 10,
      8, 10, 11,

      // +Z side.
      12, 15, 14,
      12, 14, 13,

      // -X side.
      16, 17, 18,
      16, 18, 19,

      // +X side.
      20, 23, 22,
      20, 22, 21
    ];

    vboVertices = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vboVertices);
    gl.bufferData(gl.ARRAY_BUFFER,
                  new Float32Array(cubeVertices.concat(vertexNormals)),
                  gl.STATIC_DRAW);
    gl.bindBuffer(gl.ARRAY_BUFFER, 0);

    vboIndices = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, vboIndices);
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER,
                  new Uint16Array(cubeIndices), gl.STATIC_DRAW);
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, 0);

    return cubeIndices.length;
  }

  class GLES2ClientImpl {
    constructor (contextProvider) {
      this.contextProvider_ = contextProvider;
      this.gl_ = null;
      this.width_ = 0;
      this.height_ = 0;

      var self = this;
      contextProvider.create(core.kInvalidHandle).then(
        function (result) {
          self.gl_ =
            new Context(result.gles2_client, self.contextLost.bind(self));
          self.lastTime_ = clock.seconds();
          self.angle_ = 45;

          self.program_ = loadProgram(self.gl_);
          self.positionLocation_ =
            self.gl_.getAttribLocation(self.program_, 'a_position');
          self.normalLocation_ =
            self.gl_.getAttribLocation(self.program_, 'a_normal');
          self.mvpLocation_ =
            self.gl_.getUniformLocation(self.program_, 'u_mvpMatrix');
          self.numIndices_ = generateCube(self.gl_);
          self.mvpMatrix_ = new ESMatrix();
          self.mvpMatrix_.loadIdentity();

          self.gl_.clearColor(0, 0, 0, 0);
          self.gl_.enable(self.gl_.DEPTH_TEST);
          if (self.width_ >= 0 && self.height_ >= 0)
            self.gl_.resize(self.width_, self.height_);
          self.timer_ =
              timer.createRepeating(16, self.handleTimer.bind(self));
        }).catch(function(e) {
          console.log("ContextProvider create() failed: " + e.stack);
        });
    }

    setDimensions(size) {
      this.width_ = size.width;
      this.height_ = size.height;
      if (this.gl_)
        this.gl_.resize(this.width_, this.height_);
    }

    drawCube() {
      this.gl_.viewport(0, 0, this.width_, this.height_);
      this.gl_.clear(this.gl_.COLOR_BUFFER_BIT | this.gl_.DEPTH_BUFFER_BIT);
      this.gl_.useProgram(this.program_);
      this.gl_.bindBuffer(this.gl_.ARRAY_BUFFER, vboVertices);
      this.gl_.bindBuffer(this.gl_.ELEMENT_ARRAY_BUFFER, vboIndices);
      this.gl_.vertexAttribPointer(this.positionLocation_, 3, this.gl_.FLOAT,
                                   false, 3 * 4, 0);
      this.gl_.vertexAttribPointer(this.normalLocation_, 3, this.gl_.FLOAT,
                                   false, 3 * 4, 3 * 4 * NUM_VERTICES);
      this.gl_.enableVertexAttribArray(this.positionLocation_);
      this.gl_.enableVertexAttribArray(this.normalLocation_);
      this.gl_.uniformMatrix4fv(this.mvpLocation_, false, this.mvpMatrix_.m);
      this.gl_.drawElements(this.gl_.TRIANGLES, this.numIndices_,
                            this.gl_.UNSIGNED_SHORT, 0);
      this.gl_.swapBuffers();
    };

    handleTimer() {
      if (!this.gl_)
        return;

      var now = clock.seconds();
      var secondsDelta = now - this.lastTime_;
      this.lastTime_ = now;

      this.angle_ += this.getRotationForTimeDelta(secondsDelta);
      this.angle_ = this.angle_ % 360;

      var aspect = this.width_ / this.height_;

      var perspective = new ESMatrix();
      perspective.loadIdentity();
      perspective.perspective(60, aspect, 1, 20);

      var modelView = new ESMatrix();
      modelView.loadIdentity();
      modelView.translate(0, 0, -2);
      modelView.rotate(this.angle_, 1, 0, 1);

      this.mvpMatrix_.multiply(modelView, perspective);

      this.drawCube();
    };

    getRotationForTimeDelta(secondsDelta) {
      return secondsDelta * 40;
    };

    contextLost() {
      console.log('GLES2ClientImpl.prototype.contextLost');
    };

    quit() {
      if (this.timer_) {
        this.timer_.cancel();
        this.timer_ = null;
      }
    }
  }

  class CubeDemo extends Application {
    initialize(args) {
      this.viewport = this.shell.connectToService(
          "mojo:native_viewport_service", NativeViewport, this);

      var app = this;
      var viewportSize = new Size({width: 800, height: 600});
      this.viewport.create(viewportSize).then(
        function(result) {
          app.onMetricsChanged(result.metrics);
        }).catch(function(e) {
          console.log("NativeViewport create() failed: " + e.stack);
        });

      this.viewport.setEventDispatcher(function(stub) {
        app.eventDispatcherStub = stub;
        StubBindings(stub).delegate = app;
      });
      this.viewport.show();

      var onscreenContextProvider;
      this.viewport.getContextProvider(function(onscreenContextProviderProxy) {
        onscreenContextProvider = onscreenContextProviderProxy;
      });
      this.gles2_ = new GLES2ClientImpl(onscreenContextProvider);
    }

    onEvent(event) {
      return Promise.resolve(); // This just gates the next event delivery
    }

    onMetricsChanged(metrics) {
      if (this.gles2_)
        this.gles2_.setDimensions(metrics.size);
      var self = this;
      this.viewport.requestMetrics().then(
        function(result) {
          self.onMetricsChanged(result.metrics);
        }).catch(function(e) {
          console.log("NativeViewport requestMetrics() failed: " + e.stack);
        });
    }

    onDestroyed() {
      this.quit();
    }
  }

  return CubeDemo;
});
