CC = g++
CFLAGS = -Wall -Wextra -std=c++11

cache_simulate: cache_simulate.cpp
	$(CC) $(CFLAGS) cache_simulate.cpp -o cache_simulate

clean:
	rm -f cache_simulate
