# zippydippy
A short implementation of Parallelized Serial Run-Length Encoding

## How do we parallelize data?
First, mmap the data so the whole file is in the address space.

Divide the data up into i chunks where i = number of threads. giving a chunk to each thread.

Once a zip thread finishes its chunk, store it in a buffer to avoid corrupting the data by changing order.

Divide another chunk further if one finishes early, allowing the remaining chunks to be further parallelized and finished faster.

## How do use threads?
The Number of threads = number of cores on processors by use of get_nprocs (avoiding risk of inactive processors)

Each ith thread will be given a chunk of the data corresponding to its i.



## What data structures?
A BUFFER: stored as an array to hold the chunked data corresponding to the i of the chunk after its been zipped to avoid
the issue of a thread race condition that could corrupt data by changing the order of characters once its been zipped.

