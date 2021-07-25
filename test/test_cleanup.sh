#!/bin/bash
#echo -n "LISTING SCRATCH"
#printenv $SCRATCH
#ls -l $SCRATCH/*
#echo -n "LISTING PERSISTENT"
#ls -l $PERSISTENT/*

#printenv $PERSISTENT
#rm -rf $SCRATCH/*
#rm -rf $PERSISTENT/*

rm -rf ~/persistent/*
#rm -rf /dev/shm/scratch/*
rm -rf /dev/shm/*
mkdir /dev/shm/scratch

