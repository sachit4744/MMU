#include "my_vm.h"
#define arrSize (PGSIZE/sizeof(unsigned long))
#define NUL '\0'

// function definitions
void assigningPageTables();
void calculateBitsofLevels();
static void setBit(char *bitmap, int index);
static int getBit(char *bitmap, int index);
static void resetBit(char *bitmap, int index);

//calculating the number of physical and virtual pages

unsigned long long numberOfPhyPgs = (MEMSIZE)/(PGSIZE);
unsigned long long numberOfVrtlPgs = (MAX_MEMSIZE)/(PGSIZE);


// Here taking array of unsigned long which represents size of each physical page
struct page{
    unsigned long array[arrSize];
};

//Variables for tbl miss and tlb lookups
int tlbMiss = 0;
int tlbLookup = 0;

pthread_mutex_t mutex;
struct page * phyMemory;
// Physical bitmaps
char * phybitmap;
// Virtual bitmaps
char * vrtlbitmap;

int initialized = 0;

int vrtlPgNumberBits = 0;
int offsetBits = 0;
int secondLevelBits = 0; //Here second level is outer page table 
int firstLevelBits = 0; //First level is inner page table


/*
Function responsible for allocating and setting your physical memory 
*/
void set_physical_mem() {

    //Allocate physical memory using mmap or malloc; this is the total size of
    //your memory you are simulating

    //HINT: Also calculate the number of physical and virtual pages and allocate
    //virtual and physical bitmaps and initialize them


    
    //Here calling the function to split the bits
    calculateBitsofLevels(); 

    // physical memory is assigned by allocating pages of total physical page numbers

    phyMemory = (struct page *)malloc(sizeof(struct page) * numberOfPhyPgs);

    //physical and virtual pages are declared as global variables above
    

    phybitmap = (char *)malloc(numberOfPhyPgs/8);
    
    // Initializing physical bitmap
    for(int i = 0; i < sizeof(phybitmap)/sizeof(phybitmap[0]); i++){
        phybitmap[i] = NUL;
    }
 
    //There are 8 bits in 1 byte
    vrtlbitmap = (char *)malloc(numberOfVrtlPgs/8);

    // Initializing virtual bitmap
    for(int i = 0; i < sizeof(vrtlbitmap)/sizeof(vrtlbitmap[0]); i++){
        vrtlbitmap[i] = NUL;
    }


    assigningPageTables();
    
    

}

void calculateBitsofLevels(){

	
    // Here number of outer level bits, inner level bits and offset bits are calculated
    
    // Number of offset bits
    offsetBits = log2(PGSIZE);

    // Number of bits for virtual page number
    vrtlPgNumberBits = 32 - offsetBits;

    // Considering 2 level page tables
    //Number of bits used for second level using PGSIZE = entries * sizeOfPageTableNEtry
    //By using log on entries we can get bits for the second level
    secondLevelBits = log2((PGSIZE)/sizeof(pte_t));

    //Number of bits for the first level is vpn - bits of second level
    firstLevelBits = vrtlPgNumberBits - secondLevelBits;


}

void assigningPageTables(){

        // Allocating the last page of physical memory for page directory
        // Calculate the index for last page and assigning it to 1 in physical bitmap
        int index = (numberOfPhyPgs) - 1;
        
        setBit(phybitmap, index);
        
    	
        // Getting the last physical page which is allocated as page directory
        struct page * ptr = &phyMemory[numberOfPhyPgs - 1];
        unsigned long * arrOfPgs = ptr->array;

        // The page table directory has 2^secondLevelBits number of first level page tables and are assigned to -1
        for(int i = 0; i < (1 << secondLevelBits); i++){
            *(arrOfPgs + i) = -1;
        }

}

