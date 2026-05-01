#!/usr/bin/env bash

input_file="$1"

# Some sample names to choose from
names=("Alice" "Bob" "Charlie" "Diana" "Eve" "Frank" "Grace" "Hank" "Ivy" "Jack")

while IFS=',' read -r f1 f2 f3 rest; do
    # Skip empty lines
    [[ -z "$f1" ]] && continue

    # Random number i (5 for example; adjust as needed)
    i=$((RANDOM % 30 + 1))

    for ((j=1; j<i; j++)); do
        # Random number between 1 and 10
        rand_num=$j

        # Random name
        rand_name=${names[$RANDOM % ${#names[@]}]}

        # Output: first 3 fields + random number + random name
        echo "${f1},${f2},${f3},${rand_num},${rand_name},1234567890"
    done
done < "$input_file"
