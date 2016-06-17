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

	"mojo/services/http_server/interfaces/http_request"
	"mojo/services/http_server/interfaces/http_response"
	"mojo/services/http_server/interfaces/http_server"
	"mojo/services/http_server/interfaces/http_server_factory"
	"mojo/services/network/interfaces/net_address"
)

//#include "mojo/public/c/system/handle.h"
//#include "mojo/public/c/system/result.h"
import "C"

type HttpHandler struct{}

func (h *HttpHandler) HandleRequest(request http_request.HttpRequest) (http_response.HttpResponse, error) {
	resp := "Hello, Go http handler!"
	r, producer, consumer := system.GetCore().CreateDataPipe(&system.DataPipeOptions{
		system.MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,
		1,
		uint32(len(resp)),
	})
	if r != system.MOJO_RESULT_OK {
		panic(fmt.Sprintf("can't create data pipe: %v", r))
	}

	producer.WriteData([]byte(resp), system.MOJO_WRITE_DATA_FLAG_ALL_OR_NONE)
	producer.Close()
	return http_response.HttpResponse{
		200,
		&consumer,
		int64(len(resp)),
		"text/html; charset=utf-8",
		nil,
	}, nil
}

type HttpHandlerDelegate struct {
	server      *http_server.HttpServer_Proxy
	handlerStub *bindings.Stub
}

func (d *HttpHandlerDelegate) Initialize(ctx application.Context) {
	request, pointer := http_server_factory.CreateMessagePipeForHttpServerFactory()
	ctx.ConnectToApplication("mojo:http_server").ConnectToService(&request)
	factory := http_server_factory.NewHttpServerFactoryProxy(pointer, bindings.GetAsyncWaiter())

	addr := &net_address.NetAddress{
		net_address.NetAddressFamily_Ipv4,
		&net_address.NetAddressIPv4{
			8080,
			[4]uint8{127, 0, 0, 1},
		},
		nil,
	}
	serverRequest, serverPointer := http_server.CreateMessagePipeForHttpServer()
	factory.CreateHttpServer(serverRequest, addr)
	d.server = http_server.NewHttpServerProxy(serverPointer, bindings.GetAsyncWaiter())
	handlerRequest, handlerPointer := http_server.CreateMessagePipeForHttpHandler()
	ok, err := d.server.SetHandler("/go", handlerPointer)
	if !ok {
		log.Println("can't set handler:", err)
		return
	}

	d.handlerStub = http_server.NewHttpHandlerStub(handlerRequest, &HttpHandler{}, bindings.GetAsyncWaiter())
	go func() {
		for {
			if err := d.handlerStub.ServeRequest(); err != nil {
				log.Println("can't handle http request:", err)
				return
			}
		}
	}()
	factory.Close_Proxy()
}

func (d *HttpHandlerDelegate) AcceptConnection(c *application.Connection) {
	c.Close()
}

func (d *HttpHandlerDelegate) Quit() {
	d.server.Close_Proxy()
	d.handlerStub.Close()
}

//export MojoMain
func MojoMain(handle C.MojoHandle) C.MojoResult {
	application.Run(&HttpHandlerDelegate{}, system.MojoHandle(handle))
	return C.MOJO_RESULT_OK
}

func main() {
}