static int getBit(char *bitmap, int index)
{
    // Finding the location in physical bitmap array where we want to set the bit
    char *loc = ((char *) bitmap) + (index / 8);
    
    return (int)(*loc >> (index % 8)) & 0x1;
}
static void setBit(char *bitmap, int index)
{
    // Finding the location in physical bitmap array where we want to set the bit
    char *loc = ((char *) bitmap) + (index / 8);
    
    
    // generating a mask through left indexing by 1 and doing OR operation with loc to set the bit
    char mk = 1 << (index % 8);
    *loc |= mk;
   
    return;
}
static void resetBit(char *bitmap, int index)
{
    // Finding the location in physical bitmap array where we want to set the bit
    char *loc = ((char *) bitmap) + (index / 8);
    
     // generating a mask through left indexing by 1 and doing AND operation with loc to reset the bit
    char mk = 1 << (index % 8);
    *loc &= ~mk;
   
    return;
}

/*
 * Part 2: Add a virtual to physical page translation to the TLB.
 * Feel free to extend the function arguments or return type.
 */
//int
//add_TLB(void *va, void *pa) Here chaning the return type of function
int add_TLB(int vpn, int ppn)
{

    /*Part 2 HINT: Add a virtual to physical page translation to the TLB */
    int set = vpn % TLB_ENTRIES;

	tlb_store.array[set][0] = vpn;
	tlb_store.array[set][1] = ppn;

    return -1;
}


/*
 * Part 2: Check TLB for a valid translation.
 * Returns the physical page address.
 * Feel free to extend this function and change the return type.
 */
unsigned long
check_TLB(void *va) {

    /* Part 2: TLB lookup code here */
	//Get the vpn from virtual address
    unsigned long vrtlpgnumber = (unsigned int)va >> offsetBits;
	int set = vrtlpgnumber % TLB_ENTRIES;

	if (tlb_store.array[set][0] == vrtlpgnumber){

		unsigned long phypgnumber = tlb_store.array[set][1];
		return phypgnumber;

	}else{
		return -1;

	}

}


/*
 * Part 2: Print TLB miss rate.
 * Feel free to extend the function arguments or return type.
 */
void
print_TLB_missrate()
{
    double miss_rate = 0;	

    /*Part 2 Code here to calculate and print the TLB miss rate*/
   	
	miss_rate = (tlbMiss * 1.0)/(tlbLookup * 1.0);

    fprintf(stderr, "TLB miss rate is %lf \n", miss_rate);
}



/*
The function takes a virtual address and page directories starting address and
performs translation to return the physical address
*/
pte_t *translate(pde_t *pgdir, void *va) {
    /* Part 1 HINT: Get the Page directory index (1st level) Then get the
    * 2nd-level-page table index using the virtual address.  Using the page
    * directory index and page table index get the physical address.
    *
    * Part 2 HINT: Check the TLB before performing the translation. If
    * translation exists, then you can return physical address from the TLB.
    */

        unsigned int VrtlAddrs = (unsigned int)va;

        // Creating mask to obtain the offset bits
    	unsigned long offsetBitsMask = (1 << offsetBits);
    	offsetBitsMask = offsetBitsMask-1;

    	unsigned long offset = VrtlAddrs & offsetBitsMask;

	    tlbLookup ++;
    	//check in TlB. If present return that physical address
	    int tlbPhypgNumber = check_TLB(va);

		//This indicates that physical address if ofund
	    if (tlbPhypgNumber != -1){

            char * tlbPhyAddr = (char *) &phyMemory[tlbPhypgNumber];
            tlbPhyAddr = tlbPhyAddr + offset;
            return (pte_t *) tlbPhyAddr;

	    }

		tlbMiss ++;
	

        // Obtaining outer level index

        // Get the bits other than outer level
    	int bitsToRemove = 32 - secondLevelBits; //32-bit address 
        // Perform right shift to obtain outer level index
    	unsigned long outerLevelIndex = VrtlAddrs >> bitsToRemove;

    	
        // Obtaining inner level index

    	//removing offset bits
    	int vpn = VrtlAddrs >> offsetBits;
        // Creating mask of outer level bits
    	offsetBitsMask = (1 << firstLevelBits);
    	offsetBitsMask = offsetBitsMask-1;

        // Performing AND between vpn and mask to obtain inner level index
    	unsigned long innerLevelIndex = vpn & offsetBitsMask;

    	//Checking that this page table entry is vaid using bitmap
    	//The bit we need to check: (outerIndex * innerLevelBitNum^2) + innerIndex
        // Here we need to check the index in bitmap to verify that the translation exists or not

        // Implemented the concept taught to get index
    	int index = (outerLevelIndex * (1 << firstLevelBits)) + innerLevelIndex;

        // Get bit and check whether its 1 or -1
	    int bit = getBit(vrtlbitmap, index);

        // Return NULL if entry is not valid
    	if(bit != 1){
        	return NULL;
    	}

    	//if its valid we can walk through the page table to get the valid mapping

    	//Use outer and inner index to get the physical page number

    	pde_t * pagedirEntry = pgdir + outerLevelIndex;

   	    //This entry of page directory has the page number of the inner level page table
        // Dereferencing to get the page number of inner level page table
    	unsigned long innerTblPageNumber = *pagedirEntry;

        //Getting address of inner level page table
    	pte_t * innerPgTbl_addr = (pte_t *)&phyMemory[innerTblPageNumber];

        //Getting the page table entry
    	pte_t * pgTblEntry = innerPgTbl_addr + innerLevelIndex;

    	unsigned long pNum = *pgTblEntry;

        //Getting the address of the physical page
    	pte_t * pgAddr = (pte_t *)&phyMemory[pNum];

        //Adding offset to get the physical address
    	unsigned long physicalAddress = (unsigned long)((char *)pgAddr + offset);

        //add entry to tlb
        
        add_TLB(vpn, pNum);


    	return (pte_t *)physicalAddress;


    //If translation not successful, then return NULL
    return NULL; 
}


