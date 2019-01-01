#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

//some definitions
#define FALSE 0
#define TRUE 1
#define ADDR long long
#define BOOL char

typedef struct _MEMACCESS{
    ADDR addr; 
    BOOL is_read; 
} MEMACCESS;

typedef struct _CACHE {
    // some declear
    ADDR tag;
    ADDR lruCount;
    int valid;
} CACHE;

typedef enum _RPL{LRU=0, RAND=1} RPL;

int ways; 
int numSets = 0; 
int entries = 0; 
int shift = 0; 
int set; 
int block; 
ADDR accesses = 0;
ADDR hits = 0;
ADDR misses = 0;

CACHE *cache;

//misc. function
FILE* fp = 0;
char trace_file[100]="memtrace.trc";
BOOL read_new_memaccess(MEMACCESS*);  //read new memory access from the memory trace file (already implemented)


//configure the cache
void init_cache(int cache_size, int block_size, int assoc, RPL repl_policy) {
    ways = assoc;
    numSets = (cache_size/block_size)/(assoc); 
    entries = numSets*assoc; 
    set = log2(numSets); 
    block = log2(block_size); 
    shift = set+block; 
    cache = malloc(entries * sizeof(CACHE)); 

    for(int x=0; x<entries;x++){
        cache[x].tag = -1;
        cache[x].valid = 1;
        cache[x].lruCount = 1;
    }

    srand(time(NULL));
}

//check if the memory access hits on the cache
void isHit(ADDR addr, RPL repl_policy, BOOL isRead) {
    accesses = accesses+1;
    ADDR addr_tag = addr >> (shift); 
    ADDR addr_set = ((addr - (addr_tag<<(block+set))) >> block);
    
    int under = addr_set*ways; 
    int upper = under+(ways-1); 
    
    int j;
    int hit = 0;
    int random;

    switch(repl_policy) {
        case LRU:
            for(j=under; j<=upper; j++) {
                if(cache[j].tag == addr_tag) {
                    hit = 1; 
                    hits=hits+1;

                    for(int k=under;k<=upper;k++) 
                        cache[k].lruCount++;
                    
                    cache[j].lruCount = 0;
                    break;
                }
            }
            if(hit == 0){
                misses=misses+1;

//                 if(isRead) return;

                int highestAge = 0;
                int highestSpot = 0;
                int m;
                
                for(m=under; m<=upper; m++) {
                    if(cache[m].lruCount>highestAge) {
                        highestAge = cache[m].lruCount; 
                        highestSpot = m; 
                    }
                }

                cache[highestSpot].tag = addr_tag;

                for(m=under; m<=upper; m++)
                    cache[m].lruCount++;
                
                cache[highestSpot].lruCount = 0; 
            } 
        break;
        case RAND:
            random = rand() % ways;  
            
            for(j=under; j<=upper; j++) {
                if(cache[j].tag == addr_tag) {
                    hit = 1; 
                    hits=hits+1;
                 
                    break;
                }
            }
            if(hit == 0){
                misses=misses+1;
                
//              if(isRead == 0)
                cache[under + random].tag = addr_tag;
            } 
        break;
    }
}

//print the simulation statistics
void print_stat(int, int, int, RPL);

//main
int main(int argc, char* argv[])  
{
    int i=0;
    int cache_size=32768; // 32KB
    int assoc=8;
    int block_size=32;
    RPL repl_policy=LRU;

	/*
    *  Read through command-line arguments for options.
    */
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 's') 
                cache_size=atoi(argv[i+1]);
            
            if (argv[i][1] == 'b')
                block_size=atoi(argv[i+1]);
            
            if (argv[i][1] == 'a')
                assoc=atoi(argv[i+1]);
            
            if (argv[i][1] == 'f')
                strcpy(trace_file,argv[i+1]);


            if (argv[i][1] == 'r')
            {
                if(strcmp(argv[i+1],"lru")==0)
                    repl_policy=LRU;
                else if(strcmp(argv[i+1],"rand")==0)
                    repl_policy=RAND;
                else
                {
                    printf("unsupported replacement policy:%s\n",argv[i+1]);
                    return -1;
                }           
            }
        }
    }
    
    /*
     * main body of cache simulator
    */
    
    init_cache(cache_size, block_size, assoc, repl_policy);   //configure the cache with the cache parameters specified in the input arguments
    while(1)
	{
        MEMACCESS new_access;
        
        BOOL success=read_new_memaccess(&new_access);  //read new memory access from the memory trace file

        if(success!=TRUE)   //check the end of the trace file
            break;

        isHit(new_access.addr, repl_policy, new_access.is_read);
	}

    // print statistics here
    print_stat(cache_size, block_size, assoc, repl_policy);
	return 0;
}


/*
 * read a new memory access from the memory trace file
 */
BOOL read_new_memaccess(MEMACCESS* mem_access)
{
    ADDR access_addr;
    char access_type[10];
    /*
     * open the mem trace file
     */

    if(fp==NULL)
    {
        fp=fopen(trace_file,"r");
        if(fp==NULL)
        {
            fprintf(stderr,"error opening file");
            exit(2);

        }   
    }

    if(mem_access==NULL)
    {
        fprintf(stderr,"MEMACCESS pointer is null!");
        exit(2);
    }

    if(fscanf(fp,"%llx %s", &access_addr, access_type)!=EOF)
    {
        mem_access->addr=access_addr;
        if(strcmp(access_type,"RD")==0)
            mem_access->is_read=TRUE;
        else
            mem_access->is_read=FALSE;
        
        return TRUE;
    }       
    else
        return FALSE;

}

void print_stat(int cache_size, int block_size, int assoc, RPL repl_policy) {
    /*
    -s 1024 -b 32 -a 4 -r lru -f memtrace.trc

    cache_size: 1024 B
    block_size: 32 B
    associativity: 4
    replacement policy : LRU
    cache accesses : ?
    cache_hits : ?
    cache_misses : ?
    cache_miss_rate : ?
    */
    printf("cache_size          : %d\n", cache_size);
    printf("block_size          : %d\n", block_size);
    printf("associativity       : %d\n", assoc);
    printf("replacement policy  : ");
    if(repl_policy == LRU) printf("LRU\n\n");
    else                   printf("RAND\n\n");
    printf("cache accesses  : %lld\n", accesses);
    printf("cache_hits      : %lld\n", hits);
    printf("cache_misses    : %lld\n", misses);
    printf("cache_hit_rate  : %f\n", (float)hits / (float)accesses);
    printf("cache_miss_rate : %f\n\n", (float)misses / (float)accesses);
}
