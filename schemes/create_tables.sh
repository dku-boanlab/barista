#!/bin/bash

echo "Create barista"
mysql -u root -p < barista.sql

echo "Create barista_mgmt"
mysql -u root -p < barista_mgmt.sql

echo "Create l2_learning"
mysql -u root -p < l2_learning.sql
