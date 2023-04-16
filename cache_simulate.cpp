#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <cmath>

using namespace std;

#define uint unsigned int

#define L1_ACCESS_TIME 1     // in nanoseconds
#define L2_ACCESS_TIME 20    // in nanoseconds
#define DRAM_ACCESS_TIME 200 // in nanoseconds

// Congiguration variables
uint BLOCK_SIZE,
    SIZE_L1,
    ASSOC_L1,
    SIZE_L2,
    ASSOC_L2;
char *TRACEFILE;

// Statistics variables for L1 and L2 Cache
uint reads_cnt_L1,
    read_misses_cnt_L1,
    writes_cnt_L1,
    write_misses_cnt_L1,
    writebacks_cnt_L1,
    reads_cnt_L2,
    read_misses_cnt_L2,
    writes_cnt_L2,
    write_misses_cnt_L2,
    writebacks_cnt_L2;
bool isL2available;


class CacheLine
{
public:
    uint tag;
    uint addr;
    bool dirty;
    bool valid;

    CacheLine()
    {
        tag = 0;
        addr = 0;
        dirty = false;
        valid = false;
    }
};

class CacheL2
{
public:
    uint size, blocksize, assoc, numsets, tagsize, indexsize, offsetsize, **LRU;
    CacheLine **cache_line;

    CacheL2(uint SIZE, uint BLOCKSIZE, uint ASSOC) : size(SIZE), blocksize(BLOCKSIZE), assoc(ASSOC)
    {
        if (!isL2available)
            return;
        numsets = size / (blocksize * assoc);
        indexsize = log2(numsets);
        offsetsize = log2(blocksize);
        tagsize = 32 - indexsize - offsetsize;
        cache_line = new CacheLine *[numsets];
        LRU = new uint *[numsets];
        uint i = 0, k;
        while (i < numsets)
        {
            LRU[i] = new uint[assoc];
            k = assoc;
            while (k--)
                LRU[i][k] = k;
            cache_line[i++] = new CacheLine[assoc];
        }
    }

    void update(uint index, uint column)
    {
        uint temp = -1;
        uint i = 0;
        while (i < assoc)
        {
            if (LRU[index][i] == column)
            {
                temp = i;
                break;
            }
            i++;
        }

        uint j = temp;
        while (j > 0)
        {
            LRU[index][j] = LRU[index][j - 1];
            j--;
        }
        LRU[index][0] = column;
        return;
    }

    void read(uint tag, uint index)
    {
        reads_cnt_L2++;

        uint column = -1;
        uint i = 0;
        while (i < assoc)
        {
            if (cache_line[index][i].valid == true && cache_line[index][i].tag == tag)
            {
                column = i;
                this->update(index, column);
                return;
            }
            i++;
        }
        read_misses_cnt_L2++;

        uint k = 0;
        while (k < assoc)
        {
            if (cache_line[index][k].valid == false)
            {
                cache_line[index][k].dirty = false;
                cache_line[index][k].valid = true;
                cache_line[index][k].tag = tag;
                this->update(index, k);
                return;
            }
            k++;
        }

        uint evictColumn = LRU[index][assoc - 1];
        if (cache_line[index][evictColumn].dirty)
        {
            writebacks_cnt_L2++;
        }
        cache_line[index][evictColumn].tag = tag;
        cache_line[index][evictColumn].valid = true;
        cache_line[index][evictColumn].dirty = false;
        this->update(index, evictColumn);
        return;
    }

    void write(uint tag, uint index)
    {
        writes_cnt_L2++;
        uint i = 0;
        while (i < assoc)
        {
            if (cache_line[index][i].tag == tag)
            {
                this->update(index, i);
                cache_line[index][i].dirty = true;
                cache_line[index][i].valid = true;
                return;
            }
            i++;
        }

        write_misses_cnt_L2++;
        uint j = 0;
        while (j < assoc)
        {
            if (cache_line[index][j].valid == false)
            {
                cache_line[index][j].tag = tag;
                cache_line[index][j].dirty = true;
                cache_line[index][j].valid = true;
                this->update(index, j);
                return;
            }
            j++;
        }

        uint evictColumn = LRU[index][assoc - 1];
        if (cache_line[index][evictColumn].dirty == true)
        {
            writebacks_cnt_L2++;
        }
        cache_line[index][evictColumn].tag = tag;
        cache_line[index][evictColumn].dirty = true;
        this->update(index, evictColumn);
        return;
    }
};

class CacheL1
{
public:
    uint size, blocksize, assoc, numsets, tagsize, indexsize, offsetsize, **LRU;
    CacheLine **cache_line;

