// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:typed_data';

import 'package:_mojo_for_test_only/expect.dart';
import 'package:mojo/core.dart';

invalidHandleTest() {
  MojoHandle invalidHandle = new MojoHandle(MojoHandle.INVALID);

  // Close.
  int result = invalidHandle.close();
  Expect.isTrue(result == MojoResult.kInvalidArgument);

  // Wait.
  MojoWaitResult mwr =
      invalidHandle.wait(MojoHandleSignals.kReadWrite, 1000000);
  Expect.isTrue(mwr.result == MojoResult.kInvalidArgument);

  MojoWaitManyResult mwmr = MojoHandle.waitMany([invalidHandle.h],
      [MojoHandleSignals.kReadWrite], MojoHandle.DEADLINE_INDEFINITE);
  Expect.isTrue(mwmr.result == MojoResult.kInvalidArgument);

  // Message pipe.
  MojoMessagePipe pipe = new MojoMessagePipe();
  Expect.isNotNull(pipe);
  ByteData bd = new ByteData(10);
  pipe.endpoints[0].handle.close();
  pipe.endpoints[1].handle.close();
  result = pipe.endpoints[0].write(bd);
  Expect.isTrue(result == MojoResult.kInvalidArgument);

  MojoMessagePipeReadResult readResult = pipe.endpoints[0].read(bd);
  Expect.isTrue(pipe.endpoints[0].status == MojoResult.kInvalidArgument);

  // Data pipe.
  MojoDataPipe dataPipe = new MojoDataPipe();
  Expect.isNotNull(dataPipe);
  dataPipe.producer.handle.close();
  dataPipe.consumer.handle.close();

  int bytesWritten = dataPipe.producer.write(bd);
  Expect.isTrue(dataPipe.producer.status == MojoResult.kInvalidArgument);

  ByteData writeData = dataPipe.producer.beginWrite(10);
  Expect.isNull(writeData);
  Expect.isTrue(dataPipe.producer.status == MojoResult.kInvalidArgument);
  dataPipe.producer.endWrite(10);
  Expect.isTrue(dataPipe.producer.status == MojoResult.kInvalidArgument);

  int read = dataPipe.consumer.read(bd);
  Expect.isTrue(dataPipe.consumer.status == MojoResult.kInvalidArgument);

  ByteData readData = dataPipe.consumer.beginRead(10);
  Expect.isNull(readData);
  Expect.isTrue(dataPipe.consumer.status == MojoResult.kInvalidArgument);
  dataPipe.consumer.endRead(10);
  Expect.isTrue(dataPipe.consumer.status == MojoResult.kInvalidArgument);

  // Shared buffer.
  MojoSharedBuffer sharedBuffer = new MojoSharedBuffer.create(10);
  Expect.isNotNull(sharedBuffer);
  MojoSharedBufferInformation sharedBufferInformation =
      sharedBuffer.information;
  Expect.equals(sharedBufferInformation.flags, 0);
  Expect.equals(sharedBufferInformation.sizeInBytes, 10);
  sharedBuffer.close();
  sharedBufferInformation = sharedBuffer.information;
  Expect.isNull(sharedBufferInformation);
  MojoSharedBuffer duplicate = new MojoSharedBuffer.duplicate(sharedBuffer);
  Expect.isNull(duplicate);

  sharedBuffer = new MojoSharedBuffer.create(10);
  Expect.isNotNull(sharedBuffer);
  sharedBuffer.close();
  ByteData data = sharedBuffer.map(0, 10);
  Expect.isTrue(sharedBuffer.status == MojoResult.kInvalidArgument);
  Expect.isNull(data);
}

basicMessagePipeTest() {
  MojoMessagePipe pipe = new MojoMessagePipe();
  Expect.isNotNull(pipe);
  Expect.isTrue(pipe.status == MojoResult.kOk);
  Expect.isNotNull(pipe.endpoints);

  MojoMessagePipeEndpoint end0 = pipe.endpoints[0];
  MojoMessagePipeEndpoint end1 = pipe.endpoints[1];
  Expect.isTrue(end0.handle.isValid);
  Expect.isTrue(end1.handle.isValid);

  // Not readable, yet.
  MojoWaitResult mwr = end0.handle.wait(MojoHandleSignals.kReadable, 0);
  Expect.isTrue(mwr.result == MojoResult.kDeadlineExceeded);

  // Should be writable.
  mwr = end0.handle.wait(MojoHandleSignals.kWritable, 0);
  Expect.isTrue(mwr.result == MojoResult.kOk);

  // Try to read.
  ByteData data = new ByteData(10);
  end0.read(data);
  Expect.isTrue(end0.status == MojoResult.kShouldWait);

  // Write end1.
  String hello = "hello";
  ByteData helloData =
      new ByteData.view((new Uint8List.fromList(hello.codeUnits)).buffer);
  int result = end1.write(helloData);
  Expect.isTrue(result == MojoResult.kOk);

  // end0 should now be readable.
  MojoWaitManyResult mwmr = MojoHandle.waitMany([end0.handle.h],
      [MojoHandleSignals.kReadable], MojoHandle.DEADLINE_INDEFINITE);
  Expect.isTrue(mwmr.result == MojoResult.kOk);

  // Read from end0.
  MojoMessagePipeReadResult readResult = end0.read(data);
  Expect.isNotNull(readResult);
  Expect.isTrue(readResult.status == MojoResult.kOk);
  Expect.equals(readResult.bytesRead, helloData.lengthInBytes);
  Expect.equals(readResult.handlesRead, 0);

  String hello_result = new String.fromCharCodes(
      data.buffer.asUint8List().sublist(0, readResult.bytesRead).toList());
  Expect.equals(hello_result, "hello");

  // end0 should no longer be readable.
  mwr = end0.handle.wait(MojoHandleSignals.kReadable, 10);
  Expect.isTrue(mwr.result == MojoResult.kDeadlineExceeded);

  // Close end0's handle.
  result = end0.handle.close();
  Expect.isTrue(result == MojoResult.kOk);

  // end1 should no longer be readable or writable.
  mwr = end1.handle.wait(MojoHandleSignals.kReadWrite, 1000);
  Expect.isTrue(mwr.result == MojoResult.kFailedPrecondition);

  result = end1.handle.close();
  Expect.isTrue(result == MojoResult.kOk);
}

