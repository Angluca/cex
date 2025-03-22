#!/bin/bash

# Create the output directory if it doesn't exist
mkdir -p ../build/os_test

# Iterate over all .c files in the current directory
for cfile_name in *.c; do
    # Strip the .c extension to get the base name
    cfile_base_name="${cfile_name%.c}"

    # Compile the .c file and output the executable to ../build/os_test/<cfile_base_name>
    gcc -o "../build/os_test/${cfile_base_name}" "${cfile_name}"

    # Check if the compilation was successful
    if [ $? -eq 0 ]; then
        echo "Compiled ${cfile_name} -> ../build/os_test/${cfile_base_name}"
    else
        echo "Failed to compile ${cfile_name}"
    fi
done
