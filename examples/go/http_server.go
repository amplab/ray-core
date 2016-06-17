// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package main

import (
	"fmt"
	"log"
	"net"
	"net/http"
	"time"

	"mojo/public/go/application"
	"mojo/public/go/bindings"
	"mojo/public/go/system"

	"mojo/services/network/interfaces/net_address"
	"mojo/services/network/interfaces/network_service"
	"mojo/services/network/interfaces/tcp_bound_socket"
	"mojo/services/network/interfaces/tcp_connected_socket"
	"mojo/services/network/interfaces/tcp_server_socket"
)

//#include "mojo/public/c/system/handle.h"
//#include "mojo/public/c/system/result.h"
import "C"

type MojoConnection struct {
	sendStream    system.ProducerHandle
	receiveStream system.ConsumerHandle
	socket        system.MessagePipeHandle
	localAddr     net.Addr
	remoteAddr    net.Addr
}

// Implements net.Conn.
func (c *MojoConnection) Read(b []byte) (int, error) {
	r, read := c.receiveStream.BeginReadData(system.MOJO_READ_DATA_FLAG_NONE)
	if r != system.MOJO_RESULT_OK {
		return 0, fmt.Errorf("can't read: %v", r)
	}
	l := len(b)
	if (l > len(read)) {
		l = len(read)
	}
	copy(b, read[:l])
	if r := c.receiveStream.EndReadData(l); r != system.MOJO_RESULT_OK {
		return 0, fmt.Errorf("can't read %v bytes: %v", l, r)
	}
	return l, nil
}

// Implements net.Conn.
func (c *MojoConnection) Write(b []byte) (int, error) {
	r, buf := c.sendStream.BeginWriteData(system.MOJO_WRITE_DATA_FLAG_NONE)
	if r != system.MOJO_RESULT_OK {
		return 0, fmt.Errorf("can't write: %v", r)
	}
	l := len(b)
	if l > len(buf) {
		l = len(buf)
	}
	copy(buf, b[:l])
	if r := c.sendStream.EndWriteData(l); r != system.MOJO_RESULT_OK {
		return 0, fmt.Errorf("can't write %v bytes: %v", l, r)
	}
	return l, nil
}

// Implements net.Conn.
func (c *MojoConnection) Close() error {
	c.sendStream.Close()
	c.receiveStream.Close()
	c.socket.Close()
	return nil
}

// Implements net.Conn.
func (c *MojoConnection) LocalAddr() net.Addr {
	return c.localAddr
}

// Implements net.Conn.
func (c *MojoConnection) RemoteAddr() net.Addr {
	return c.remoteAddr
}

// Implements net.Conn.
func (c *MojoConnection) SetDeadline(t time.Time) error {
	if err := c.SetReadDeadline(t); err != nil {
		return err
	}
	return c.SetWriteDeadline(t)
}

// Implements net.Conn.
func (c *MojoConnection) SetReadDeadline(t time.Time) error {
	// Do nothing.
	return nil
}

// Implements net.Conn.
func (c *MojoConnection) SetWriteDeadline(t time.Time) error {
	// Do nothing.
	return nil
}

type MojoListener struct {
	serverSocket *tcp_server_socket.TcpServerSocket_Proxy
}

// Implements net.Listener.
func (l *MojoListener) Accept() (net.Conn, error) {
	r, sendProducer, sendConsumer := system.GetCore().CreateDataPipe(nil)
	if r != system.MOJO_RESULT_OK {
		panic(fmt.Sprintf("can't create data pipe: %v", r))
	}
	r, receiveProducer, receiveConsumer := system.GetCore().CreateDataPipe(nil)
	if r != system.MOJO_RESULT_OK {
		panic(fmt.Sprintf("can't create data pipe: %v", r))
	}
	request, pointer := tcp_connected_socket.CreateMessagePipeForTcpConnectedSocket()
	networkError, remoteAddress, err := l.serverSocket.Accept(sendConsumer, receiveProducer, request)
	if err != nil {
		return nil, err
	}
	if networkError.Code != 0 {
		return nil, fmt.Errorf("%s", *networkError.Description)
	}

	var addr net.Addr
	if remoteAddress.Ipv4 != nil {
		addr = &net.TCPAddr{
			IP:   remoteAddress.Ipv4.Addr[:4],
			Port: int(remoteAddress.Ipv4.Port),
		}
	} else {
		addr = &net.TCPAddr{
			IP:   remoteAddress.Ipv6.Addr[:16],
			Port: int(remoteAddress.Ipv6.Port),
		}
	}
	return &MojoConnection{
		sendProducer,
		receiveConsumer,
		pointer.PassMessagePipe(),
		l.Addr(),
		addr,
	}, nil
}