/*
The function takes a page directory address, virtual address, physical address
as an argument, and sets a page table entry. This function will walk the page
directory to see if there is an existing mapping for a virtual address. If the
virtual address is not present, then a new entry will be added
*/
int
page_map(pde_t *pgdir, void *va, void *pa)
{

    /*HINT: Similar to translate(), find the page directory (1st level)
    and page table (2nd-level) indices. If no mapping exists, set the
    virtual to physical mapping */


        //Obtaining outer level index

        //Getting the bits to remove
    	int bitsToRemove = 32 - secondLevelBits; //32 assuming we are using 32-bit address 
    
    	unsigned int VrtlAddrs = (unsigned int)va;
    	unsigned long outerLevelIndex = VrtlAddrs >> bitsToRemove;


        //Obtaining physical page number and virtual page number by shifting the offset bits
	
    	unsigned long ppn = (unsigned int)pa >> offsetBits;
    	unsigned long vpn = VrtlAddrs >> offsetBits;

        //check if this virtual to physical page translation is already in the TLB

        int recievedPage = check_TLB(va);
        tlbLookup ++;

        if(recievedPage == ppn){

            return 0;   
            
        }

        

        //Get the inner level index by creating mask and doing AND with vpn
    	unsigned long firstLevelBitsMask = (1 << firstLevelBits);
    	firstLevelBitsMask = firstLevelBitsMask-1;
    	unsigned long innerLevelIndex = vpn & firstLevelBitsMask;

    	pde_t * pagedirEntry = pgdir + outerLevelIndex;

    	if (*pagedirEntry == -1){
		//the corresponding page table needs to be reserved
		int lastPage = numberOfPhyPgs-1;
		while(lastPage >= 0){
			//get the bit at index 'last page' from physical bitmap
			int bit = getBit(phybitmap, lastPage);
			if(bit == 0){
				//set bit as 1
				setBit(phybitmap, lastPage);
				//set this page as the page directory
				*pagedirEntry = lastPage;
				break;
			}

			lastPage --;
		}

	}
		
    	//This entry of page directory has the page number of the inner level page table
    	unsigned long innerTblPageNumber = *pagedirEntry;
	
        //Getting the address of inner level page table from physical memory
    	pte_t * innerPgTbl_addr = (pte_t *)&phyMemory[innerTblPageNumber];
    
        //Getting the page table entry in the inner level page table by adding the inner level index
    	pte_t * pgTblEntry = innerPgTbl_addr + innerLevelIndex;

        //Assigning the page table entry value of physical page number 
    	*pgTblEntry = ppn;

		//If not in TLB increment the tlb miss
        tlbMiss++;
	
	    //add this vpn to ppn translation to tlb
	    add_TLB(vpn, ppn);	
    	return 1;

    return -1;
}


