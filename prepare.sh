#!/bin/bash
set -e

# Download necessary datasets
cd datasets
./download.sh
python3 gen_linear_fb-1.py

# Build the RoBin project
cd ..
./build.sh release