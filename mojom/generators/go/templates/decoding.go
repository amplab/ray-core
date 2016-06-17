// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package templates

import (
	"text/template"
)

const fieldDecodingTmplText = `
{{- define "FieldDecodingTmpl" -}}
{{- $info := . -}}
{{- if $info.IsPointer -}}
pointer, err := decoder.ReadPointer()
if err != nil {
	return err
}
if pointer == 0 {
{{- if $info.IsNullable}}
	{{$info.Identifier}} = nil
{{- else if $info.IsUnion}}
	return &bindings.ValidationError{bindings.UnexpectedNullPointer, "unexpected null union pointer"}
{{- else}}
	return &bindings.ValidationError{bindings.UnexpectedNullPointer, "unexpected null pointer"}
{{- end }}
} else {
	{{ template "NonNullableFieldDecodingTmpl" $info }}
}
{{- else -}}
{{ template "NonNullableFieldDecodingTmpl" $info }}
{{- end -}}
{{- end -}}
`

const nonNullableFieldDecodingTmplText = `
{{- define "NonNullableFieldDecodingTmpl" -}}
{{- $info := . -}}
{{- if or $info.IsEnum $info.IsSimple -}}
value, err := decoder.{{$info.ReadFunction}}()
if err != nil {
	return err
}
{{if $info.IsSimple -}}
{{$info.Identifier}} = value
{{- else -}}
{{$info.Identifier}} = {{$info.GoType}}(value)
{{- end -}}
{{- else if $info.IsHandle -}}
handle, err := decoder.{{$info.ReadFunction}}()
if err != nil {
	return err
}
if handle.IsValid() {
{{- if $info.IsNullable -}}
	{{$info.Identifier}} = &handle
} else {
	{{$info.Identifier}} = nil
{{- else -}}
	{{$info.Identifier}} = handle
} else {
	return &bindings.ValidationError{bindings.UnexpectedInvalidHandle, "unexpected invalid handle"}
{{- end -}}
}
{{- else if $info.IsStruct -}}
{{- if $info.IsNullable -}}
{{$info.Identifier}} = new({{$info.GoType}})
{{end -}}
if err := {{$info.Identifier}}.Decode(decoder); err != nil {
	return err
}
{{- else if $info.IsUnion -}}
{{- if $info.IsPointer -}}
if err := decoder.StartNestedUnion(); err != nil {
	return err
}
{{end -}}
var err error
{{$info.Identifier}}, err = Decode{{$info.GoType}}(decoder)
if err != nil {
	return err
}
{{- if not $info.IsNullable}}
if {{$info.Identifier}} == nil {
	return &bindings.ValidationError{bindings.UnexpectedNullUnion, "unexpected null union"}
}
{{- end -}}
{{- if $info.IsPointer}}
decoder.Finish()
{{- end -}}
{{- else if $info.IsArray -}}
{{ $elInfo := $info.ElementEncodingInfo -}}
len0, err := decoder.StartArray({{$elInfo.BitSize}})
if err != nil {
	return err
}
{{$info.Identifier}} = make({{$info.GoType}}, len0)
for i := uint32(0); i < len0; i++ {
	{{ template "FieldDecodingTmpl" $elInfo }}
	{{$info.Identifier}}[i] = {{$elInfo.Identifier}}
}
if err := decoder.Finish(); err != nil {
	return nil
}
{{- else if $info.IsMap -}}
{{ $keyInfo := $info.KeyEncodingInfo -}}
{{ $keyElId := $info.KeyEncodingInfo.ElementEncodingInfo.Identifier -}}
{{ $valueInfo := $info.ValueEncodingInfo -}}
{{ $valueElId := $info.ValueEncodingInfo.ElementEncodingInfo.Identifier -}}
{{$info.Identifier}} = new({{$info.GoType}})
if err := decoder.StartMap(); err != nil {
	return err
}
var {{$keyInfo.Identifier}} {{$keyInfo.GoType}}
{
	{{ template "FieldDecodingTmpl" $keyInfo }}
}
var {{$valueInfo.Identifier}} {{$valueInfo.GoType}}
{
	{{ template "FieldDecodingTmpl" $valueInfo }}
}
if err := decoder.Finish(); err != nil {
	return nil
}
if len({{$keyInfo.Identifier}}) == len({{$valueInfo.Identifier}}) {
	return &bindings.ValidationError{bindings.DifferentSizedArraysInMap,
		fmt.Sprintf("Number of keys %d is different from number of values %d",
		len({{$keyInfo.Identifier}}), len({{$valueInfo.Identifier}}))}
}
for i := 0; i < len({{$keyInfo.Identifier}}); i++ {
	(*{{$info.Identifier}})[{{$keyInfo.Identifier}}[i]] = {{$valueInfo.Identifier}}[i]
}
{{- end -}}
{{- end -}}
`

func initDecodingTemplates() {
	template.Must(goFileTmpl.Parse(nonNullableFieldDecodingTmplText))
	template.Must(goFileTmpl.Parse(fieldDecodingTmplText))
}