// Implements net.Listener.
func (l *MojoListener) Close() error {
	l.serverSocket.Close_Proxy()
	return nil
}

// Implements net.Listener.
func (l *MojoListener) Addr() net.Addr {
	return &net.TCPAddr{
		IP:   []byte{127, 0, 0, 1},
		Port: 8080,
	}
}

type HttpServerDelegate struct {
	networkService *network_service.NetworkService_Proxy
	tcpBoundSocket *tcp_bound_socket.TcpBoundSocket_Proxy
	serverSocket   *tcp_server_socket.TcpServerSocket_Proxy
}

func (d *HttpServerDelegate) InitTCPBoundSocket() error {
	addr := &net_address.NetAddress{
		net_address.NetAddressFamily_Ipv4,
		&net_address.NetAddressIPv4{
			8080,
			[4]uint8{127, 0, 0, 1},
		},
		nil,
	}
	request, pointer := tcp_bound_socket.CreateMessagePipeForTcpBoundSocket()
	networkError, _, err := d.networkService.CreateTcpBoundSocket(addr, request)
	if err != nil {
		return err
	}
	if networkError.Code != 0 {
		return fmt.Errorf("%s", *networkError.Description)
	}
	d.tcpBoundSocket = tcp_bound_socket.NewTcpBoundSocketProxy(pointer, bindings.GetAsyncWaiter())
	return nil
}

func (d *HttpServerDelegate) InitServerSocket() error {
	request, pointer := tcp_server_socket.CreateMessagePipeForTcpServerSocket()
	networkError, err := d.tcpBoundSocket.StartListening(request)
	if err != nil {
		return err
	}
	if networkError.Code != 0 {
		return fmt.Errorf("%s", *networkError.Description)
	}
	d.serverSocket = tcp_server_socket.NewTcpServerSocketProxy(pointer, bindings.GetAsyncWaiter())
	return nil
}

func (d *HttpServerDelegate) Initialize(ctx application.Context) {
	request, pointer := network_service.CreateMessagePipeForNetworkService()
	ctx.ConnectToApplication("mojo:network_service").ConnectToService(&request)
	d.networkService = network_service.NewNetworkServiceProxy(pointer, bindings.GetAsyncWaiter())

	if err := d.InitTCPBoundSocket(); err != nil {
		log.Printf("can't create TCP socket: %v\n", err)
		return
	}
	if err := d.InitServerSocket(); err != nil {
		log.Printf("can't create server socket: %v\n", err)
		return
	}

	http.HandleFunc("/go", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprintf(w, "Hello, Go http server!")
	})
	l := &MojoListener{d.serverSocket}
	if err := http.Serve(l, nil); err != nil {
		log.Printf("can't serve request: %v\n", err)
		return
	}
}

func (d *HttpServerDelegate) AcceptConnection(c *application.Connection) {
	c.Close()
}

func (d *HttpServerDelegate) Quit() {
	d.networkService.Close_Proxy()
	d.tcpBoundSocket.Close_Proxy()
	d.serverSocket.Close_Proxy()
}

//export MojoMain
func MojoMain(handle C.MojoHandle) C.MojoResult {
	application.Run(&HttpServerDelegate{}, system.MojoHandle(handle))
	return C.MOJO_RESULT_OK
}

func main() {
}