    CacheL1(uint SIZE, uint BLOCKSIZE, uint ASSOC) : size(SIZE), blocksize(BLOCKSIZE), assoc(ASSOC)
    {
        numsets = size / (blocksize * assoc);
        indexsize = log2(numsets);

        offsetsize = log2(blocksize);
        tagsize = 32 - indexsize - offsetsize;
        cache_line = new CacheLine *[numsets];
        LRU = new uint *[numsets];
        uint i = 0, k;
        while (i < numsets)
        {
            LRU[i] = new uint[assoc];
            k = assoc;
            while (k--)
                LRU[i][k] = k;
            cache_line[i++] = new CacheLine[assoc];
        }
    }
    void update(uint index, uint column)
    {
        uint i = 0, j = -1;
        while (i < assoc && LRU[index][i] != column)
            i++;

        if (i < assoc)
            j = i;

        // shifts used row to front of array. shifts other vals back
        if (j > 0)
        {
            LRU[index][j] = LRU[index][j - 1];
            while (--j > 0)
            {
                LRU[index][j] = LRU[index][j - 1];
            }
        }
        LRU[index][0] = column;

        return;
    }
    

    void read(uint tag, uint index, CacheL2 L2, uint hexaddress)
    {
        reads_cnt_L1++;
        // reads for direct hit
        uint column = -1;
        uint i = 0;
        while (i < assoc)
        {
            if (!(cache_line[index][i].valid == true && cache_line[index][i].tag == tag))
            {
                i++;
            }
            else
            {
                column = i;

                this->update(index, column);
                return;
            }
        }
        uint L2tag;
        uint L2index;
        if (!isL2available)
        {
            L2index = 0;
            L2tag = 0;
        }
        else
        {
            L2tag = hexaddress >> (L2.offsetsize + L2.indexsize);
            if (L2.numsets > 1)
            {
                L2index = (hexaddress << L2.tagsize) >> (L2.tagsize + L2.offsetsize);
            }
            else
            {
                L2index = 0;
            }
        }

        // read is not a hit, read miss up, check for empty blocks
        read_misses_cnt_L1++;
        uint k = 0;
        while (k < assoc)
        {
            if (!(cache_line[index][k].valid == false))
            {
                k++;
            }
            else
            {
                cache_line[index][k].dirty = false;
                cache_line[index][k].valid = true;
                cache_line[index][k].tag = tag;
                cache_line[index][k].addr = hexaddress;
                this->update(index, k);
                if (isL2available)
                    L2.read(L2tag, L2index);
                return;
            }
        }
        // No empty blocks, look for LRU cache_line and evict
        uint evictColumn = LRU[index][assoc - 1];
        if (cache_line[index][evictColumn].dirty)
        {
            writebacks_cnt_L1++;
            if (isL2available)
            {
                uint writebackaddress = cache_line[index][evictColumn].addr;
                uint writebacktag = writebackaddress >> (L2.offsetsize + L2.indexsize);
                uint writebackindex;
                if (L2.numsets > 1)
                {
                    writebackindex = (writebackaddress << L2.tagsize) >> (L2.tagsize + L2.offsetsize);
                }
                else
                {
                    writebackindex = 0;
                }
                L2.write(writebacktag, writebackindex);
            }
        }
        if (isL2available)
            L2.read(L2tag, L2index);

        cache_line[index][evictColumn].tag = tag;
        cache_line[index][evictColumn].valid = true;
        cache_line[index][evictColumn].dirty = false;
        cache_line[index][evictColumn].addr = hexaddress;
        this->update(index, evictColumn);
        return;
    }


    void write(uint tag, uint index, CacheL2 L2, uint hexaddress)
    {
        writes_cnt_L1++;
        uint i = 0;
        while (i < assoc)
        {
            if (!(cache_line[index][i].tag == tag))
            {
                i++;
            }
            else
            {
                this->update(index, i);
                cache_line[index][i].dirty = true;
                cache_line[index][i].valid = true;
                // hit found all good
                return;
            }
        }
        write_misses_cnt_L1++;
        uint L2tag;
        uint L2index;

        if (!isL2available)
        {
            L2index = 0;
            L2tag = 0;
        }
        else
        {
            L2tag = hexaddress >> (L2.offsetsize + L2.indexsize);
            if (L2.numsets > 1)
            {
                L2index = (hexaddress << L2.tagsize) >> (L2.tagsize + L2.offsetsize);
            }
            else
            {
                L2index = 0;
            }
        }

        uint j = 0;
        while (j < assoc)
        {
            if (!(cache_line[index][j].valid == false))
            {
                j++;
            }
            else
            {
                cache_line[index][j].tag = tag;
                cache_line[index][j].dirty = true;
                cache_line[index][j].valid = true;
                cache_line[index][j].addr = hexaddress;
                if (isL2available)
                    L2.read(L2tag, L2index);
                this->update(index, j);
                return;
            }
        }

        uint evictColumn = LRU[index][assoc - 1];
        if (!(cache_line[index][evictColumn].dirty == true))
        {
            if (isL2available)
                L2.read(L2tag, L2index);

            cache_line[index][evictColumn].addr = hexaddress;
            cache_line[index][evictColumn].tag = tag;
            cache_line[index][evictColumn].dirty = true;
            this->update(index, evictColumn);
            return;
        }
        else
        {
            writebacks_cnt_L1++;
            if (isL2available)
            {
                uint writebackaddress = cache_line[index][evictColumn].addr;
                uint writebacktag = writebackaddress >> (L2.offsetsize + L2.indexsize);
                uint writebackindex;
                if (L2.numsets > 1)
                {
                    writebackindex = (writebackaddress << L2.tagsize) >> (L2.tagsize + L2.offsetsize);
                }
                else
                {
                    writebackindex = 0;
                }
                L2.write(writebacktag, writebackindex);
            }

            if (isL2available)
                L2.read(L2tag, L2index);

            cache_line[index][evictColumn].addr = hexaddress;
            cache_line[index][evictColumn].tag = tag;
            cache_line[index][evictColumn].dirty = true;
            this->update(index, evictColumn);
            return;
        }
    }

};


