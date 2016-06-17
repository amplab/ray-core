// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package templates

// These declarations go in the header file so that we can avoid some
// circular-dependencies. We call these "public" to say that other types can
// refer to them.
const GenerateTypeTableDeclarations = `
{{define "GenerateTypeTableDeclarations"}}
{{range $union := .PublicUnionNames -}}
extern struct MojomPointerTableUnionEntry {{$union}}[];
{{end}}

{{range $struct := .PublicStructNames -}}
extern struct MojomPointerTableStructEntry {{$struct}}[];
{{end -}}
{{end}}
`

const GenerateTypeTableDefinitions = `
{{define "GenerateTypeTableDefinitions"}}
// Declarations for array type entries.
{{range $array := .Arrays -}}
static struct MojomPointerTableArrayEntry {{$array.Name}};
{{end -}}

// Declarations for struct type tables.
{{range $struct := .Structs -}}
struct MojomPointerTableStructEntry {{$struct.Name}}[];
{{end -}}

// Declarations for union type tables.
{{range $union := .Unions -}}
struct MojomPointerTableUnionEntry {{$union.Name}}[];
{{end -}}

// Array type entry definitions.
{{range $array := .Arrays -}}
static struct MojomPointerTableArrayEntry {{$array.Name}} = {
  {{$array.ElemTable}}, {{$array.NumElements}},
  {{$array.ElemType}}, {{$array.Nullable}},
};
{{end -}}

// Struct type table definitions.
{{range $struct := .Structs -}}
struct MojomPointerTableStructEntry {{$struct.Name}}[] = {
{{- range $entry := $struct.Entries}}
  {
    {{$entry.ElemTable}}, {{$entry.Offset}}, {{$entry.MinVersion}},
    {{$entry.ElemType}}, {{$entry.Nullable}}, {{$entry.KeepGoing}},
  },
{{end -}}
};
{{end -}}

// Union type table definitions.
{{range $union := .Unions -}}
struct MojomPointerTableUnionEntry {{$union.Name}}[] = {
{{- range $entry := $union.Entries}}
  {
    {{$entry.ElemTable}}, {{$entry.Tag}}, {{$entry.ElemType}},
    {{$entry.Nullable}}, {{$entry.KeepGoing}},
  },
{{end -}}
};
{{end}}

{{end}}
`
