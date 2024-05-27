#!/bin/bash

# Compile and run the C program in the background
gcc p2.c
./a.out &

# Run the Love2D program
love .

# Wait for the background process to finish
wait
