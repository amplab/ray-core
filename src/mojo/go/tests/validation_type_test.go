// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package tests

import (
	"reflect"
	"testing"

	"mojo/public/go/application"
	"mojo/public/interfaces/bindings/mojom_types"
	"mojo/public/interfaces/bindings/service_describer"
	"mojo/public/interfaces/bindings/tests/test_unions"
	test "mojo/public/interfaces/bindings/tests/validation_test_interfaces"
)

// Perform a sanity check where we examine an MojomEnum's contents for correctness.
func TestEnumType(t *testing.T) {
	shortName := "BasicEnum"
	fullIdentifier := "mojo.test.BasicEnum"
	labelMap := map[string]int32{
		"A": 0,
		"B": 1,
		"C": 0,
		"D": -3,
		"E": 0xA,
	}

	// Extract *UserDefinedType from the validation test's Descriptor using BasicEnum.
	udt := test.BasicEnumMojomType()
	if udt == nil {
		t.Fatalf("Descriptor did not contain BasicEnum")
	}

	// The *UserDefinedType must be a *UserDefinedTypeEnumType.
	udet, ok := udt.(*mojom_types.UserDefinedTypeEnumType)
	if !ok {
		t.Fatalf("*UserDefinedType for %q was not *UserDefinedTypeEnumType", "BasicEnum")
	}

	// The UserDefinedTypeEnumType has a MojomEnum inside.
	me := udet.Value

	// Check that the generator produced the short name.
	if me.DeclData == nil || me.DeclData.ShortName == nil {
		t.Fatalf("Declaration Data's ShortName for BasicEnum is missing")
	}
	if *me.DeclData.ShortName != shortName {
		t.Fatalf("Declaration Data's ShortName for BasicEnum was %s, expected %s", *me.DeclData.ShortName, shortName)
	}
	if me.DeclData.FullIdentifier == nil {
		t.Fatalf("Declaration Data's FullIdentifier for BasicEnum is missing")
	}
	if *me.DeclData.FullIdentifier != fullIdentifier {
		t.Fatalf("Declaration Data's FullIdentifier for BasicEnum was %s, expected %s", *me.DeclData.FullIdentifier, fullIdentifier)
	}

	// Verify that the number of entries matches the expected ones.
	if got, expected := len(me.Values), len(labelMap); got != expected {
		t.Fatalf("MojomEnum for BasicEnum had %d labels, but expected %d", got, expected)
	}

	// Go through each entry, verify type info and that the enum label matches the expected one.
	for _, label := range me.Values {
		// label is an EnumValue.
		// Check that the generator produced the field's short name.
		if label.DeclData == nil || label.DeclData.ShortName == nil {
			t.Fatalf("Declaration Data's ShortName for BasicEnum's label is missing")
		}
		if _, ok := labelMap[*label.DeclData.ShortName]; !ok {
			t.Fatalf("Declaration Data's ShortName for BasicEnum's label %s is unknown ", *label.DeclData.ShortName)
		}
		// Check that the label's IntValue matches the expected one.
		if expectedOrdinal := labelMap[*label.DeclData.ShortName]; label.IntValue != expectedOrdinal {
			t.Fatalf("IntValue for Enum BasicEnum's label %s was %d but expected %d",
				*label.DeclData.ShortName, label.IntValue, expectedOrdinal)
		}
	}
}

