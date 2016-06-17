// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// TODO(vardhan): The generated names for type tables may need to be reworked
// to keep within C's identifier-length limit.

package cgen

import (
	"fmt"
	"log"
	"mojom/generated/mojom_files"
	"mojom/generated/mojom_types"
)

type StructPointerTableEntry struct {
	ElemTable  string
	Offset     uint32
	MinVersion uint32
	ElemType   string
	Nullable   bool
	KeepGoing  bool
}

type UnionPointerTableEntry struct {
	ElemTable string
	Tag       uint32
	Nullable  bool
	ElemType  string
	KeepGoing bool
}

type ArrayPointerTableEntry struct {
	Name        string
	ElemTable   string
	NumElements uint32
	Nullable    bool
	ElemType    string
}

type StructPointerTable struct {
	Name    string
	Entries []StructPointerTableEntry
}
type UnionPointerTable struct {
	Name    string
	Entries []UnionPointerTableEntry
}

type TypeTableTemplate struct {
	Structs []StructPointerTable
	Unions  []UnionPointerTable
	Arrays  []ArrayPointerTableEntry

	// These are used for declarations in header files.
	PublicUnionNames  []string
	PublicStructNames []string

	// This counter is used to name recursive types (array of arrays, maps, etc.).
	counter uint32

	// Used to look up user-defined references.
	fileGraph *mojom_files.MojomFileGraph
}

func (table *TypeTableTemplate) getTableForUDT(typeRef mojom_types.TypeReference) (elemTable string, elemType string, nullable bool) {
	nullable = typeRef.Nullable
	if typeRef.IsInterfaceRequest {
		elemTable = "NULL"
		elemType = "MOJOM_ELEMENT_TYPE_HANDLE"
		return
	}
	if typeRef.TypeKey == nil {
		log.Fatalf("Unresolved type reference %s", typeRef.Identifier)
	}
	udt, _ := table.fileGraph.ResolvedTypes[*typeRef.TypeKey]
	switch udt.(type) {
	case *mojom_types.UserDefinedTypeStructType:
		structName := *udt.Interface().(mojom_types.MojomStruct).DeclData.FullIdentifier
		elemTable = mojomToCName(structName) + "__PointerTable"
		elemType = "MOJOM_ELEMENT_TYPE_STRUCT"
	case *mojom_types.UserDefinedTypeUnionType:
		unionName := *udt.Interface().(mojom_types.MojomUnion).DeclData.FullIdentifier
		elemTable = mojomToCName(unionName) + "__PointerTable"
		elemType = "MOJOM_ELEMENT_TYPE_UNION"
	case *mojom_types.UserDefinedTypeInterfaceType:
		elemTable = "NULL"
		elemType = "MOJOM_ELEMENT_TYPE_INTERFACE"
	default:
		elemTable = "NULL"
		elemType = "MOJOM_ELEMENT_TYPE_POD"
	}

	return
}

func (table *TypeTableTemplate) makeTableForType(prefix string, dataType mojom_types.Type) (elemTable string, elemType string, nullable bool) {
	switch dataType.(type) {
	case *mojom_types.TypeStringType:
		elemTable = "(void*)&MojomStringPointerEntry"
		elemType = "MOJOM_ELEMENT_TYPE_ARRAY"
		nullable = dataType.Interface().(mojom_types.StringType).Nullable
	case *mojom_types.TypeArrayType:
		arrayTableName := fmt.Sprintf("%s_%d", prefix, table.counter)
		table.counter++
		typ := dataType.Interface().(mojom_types.ArrayType)
		elemTable = "&" + table.makeArrayPointerEntry(arrayTableName, typ)
		elemType = "MOJOM_ELEMENT_TYPE_ARRAY"
		nullable = typ.Nullable
	case *mojom_types.TypeMapType:
		mapTableName := fmt.Sprintf("%s_%d", prefix, table.counter)
		table.counter++
		typ := dataType.Interface().(mojom_types.MapType)
		elemTable = "&" + table.makeMapPointerTable(mapTableName, typ)
		elemType = "MOJOM_ELEMENT_TYPE_STRUCT"
		nullable = typ.Nullable
	case *mojom_types.TypeHandleType:
		typ := dataType.Interface().(mojom_types.HandleType)
		elemTable = "NULL"
		elemType = "MOJOM_ELEMENT_TYPE_HANDLE"
		nullable = typ.Nullable
	case *mojom_types.TypeTypeReference:
		return table.getTableForUDT(dataType.Interface().(mojom_types.TypeReference))
	case *mojom_types.TypeSimpleType:
		elemTable = "NULL"
		elemType = "MOJOM_ELEMENT_TYPE_POD"
	default:
		log.Fatal("uhoh, should not be here.")
	}

	return
}

