// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package tests

import (
	test_enums_no_type_info "mojo/go/tests/test_enums"
	"mojo/public/interfaces/bindings/tests/test_enums"
	"testing"
)

// TestEnums just exercises the generated go code for enums. It tests that
// the generated enum value constants are of the correct type. The difference
// between test_enums and test_enums_no_type_info is that the former has been generated
// using 'generate_type_info = true' and the latter has not. Our only goal
// in including both is to minimally exercise both generation pathways.
func TestEnums(t *testing.T) {
	if test_enums.TestEnum_Foo != IdentityEnum1(test_enums.TestEnum_Foo) {
		t.Errorf("IdentityEnum1 failure.")
	}

	if test_enums.TestEnum2_Foo != IdentityEnum2(test_enums.TestEnum2_Foo) {
		t.Errorf("IdentityEnum1 failure.")
	}

	if test_enums_no_type_info.TestEnum_Foo != IdentityEnumB1(test_enums_no_type_info.TestEnum_Foo) {
		t.Errorf("IdentityEnum1 failure.")
	}

	if test_enums_no_type_info.TestEnum2_Foo != IdentityEnumB2(test_enums_no_type_info.TestEnum2_Foo) {
		t.Errorf("IdentityEnum1 failure.")
	}
}

func IdentityEnum1(x test_enums.TestEnum) test_enums.TestEnum {
	return x
}

func IdentityEnum2(x test_enums.TestEnum2) test_enums.TestEnum2 {
	return x
}

func IdentityEnumB1(x test_enums_no_type_info.TestEnum) test_enums_no_type_info.TestEnum {
	return x
}

func IdentityEnumB2(x test_enums_no_type_info.TestEnum2) test_enums_no_type_info.TestEnum2 {
	return x
}