// Perform a sanity check where we examine a MojomStruct's contents for correctness.
func TestStructType(t *testing.T) {
	shortName := "StructE"
	fullIdentifier := "mojo.test.StructE"
	fields := map[int]string{
		0: "struct_d",
		1: "data_pipe_consumer",
	}

	// Extract *UserDefinedType from the validation test's Descriptor using structID.
	udt := test.StructEMojomType()
	if udt == nil {
		t.Fatalf("Descriptor did not contain StructE")
	}

	// The *UserDefinedType must be a *UserDefinedTypeStructType.
	udst, ok := udt.(*mojom_types.UserDefinedTypeStructType)
	if !ok {
		t.Fatalf("*UserDefinedType for StructE was not *UserDefinedTypeStructType")
	}

	// The UserDefinedTypeStructType has a MojomStruct inside.
	ms := udst.Value

	// Check that the generator produced the short name.
	if ms.DeclData == nil || ms.DeclData.ShortName == nil {
		t.Fatalf("Declaration Data's ShortName for StructE is missing")
	}
	if *ms.DeclData.ShortName != shortName {
		t.Fatalf("Declaration Data's ShortName for StructE was %s, expected %s", *ms.DeclData.ShortName, shortName)
	}
	if ms.DeclData.FullIdentifier == nil {
		t.Fatalf("Declaration Data's FullIdentifier for StructE is missing")
	}
	if *ms.DeclData.FullIdentifier != fullIdentifier {
		t.Fatalf("Declaration Data's FullIdentifier for StructE was %s, expected %s", *ms.DeclData.FullIdentifier, fullIdentifier)
	}

	// Verify that the number of fields matches the expected ones.
	if got, expected := len(ms.Fields), len(fields); got != expected {
		t.Fatalf("MojomStruct for StructE had %d fields, but expected %d", got, expected)
	}

	// Go through each StructField, checking DeclData and the Type of each field.
	// Note that since ms.Fields is a slice, the "ordinal" is the index.
	for i, field := range ms.Fields {
		expectedFieldShortName := fields[i]
		// Check that the ShortName is correct.
		if field.DeclData == nil {
			t.Fatalf("StructField for StructE at ordinal %d had nil DeclData", i)
		}
		if field.DeclData.ShortName == nil {
			t.Fatalf("StructField for StructE at ordinal %d had nil DeclData.ShortName", i)
		}
		if *field.DeclData.ShortName != expectedFieldShortName {
			t.Fatalf("StructField for StructE at ordinal %d had ShortName %s, expecting %s", i, *field.DeclData.ShortName, expectedFieldShortName)
		}

		// Special case each field since we know what's inside.
		switch i {
		case 0:
			ttr, ok := field.Type.(*mojom_types.TypeTypeReference)
			if !ok {
				t.Fatalf("StructE's field 0 didn't have Type *TypeTypeReference")
			}

			// TypeTypeReference.Value is a TypeReference
			expectedReferenceID := "StructD"
			if *ttr.Value.Identifier != expectedReferenceID {
				t.Fatalf("TypeReference Identifier got %s, but expected %s", *ttr.Value.Identifier, expectedReferenceID)
			}
			fullID := test.GetAllMojomTypeDefinitions()[*ttr.Value.TypeKey].(*mojom_types.UserDefinedTypeStructType).Value.DeclData.FullIdentifier
			if *fullID != "mojo.test.StructD" {
				t.Fatalf("Unexpected fullID: %s", *fullID)
			}
		case 1:
			tht, ok := field.Type.(*mojom_types.TypeHandleType)
			if !ok {
				t.Fatalf("StructE's field 1 didn't have Type *TypeHandleType")
			}

			// TypeHandleType.Value is a HandleType
			if tht.Value.Nullable {
				t.Fatalf("StructE's field 1 should have a non-nullable TypeHandle")
			}
			if tht.Value.Kind != mojom_types.HandleType_Kind_DataPipeConsumer {
				t.Fatalf("StructE's field 1 has the wrong kind")
			}
		default:
			t.Fatalf("There should not be a field %d for StructE", i)
		}
	}
}

