// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package main

import (
	"crypto/ecdsa"
	"crypto/elliptic"
	"crypto/rand"
	"fmt"
	"reflect"
	"sync"

	vpkg "mojo/services/vanadium/security/interfaces/principal"
)

type principal struct {
	private  *ecdsa.PrivateKey
	mu       sync.Mutex
	userList []vpkg.User // GUARDED_BY(mu)
	curr     *vpkg.User  // GUARDED_BY(mu)
}

func (p *principal) publicKey() publicKey {
	return newECDSAPublicKey(&p.private.PublicKey)
}

func (p *principal) users() []vpkg.User {
	p.mu.Lock()
	defer p.mu.Unlock()
	userList := make([]vpkg.User, len(p.userList))
	copy(userList, p.userList)
	return userList
}

func (p *principal) addUser(user vpkg.User) {
	p.mu.Lock()
	defer p.mu.Unlock()
	p.userList = append(p.userList, user)
	p.curr = &user
}

func (p *principal) setCurrentUser(user vpkg.User) (err *string) {
	p.mu.Lock()
	defer p.mu.Unlock()
	for _, u := range p.userList {
		if !reflect.DeepEqual(u, user) {
			str := fmt.Sprintf("User %v does not exist", user)
			return &str
		}
	}
	p.curr = &user
	return
}

func (p *principal) clearCurrentUser() {
	p.mu.Lock()
	defer p.mu.Unlock()
	p.curr = nil
}

func newPrincipal() (*principal, error) {
	priv, err := ecdsa.GenerateKey(elliptic.P256(), rand.Reader)
	if err != nil {
		return nil, err
	}
	return &principal{private: priv}, nil
}
