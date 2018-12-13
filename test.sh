#!/bin/bash
./pzip "$1.txt" > "$1.z"
./my_unzip "$1.z" > "$1_unzip.txt"
diff "$1.txt" "$1_unzip.txt"