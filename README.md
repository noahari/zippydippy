# zippydippy
A phenomenology of Parallelized Serial Run-Length Encoding

## How do we parallelize data?
Get the number of processes on the computer and store it locally.

Open the file and seek to find its length, storing that locally.

Divide the data up into i chunks where i = file size/4kb (i.e. chunk size = 4kb).

If there are fewer chunks than processors, we overwrite our number of processors.

We fill occupy our available processors with a thread each running Producer.

When Producer is done, it stores its parsed data in a struct.

Consumer is run in main thread.

##How are chunks divided?
Chunks are mapped based on page side to work with mmap

Most of the time then chunks will be of size 4096

## How do we use threads?
The Number of threads = number of cores on processors by use of get_nprocs (avoiding risk of inactive processors)

Each ith thread will be given a chunk of the data.

When a thread finishes its chunk, it finds another available chunk, if there are none it finishes.

When a thread finishes it stores its data in the struct before finding another chunk.
## What data structures?
A struct containing:

	Array containing pointers to parsed chunks. 

	A BITMAP that holds a valid bit

	A pointer to the input file

	An int of the total number of chunks in the file

	An int to hold error codes
