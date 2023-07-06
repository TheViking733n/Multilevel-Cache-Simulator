# Multilevel-Cache-Simulator
A Command-Line Multi-Level Cache Simulator in C++

`Multilevel Cache Simulator` is a powerful multi-level cache simulator written in C++. This project provides a comprehensive and efficient platform to simulate L1 and L2 caches and analyze their performance. With a command-line interface, `Multilevel Cache Simulator` offers flexibility and ease of use for researchers, students, and enthusiasts exploring cache behavior and memory performance.

## Key Features:
* Simulate L1 and L2 cache hierarchies with customizable parameters.
* Evaluate cache hit rates, miss rates, and memory access latencies.
* Analyze the impact of cache size, associativity, replacement policies, and write policies.
* Supports various cache replacement policies such as LRU, FIFO, and random.
* Command-line interface for easy integration into scripts and automation.
* Generate detailed reports and statistics for in-depth analysis.
* Efficient and optimized codebase for fast simulations.
* Portable and cross-platform compatibility.


## Configurable Parameters
* BLOCKSIZE: Number of bytes in a block/line.
* L1_Size: Total bytes of data storage for L1 Cache.
* L1_Assoc: Associativity of L1 Cache.
* L2_Size: Total bytes of data storage for L2 Cache.
* L2_Assoc: Associativity of L2 Cache.

## Usage
Compile the source code using the following command:
```
g++ cache_simulate.cpp -o cache_simulate
```

Run the executable with the following command:
```
./cache_simulate <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <trace_file>
```

Example:
```
./cache_simulate 64 1024 2 65536 8 memory_trace_files/trace1.txt
```
./cache_simulate 64 1024 2 65536 8 memory_trace_files/trace1.txt

## Sample Output
```
Simulator configurations:
  BLOCK SIZE:       64   
  L1 SIZE:          1024 
  L1 ASSOCIATIVITY: 2    
  L2 SIZE:          65536
  L2 ASSOCIATIVITY: 8
  Trace File:       memory_trace_files/trace1.txt

+===================================+
|        L1 Cache Statistics        |
+-----------------------------------+
| Number of reads:            63640 |
| Number of read misses:      11757 |
| Number of writes:           36360 |
| Number of write misses:      5387 |
| Miss rate:                0.17144 |
| Number of writebacks:        6713 |
+-----------------------------------+

+===================================+
|        L2 Cache Statistics        |
+-----------------------------------+
| Number of reads:            17144 |
| Number of read misses:       1461 |
| Number of writes:            6713 |
| Number of write misses:         0 |
| Miss rate:                0.06124 |
| Number of writebacks:         232 |
+-----------------------------------+

Total access time (in miliseconds): 0.91574
```


## [License](/LICENSE)

For open-source projects, Under [MIT License](/LICENSE).