// Perform a sanity check where we examine a MojomUnion's contents for correctness.
func TestUnionType(t *testing.T) {
	shortName := "UnionB"
	fullIdentifier := "mojo.test.UnionB"
	fields := map[int]string{
		0: "a",
		1: "b",
		2: "c",
		3: "d",
	}

	// Extract *UserDefinedType from the validation test's Descriptor using UnionB.
	udt := test.UnionBMojomType()
	if udt == nil {
		t.Fatalf("Descriptor did not contain UnionB")
	}

	// The *UserDefinedType must be a *UserDefinedTypeUnionType.
	udut, ok := udt.(*mojom_types.UserDefinedTypeUnionType)
	if !ok {
		t.Fatalf("*UserDefinedType for UnionB was not *UserDefinedTypeUnionType")
	}

	// The UserDefinedTypeUnionType has a MojomUnion inside.
	mu := udut.Value

	// Check that the generator produced the short name.
	if mu.DeclData == nil || mu.DeclData.ShortName == nil {
		t.Fatalf("Declaration Data's ShortName for UnionB was missing")
	}
	if *mu.DeclData.ShortName != shortName {
		t.Fatalf("Declaration Data's ShortName for UnionB was %s, expected %s", *mu.DeclData.ShortName, shortName)
	}
	if mu.DeclData.FullIdentifier == nil {
		t.Fatalf("Declaration Data's FullIdentifier for UnionB is missing")
	}
	if *mu.DeclData.FullIdentifier != fullIdentifier {
		t.Fatalf("Declaration Data's FullIdentifier for UnionB was %s, expected %s", *mu.DeclData.FullIdentifier, fullIdentifier)
	}

	// Verify that the number of fields matches the expected ones.
	if got, expected := len(mu.Fields), len(fields); got != expected {
		t.Fatalf("MojomUnion for UnionB had %d fields, but expected %d", got, expected)
	}

	// Go through each UnionField, checking DeclData and the Type of each field.
	// Note that UnionField's rely on their Tag to determine their ordinal.
	// It is NOT in definition order, like with MojomStruct's.
	for i, field := range mu.Fields {
		//ordinal := field.Tag
		// TODO(rudominer) Currently the field.Tag is not being set by the Mojom parser.
		ordinal := i
		expectedFieldShortName := fields[int(ordinal)]
		// Check that the ShortName is correct.
		if field.DeclData == nil || field.DeclData.ShortName == nil || *field.DeclData.ShortName != expectedFieldShortName {
			t.Fatalf("UnionField for UnionB at ordinal %d had ShortName %s, expecting %s", ordinal, *field.DeclData.ShortName, expectedFieldShortName)
		}

		// It turns out that regardless of field ordinal, this union has TypeSimpleType for the type.
		tst, ok := field.Type.(*mojom_types.TypeSimpleType)
		if !ok {
			t.Fatalf("UnionB's field %d didn't have Type *TypeSimpleType", ordinal)
		}

		// Special case each field since we know what's inside.
		switch ordinal {
		case 0:
			if tst.Value != mojom_types.SimpleType_Uint16 {
				t.Fatalf("UnionB's field %d's Type value was not SimpleType_Uint16", ordinal)
			}
		case 1:
		case 3:
			if tst.Value != mojom_types.SimpleType_Uint32 {
				t.Fatalf("UnionB's field %d's Type value was not SimpleType_Uint32", ordinal)
			}
		case 2:
			if tst.Value != mojom_types.SimpleType_Uint64 {
				t.Fatalf("UnionB's field %d's Type value was not SimpleType_Uint64", ordinal)
			}
		default:
			t.Fatalf("There should not be a field Tag %d for UnionB", ordinal)
		}
	}
}

