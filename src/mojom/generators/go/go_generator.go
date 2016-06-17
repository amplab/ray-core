// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package main

import (
	"log"
	"os"
	"path/filepath"

	"mojom/generators/common"
	"mojom/generators/go/templates"
	"mojom/generators/go/translator"
)

func main() {
	log.SetFlags(0)
	config := common.GetCliConfig(os.Args)
	t := translator.NewTranslator(config.FileGraph())
	goConfig := goConfig{config, t}
	common.GenerateOutput(WriteGoFile, goConfig)
}

type goConfig struct {
	common.GeneratorConfig
	translator translator.Translator
}

func (c goConfig) OutputDir() string {
	return filepath.Join(c.GeneratorConfig.OutputDir(), "go", "src")
}

func WriteGoFile(fileName string, config common.GeneratorConfig) {
	writer := common.OutputWriterByFilePath(fileName, config, ".mojom.go")
	goConfig := config.(goConfig)
	fileTmpl := goConfig.translator.TranslateMojomFile(fileName)
	writer.WriteString(templates.ExecuteTemplates(fileTmpl))
}
