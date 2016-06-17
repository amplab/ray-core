// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package templates

import (
	"bytes"
	"text/template"

	"mojom/generators/go/gofmt"
	"mojom/generators/go/translator"
)

// goFileTmpl is the template object for a go file.
var goFileTmpl *template.Template

// ExecuteTemplates accepts a translator.TmplFile and returns the formatted
// source code for the go bindings.
func ExecuteTemplates(tmplFile *translator.TmplFile) string {
	buffer := &bytes.Buffer{}
	if err := goFileTmpl.ExecuteTemplate(buffer, "FileTemplate", tmplFile); err != nil {
		panic(err)
	}

	src, err := gofmt.FormatGoFile(buffer.String())

	if err != nil {
		panic(err)
	}
	return src
}

func init() {
	// We parse the subtemplates only once.
	goFileTmpl = template.New("GoFileTemplate")

	template.Must(goFileTmpl.Parse(goFileTemplate))

	initEncodingTemplates()
	initDecodingTemplates()
	initStructTemplates()
	initUnionTemplates()
	initEnumTemplates()
}

const goFileTemplate = `
{{- define "FileTemplate" -}}
{{- $fileTmpl := . -}}
package {{$fileTmpl.PackageName}}

import (
	{{range $import := $fileTmpl.Imports}}
	{{$import.PackageName}} "{{$import.PackagePath}}"
	{{end}}
)

{{- range $struct := $fileTmpl.Structs}}
	{{ template "Struct" $struct }}
{{- end}}

{{- range $union := $fileTmpl.Unions}}
	{{ template "Union" $union }}
{{- end}}

{{- range $enum := $fileTmpl.Enums}}
	{{ template "Enum" $enum }}
{{- end}}
{{- end -}}
`
