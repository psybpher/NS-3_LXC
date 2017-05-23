# -*- mode: ruby -*-
# vi: set ft=ruby :

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure("2") do |config|
   config.vm.box = "bento/ubuntu-16.04"
   config.vm.synced_folder "./", "/code"

   config.vm.provider "virtualbox" do |vb|
     vb.memory = "1024"
   end
  #documentation for more information about their specific syntax and use.
   config.vm.provision "shell", inline: <<-SHELL
     apt-get update
     apt-get install -y vim sudo tmux htop g++ clang libyaml-cpp-dev libboost-dev lxc lxc-dev cmake
     apt-get install -y pkg-config valgrind doxygen sqlite3 cmake
     wget https://www.nsnam.org/release/ns-allinone-3.26.tar.bz2
     tar -xbvf ./ns-allinone-3.26.tar.bz2
     cd ns-allinone-3.26
     ./build.py
     cd ns-3.26
     CXXFLAGS="-std=c++11" ./waf -d debug --enable-examples --enable-tests configure
     cd ~
     git clone https://github.com/buzz66boy/NS-3_LXC.git
     cd NS-3_LXC
     cmake .
     make     
     
   SHELL
end
