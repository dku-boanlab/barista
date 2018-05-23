# Introduction
- Barista: An Event-centric NOS Composition Framework for Software-Defined Networks 

# Notification
- If you find any bugs or have any questions, please send an e-mail to us.  

# Configuration
- The configuration of the Barista NOS: config.mk  
- The configurations of components and applications: bin/config/  
- The internal configurations for developers: src/include/common.h  

# Test environment
- The current Barista is fully tested on Ubuntu 14.04, Ubuntu 16.04.  
- It may work on other Linux platforms if its dependency issues are solved.  

# Compilation
1. Install dependencies  
> $ ./deps.sh  
2. Compile the code  
> $ make  
3. Clean up the code  
> $ make clean  
4. Setup cluster environments (optional)  
> $ vi Makefile  
  > (CLUSTER = no -> CLUSTER = yes)  
> $ cd cluster  
> $ ./dep.sh  
> $ ./setup_cluster.sh [current NOS IP address] [another NOS IP address],[the other NOS IP address]  
  > (At least 3 nodes are required)  
> $ ./start_cluster.sh new (at the first NOS)  
> $ ./start_cluster.sh (at the other NOSs)  

# Execution
- Run the Barista NOS  
> $ cd bin  
> $ ./barista  
- Connect the Barista NOS  
> $ telnet localhost 8000 (default CLI port)  

# CLI commands
Barista> help  

# Documents
- Doxygen: http://www.sdx4u.net/~barista  

# Author
- Jaehyun Nam <namjh@kaist.ac.kr>  

# Contributors
- Hyeonseong Jo <hsjjo@kaist.ac.kr>  
- Yeonkeun kim <yeonk@kaist.ac.kr>  
