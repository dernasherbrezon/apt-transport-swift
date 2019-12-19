About
=======

Additional "swift" protocol for apt so you can host your giant apt repository in Openstack swift

apt is a great packaging system and Openstack swift is a great place to backup/store static files.  apt-swift is especially useful and fast if you are hosting your servers within Openstack.

Installation
=================

	sudo add-apt-repository ppa:rodionovamp/apt-transport-swift
	sudo apt-get update
	sudo apt-get install apt-transport-swift


Compile
=======


	mkdir build && cd build
	cmake ..
	make
	make test


Once compiled, the resulting swift binary must be placed in /usr/lib/apt/methods/ along with the other protocol binaries.

Configuration
=======

This is how you add it to the /etc/apt/sources.list file:

	deb swift://container prod main

Configuration should be put into /etc/apt/apt.conf.d/:

	Swift {
	 Container0 {
	   Name "container";
	   URL "https://example.com";
	   Username "username";
	   Password "password";
	 };
	};

You could configure several private containers on different hosts:

	Swift {
	 Container0 {
	   Name "container";
	   URL "https://example.com";
	   Username "username";
	   Password "password";
	 };
	 Container1 {
	   Name "container2";
	   URL "https://2.example.com";
	   Username "username";
	   Password "password";
	 }; 
	};


Simply upload all of your .deb packages and Packages.gz file into the Openstack swift container you chose with the file key mapping that matches the file system layout.