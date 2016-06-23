#ifndef RAYLIB_EXCHANGE_FILE_DESCRIPTOR_H
#define RAYLIB_EXCHANGE_FILE_DESCRIPTOR_H

#include <string>

class FileDescriptorSender {
public:
  FileDescriptorSender(const std::string& address);
  ~FileDescriptorSender();
  bool Send(int file_descriptor);
private:
  int socket_;
};

class FileDescriptorReceiver {
public:
  FileDescriptorReceiver(const std::string& address);
  ~FileDescriptorReceiver();
  int Receive();
private:
  int socket_;
};

#endif
