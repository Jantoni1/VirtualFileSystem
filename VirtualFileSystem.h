#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define kbSize 1024
#define MAX_FILE_NAME 18
#define HEADINFO 24
#define FILE_SIZE 32
#define NODESIZE 12
#define NODEANDBLOCK 524
#define BLOCKSIZE 512
#define HEAD_INFO 6


unsigned long FREESIZE;
unsigned long NUMBER_OF_FILES;
unsigned long MAX_FILES;
unsigned long OFFSET;
unsigned long FREE_BLOCKS;
int fd;

struct FileNode;
struct FileNodeDescriptor;

struct FileNode
{
	unsigned long next;
	unsigned long File;
	char flags;
};

struct FileDescriptor
{
	char FileName[MAX_FILE_NAME];
	unsigned long FileSize;
	unsigned long DataNode;
	char Created;
};

typedef struct FileNode iNode;
typedef struct FileDescriptor FDesc;