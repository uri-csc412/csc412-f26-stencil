#!/bin/bash

# make an input file in the order your program expects:
#   threads, then (arrival, burst) pairs, then time quantum
# remember, burst is how long it runs for ;)
cat > input.txt << 'EOF'
3
0 5
1 3
2 5
1
EOF

### EXPLANATION OF VALUES
# 4    :: number of threads
### per thread info
# 0 5  :: arrival, burst
# 1 3  :: arrival, burst
# 2 5  :: arrival, burst
###
# 2    :: quantum

# run with that input
./main < input.txt
