// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package translator

import (
	"fmt"
	"path/filepath"
	"strings"
	"unicode"
	"unicode/utf8"

	"mojom/generated/mojom_types"
)

func (t *translator) goTypeName(typeKey string) string {
	if cachedType, ok := t.goTypeCache[typeKey]; ok {
		return cachedType
	}

	userDefinedType := t.fileGraph.ResolvedTypes[typeKey]
	shortName := userDefinedTypeShortName(userDefinedType)

	// TODO(azani): Resolve conflicts somehow.
	goType := formatName(shortName)

	if e, ok := userDefinedType.(*mojom_types.UserDefinedTypeEnumType); ok {
		if e.Value.DeclData.ContainerTypeKey != nil {
			containerName := t.goTypeName(*e.Value.DeclData.ContainerTypeKey)
			goType = fmt.Sprintf("%s_%s", containerName, goType)
		}
	}

	t.goTypeCache[typeKey] = goType
	return goType
}

func fileNameToPackageName(fileName string) string {
	base := filepath.Base(fileName)
	ext := filepath.Ext(base)
	return base[:len(base)-len(ext)]
}

// userDefinedTypeShortName extracts the ShortName from a user-defined type.
func userDefinedTypeShortName(userDefinedType mojom_types.UserDefinedType) string {
	switch u := userDefinedType.(type) {
	case *mojom_types.UserDefinedTypeEnumType:
		return *u.Value.DeclData.ShortName
	case *mojom_types.UserDefinedTypeStructType:
		return *u.Value.DeclData.ShortName
	case *mojom_types.UserDefinedTypeUnionType:
		return *u.Value.DeclData.ShortName
	case *mojom_types.UserDefinedTypeInterfaceType:
		return *u.Value.DeclData.ShortName
	}
	panic("Non-handled mojom UserDefinedType. This should never happen.")
}

// privateName accepts a string and returns that same string with the first rune
// set to lowercase.
func privateName(name string) string {
	_, size := utf8.DecodeRuneInString(name)
	return strings.ToLower(name[:size]) + name[size:]
}

// formatName formats a name to match go style.
func formatName(name string) string {
	var parts []string
	for _, namePart := range strings.Split(name, "_") {
		partStart := 0
		lastRune, _ := utf8.DecodeRuneInString(name)
		lastRuneStart := 0
		for i, curRune := range namePart {
			if i == 0 {
				continue
			}

			if unicode.IsUpper(curRune) && !unicode.IsUpper(lastRune) {
				parts = append(parts, namePart[partStart:i])
				partStart = i
			}

			if !unicode.IsUpper(curRune) && unicode.IsUpper(lastRune) && partStart != lastRuneStart {
				parts = append(parts, namePart[partStart:lastRuneStart])
				partStart = lastRuneStart
			}

			lastRuneStart = i
			lastRune = curRune
		}
		parts = append(parts, namePart[partStart:])
	}

	for i := range parts {
		part := strings.Title(strings.ToLower(parts[i]))
		_, size := utf8.DecodeRuneInString(part)
		parts[i] = strings.ToUpper(part[:size]) + part[size:]
	}

	return strings.Join(parts, "")
}