// Returns the name of the array type table.
// Takes in |prefix|, the name this array table entry should be.
func (table *TypeTableTemplate) makeArrayPointerEntry(prefix string, f mojom_types.ArrayType) string {
	numElements := uint32(0)
	if f.FixedLength > 0 {
		numElements = uint32(f.FixedLength)
	}
	entry := ArrayPointerTableEntry{
		Name:        prefix + "__PointerEntry",
		NumElements: numElements,
		Nullable:    f.Nullable,
	}
	entry.ElemTable, entry.ElemType, entry.Nullable = table.makeTableForType(prefix, f.ElementType)

	table.Arrays = append(table.Arrays, entry)
	return entry.Name
}

func (table *TypeTableTemplate) makeMapPointerTable(prefix string, f mojom_types.MapType) string {
	structTable := StructPointerTable{
		Name: prefix + "__PointerTable",
	}
	// The key array has offset 0.
	// The value array has offset 8.
	structTable.Entries = append(structTable.Entries,
		table.makeStructPointerTableEntry(fmt.Sprintf("%s_%d", prefix, 0), 0, 0, f.KeyType))
	structTable.Entries = append(structTable.Entries,
		table.makeStructPointerTableEntry(fmt.Sprintf("%s_%d", prefix, 8), 8, 0, f.ValueType))
	structTable.Entries[1].KeepGoing = false

	table.Structs = append(table.Structs, structTable)
	return structTable.Name
}

// A union in a struct is inlined. It could possibly have a pointer type in
// there, so we consider unions to be pointers for the purposes of this method.
// TODO(vardhan): To optimize, check that, if union, there is a reference
// type inside the union before deeming the union a pointer type.
func (table *TypeTableTemplate) isPointerOrHandle(typ mojom_types.Type) bool {
	switch typ.(type) {
	case *mojom_types.TypeStringType:
		return true
	case *mojom_types.TypeArrayType:
		return true
	case *mojom_types.TypeMapType:
		return true
	case *mojom_types.TypeHandleType:
		return true
	case *mojom_types.TypeTypeReference:
		typRef := typ.Interface().(mojom_types.TypeReference)
		if udt, exists := table.fileGraph.ResolvedTypes[*typRef.TypeKey]; exists {
			switch udt.(type) {
			case *mojom_types.UserDefinedTypeStructType:
				return true
			case *mojom_types.UserDefinedTypeUnionType:
				return true
			case *mojom_types.UserDefinedTypeInterfaceType:
				return true
			}
		} else {
			log.Fatal("No such type reference.")
		}
	}
	return false
}

// Returns the StructPointerTableEntry pertaining to the given |fieldType|,
// but won't insert it into the `table`; it is the caller's responsibility to
// insert it. However, this operation is NOT totally immutable, since it may
// create type tables for sub types of |fieldType| (e.g. if fieldType is a map).
func (table *TypeTableTemplate) makeStructPointerTableEntry(prefix string, offset uint32, minVersion uint32, fieldType mojom_types.Type) StructPointerTableEntry {
	elemTableName := fmt.Sprintf("%s_%d", prefix, offset)
	elemTable, elemType, nullable := table.makeTableForType(elemTableName, fieldType)
	return StructPointerTableEntry{
		ElemTable:  elemTable,
		Offset:     offset,
		MinVersion: minVersion,
		ElemType:   elemType,
		Nullable:   nullable,
		KeepGoing:  true,
	}
}

