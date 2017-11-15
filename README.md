# R.M.S.
A lightweight, shared memory based ROS like system for robotics.

The idea behind RMS was that ROS is great...but it's really sad on mobile and embedded systems.
So instead of waiting for ROS2 to come out, I chose to make my own version of ROS.

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
