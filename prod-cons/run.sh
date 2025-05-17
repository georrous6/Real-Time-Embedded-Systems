#!/bin/bash

gcc prod_cons.c -lpthread

# Clear previous results
echo "" > results.txt

# Range of P and Q to test
for P in 1 2 4 8
do
    for Q in 1 2 4 8
    do
        echo "Running with P=$P, Q=$Q"
        ./a.out $P $Q
    done
done

echo "All tests completed. Results in results.txt"