basicDataPipeTest() {
  MojoDataPipe pipe = new MojoDataPipe();
  Expect.isNotNull(pipe);
  Expect.isTrue(pipe.status == MojoResult.kOk);
  Expect.isTrue(pipe.consumer.handle.isValid);
  Expect.isTrue(pipe.producer.handle.isValid);

  MojoDataPipeProducer producer = pipe.producer;
  MojoDataPipeConsumer consumer = pipe.consumer;
  Expect.isTrue(producer.handle.isValid);
  Expect.isTrue(consumer.handle.isValid);

  // Consumer should not be readable.
  MojoWaitResult mwr = consumer.handle.wait(MojoHandleSignals.kReadable, 0);
  Expect.isTrue(mwr.result == MojoResult.kDeadlineExceeded);

  // Producer should be writable.
  mwr = producer.handle.wait(MojoHandleSignals.kWritable, 0);
  Expect.isTrue(mwr.result == MojoResult.kOk);

  // Try to read from consumer.
  ByteData buffer = new ByteData(20);
  consumer.read(buffer, buffer.lengthInBytes, MojoDataPipeConsumer.FLAG_NONE);
  Expect.isTrue(consumer.status == MojoResult.kShouldWait);

  // Try to begin a two-phase read from consumer.
  ByteData b = consumer.beginRead(20, MojoDataPipeConsumer.FLAG_NONE);
  Expect.isNull(b);
  Expect.isTrue(consumer.status == MojoResult.kShouldWait);

  // Write to producer.
  String hello = "hello ";
  ByteData helloData =
      new ByteData.view((new Uint8List.fromList(hello.codeUnits)).buffer);
  int written = producer.write(
      helloData, helloData.lengthInBytes, MojoDataPipeProducer.FLAG_NONE);
  Expect.isTrue(producer.status == MojoResult.kOk);
  Expect.equals(written, helloData.lengthInBytes);

  // Now that we have written, the consumer should be readable.
  MojoWaitManyResult mwmr = MojoHandle.waitMany([consumer.handle.h],
      [MojoHandleSignals.kReadable], MojoHandle.DEADLINE_INDEFINITE);
  Expect.isTrue(mwr.result == MojoResult.kOk);

  // Do a two-phase write to the producer.
  ByteData twoPhaseWrite =
      producer.beginWrite(20, MojoDataPipeProducer.FLAG_NONE);
  Expect.isTrue(producer.status == MojoResult.kOk);
  Expect.isNotNull(twoPhaseWrite);
  Expect.isTrue(twoPhaseWrite.lengthInBytes >= 20);

  String world = "world";
  twoPhaseWrite.buffer.asUint8List().setAll(0, world.codeUnits);
  producer.endWrite(Uint8List.BYTES_PER_ELEMENT * world.codeUnits.length);
  Expect.isTrue(producer.status == MojoResult.kOk);

  // Read one character from consumer.
  int read = consumer.read(buffer, 1, MojoDataPipeConsumer.FLAG_NONE);
  Expect.isTrue(consumer.status == MojoResult.kOk);
  Expect.equals(read, 1);

  // Close the producer.
  int result = producer.handle.close();
  Expect.isTrue(result == MojoResult.kOk);

  // Consumer should still be readable.
  mwr = consumer.handle.wait(MojoHandleSignals.kReadable, 0);
  Expect.isTrue(mwr.result == MojoResult.kOk);

  // Get the number of remaining bytes.
  int remaining = consumer.read(null, 0, MojoDataPipeConsumer.FLAG_QUERY);
  Expect.isTrue(consumer.status == MojoResult.kOk);
  Expect.equals(remaining, "hello world".length - 1);

  // Do a two-phase read.
  ByteData twoPhaseRead =
      consumer.beginRead(remaining, MojoDataPipeConsumer.FLAG_NONE);
  Expect.isTrue(consumer.status == MojoResult.kOk);
  Expect.isNotNull(twoPhaseRead);
  Expect.isTrue(twoPhaseRead.lengthInBytes <= remaining);

  Uint8List uint8_list = buffer.buffer.asUint8List();
  uint8_list.setAll(1, twoPhaseRead.buffer.asUint8List());
  uint8_list = uint8_list.sublist(0, 1 + twoPhaseRead.lengthInBytes);

  consumer.endRead(twoPhaseRead.lengthInBytes);
  Expect.isTrue(consumer.status == MojoResult.kOk);

  String helloWorld = new String.fromCharCodes(uint8_list.toList());
  Expect.equals("hello world", helloWorld);

  result = consumer.handle.close();
  Expect.isTrue(result == MojoResult.kOk);
}

