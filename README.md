# Introduction
- Barista: An Event-centric NOS Composition Framework for Software-Defined Networks

# Notification
- If you find any bugs or have any questions, please send an e-mail to us.

# Configuration
- The configuration of the Barista NOS: config.mk
- The configurations of components and applications: bin/config
- The internal configurations for developers: src/include/common.h

# Test environment
- The current Barista is tested on Ubuntu 16.04.
- It may work on other Linux platforms if its dependency issues are solved.

# Compilation
1. Install dependencies
> $ ./deps.sh

2. Compile the code
> $ make

3. Clean up the code
> $ make clean

# Execution
- Run the Barista NOS
> $ cd bin  
> $ ./barista

- Run the Barista NOS automatically
> $ ./barista -r

- Run the Barista NOS as a daemon
> $ ./barista -d

- Run the Barista NOS with base components
> $ ./barista -b

- Connect the CLI of the Barista NOS
> $ telnet localhost 8000 (default port)
> (ID: admin, PW: password, privileged PW: admin\_password)  

# CLI commands
- Change a configuration mode
> Barista> turn on
- Load configurations
> Barista# load
- Start the Barista NOS
> Barista# start
- Stop the Barista NOS
> Barista# stop
- More information?
> Barista> help

# Documents
- Documents: http://www.sdx4u.net/category/barista
- Source browser: http://www.sdx4u.net/~barista

# Author
- Jaehyun Nam <namjh@kaist.ac.kr>

# Contributors
- Hyeonseong Jo <hsjjo@kaist.ac.kr>
- Yeonkeun kim <yeonk@kaist.ac.kr>