/*Function that gets the next available page
*/
//void *get_next_avail(int num_pages) { Changing the return type as int is returned here
    unsigned long get_next_avail(int num_pages) {
 
    //Use virtual address bitmap to find the next free page
    
    int pageBegin = 0;
	int index = 0;
	while(index < numberOfVrtlPgs){


		int bit = getBit(vrtlbitmap,index);


        	if(bit == 0){
            		int temp = 1;
            		int nxt_index = index+1;

            		while(nxt_index < numberOfVrtlPgs && temp < num_pages){
				    bit = getBit(vrtlbitmap,nxt_index);
                		if(bit == 1){
                    			break;
                		}else{
                    			temp++;
                    			nxt_index++;
                		}
            		}

            		if (temp == num_pages){
                		//we found continuous pages
                		//The virtual pages we should return are in the index and index+numPages-1
                		pageBegin = index;
                		return pageBegin;

            		}
			index = nxt_index;
			continue;

        	}
        	index++;
	}

    	return -1;
	
}


/* Function responsible for allocating pages
and used by the benchmark
*/
void *t_malloc(unsigned int num_bytes) {

    /* 
     * HINT: If the physical memory is not yet initialized, then allocate and initialize.
     */

   /* 
    * HINT: If the page directory is not initialized, then initialize the
    * page directory. Next, using get_next_avail(), check if there are free pages. If
    * free pages are available, set the bitmaps and map a new page. Note, you will 
    * have to mark which physical pages are used. 
    */

    pthread_mutex_lock(&mutex);
	
    //Calling set_physical_mem to initialize and allocate the physical memory for once
	if (initialized == 0){
		set_physical_mem();
		initialized = 1;
	}
	
	//Getting number of pages to allocate in physical memory
	int numOfPages = num_bytes / (PGSIZE);
	int leftBytes = num_bytes % (PGSIZE);
	if (leftBytes > 0){
		//and extra page if there are any extra bytes left
		numOfPages += 1;

	}

	
        //Declaring an array of the physical pages
    	int physicalPages[numOfPages];

    	int count = 0;
    	int index = 0;

		//check if there are  free physical pages using physical bitmap
	    //if not return NULL
    	while(count < numOfPages && index < numberOfPhyPgs){
		int bit = getBit(phybitmap, index);

        	if (bit == 0){
            		physicalPages[count] = index;
            		count++;
        	}
        	index++;

    	}

        //If that many number of pages were not found in physical memory, return null as allocation is not possible
    	if(count < numOfPages){
		    pthread_mutex_unlock(&mutex);
        	return NULL;
    	}

    	
        //Here checking if there equally virtual pages available which are further used for mapping
    	int pageBegin = get_next_avail(numOfPages);

        //Return null if that many virtual pages are not available
    	if (pageBegin == -1){
		pthread_mutex_unlock(&mutex);
        	return NULL;
    	}

    	int pageEnd = pageBegin+numOfPages-1;

	
	

    	count = 0;
	    //Using pagemap to map the physical pages to virtual pages
    	for(int i = pageBegin; i <= pageEnd; i++){

        	unsigned long tempVrtlAddr = i << offsetBits;
        	unsigned long tempPhyAddr = physicalPages[count] << offsetBits;
      		//updating virtual bitmap 
		    setBit(vrtlbitmap, i);

        	//updating physical bitmap
		    setBit(phybitmap, physicalPages[count]);

        	count++;
        	page_map((pde_t *)(phyMemory+numberOfPhyPgs-1), (void *)tempVrtlAddr, (void *)tempPhyAddr);


    	}

    	//The vpn is stored in firstPage
    	unsigned long va = pageBegin << offsetBits;
	    pthread_mutex_unlock(&mutex);

	    return (void *)va;

}

