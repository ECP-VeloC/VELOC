#!/bin/bash
echo -n "LISTING SCRATCH"
ls -l $SCRATCH/*
echo -n "LISTING PERSISTENT"
ls -l $PERSISTENT/*

#rm -rf $SCRATCH/*
#rm -rf $PERSISTENT/*
