// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package tests

import (
	"fmt"
	"sync"
	"testing"

	test "mojo/public/interfaces/bindings/tests/regression_tests"
)

type methodWithEmptyResponseValidator struct {
	mu           sync.Mutex
	methodCalled bool
}

func (v *methodWithEmptyResponseValidator) WithoutParameterAndEmptyResponse() error {
	v.mu.Lock()
	v.methodCalled = true
	v.mu.Unlock()
	return nil
}

func (v *methodWithEmptyResponseValidator) WithParameterAndEmptyResponse(b bool) error {
	return nil
}

// TestCheckMethodWithEmptyResponse checks that the proxy waits for a server
// response if a method has 0 response arguments.
func TestCheckMethodWithEmptyResponse(t *testing.T) {
	iterations := 50
	for i := 0; i < iterations; i++ {
		interfaceRequest, interfacePointer := test.CreateMessagePipeForCheckMethodWithEmptyResponse()
		proxy := test.NewCheckMethodWithEmptyResponseProxy(interfacePointer, waiter)
		v := &methodWithEmptyResponseValidator{}
		stub := test.NewCheckMethodWithEmptyResponseStub(interfaceRequest, v, waiter)
		go func() {
			if err := stub.ServeRequest(); err != nil {
				panic(fmt.Sprintf("can't serve a request: %v", err))
			}
		}()
		if err := proxy.WithoutParameterAndEmptyResponse(); err != nil {
			t.Fatalf("error sending request: %v", err)
		}
		v.mu.Lock()
		if !v.methodCalled {
			t.Fatal("proxy returned from the call before the stub sent the response")
		}
		v.mu.Unlock()
		proxy.Close_Proxy()
		stub.Close()
	}
}
