# Virtual-Memory-Manager-Single-level-Paging

The goal of this project is to simulate the steps involved in translating logical to physical addresses, using single level paging.

This project consists of writing a simulated memory manager that translates logical to physical addresses for a
virtual address space of size 216 = 65,536 bytes. The program will read from a file containing
logical addresses and, using a page table, will translate each logical address to its
corresponding physical address and will output the value of the byte stored at the translated
physical address. 

Since the logical memory is bigger than the physical memory, a page
replacement strategy is needed to replace the frames as needed. Swapped out
frames are written to a backing store file (BACKING_STORE_1.bin), and reloaded if needed later on. The
strategy used is the Enhanced Second Chance Algorithm

## Running the code
Compile main.c in any C compiler, preferably in a Linux environment. Execute the a.out file