/* Responsible for releasing one or more memory pages using virtual address (va)
*/
void t_free(void *va, int size) {

    /* Part 1: Free the page table entries starting from this virtual address
     * (va). Also mark the pages free in the bitmap. Perform free only if the 
     * memory from "va" to va+size is valid.
     *
     * Part 2: Also, remove the translation from the TLB
     */
     pthread_mutex_lock(&mutex);

    unsigned long pageBegin = (int)va >> offsetBits;
    //Getting the pages that are needed to free
	int freePages = size/(PGSIZE);
	if (size % (PGSIZE) > 0){

		freePages ++;

	}

    	int check = 1;

    	//making sure memory from "va" to va+size is valid

    	for (unsigned long i = pageBegin; i < pageBegin + freePages; i++){
       
		int bit = getBit(vrtlbitmap, i);

        	if(bit == 0){
            		check = 0;
            		break;
        	}

    	}

        //If not valid then return null
    	if(check == 0){
		    pthread_mutex_unlock(&mutex);
        	return;
    	}

    	//If valid then obtain the index of both virtual and physical pages and reset their bits to 0 in their respective bitmaps
    	for (unsigned long i = pageBegin; i < pageBegin + freePages; i++){

        	void * va = (void *)(pageBegin << offsetBits);

        	pte_t pa = (pte_t)(translate((pde_t *)(phyMemory+numberOfPhyPgs-1), va));

        	unsigned long physicalPage = pa >> offsetBits;

        	//reset bit at index i in virtual bitMap to 0
		    resetBit(vrtlbitmap, i);
        
        	//reset bit at index physicalPage in physical bitMap to 0
		    resetBit(phybitmap, physicalPage);
        

    	}

	
	//Removing the virtual pages form tlb
	for (unsigned long i = pageBegin; i < pageBegin + freePages; i++){

		//remove this vpn 
		int set = i % TLB_ENTRIES;
		if(tlb_store.array[set][0] == i){
			tlb_store.array[set][0] = -1;
			tlb_store.array[set][1] = -1;

		}
		
	}

	pthread_mutex_unlock(&mutex);

    
    
}


/* The function copies data pointed by "val" to physical
 * memory pages using virtual address (va)
 * The function returns 0 if the put is successfull and -1 otherwise.
*/
void put_value(void *va, void *val, int size) {

    /* HINT: Using the virtual address and translate(), find the physical page. Copy
     * the contents of "val" to a physical page. NOTE: The "size" value can be larger 
     * than one page. Therefore, you may have to find multiple pages using translate()
     * function.
     */

    
	pthread_mutex_lock(&mutex);
	

    char * value = (char *)val;
    pte_t physical_address = (pte_t)(translate((pde_t *)(phyMemory+numberOfPhyPgs-1), va));
    char * PhyAddr = (char *)physical_address;
     char * VrtlAddr = (char *) va;
        
	char * VrtlAddrEnds = VrtlAddr + size;
	
	//Getting the vpn for first and last virtual page
	unsigned int vrtlPg_start = (unsigned int)VrtlAddr >> offsetBits;
	unsigned int vrtlPg_end = (unsigned int)VrtlAddrEnds >> offsetBits;

    //Check if virtual pages exist for the vpn provided above
	for(unsigned int j = vrtlPg_start; j <= vrtlPg_end; j++){
		//get the bit at index 'vpn' form vbitmap
		int bit = getBit(vrtlbitmap, j);

        //If bit is 0 return null
		if (bit == 0){
			pthread_mutex_unlock(&mutex);
			return;
		}

	}

        //As virtual pages are valid we can now put the value
    	int tempsize = 0;
    	while(tempsize < size){


        	*PhyAddr = *value;
        	PhyAddr++;
        	value++;
        	tempsize++;
        	VrtlAddr++;

        	unsigned int vrtladdr = (unsigned int)VrtlAddr;
		
        	int offsetBitsMask = (1 << offsetBits);
        	offsetBitsMask = offsetBitsMask-1;

        	int offset = vrtladdr & offsetBitsMask;

            //Updating the physical page
        	if (offset == 0){

            physical_address = (pte_t)(translate((pde_t *)(phyMemory+numberOfPhyPgs-1), (void *)VrtlAddr));
			PhyAddr = (char *) physical_address;

        	}

    	}

	pthread_mutex_unlock(&mutex);


}


