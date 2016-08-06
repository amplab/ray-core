sudo apt-get install -y build-essential g++ wget libcap-dev g++-multilib python-dev
wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add -
sudo apt-get update -y
sudo apt-get install -y clang-3.8 lldb-3.8
