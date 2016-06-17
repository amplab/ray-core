// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

package main

import (
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/md5"
	"crypto/rand"
	"crypto/x509"
	"encoding"
	"errors"
	"fmt"
)

// hash identifies a cryptographic hash function approved for use in signature algorithms.
type hash string

const (
	sha1Hash   = hash("SHA1")   // sha1 cryptographic hash function defined in RFC3174.
	sha256Hash = hash("SHA256") // sha256 cryptographic hash function defined  in FIPS 180-4.
	sha384Hash = hash("SHA384") // sha384 cryptographic hash function defined in FIPS 180-2.
	sha512Hash = hash("SHA512") // sha512 cryptographic hash function defined in FIPS 180-2.
)

// newPrincipalKey generates an ECDSA (public, private) key pair.
func newPrincipalKey() (publicKey, *ecdsa.PrivateKey, error) {
	priv, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	if err != nil {
		return nil, nil, err
	}
	return newECDSAPublicKey(&priv.PublicKey), priv, nil
}

// publicKey represents a public key using an unspecified algorithm.
//
// MarshalBinary returns the DER-encoded PKIX representation of the public key,
// while UnmarshalPublicKey creates a PublicKey object from the marshaled bytes.
//
// String returns a human-readable representation of the public key.
type publicKey interface {
	encoding.BinaryMarshaler
	fmt.Stringer

	// hashFn returns a cryptographic hash function whose security strength is
	// appropriate for creating message digests to sign with this public key.
	// For example, an ECDSA public key with a 512-bit curve would require a
	// 512-bit hash function, whilst a key with a 256-bit curve would be
	// happy with a 256-bit hash function.
	hashFn() hash
	implementationsOnlyInThisPackage()
}

type ecdsaPublicKey struct {
	key *ecdsa.PublicKey
}

func (pk *ecdsaPublicKey) MarshalBinary() ([]byte, error)    { return x509.MarshalPKIXPublicKey(pk.key) }
func (pk *ecdsaPublicKey) String() string                    { return publicKeyString(pk) }
func (pk *ecdsaPublicKey) implementationsOnlyInThisPackage() {}
func (pk *ecdsaPublicKey) hashFn() hash {
	if nbits := pk.key.Curve.Params().BitSize; nbits <= 160 {
		return sha1Hash
	} else if nbits <= 256 {
		return sha256Hash
	} else if nbits <= 384 {
		return sha384Hash
	} else {
		return sha512Hash
	}
}

func publicKeyString(pk publicKey) string {
	bytes, err := pk.MarshalBinary()
	if err != nil {
		return fmt.Sprintf("<invalid public key: %v>", err)
	}
	const hextable = "0123456789abcdef"
	h := md5.Sum(bytes)
	var repr [md5.Size * 3]byte
	for i, v := range h {
		repr[i*3] = hextable[v>>4]
		repr[i*3+1] = hextable[v&0x0f]
		repr[i*3+2] = ':'
	}
	return string(repr[:len(repr)-1])
}

// unmarshalPublicKey returns a PublicKey object from the DER-encoded PKIX representation of it
// (typically obtianed via PublicKey.MarshalBinary).
func unmarshalPublicKey(bytes []byte) (publicKey, error) {
	key, err := x509.ParsePKIXPublicKey(bytes)
	if err != nil {
		return nil, err
	}
	switch v := key.(type) {
	case *ecdsa.PublicKey:
		return &ecdsaPublicKey{v}, nil
	default:
		return nil, errors.New(fmt.Sprintf("Unrecognized key: %T", key))
	}
}

// newECDSAPublicKey creates a publicKey object that uses the ECDSA algorithm and the provided ECDSA public key.
func newECDSAPublicKey(key *ecdsa.PublicKey) publicKey {
	return &ecdsaPublicKey{key}
}
