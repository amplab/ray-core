// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package main

// wireBlessings encapsulates wire format of a set of a Vanadium blessings
// and the corresponding cryptographic proof that binds them to a principal
// (identified by a public key).
type wireBlessings struct {
	// CertificateChains is an array of chains of certificates that bind
	// a blessing to the public key in the last certificate of the chain.
	CertificateChains [][]certificate
}

// certificate represents the cryptographic proof of the binding of
// extensions of a blessing held by one principal to another (represented by
// a public key) under specific caveats.
//
// For example, if a principal P1 has a blessing "alice", then it can
// extend it with a Certificate to generate the blessing "alice/friend" for
// another principal P2.
type certificate struct {
	Extension string    // Human-readable string extension bound to PublicKey.
	PublicKey []byte    // DER-encoded PKIX public key.
	Caveats   []caveat  // Caveats on the binding of Name to PublicKey.
	Signature signature // Signature by the blessing principal that binds the extension to the public key.
}

// signature represents a digital signature.
type signature struct {
	// Purpose of the signature. Can be used to prevent type attacks.
	// (See Section 4.2 of http://www-users.cs.york.ac.uk/~jac/PublishedPapers/reviewV1_1997.pdf for example).
	// The actual signature (R, S values for ECDSA keys) is produced by signing: Hash(Hash(message), Hash(Purpose)).
	Purpose []byte
	// Cryptographic hash function applied to the message before computing the signature.
	Hash hash
	// Pair of integers that make up an ECDSA signature.
	R []byte
	S []byte
}

// caveat is a condition on the validity of a blessing/discharge.
//
// These conditions are provided when asking a principal to create
// a blessing/discharge, and are verified when validating blessings
//
// Given a Hash, the message digest of a caveat is:
// Hash(Hash(Id), Hash(ParamVom))
type caveat struct {
	Id       [16]byte // The identifier of the caveat validation function.
	ParamVom []byte   // VOM-encoded bytes of the parameters to be provided to the validation function.
}
