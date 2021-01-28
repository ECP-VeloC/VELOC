#!/bin/bash
echo -n "LISTING SCRATCH"
printenv $SCRATCH
#ls -l $SCRATCH/*
echo -n "LISTING PERSISTENT"
#ls -l $PERSISTENT/*

printenv $PERSISTENT
#rm -rf $SCRATCH/*
#rm -rf $PERSISTENT/*
