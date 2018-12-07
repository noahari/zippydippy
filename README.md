# zippydippy
A short implementation of Parallelized Serial Run-Length Encoding

## How do we parallelize data?
First, mmap the data so the whole file is in the address space.

Divide the data up into i chunks where i = file size/4kb (i.e. chunk size = 4kb).

Once a zip thread finishes its chunk, print as we go.

##How are chunks divided
Currently, we read in the data at once and store it, increasing the pointer passed into each thread's function by length/numprocs * i of thread.

We do this because it 

## How do use threads?
The Number of threads = number of cores on processors by use of get_nprocs (avoiding risk of inactive processors)

Each ith thread will be given a chunk of the data.

When a thread finishes its chunk, it finds another available chunk, if there are none it finishes.

In this implementation it treats the data as one producer and the four threads as four consumers.
## What data structures?
Array containing info defining what chunks have been allocated to a thread, so we know what chunk to allocate. 