basicSharedBufferTest() {
  MojoSharedBuffer mojoBuffer =
      new MojoSharedBuffer.create(100, MojoSharedBuffer.createFlagNone);
  Expect.isNotNull(mojoBuffer);
  Expect.isNotNull(mojoBuffer.status);
  Expect.isTrue(mojoBuffer.status == MojoResult.kOk);
  Expect.isNotNull(mojoBuffer.handle);
  Expect.isTrue(mojoBuffer.handle is MojoHandle);
  Expect.isTrue(mojoBuffer.handle.isValid);

  var mapping = mojoBuffer.map(0, 100, MojoSharedBuffer.mapFlagNone);
  Expect.isNotNull(mojoBuffer.status);
  Expect.isTrue(mojoBuffer.status == MojoResult.kOk);
  Expect.isNotNull(mapping);
  Expect.isTrue(mapping is ByteData);

  mapping.setInt8(50, 42);

  MojoSharedBuffer duplicate = new MojoSharedBuffer.duplicate(
      mojoBuffer, MojoSharedBuffer.duplicateFlagNone);
  Expect.isNotNull(duplicate);
  Expect.isNotNull(duplicate.status);
  Expect.isTrue(duplicate.status == MojoResult.kOk);
  Expect.isTrue(duplicate.handle is MojoHandle);
  Expect.isTrue(duplicate.handle.isValid);

  mapping = duplicate.map(0, 100, MojoSharedBuffer.mapFlagNone);
  Expect.isTrue(duplicate.status == MojoResult.kOk);
  Expect.isNotNull(mapping);
  Expect.isTrue(mapping is ByteData);

  mojoBuffer.close();
  mojoBuffer = null;

  mapping.setInt8(51, 43);

  mapping = duplicate.map(50, 50, MojoSharedBuffer.mapFlagNone);
  Expect.isNotNull(duplicate.status);
  Expect.isTrue(duplicate.status == MojoResult.kOk);
  Expect.isNotNull(mapping);
  Expect.isTrue(mapping is ByteData);

  Expect.equals(mapping.getInt8(0), 42);
  Expect.equals(mapping.getInt8(1), 43);

  duplicate.close();
  duplicate = null;
}

fillerDrainerTest() async {
  MojoDataPipe pipe = new MojoDataPipe();
  Expect.isNotNull(pipe);
  Expect.isTrue(pipe.status == MojoResult.kOk);
  Expect.isTrue(pipe.consumer.handle.isValid);
  Expect.isTrue(pipe.producer.handle.isValid);

  MojoDataPipeProducer producer = pipe.producer;
  MojoDataPipeConsumer consumer = pipe.consumer;
  Expect.isTrue(producer.handle.isValid);
  Expect.isTrue(consumer.handle.isValid);

  String sentMessage = "Hello world!" * 1000;
  ByteData producerData = new ByteData.view(UTF8.encode(sentMessage).buffer);
  DataPipeFiller.fillHandle(producer, producerData);

  ByteData consumerData = await DataPipeDrainer.drainHandle(consumer);
  Uint8List receivedBytes = new Uint8List.view(consumerData.buffer);
  String receivedMessage = new String.fromCharCodes(receivedBytes);

  Expect.equals(sentMessage, receivedMessage);
  Expect.isTrue(producer.status == MojoResult.kOk);
  Expect.isTrue(consumer.status == MojoResult.kOk);
}

utilsTest() {
  int ticksa = getTimeTicksNow();
  Expect.isTrue(1000 < ticksa);

  // Wait for the clock to advance.
  MojoWaitResult mwr = (new MojoMessagePipe()).endpoints[0]
      .handle
      .wait(MojoHandleSignals.kReadable, 1);
  Expect.isTrue(mwr.result == MojoResult.kDeadlineExceeded);

  int ticksb = getTimeTicksNow();
  Expect.isTrue(ticksa < ticksb);
}

// TODO(rudominer) This probably belongs in a different file.
processTest() {
  Expect.isTrue(pid > 0);
}

main() async {
  invalidHandleTest();
  basicMessagePipeTest();
  basicDataPipeTest();
  basicSharedBufferTest();
  await fillerDrainerTest();
  utilsTest();
  processTest();
}
