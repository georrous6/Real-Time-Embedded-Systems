#!/bin/bash

gcc prod_cons.c -lpthread

filename="data.txt"

# Clear previous results
echo "" > $filename

# Range of P and Q to test
for P in 1 2 4 8;
do
    for Q in {1..20};
    do
        echo "Running with P=$P, Q=$Q"
        ./a.out $P $Q $filename
    done
done

echo "All tests completed. Results in $filename"

python3 plot_data.py $filename