/*Given a virtual address, this function copies the contents of the page to val*/
void get_value(void *va, void *val, int size) {

    /* HINT: put the values pointed to by "va" inside the physical memory at given
    * "val" address. Assume you can access "val" directly by derefencing them.
    */
    
	pthread_mutex_lock(&mutex);
	
    char * physical_address = (char *)translate((pde_t *)(phyMemory+numberOfPhyPgs-1), va);
    char * VrtlAddr = (char *) va;
	char * value = (char *) val;
	char * VrtlAddrEnds = VrtlAddr + size;
	//checking for the validity of the virtual pages from va to va+size

		
	unsigned int vrtlPg_start = (unsigned int)VrtlAddr >> offsetBits;
	unsigned int vrtlPg_end = (unsigned int)VrtlAddrEnds >> offsetBits;

	for(unsigned int j = vrtlPg_start; j <= vrtlPg_end; j++){
		//get the bit at index 'vpn' form vbitmap
		int bit = getBit(vrtlbitmap, j);

		if (bit == 0){
			//not a valid virtual address
			pthread_mutex_unlock(&mutex);
			return;
		}

	}

	//at this point we know that all virtual page numbers from va to va + size are valid.

	for (int i = 0; i < size; i++){

		*value = *physical_address;
		value++;
		physical_address++;
		VrtlAddr++;

		
        	unsigned int vrtladdr = (unsigned int)VrtlAddr;
		
        	int offsetBitsMask = (1 << offsetBits);
        	offsetBitsMask = offsetBitsMask-1;

        	int offset = vrtladdr & offsetBitsMask;

        	if (offset == 0){
			//update the physical page
            physical_address = (char *)(translate((pde_t *)(phyMemory+numberOfPhyPgs-1), (void *)VrtlAddr));	

        	}

	}

	pthread_mutex_unlock(&mutex);


}



/*
This function receives two matrices mat1 and mat2 as an argument with size
argument representing the number of rows and columns. After performing matrix
multiplication, copy the result to answer.
*/
void mat_mult(void *mat1, void *mat2, int size, void *answer) {

    /* Hint: You will index as [i * size + j] where  "i, j" are the indices of the
     * matrix accessed. Similar to the code in test.c, you will use get_value() to
     * load each element and perform multiplication. Take a look at test.c! In addition to 
     * getting the values from two matrices, you will perform multiplication and 
     * store the result to the "answer array"
     */
    int x, y, val_size = sizeof(int);
    int i, j, k;
    for (i = 0; i < size; i++) {
        for(j = 0; j < size; j++) {
            unsigned int a, b, c = 0;
            for (k = 0; k < size; k++) {
                int address_a = (unsigned int)mat1 + ((i * size * sizeof(int))) + (k * sizeof(int));
                int address_b = (unsigned int)mat2 + ((k * size * sizeof(int))) + (j * sizeof(int));
                get_value( (void *)address_a, &a, sizeof(int));
                get_value( (void *)address_b, &b, sizeof(int));
                // printf("Values at the index: %d, %d, %d, %d, %d\n", 
                //     a, b, size, (i * size + k), (k * size + j));
                c += (a * b);
            }
            int address_c = (unsigned int)answer + ((i * size * sizeof(int))) + (j * sizeof(int));
            // printf("This is the c: %d, address: %x!\n", c, address_c);
            put_value((void *)address_c, (void *)&c, sizeof(int));
        }
    }
}