// Perform a sanity check for a struct that imports a union from another file.
// The descriptor should still handle it.
func TestStructWithImportType(t *testing.T) {
	shortName := "IncludingStruct"
	fullIdentifier := "mojo.test.IncludingStruct"
	fields := map[int]string{
		0: "a",
	}

	// Extract *UserDefinedType from the validation test's Descriptor using structID.
	udt := test_unions.IncludingStructMojomType()
	if udt == nil {
		t.Fatalf("Descriptor did not contain IncludingStruct")
	}

	// The *UserDefinedType must be a *UserDefinedTypeStructType.
	udst, ok := udt.(*mojom_types.UserDefinedTypeStructType)
	if !ok {
		t.Fatalf("*UserDefinedType for IncludingStruct was not *UserDefinedTypeStructType")
	}

	// The UserDefinedTypeStructType has a MojomStruct inside.
	ms := udst.Value

	// Check that the generator produced the short name.
	if ms.DeclData == nil || ms.DeclData.ShortName == nil {
		t.Fatalf("Declaration Data's ShortName for IncludingStruct is missing")
	}
	if *ms.DeclData.ShortName != shortName {
		t.Fatalf("Declaration Data's ShortName for IncludingStruct was %s, expected %s", *ms.DeclData.ShortName, shortName)
	}
	if ms.DeclData.FullIdentifier == nil {
		t.Fatalf("Declaration Data's FullIdentifier for IncludingStruct is missing")
	}
	if *ms.DeclData.FullIdentifier != fullIdentifier {
		t.Fatalf("Declaration Data's FullIdentifier for IncludingStruct was %s, expected %s", *ms.DeclData.FullIdentifier, fullIdentifier)
	}

	// Verify that the number of fields matches the expected ones.
	if got, expected := len(ms.Fields), len(fields); got != expected {
		t.Fatalf("MojomStruct for IncludingStruct had %d fields, but expected %d", got, expected)
	}

	includedUnionShortName := "IncludedUnion"
	includedUnionFullIdentifier := "mojo.test.IncludedUnion"

	// Go through each StructField, checking DeclData and the Type of each field.
	// Note that since ms.Fields is a slice, the "ordinal" is the index.
	for i, field := range ms.Fields {
		expectedFieldShortName := fields[i]
		// Check that the ShortName is correct.
		if field.DeclData == nil || field.DeclData.ShortName == nil || *field.DeclData.ShortName != expectedFieldShortName {
			t.Fatalf("StructField for IncludingStruct at ordinal %d had ShortName %s, expecting %s", i, *field.DeclData.ShortName, expectedFieldShortName)
		}

		// Special case each field since we know what's inside.
		switch i {
		case 0:
			ttr, ok := field.Type.(*mojom_types.TypeTypeReference)
			if !ok {
				t.Fatalf("IncludingStruct's field 0 didn't have Type *TypeTypeReference")
			}
			if *ttr.Value.Identifier != includedUnionShortName {
				t.Fatalf("TypeReference Identifier got %s, but expected %s", *ttr.Value.Identifier, includedUnionShortName)
			}
			fullID := test_unions.GetAllMojomTypeDefinitions()[*ttr.Value.TypeKey].(*mojom_types.UserDefinedTypeUnionType).Value.DeclData.FullIdentifier
			if *fullID != includedUnionFullIdentifier {
				t.Fatalf("Unexpected: %s", *fullID)
			}
		default:
			t.Fatalf("There should not be a field %d for IncludingStruct", i)
		}
	}
}

// Check that a MojomInterface has the right methods and the right struct definitions
// for its input and output. Further, its Interface_Request and Interface_Factory
// must expose a usable ServiceDescription.
func TestInterfaceType(t *testing.T) {
	// interface BoundsCheckTestInterface {
	//   Method0(uint8 param0) => (uint8 param0);
	//   Method1(uint8 param0);
	// };
	shortName := "BoundsCheckTestInterface"
	fullIdentifier := "mojo.test.BoundsCheckTestInterface"
	methodMap := map[uint32]string{
		0: "Method0",
		1: "Method1",
	}

	// Extract *UserDefinedType from the validation test's Descriptor using interfaceID.
	udt := test.BoundsCheckTestInterfaceMojomType()
	if udt == nil {
		t.Fatalf("Descriptor did not contain .BoundsCheckTestInterface")
	}

	// The *UserDefinedType must be a *UserDefinedTypeInterfaceType.
	udit, ok := udt.(*mojom_types.UserDefinedTypeInterfaceType)
	if !ok {
		t.Fatalf("*UserDefinedType for .BoundsCheckTestInterface was not *UserDefinedTypeInterfaceType")
	}

	// The UserDefinedTypeInterfaceType has a MojomInterface inside.
	mi := udit.Value
	checkMojomInterface(t, mi, shortName, fullIdentifier, methodMap)

	// Now, we must check the ServiceDescription(s) exposed by the autogenerated
	// ServiceRequest and ServiceFactory.
	var bcti_r test.BoundsCheckTestInterface_Request
	checkServiceDescription(t, bcti_r.ServiceDescription(), shortName, fullIdentifier, methodMap)

	var bcti_sf test.BoundsCheckTestInterface_ServiceFactory
	checkServiceDescription(t, bcti_sf.ServiceDescription(), shortName, fullIdentifier, methodMap)
}

