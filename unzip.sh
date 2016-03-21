#!/bin/bash
mkdir $1/tmp 2> /dev/null
rm $1/tmp/* 2> /dev/null
pat='completed_'
exit_pat='bb_exit_unzip'

for archive in $1/archive/*
 do
  if ! [[ $archive =~ $pat ]]; then
    rm $archive 2> /dev/null
  fi
done

while true
do
 sleep 1
 for archive in $1/archive/*
  do
   if [[ $archive =~ $exit_pat ]]; then
    echo > unzip.out "Unzip exit signal recieved\n"
    rm $archive
    exit
   fi
   
   if [[ $archive =~ $pat ]]; then
    tar --to-command='tar -C "./tmp" -xf -' -xzf $archive --strip-components=2
    mv `find $1/tmp/ -type f` "$1/csv/"
    rm $archive
   fi
  done
done