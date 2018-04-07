#!/bin/bash

# check the cluster information
mysql -u root -p -e "show status like 'wsrep%';"