func checkServiceDescription(t *testing.T, sd service_describer.ServiceDescription, shortName, fullIdentifier string, methodMap map[uint32]string) {
	// Check out the top level interface. This must pass checkMojomInterface.
	mi, err := sd.GetTopLevelInterface()
	if err != nil {
		t.Fatalf("Unexpected error for %s: %s", fullIdentifier, err)
	}
	checkMojomInterface(t, mi, shortName, fullIdentifier, methodMap)

	// Look at all the type definitions. Reflect-wise, all data inside should match the imported Descriptor.
	outDesc, err := sd.GetAllTypeDefinitions()
	if err != nil {
		t.Fatalf("Unexpected error %s", err)
	}
	if !reflect.DeepEqual(*outDesc, test.GetAllMojomTypeDefinitions()) {
		t.Fatalf("Descriptions did not match")
	}
}

func checkMojomInterface(t *testing.T, mi mojom_types.MojomInterface, shortName, fullIdentifier string, methodMap map[uint32]string) {
	// Check that the generator produced the short name.
	if mi.DeclData == nil || mi.DeclData.ShortName == nil {
		t.Fatalf("Declaration Data's ShortName for %s was missing", fullIdentifier)
	}
	if *mi.DeclData.ShortName != shortName {
		t.Fatalf("Declaration Data's ShortName for %s was %s, expected %s", fullIdentifier, *mi.DeclData.ShortName, shortName)
	}
	if mi.DeclData.FullIdentifier == nil {
		t.Fatalf("Declaration Data's FullIdentifier for %s was missing", fullIdentifier)
	}
	if *mi.DeclData.FullIdentifier != fullIdentifier {
		t.Fatalf("Declaration Data's FullIdentifier for %s was %s, expcected %s", fullIdentifier, *mi.DeclData.FullIdentifier, fullIdentifier)
	}

	// Verify that the number of methods matches the expected ones.
	if got, expected := len(mi.Methods), len(methodMap); got != expected {
		t.Fatalf("MojomInterface for %s had %d methods, but expected %d", fullIdentifier, got, expected)
	}

	// Go through each MojomMethod, checking DeclData and the Type of each field.
	// Note that since mi.Methods is a map, the "ordinal" is the key.
	for ordinal, method := range mi.Methods {
		expectedMethodShortName := methodMap[ordinal]
		// Check that the ShortName is correct.
		if method.DeclData == nil || method.DeclData.ShortName == nil || *method.DeclData.ShortName != expectedMethodShortName {
			t.Fatalf("MojomMethod for %s at ordinal %d did not have ShortName %s", fullIdentifier, ordinal, expectedMethodShortName)
		}

		// Special case each field since we know what's inside.
		switch ordinal {
		case 0:
			// We expect 0 to be a MojomMethod with both request and response params.
			params := method.Parameters
			if len(params.Fields) != 1 {
				t.Fatalf("Method0 Request had %d arguments, but should have had 1", len(params.Fields))
			}
			if tst, ok := params.Fields[0].Type.(*mojom_types.TypeSimpleType); !ok {
				t.Fatalf("Method0 Request param 0's Type should be a *TypeSimpleType")
			} else if tst.Value != mojom_types.SimpleType_Uint8 {
				t.Fatalf("Method0 Request param 0's Type's Value should be a SimpleType_Uint8")
			}

			response := method.ResponseParams
			if response == nil {
				t.Fatalf("Method0 Response should not be nil")
			}
			if len(response.Fields) != 1 {
				t.Fatalf("Method0 Response had %d arguments, but should have had 1", len(response.Fields))
			}
			if tst, ok := response.Fields[0].Type.(*mojom_types.TypeSimpleType); !ok {
				t.Fatalf("Method0 Response param 0's Type should be a *TypeSimpleType")
			} else if tst.Value != mojom_types.SimpleType_Uint8 {
				t.Fatalf("Method0 Response param 0's Type's Value should be a SimpleType_Uint8")
			}
		case 1:
			// We expect 1 to be a MojomMethod with a request and nil response params.
			params := method.Parameters
			if len(params.Fields) != 1 {
				t.Fatalf("Method1 Request had %d arguments, but should have had 1", len(params.Fields))
			}
			if tst, ok := params.Fields[0].Type.(*mojom_types.TypeSimpleType); !ok {
				t.Fatalf("Method1 Request param 0's Type should be a *TypeSimpleType")
			} else if tst.Value != mojom_types.SimpleType_Uint8 {
				t.Fatalf("Method1 Request param 0's Type's Value should be a SimpleType_Uint8")
			}

			if method.ResponseParams != nil {
				t.Fatalf("MojomMethod %d had a Response but should not have", ordinal)
			}
		default:
			t.Fatalf("There should not be a method %d for MojomInterface %s", ordinal, fullIdentifier)
		}
	}
}

