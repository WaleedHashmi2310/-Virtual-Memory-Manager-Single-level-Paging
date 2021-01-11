#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TABLE_SIZE 256
#define PAGE_SIZE 256
#define FRAMES 64
#define FRAME_SIZE 256
#define INPUT_SIZE 1000
#define BUFFER_SIZE 256

int pf_counter = 0;
int add_counter = 0;

size_t fpread(void *buffer, size_t size, size_t mitems, size_t offset, FILE *fp)
{
     if (fseek(fp, offset, SEEK_SET) != 0)
         return 0;
     return fread(buffer, size, mitems, fp);
}

char* substr(const char *src, int m, int n)
{
	int len = n - m;

	char *dest = (char*)malloc(sizeof(char) * (len + 1));

	for (int i = m; i < n && (*src != '\0'); i++)
	{
		*dest = *(src + i);
		dest++;
	}

	*dest = '\0';
	return dest - len;
}

struct entry{

	int exists;
	int reference;
	int modified;
	int dirty;
	int valid;
	int frame_no;
	int page_fault;
};

struct memory{

	unsigned char frame[500];
	int my_page;
};

int main()
{
	
	int frame_pool = 0;

	struct entry page_table[TABLE_SIZE];
	struct memory my_memory[FRAMES];

	for(int i=0; i<TABLE_SIZE; i++)
	{
		page_table[i].exists = 0;
		page_table[i].reference = 0;
		page_table[i].modified = 0;
		page_table[i].dirty = 0;
		page_table[i].valid = 0;
		page_table[i].frame_no = -1;
		page_table[i].page_fault = 0; 
	}

	for(int i=0; i<FRAMES; i++)
	{
		my_memory[i].my_page = -1;
	}

	FILE *infile;
	infile = fopen("addresses.txt", "r");

	FILE *binary = fopen("BACKING_STORE_1.bin", "r+b");

	char address[INPUT_SIZE];

	int p_no;
	int offset;
	int rw;

	int counter = 0;

	printf("\n%s   %s   %s   %s    %s    %s     %s\n", "Logical", "Physical", "Read/Write", "Value", "PF", "P", "F");
	printf("\n");

	while(fgets(address, INPUT_SIZE, infile))
	{
		add_counter++;

		int size = strlen(address)-1; //Size of L_Address
		
		rw = address[size-1] - '0'; //R/W

		char *p_temp = substr(address, 2, 4);
		p_no = (int)strtol(p_temp, NULL, 16); //Page_No in Int


		char *o_temp = substr(address, 4, 6);
		offset = (int)strtol(o_temp, NULL, 16); //Offset in Int

		int start = p_no*PAGE_SIZE;

		unsigned char value;

		int p_check;
		int f_check;

		if(page_table[p_no].valid == 0)
		{
			if(frame_pool < 63) //IF Page Fault && Frame Available
			{
				if(page_table[p_no].exists == 0)
				{
					p_check = 1;
					page_table[p_no].exists = 1;
				}

				f_check = 1;

				page_table[p_no].page_fault = 1;
				pf_counter++;
				page_table[p_no].frame_no = frame_pool;
				my_memory[frame_pool].my_page = p_no;
				page_table[p_no].valid = 1;

				fpread(my_memory[frame_pool].frame, 256, 1, start, binary);
				
    			frame_pool = frame_pool + 1;
			}

			else if(frame_pool == 63) //ENHANCED SECOND CHANCE!
			{
				if(page_table[p_no].exists == 0)
				{
					p_check = 1;
					page_table[p_no].exists = 1;
				}

				f_check = 1;

				page_table[p_no].page_fault = 1;
				pf_counter++;
				int victim_frame = -1;
				int victim_page = -1;
				int found;
				int write_back = 0;
				
				while(1)
				{

					for(int i=0; i<64; i++) //FIND A FRAME WITH BITS <0,0>
					{
						int curr_page = my_memory[i].my_page;
						if(curr_page != -1 && (page_table[curr_page].reference == 0) && (page_table[curr_page].modified == 0))
						{
							victim_frame = i;
							victim_page = curr_page;
							break;
						}
					}

					if(victim_frame != -1)
						break;

					for(int i=0; i<64; i++) //FIND A FRAME WITH BITS <0,1> WHILE SEETING R BIT TO 0 OF EVERY FRAME PASSED
					{
						int curr_page = my_memory[i].my_page;
						if(curr_page != -1 && (page_table[curr_page].reference == 0) && (page_table[curr_page].modified == 1))
						{
							write_back = 1;
							victim_frame = i;
							victim_page = curr_page;
							break;
						}
						else
						{
							page_table[curr_page].reference = 0;
						}
					}

					if(victim_frame != -1)
						break;
				}

				if(victim_frame != -1 && write_back == 0) //NEITHER RECENTLY USED NOR MODIFIED
				{
					/*page_table[p_no].page_fault = 1;*/
					page_table[victim_page].valid = 0;
					page_table[victim_page].frame_no = -1;

					page_table[p_no].frame_no = victim_frame;
					my_memory[victim_frame].my_page = p_no;
					page_table[p_no].valid = 1;
					page_table[p_no].reference = 1;

					memset(my_memory[victim_frame].frame, 0, 500); //Emptying the frame
				}

				else if(victim_frame != -1 && write_back == 1) //NOT RECENTLY USED BUT MODIFIED
				{
					/*page_table[p_no].page_fault = 1;*/
					page_table[victim_page].valid = 0;
					page_table[victim_page].frame_no = -1;

					page_table[p_no].frame_no = victim_frame;
					my_memory[victim_frame].my_page = p_no;
					page_table[p_no].valid = 1;
					page_table[p_no].reference = 1;

					int start1 = victim_page*PAGE_SIZE;
					fseek(binary, start1, SEEK_SET);
					fwrite(my_memory[victim_frame].frame, 1, 255, binary); //Write back to backing store
					
					memset(my_memory[victim_frame].frame, 0, 500);
				}
			}
		}

		if(page_table[p_no].valid == 1) // If a page exists in memory and requests to r/w.
		{

			int v_frame = page_table[p_no].frame_no;

			if(rw == 0)
			{
				value = my_memory[v_frame].frame[offset];
/*				if(page_table[p_no].reference == 0)
					page_table[p_no].reference = 1;*/

			}

			else
			{
				if(page_table[p_no].modified == 0)
					page_table[p_no].modified = 1;
				
				page_table[p_no].dirty = 1;
				unsigned char newByte = my_memory[v_frame].frame[offset];

				newByte = newByte >> 1;

				my_memory[v_frame].frame[offset] = newByte; //Write back to frame;
				value = newByte;
			}
		}

	    printf("0x%s   ", substr(address,2,6));
		printf(" 0x%02X",page_table[p_no].frame_no);
		printf("%02X   ", offset);

		char *str;
		if(rw == 0)
		{
			printf("  %s", "Read    ");
		}

		else if(rw == 1)
		{
			printf("  %s", "Write   ");
		}

		printf("     0x%02X   ", value);

		if(page_table[p_no].page_fault == 1)
			printf("  Y   ");
		else
			printf("  N   ");

		if(p_check == 1)
			printf("  Y   ");
		else
			printf("  N   ");


		if(f_check == 1)
			printf("  Y   ");
		else
			printf("  N   ");

		printf("\n");

		page_table[p_no].page_fault = 0;

		address[0] = '\0';

		p_check = 0;
		f_check = 0;

/*		counter++;

		if(counter == 256)
			break;*/
	}



	double pf_rate = (float)pf_counter/(float)add_counter;
	pf_rate = pf_rate*100;
	printf("\nPage Fault Rate: %0.2f%s\n", pf_rate, "%");


	return 0;
}