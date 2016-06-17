# Copyright 2016 The Chromium Authors. All rights reserved. 
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This is a simplistic sample Makefile for building a Mojo application. It
# produces output in the out subdirectory (which it creates if necessary), and
# produces "debug" builds. It assumes that the mojom tool and clang have been
# obtained by running mojo_sdk_setup/setup.sh.
#
# Note: It is very primitive, and doesn't try to correctly handle dependencies.
#
# TODO(vtl): Even so, it could use a lot of improvements.
# TODO(vtl): Should probably support other toolchains, when appropriate.

# Default target ---------------------------------------------------------------

all: default

# Build directories ------------------------------------------------------------

OUT_DIR := out
GEN_DIR := $(OUT_DIR)/gen
OBJ_DIR := $(OUT_DIR)/obj

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

$(GEN_DIR): | $(OUT_DIR)
	mkdir -p $(GEN_DIR)

$(OBJ_DIR): | $(OUT_DIR)
	mkdir -p $(OBJ_DIR)

# Tools ------------------------------------------------------------------------

MOJOM_BINDINGS_GENERATOR := \
		mojo/public/tools/bindings/mojom_bindings_generator.py

CC := toolchain/clang/bin/clang
CFLAGS := -m64 -march=x86-64 -fPIC -fvisibility=hidden -fno-strict-aliasing \
		-pthread -O0 -g2 -Wall -Werror -I. -I$(GEN_DIR)

CXX := toolchain/clang/bin/clang++
CXXFLAGS := $(CFLAGS) -std=c++11 -fno-rtti -fno-exceptions

CXX_MAKE_SO_FLAGS := $(CXXFLAGS) -shared -ldl -Wl,--fatal-warnings \
		-Wl,-z,noexecstack -Wl,-z,now -Wl,-z,relro -Wl,-z,defs

# Build patterns ---------------------------------------------------------------

$(GEN_DIR)/%.mojom.cc $(GEN_DIR)/%.mojom.h $(GEN_DIR)/%.mojom-internal.h \
		$(GEN_DIR)/%.mojom-common.cc $(GEN_DIR)/%.mojom-common.h \
		$(GEN_DIR)/%.mojom-sync.cc $(GEN_DIR)/%.mojom-sync.h: %.mojom | $(GEN_DIR)
	$(MOJOM_BINDINGS_GENERATOR) --use_bundled_pylibs -d . -I . -o $(GEN_DIR) \
			--no-gen-imports --no-generate-type-info $<

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $< 

$(OBJ_DIR)/%.o: %.cc
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ -c $< 

# "Functions" (to use with $(call function_name,...)) --------------------------

# Function to get a list of generated .cc files from a list of .mojom files.
cc_files_from_mojom_files = $(patsubst %.mojom,$(GEN_DIR)/%.mojom.cc,$(1)) \
		$(patsubst %.mojom,$(GEN_DIR)/%.mojom-common.cc,$(1)) \
		$(patsubst %.mojom,$(GEN_DIR)/%.mojom-sync.cc,$(1))

# Function to get a list of generated .o files from a list of .c files.
o_files_from_c_files = $(patsubst %.c,$(OBJ_DIR)/%.o,$(1))

# Function to get a list of generated .o files from a list of .cc files.
o_files_from_cc_files = $(patsubst %.cc,$(OBJ_DIR)/%.o,$(1))

# Target: mojo_public.a --------------------------------------------------------

