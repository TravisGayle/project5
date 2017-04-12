/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct disk *disk;
int numFaults = 0;      // number of page faults
int numWrites = 0;      // number of disk writes
int numReads = 0;       // number of disk reads
int counter = 0;        // counter
int *myArray;
int option;             // user-specified algorithm (rand, fifo, custom)

int pageSearch(int begin, int end, int key) {
    int i;
    for(i = begin; i <= end; i++) {
        if(myArray[i] == key)
            return i;
    }
    return -1;
}

void page_fault_handler( struct page_table *pt, int page ) {
    int no_pages=page_table_get_npages(pt);
    int no_frames=page_table_get_nframes(pt);
    char *physmem = page_table_get_physmem(pt);

    if(no_frames >= no_pages) {
	    // printf("page fault on page #%d\n",page);
	    page_table_set_entry(pt, page, page, PROT_READ|PROT_WRITE);
	    numFaults++;
	    numWrites = 0;
	    numReads = 0;
    } else {

        // Random
        if(option == 1) {
            numFaults++;
            int result = pageSearch(0, no_frames-1, page);
            int temp = lrand48() % no_frames;
            if(result > -1) {
                page_table_set_entry(pt, page, result, PROT_READ|PROT_WRITE);

                numFaults--;
            }
            else if(counter < no_frames) {
                while(myArray[temp] != -1) {
                    temp = lrand48() % no_frames;
                    numFaults++;
                }
                page_table_set_entry(pt, page, temp, PROT_READ);
                disk_read(disk, page, &physmem[temp * PAGE_SIZE]);

                numReads++;
                myArray[temp] = page;
                counter++;
            } else {
                disk_write(disk, myArray[temp], &physmem[temp * PAGE_SIZE]);
                disk_read(disk, page, &physmem[temp * PAGE_SIZE]);
                page_table_set_entry(pt, page, temp, PROT_READ);

                numWrites++;
                numReads++;
                myArray[temp] = page;
            }
            // page_table_print(pt);
        }

        // FIFO
        else if(option == 2) {
            numFaults++;
            int result = pageSearch(0, no_frames - 1, page);
            if(result > -1) {
	            page_table_set_entry(pt, page, result, PROT_READ|PROT_WRITE);
	            counter--;
	            numFaults--;
            } else if(myArray[counter] == -1) {
	            page_table_set_entry(pt, page, counter, PROT_READ);
	            disk_read(disk, page, &physmem[counter * PAGE_SIZE]);

	            numReads++;
            } else {
	            disk_write(disk, myArray[counter], &physmem[counter * PAGE_SIZE]);
	            disk_read(disk, page, &physmem[counter * PAGE_SIZE]);
                page_table_set_entry(pt, page, counter, PROT_READ);

	            numWrites++;
	            numReads++;
            }
            myArray[counter] = page;
            counter = (counter + 1) % no_frames;
            // page_table_print(pt);
        }

        // Custom
        else {
            numFaults++;
            int temp = page % no_frames;
            if(myArray[temp] == page) {
                page_table_set_entry(pt, page, temp, PROT_READ|PROT_WRITE);

                numFaults--;
            } else if(myArray[temp] == -1) {
                page_table_set_entry(pt, page, temp, PROT_READ);
                disk_read(disk, page, &physmem[temp * PAGE_SIZE]);

                numReads++;
            } else {
                disk_write(disk, myArray[temp], &physmem[temp * PAGE_SIZE]);
                disk_read(disk, page, &physmem[temp * PAGE_SIZE]);
                page_table_set_entry(pt, page, temp, PROT_READ);

                numWrites++;
                numReads++;
            }
            myArray[temp] = page;
            // page_table_print(pt);
        }
    }
}

int main( int argc, char *argv[] )
{
	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|lru|custom> <sort|scan|focus>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	if(!strcmp(argv[3], "rand"))
        option = 1;
    else if(!strcmp(argv[3], "fifo"))
        option = 2;
    else
        option = 3;
	const char *program = argv[4];

	if(npages < 3) {
		printf("Your page count is too small. The minimum number of page required is 3.\n");
		return 1;
	}

	if(nframes < 3) {
		printf("Your frame count is too small. The minimum number of frames required is 3.\n");
		return 1;
	}

	myArray = (int *)malloc(nframes * sizeof(int));
	int i;
	for(i = 0; i < nframes; i++)
		myArray[i] = -1;

	disk = disk_open("myvirtualdisk", npages);
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}


	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	char *virtmem = page_table_get_virtmem(pt);

	char *physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"sort")) {
		sort_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"scan")) {
		scan_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"focus")) {
		focus_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[3]);
		return 1;
	}

	printf("%s, %s, %d, %d, %d, %d, %d\n", argv[3], program, npages, nframes, numFaults, numReads, numWrites);

	page_table_delete(pt);
	disk_close(disk);

	return 0;
}