// TestNonStaticMojomTypeAccessors tests the non-static mojom type accessors that are
// generated for structs, enums, union variants and interface requests.
func TestNonStaticMojomTypeAccessors(t *testing.T) {
	// Make an instance of a Mojo struct for StructB.
	var structB test.StructB

	// Get its MojomStruct
	mojomStruct := getMojomType(&structB).(*mojom_types.UserDefinedTypeStructType).Value
	if *mojomStruct.DeclData.FullIdentifier != "mojo.test.StructB" {
		t.Fatalf("*mojomStruct.DeclData.FullIdentifier = %q", *mojomStruct.DeclData.FullIdentifier)
	}

	// Get the type ref of its zeroth field.
	typeRef := mojomStruct.Fields[0].Type.(*mojom_types.TypeTypeReference).Value

	// Use AllMojomTypes() to get the MojomStruct that the reference has resolved to.
	userDefinedType, ok := getAllMojomTypes(&structB)[*typeRef.TypeKey]
	if !ok {
		t.Fatalf("No definition found for %q", *typeRef.TypeKey)
	}
	mojomStruct2 := userDefinedType.(*mojom_types.UserDefinedTypeStructType).Value
	if *mojomStruct2.DeclData.FullIdentifier != "mojo.test.StructA" {
		t.Fatalf("*mojomStruct2.DeclData.FullIdentifier = %q", *mojomStruct2.DeclData.FullIdentifier)
	}

	// Make an instance of a Mojo union for variant a of UnionA.
	var unionAVariantA test.UnionAA

	// Get its MojomUnion
	mojomUnion := getMojomType(&unionAVariantA).(*mojom_types.UserDefinedTypeUnionType).Value
	if *mojomUnion.DeclData.FullIdentifier != "mojo.test.UnionA" {
		t.Fatalf("*mojomUnion.DeclData.FullIdentifier = %q", *mojomUnion.DeclData.FullIdentifier)
	}

	// Get the type ref of field2
	typeRef = mojomUnion.Fields[2].Type.(*mojom_types.TypeTypeReference).Value

	// Use AllMojomTypes() to get the MojomStruct that the reference has resolved to.
	userDefinedType, ok = getAllMojomTypes(&unionAVariantA)[*typeRef.TypeKey]
	if !ok {
		t.Fatalf("No definition found for %q", *typeRef.TypeKey)
	}
	mojomStruct2 = userDefinedType.(*mojom_types.UserDefinedTypeStructType).Value
	if *mojomStruct2.DeclData.FullIdentifier != "mojo.test.StructA" {
		t.Fatalf("*mojomStruct2.DeclData.FullIdentifier = %q", *mojomStruct2.DeclData.FullIdentifier)
	}

	// Make an instance of a BasicEnum
	var basicEnum test.BasicEnum

	// Get its MojomEnum,
	mojomEnum := getMojomType(&basicEnum).(*mojom_types.UserDefinedTypeEnumType).Value
	if *mojomEnum.DeclData.FullIdentifier != "mojo.test.BasicEnum" {
		t.Fatalf("*mojomEnum.DeclData.FullIdentifier = %q", *mojomEnum.DeclData.FullIdentifier)
	}

	// Make an instance of an InterfaceA_Request
	var interfaceA_Request test.InterfaceA_Request

	// Get its MojomInterface,
	mojomInterface := getMojomType(&interfaceA_Request).(*mojom_types.UserDefinedTypeInterfaceType).Value
	if *mojomInterface.DeclData.FullIdentifier != "mojo.test.InterfaceA" {
		t.Fatalf("*mojomInterface.DeclData.FullIdentifier = %q", *mojomInterface.DeclData.FullIdentifier)
	}
}

func getMojomType(o application.ObjectWithMojomTypeSupport) mojom_types.UserDefinedType {
	return o.MojomType()
}

func getAllMojomTypes(o application.ObjectWithMojomTypeSupport) map[string]mojom_types.UserDefinedType {
	return o.AllMojomTypes()
}
