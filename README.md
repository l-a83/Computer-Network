# Project

This repository holds the projects made in the Computer Network course.

To compile the server:

gcc controller.c -o controller

To execute controller:

./controller 51717

//51717 is an arbitrary port, any available port from 2000 to 65535 can be used
//For the Texas State Servers, athena was used, but zeus, hercules, and eros can also be used to run the server instead

To compile the agent:

gcc agent.c -o agent

To execute agent:

./agent <hostName> 51717 “<action>”

//replace <hostName> with which ever SSH server was used for server implementation
//replace <action> with #JOIN, #LEAVE, #LIST, #LOG
