// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package templates

import (
	"text/template"
)

const fieldEncodingTmplText = `
{{- define "FieldEncodingTmpl" -}}
{{- $info := . -}}
{{- if and (not $info.IsUnion) $info.IsNullable -}}
if {{$info.Identifier}} == nil {
{{- if $info.IsPointer -}}
	encoder.WriteNullPointer()
{{- else if $info.IsHandle -}}
	encoder.WriteInvalidHandle()
{{- end -}}
} else {
	{{ template "NonNullableFieldEncodingTmpl" $info }}
}
{{- else -}}
{{ template "NonNullableFieldEncodingTmpl" $info }}
{{- end -}}
{{- end -}}
`

const nonNullableFieldEncodingTmplText = `
{{- define "NonNullableFieldEncodingTmpl" -}}
{{- $info := . -}}
{{- if $info.IsPointer -}}
if err := encoder.WritePointer(); err != nil {
	return err
}
{{end -}}
{{- if $info.IsSimple -}}
if err := encoder.{{$info.WriteFunction}}({{$info.Identifier}}); err != nil {
	return err
}
{{- else if $info.IsEnum -}}
if err := encoder.WriteInt32(int32({{$info.Identifier}})); err != nil {
	return err
}
{{- else if $info.IsHandle -}}
{{- if $info.IsNullable -}}
if err := encoder.WriteHandle(*({{$info.Identifier}})); err != nil {
{{- else -}}
if err := encoder.WriteHandle({{$info.Identifier}}); err != nil {
{{- end -}}
	return err
}
{{- else if $info.IsStruct -}}
if err := {{$info.Identifier}}.Encode(encoder); err != nil {
	return err
}
{{- else if $info.IsUnion -}}
{{- if $info.IsPointer -}}
encoder.StartNestedUnion()
{{end -}}
if {{$info.Identifier}} == nil {
{{- if $info.IsNullable -}}
	encoder.WriteNullUnion()
{{- else -}}
	return &bindings.ValidationError{bindings.UnexpectedNullUnion, "unexpected null union"}
{{- end -}}
} else {
	if err := {{$info.Identifier}}.Encode(encoder); err != nil {
		return err
	}
}
{{- else if $info.IsArray -}}
{{ $elInfo := $info.ElementEncodingInfo -}}
encoder.StartArray(uint32(len({{$info.Identifier}})), {{$elInfo.BitSize}})
for _, {{$elInfo.Identifier}} := range {{$info.Identifier}} {
	{{ template "FieldEncodingTmpl" $elInfo }}
}
if err := encoder.Finish(); err != nil {
	return err
}
{{- else if $info.IsMap -}}
{{ $keyInfo := $info.KeyEncodingInfo -}}
{{ $keyElId := $info.KeyEncodingInfo.ElementEncodingInfo.Identifier -}}
{{ $valueInfo := $info.ValueEncodingInfo -}}
{{ $valueElId := $info.ValueEncodingInfo.ElementEncodingInfo.Identifier -}}
encoder.StartMap()
{
	var {{$keyInfo.Identifier}} {{$keyInfo.GoType}}
	var {{$valueInfo.Identifier}} {{$valueInfo.GoType}}
	for {{$keyElId}} := range {{$info.Identifier}} {
		{{$keyInfo.Identifier}} = append({{$keyInfo.Identifier}}, {{$keyElId}})
	}
	if encoder.Deterministic() {
		bindings.SortMapKeys(&{{$keyInfo.Identifier}})
	}
	for {{$keyElId}} := range {{$keyInfo.Identifier}} {
		{{$valueInfo.Identifier}} = append({{$valueInfo.Identifier}}, {{$info.Identifier}}[{{$keyElId}}])
	}
	{{ template "FieldEncodingTmpl" $keyInfo }}
	{{ template "FieldEncodingTmpl" $valueInfo }}
}
if err := encoder.Finish(); err != nil {
	return err
}
{{- end -}}
{{- end -}}
`

func initEncodingTemplates() {
	template.Must(goFileTmpl.Parse(nonNullableFieldEncodingTmplText))
	template.Must(goFileTmpl.Parse(fieldEncodingTmplText))
}
