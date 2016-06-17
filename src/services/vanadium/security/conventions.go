// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package main

import (
	"encoding/json"
	"fmt"
	"strings"
)

// TODO(ataly, ashankar): This constant is declared in the Mojom however it never
// makes it to the Go generated code. We should fix this.
const chainSeparator = "/"

// TODO(ataly): This is a hack! We should implement the security.BlessingNames
// function from the Vanadium API.
func name(chain []certificate) string {
	if len(chain) == 0 {
		return ""
	}
	name := chain[0].Extension
	for i := 1; i < len(chain); i++ {
		name = name + chainSeparator + chain[i].Extension
	}
	return name
}

// emailFromBlessing returns the email address from the provided JSON-encoded
// wire blessings obtained from the Vanadium identity provider. This function
// relies on the Vanadium identity provider employing the following convention
// for blessings returned in exchange for OAuth2 tokens: All blessings must be
// of the form: dev.v.io/u/<OAuth2 ClientID>/<user email>.
// See Also: https://godoc.org/v.io/v23/conventions
// TODO(ataly): Import "v23/conventions" here rather than duplicating
// the code.
func emailFromBlessing(b []uint8) (string, error) {
	var wb wireBlessings
	if err := json.Unmarshal(b, &wb); err != nil {
		return "", fmt.Errorf("failed to unmarshal response (blessings) from Vanadium Identity Provider: %v", err)
	}
	// TODO(ataly, gauthamt): Should we verify all signatures on the
	// certificate chains in the wire blessings to ensure that it was
	// not tampered with.
	var rejected []string
	for _, chain := range wb.CertificateChains {
		n := name(chain)
		// n is valid OAuth2 token based blessing name iff
		// n is of the form "dev.v.io/u/<clientID>/<email>"
		parts := strings.Split(n, chainSeparator)
		if len(parts) < 4 {
			rejected = append(rejected, n)
			continue
		}
		if (parts[0] != "dev.v.io") || (parts[1] != "u") {
			rejected = append(rejected, n)
			continue
		}
		// We assume that parts[2] must be the OAuth2 ClientID of
		// this service, and parts[3] must be the user's email.
		return parts[3], nil
	}
	return "", fmt.Errorf("the set of blessings (%v) obtained from the Vanadium identity provider does not contain any user blessing chain", rejected)
}
