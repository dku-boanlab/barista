# Introduction
- Barista: An Event-centric Composable NOS for Software-Defined Networks (SDN Controller)

# Notification
- If you find any bugs or have any questions, please send an e-mail to us.

# Configuration
- The configuration of the Barista NOS: config.mk
- The configurations of components and applications: bin/config
- The credentials for a CLI and a database: bin/secrect

# Test environment
- The current Barista is tested on Ubuntu 16.04.
- It may work on other Linux platforms if its dependency issues are solved.

# Compilation
1. Install dependencies
> $ ./deps.sh  
> After setting 'root' password, please update 'secret/db_password.txt' as well

2. Compile the source code of Barista NOS
> $ make

3. Clean up the compiled files for Barista NOS
> $ make clean

4. Measure the performance of Barista NOS with Cbench
> $ cd  
> $ git clone https://github.com/sdx4u/benchmark  
> $ cd benchmark/cbench  
> $ ./setup.sh real  
> $ cd ~/barista  
> $ vi config.mk  
> Uncommet '__ENABLE_CBENCH'  
> $ make

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
> (default ID: admin, default PW: password)  

# CLI commands
- Change a configuration mode
> Barista> turn on/off

- Load configurations
> Barista# load

- Start the Barista NOS
> Barista# start

- Stop the Barista NOS
> Barista# stop

- More information?
> Barista> help

# Documents
- Documents: https://www.sdx4u.net/?cat=16
- Source browser: http://www.sdx4u.net/~barista

# Author
- Jaehyun Nam <namjh@kaist.ac.kr>
