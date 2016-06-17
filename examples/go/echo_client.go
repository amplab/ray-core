// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package main

import (
	"fmt"
	"log"

	"mojo/public/go/application"
	"mojo/public/go/bindings"
	"mojo/public/go/system"

	"examples/echo/echo"
)

//#include "mojo/public/c/system/handle.h"
//#include "mojo/public/c/system/result.h"
import "C"

type EchoClientDelegate struct{}

func (delegate *EchoClientDelegate) Initialize(ctx application.Context) {
	echoRequest, echoPointer := echo.CreateMessagePipeForEcho()
	ctx.ConnectToApplication("mojo:go_echo_server").ConnectToService(&echoRequest)
	echoProxy := echo.NewEchoProxy(echoPointer, bindings.GetAsyncWaiter())
	response, err := echoProxy.EchoString(bindings.StringPointer("Hello, Go world!"))
	if response != nil {
		fmt.Printf("client: %s\n", *response)
	} else {
		log.Println(err)
	}
	echoProxy.Close_Proxy()
	ctx.Close()
}

func (delegate *EchoClientDelegate) AcceptConnection(connection *application.Connection) {
	connection.Close()
}

func (delegate *EchoClientDelegate) Quit() {
}

//export MojoMain
func MojoMain(handle C.MojoHandle) C.MojoResult {
	application.Run(&EchoClientDelegate{}, system.MojoHandle(handle))
	return C.MOJO_RESULT_OK
}

func main() {
}
