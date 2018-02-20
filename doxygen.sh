#!/bin/bash

TARGET=svn

# remove the old documents
if [ "$TARGET" == "git" ]
then
	git rm -r docs/html
else # svn
	svn del docs/html
fi

# generate new documents
doxygen

# check errors
if [ $? -ne 0 ]
then
	exit
else
	rm -f G*
fi

# add and commit the new documents
if [ "$TARGET" == "git" ]
then
	git add docs/html
	git commit -m "update doxygen"
else # svn
	svn add docs/html
	svn ci -m "update doxygen"
fi