MOJO_PUBLIC_MOJOM_FILES := \
		$(wildcard mojo/public/interfaces/application/*.mojom) \
		$(wildcard mojo/public/interfaces/bindings/*.mojom) \
		$(wildcard mojo/public/interfaces/network/*.mojom)
MOJO_PUBLIC_MOJOM_CC_FILES := \
		$(call cc_files_from_mojom_files,$(MOJO_PUBLIC_MOJOM_FILES))
MOJO_PUBLIC_C_FILES := \
		$(wildcard mojo/public/c/bindings/lib/*.c)
MOJO_PUBLIC_CC_FILES := \
		$(wildcard mojo/public/cpp/application/lib/*.cc) \
		$(wildcard mojo/public/cpp/bindings/lib/*.cc) \
		$(wildcard mojo/public/cpp/environment/lib/*.cc) \
		$(wildcard mojo/public/cpp/system/lib/*.cc) \
		$(wildcard mojo/public/cpp/utility/lib/*.cc)
# We have to choose *one* implementation of |Environment|, so remove the
# logging-only environment.
MOJO_PUBLIC_CC_FILES := \
		$(filter-out mojo/public/cpp/environment/lib/logging_only_environment.cc, \
				$(MOJO_PUBLIC_CC_FILES))

$(MOJO_PUBLIC_CC_FILES): $(MOJO_PUBLIC_MOJOM_CC_FILES)

# TODO(vtl): We could really update the archive instead of re-creating it each
# time.
$(OUT_DIR)/mojo_public.a: \
		$(MOJO_PUBLIC_MOJOM_CC_FILES) \
		$(call o_files_from_cc_files,$(MOJO_PUBLIC_MOJOM_CC_FILES)) \
		$(call o_files_from_c_files,$(MOJO_PUBLIC_C_FILES)) \
		$(call o_files_from_cc_files,$(MOJO_PUBLIC_CC_FILES)) \
		| $(OUT_DIR)
	rm -f $@
	ar rc $@ $(filter %.o,$^)

# Target: mojo_system_thunks.o -------------------------------------------------

# TODO(vtl): Support other thunk libraries.

$(OBJ_DIR)/mojo_system_thunks.o: \
		$(OBJ_DIR)/mojo/public/platform/native/system_thunks.o
	cp $^ $@

# Targets: hello_mojo_client.mojo and hello_mojo_server.mojo -------------------

EXAMPLES_HELLO_MOJO_MOJOM_FILES := \
		$(wildcard examples/hello_mojo/*.mojom)
EXAMPLES_HELLO_MOJO_MOJOM_CC_FILES := \
		$(call cc_files_from_mojom_files,$(EXAMPLES_HELLO_MOJO_MOJOM_FILES))

EXAMPLES_HELLO_MOJO_CLIENT_CC_FILES := \
		examples/hello_mojo/hello_mojo_client.cc
$(EXAMPLES_HELLO_MOJO_CLIENT_CC_FILES): $(EXAMPLES_HELLO_MOJO_MOJOM_CC_FILES)
$(OUT_DIR)/hello_mojo_client.mojo: \
		$(OUT_DIR)/mojo_public.a \
		$(OBJ_DIR)/mojo_system_thunks.o \
		$(call o_files_from_cc_files,$(EXAMPLES_HELLO_MOJO_MOJOM_CC_FILES)) \
		$(call o_files_from_cc_files,$(EXAMPLES_HELLO_MOJO_CLIENT_CC_FILES))
	$(CXX) $(CXX_MAKE_SO_FLAGS) \
			-Wl,--whole-archive $(filter %.o,$^) -Wl,-no-whole-archive \
			$(filter %.a,$^) -o $@

EXAMPLES_HELLO_MOJO_SERVER_CC_FILES := \
		examples/hello_mojo/hello_mojo_server.cc
$(EXAMPLES_HELLO_MOJO_SERVER_CC_FILES): $(EXAMPLES_HELLO_MOJO_MOJOM_CC_FILES)
$(OUT_DIR)/hello_mojo_server.mojo: \
		$(OUT_DIR)/mojo_public.a \
		$(OBJ_DIR)/mojo_system_thunks.o \
		$(call o_files_from_cc_files,$(EXAMPLES_HELLO_MOJO_MOJOM_CC_FILES)) \
		$(call o_files_from_cc_files,$(EXAMPLES_HELLO_MOJO_SERVER_CC_FILES))
	$(CXX) $(CXX_MAKE_SO_FLAGS) \
			-Wl,--whole-archive $(filter %.o,$^) -Wl,-no-whole-archive \
			$(filter %.a,$^) -o $@

# Default targets --------------------------------------------------------------

default: $(OUT_DIR)/hello_mojo_client.mojo $(OUT_DIR)/hello_mojo_server.mojo
