// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package templates

const GenerateInterface = `
{{- /* . (dot) refers to the Go type |cgen.InterfaceTemplate| */ -}}
{{define "GenerateInterface" -}}
{{- $interface := . -}}
// --- {{$interface.Name}} ---
#define {{$interface.Name}}__ServiceName ((const char*)"{{$interface.ServiceName}}")
#define {{$interface.Name}}__CurrentVersion ((uint32_t){{$interface.Version}})

// Enums
{{range $enum := $interface.Enums -}}
{{template "GenerateEnum" $enum}}
{{end -}}

// Constants
{{range $const := $interface.Constants -}}
const {{$const.Type}} {{$const.Name}};
{{end}}

{{range $message := $interface.Messages -}}
// Message: {{$message.Name}}

#define {{$interface.Name}}_{{$message.Name}}__Ordinal \
    ((uint32_t){{$message.MessageOrdinal}})
#define {{$interface.Name}}_{{$message.Name}}__MinVersion \
    ((uint32_t){{$message.MinVersion}})

struct {{$message.RequestStruct.Name}};
{{template "GenerateStruct" $message.RequestStruct}}
{{if ne $message.ResponseStruct.Name "" -}}
struct {{$message.ResponseStruct.Name}};
{{template "GenerateStruct" $message.ResponseStruct}}
{{end}}
{{end}}
{{end}}
`
