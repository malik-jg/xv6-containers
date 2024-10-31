#!/bin/sh

clang-format -i kernel/*.c kernel/*.h mkfs/mkfs.c user/*.c user/*.h