void displayConfig()
{
    // Printing the configuration
    cout << "Simulator configurations:" << endl;
    cout << "  BLOCK SIZE:       " << BLOCK_SIZE << endl;
    cout << "  L1 SIZE:          " << SIZE_L1 << endl;
    cout << "  L1 ASSOCIATIVITY: " << ASSOC_L1 << endl;
    cout << "  L2 SIZE:          " << SIZE_L2 << endl;
    cout << "  L2 ASSOCIATIVITY: " << ASSOC_L2 << endl;
    cout << "  Trace File:       " << TRACEFILE << endl;
}

void displayStats(string cache_name, uint reads_cnt, uint read_misses_cnt, uint writes_cnt, uint write_misses_cnt, uint writebacks_cnt)
{
    // Print cache statistics
    cout << "\n+===================================+" << endl;
    cout << "|        " << cache_name << " Statistics        |" << endl;
    cout << "+-----------------------------------+" << endl;
    printf("| Number of reads:       %10d |\n", reads_cnt);
    printf("| Number of read misses: %10d |\n", read_misses_cnt);
    printf("| Number of writes:      %10d |\n", writes_cnt);
    printf("| Number of write misses:%10d |\n", write_misses_cnt);
    printf("| Miss rate:             %10.5f |\n", (read_misses_cnt + write_misses_cnt) / (double)(reads_cnt + writes_cnt));
    printf("| Number of writebacks:  %10d |\n", writebacks_cnt);
    cout << "+-----------------------------------+" << endl;
}

int main(int argc, char **argv)
{
    // Checking the no. of arguments
    if (argc != 7)
    {
        printf("Please input correct number of arguments\n");
        return 0;
    }

    BLOCK_SIZE = atoi(argv[1]);
    SIZE_L1 = atoi(argv[2]);
    ASSOC_L1 = atoi(argv[3]);
    SIZE_L2 = atoi(argv[4]);
    ASSOC_L2 = atoi(argv[5]);
    TRACEFILE = argv[6];

    // Printing the configuration
    displayConfig();

    // Initialize the L1 and L2 Cache
    CacheL1 L1 = CacheL1(SIZE_L1, BLOCK_SIZE, ASSOC_L1);
    isL2available = SIZE_L2 != 0 && ASSOC_L2 != 0;
    CacheL2 L2 = CacheL2(SIZE_L2, BLOCK_SIZE, ASSOC_L2);

    // Reading trace files and simulating
    freopen(TRACEFILE, "r", stdin);

    char operation;
    uint address, tag, cnt, index;
    while (scanf("%c %x\n", &operation, &address) != EOF) {
        tag = address >> (L1.offsetsize + L1.indexsize);
        if (L1.numsets <= 1)
            index = 0;
        else
            index = (address << L1.tagsize) >> (L1.tagsize + L1.offsetsize);
        if (operation == 'r') {
            L1.read(tag, index, L2, address);
        }
        else if (operation == 'w') {
            L1.write(tag, index, L2, address);
        }
    }

    // Print all the statistics
    displayStats("L1 Cache", reads_cnt_L1, read_misses_cnt_L1, writes_cnt_L1, write_misses_cnt_L1, writebacks_cnt_L1);
    displayStats("L2 Cache", reads_cnt_L2, read_misses_cnt_L2, writes_cnt_L2, write_misses_cnt_L2, writebacks_cnt_L2);

    // Computing total access time (in miliseconds)
    float time = ((reads_cnt_L1 + writes_cnt_L1) * L1_ACCESS_TIME + (reads_cnt_L2 + writes_cnt_L2) * L2_ACCESS_TIME + (read_misses_cnt_L2 + write_misses_cnt_L2 + writebacks_cnt_L2) * DRAM_ACCESS_TIME) * (1e-6);

    cout << "\nTotal access time (in miliseconds): " << time << endl;


    return 0;
}
