" Copyright 2015 The Chromium Authors. All rights reserved.
" Use of this source code is governed by a BSD-style license that can be
" found in the LICENSE file.
" Vim syntax file " Language: Mojom
" To get syntax highlighting for .mojom files, add the following to your .vimrc
" file:
"     set runtimepath^=/path/to/src/tools/vim/mojom

if exists("b:did_indent")
   finish
endif
let b:did_indent = 1

setlocal cindent
setlocal formatoptions=tcroq