// Given a MojomStruct, creates the templates required to make the type table
// for it, and inserts it into |table|.
func (table *TypeTableTemplate) insertStructPointerTable(s mojom_types.MojomStruct) {
	structTablePrefix := mojomToCName(*s.DeclData.FullIdentifier)
	structTable := StructPointerTable{
		Name: structTablePrefix + "__PointerTable",
	}
	for _, field := range s.Fields {
		if table.isPointerOrHandle(field.Type) {
			structTable.Entries = append(structTable.Entries, table.makeStructPointerTableEntry(
				structTablePrefix, uint32(field.Offset), field.MinVersion, field.Type))
		}
	}
	if len(structTable.Entries) > 0 {
		structTable.Entries[len(structTable.Entries)-1].KeepGoing = false
	}
	table.PublicStructNames = append(table.PublicStructNames, structTable.Name)
	table.Structs = append(table.Structs, structTable)
}

func (table *TypeTableTemplate) makeUnionPointerTableEntry(prefix string, tag uint32, fieldType mojom_types.Type) UnionPointerTableEntry {
	elemTableName := fmt.Sprintf("%s_%d", prefix, tag)
	elemTable, elemType, nullable := table.makeTableForType(elemTableName, fieldType)
	return UnionPointerTableEntry{
		ElemTable: elemTable,
		Tag:       tag,
		Nullable:  nullable,
		ElemType:  elemType,
		KeepGoing: true,
	}
}

// Given a MojomUnion, creates the templates required to make the type table
// for it, and inserts it into |table|.
func (table *TypeTableTemplate) insertUnionPointerTable(u mojom_types.MojomUnion) {
	unionTablePrefix := mojomToCName(*u.DeclData.FullIdentifier)
	unionTable := UnionPointerTable{
		Name: unionTablePrefix + "__PointerTable",
	}
	for _, field := range u.Fields {
		if table.isPointerOrHandle(field.Type) {
			unionTable.Entries = append(unionTable.Entries, table.makeUnionPointerTableEntry(unionTablePrefix, uint32(field.Tag), field.Type))
		}
	}
	if len(unionTable.Entries) > 0 {
		unionTable.Entries[len(unionTable.Entries)-1].KeepGoing = false
	}
	table.PublicUnionNames = append(table.PublicUnionNames, unionTable.Name)
	table.Unions = append(table.Unions, unionTable)
}

func NewTypeTableTemplate(fileGraph *mojom_files.MojomFileGraph, file *mojom_files.MojomFile) TypeTableTemplate {
	table := TypeTableTemplate{
		fileGraph: fileGraph,
	}
	if file.DeclaredMojomObjects.Structs != nil {
		for _, mojomStructKey := range *(file.DeclaredMojomObjects.Structs) {
			mojomStruct := fileGraph.ResolvedTypes[mojomStructKey].Interface().(mojom_types.MojomStruct)
			table.insertStructPointerTable(mojomStruct)
		}
	}
	if file.DeclaredMojomObjects.Unions != nil {
		for _, mojomUnionKey := range *(file.DeclaredMojomObjects.Unions) {
			mojomUnion := fileGraph.ResolvedTypes[mojomUnionKey].Interface().(mojom_types.MojomUnion)
			table.insertUnionPointerTable(mojomUnion)
		}
	}
	if file.DeclaredMojomObjects.Interfaces != nil {
		for _, mojomInterfaceKey := range *(file.DeclaredMojomObjects.Interfaces) {
			mojomIface := fileGraph.ResolvedTypes[mojomInterfaceKey].Interface().(mojom_types.MojomInterface)
			for _, mojomMethod := range mojomIface.Methods {
				params := mojomMethod.Parameters
				fullId := requestMethodToCName(&mojomIface, &params)
				params.DeclData.FullIdentifier = &fullId
				table.insertStructPointerTable(params)
				if mojomMethod.ResponseParams != nil {
					params := *mojomMethod.ResponseParams
					fullId := responseMethodToCName(&mojomIface, &params)
					params.DeclData.FullIdentifier = &fullId
					table.insertStructPointerTable(params)
				}
			}
		}
	}
	return table
}
