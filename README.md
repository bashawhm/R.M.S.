# R.M.S.
A lightweight, shared memory based ROS like system for robotics.

The idea behind RMS was that ROS is great...but it's really sad on mobile and embedded systems.
So instead of waiting for ROS2 to come out, I chose to make my own version of ROS.

The TCP/IP Testing folder contains test code for, as one would expect, a TCP implementation of RMS. Currently only the Publish and Subscribe functions are working, all others *technically* work, but those nodes have to be on the same machine.

This TCP functionality will eventually be integrated into Version 3 of RMS, while will support both Shared memory when posible, but also supports TCP all in the same node. This is to be able to have the speed of shared memory when possible, but when not it will use TCP.

Version 2:
R.M.S. Argo


To Compile:

	Requirements:

		Debian-Based system (Idealy Ubuntu)
		nanomsg

	run: 

		gcc [name of program] -lnanomsg -lpthread
		Alternatively:
		clang [name of program] -lnanomsg -lpthread
