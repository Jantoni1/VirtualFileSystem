#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "VirtualFileSystem.h"

int CreateDisc( int FileMaxSize )
{
	int fd, i;
	char buf[1];
	struct stat st;
	if( fd = open( "VirtualDisc", O_WRONLY) >=0)
	{
	  printf("Disc already exists.\n");
	  close(fd);
	  return -1;
	}
	else
	{
	  fd = creat( "/lab6/VirtualDisc" , S_IRWXU);
	  if( fd < 0 )
	  {
	    printf("Could not create file.\n");
	    return -1;
	  }
	  FREESIZE   = kbSize * FileMaxSize;
	  lseek( fd, FREESIZE - 1, SEEK_SET);
	  FREESIZE  -= HEADINFO;
	  MAX_FILES  = FileMaxSize*2/3 ;
	  FREESIZE  -= MAX_FILES * FILE_SIZE + HEADINFO;
	  MAX_BLOCKS = FREESIZE/NODEANDBLOCK;
	  FREESIZE  -= MAX_BLOCKS*NODESIZE;
          FREESIZE  -= FREESIZE % BLOCKSIZE;
	  FREE_BLOCKS = MAX_BLOCKS;
	  NUMBER_OF_FILES = 0;
	  OFFSET = HEADINFO + MAX_FILES * FILE_SIZE + MAX_BLOCKS * NODESIZE;
	  files = malloc(MAX_FILES * sizeof(struct FileDescriptor));
	  blocks = malloc(MAX_BLOCKS * sizeof(struct FileNode));
	  for(i=0; i<MAX_FILES; ++i)
	     files[i].Created = 0;
	  for(i=0; i<MAX_BLOCKS; ++i)
	     blocks[i].flags = 0;
	  lseek( fd,  FREESIZE -1 , SEEK_SET);
	  buf[0]='\0';
	  write( fd, &buf, 1);
	  stat("VirtualDisc", & st);
	  printf("Virtual Disc created succefuly. Size: %d B.\n", st.st_size);
   	  Write_Metainfo( );
	 /* close( fd );*/
	  return 0;
 	}
}

void Write_Metainfo( void )
{
	FILE *fd;
        unsigned long buffer[HEAD_INFO];
	unsigned long buf[HEAD_INFO];
	fd = fopen("/lab6/VirtualDisc", "r+");
	buf[0] = FREESIZE;
	buf[1] = NUMBER_OF_FILES;
	buf[2] = MAX_FILES;
	buf[3] = MAX_BLOCKS;
	buf[4] = FREE_BLOCKS;
	buf[5] = OFFSET;
	fseek( fd, 0, 0 );
	fwrite( buf, sizeof(unsigned long), HEAD_INFO, fd );
	fwrite( files, sizeof(struct FileDescriptor), MAX_FILES, fd );
	fwrite( blocks,sizeof(struct FileNode),  MAX_BLOCKS, fd );
	fclose(fd);
}

int Read_Metainfo( void ) 
{
	FILE *fd;
        int i;
	unsigned long buf[HEAD_INFO];
	if( (fd = fopen("/lab6/VirtualDisc", "r+")) == NULL)
		return -1;
	fseek( fd, 0, 0 );
	fread( buf, sizeof(unsigned long), HEAD_INFO, fd);
	 FREESIZE        = buf[0];
	 NUMBER_OF_FILES = buf[1];
	 MAX_FILES       = buf[2];
	 MAX_BLOCKS      = buf[3];
	 FREE_BLOCKS     = buf[4];
	 OFFSET          = buf[5];
	files  = malloc(MAX_FILES * sizeof(struct FileDescriptor) );
	blocks = malloc(MAX_BLOCKS * sizeof(struct FileNode) ); 
	fread( files, sizeof(struct FileDescriptor), MAX_FILES, fd);
	fread( blocks, sizeof(struct FileNode), MAX_BLOCKS, fd);
	fclose(fd);
	
	return 0;

}

void Detailed_Info( void )
{
	int i;
	printf("All existing data blocks:\n");
	for(i=0; i< MAX_BLOCKS ; ++i)
	{
	  printf("Block no.%d:\t", i);
	  if( blocks[i].flags == 0)
	    printf("free.\n");
	  else
	    printf("in use by file: %s\n", files[blocks[i].File].FileName);
	}
}

int RemoveDisc( void )
{
	if(unlink("VirtualDisc") != 0)
	  return -1 ;
	return 0 ;
}

int CopyToVDisc( int argc, char **argv)
{
	int fd1, check, first;
	int prev, nxt, blockcounter, swap, namelength;
	char buf[BLOCKSIZE];
	struct stat st;
/*	buf[BLOCKSIZE] = '\0'; */
	char* tab;
	if( (fd1 = open(argv[argc], O_RDWR)) <0)
	{
		printf("Cannot open given file.\n");
		return -1;
	}
	stat(argv[argc], &st);
	if(NUMBER_OF_FILES == MAX_FILES) 
	{
		printf("Disc contains maximum number of files.\n");
		return -1;
	}
	if( FREESIZE < st.st_size)
	{
		printf("Not enough space left. Cannot copy file.\n");
		return -1;
	}
	for( swap = 0; swap < MAX_FILES ; ++swap)
	{
		if( files[swap].Created != 0 &&  strcmp(files[swap].FileName, argv[argc] ) == 0 )
		{
			printf("Given filename already exists.\n");
			return -1;
		}
	} 
	if( strlen(argv[argc]) > 17)
	{
		printf("Filename is too long. Aborted.\n");
		return -1;
	}
	++NUMBER_OF_FILES;
	blockcounter = (st.st_size-1) / BLOCKSIZE +1 ;
	first = -1;
	for(nxt = 0; nxt <MAX_FILES; ++nxt)
	{
		if(files[nxt].Created == 0)
		{
		  files[nxt].Created = 1;
		  files[nxt].FileSize = st.st_size;
		  strcpy(files[nxt].FileName, argv[argc]);  
		  prev = nxt;
		  break;
		}
	}
	swap = -1;
	FREE_BLOCKS -= blockcounter;
	FREESIZE    -= blockcounter * BLOCKSIZE;
	for(nxt = 0; nxt < MAX_BLOCKS ; ++nxt)
	{
		if(swap == -1 && blocks[nxt].flags == 0)
		{
		  if(first == -1)
		    first = nxt;
		  swap = prev;
		  blocks[nxt].flags = 1;
		  files[prev].DataNode = nxt;
		  blocks[nxt].File = prev;
		  prev = nxt;
		  --blockcounter;
		}
		if(swap != -1 && blocks[nxt].flags == 0)
		{
		  blocks[nxt].flags = 1;
		  blocks[prev].next = nxt;
		  blocks[nxt].File = swap;
		  prev = nxt;
		  --blockcounter;
		}
		if(blockcounter == 0)
		{
		  blocks[nxt].next = nxt;
		  break;
		}
	}
	if(nxt - first - 1 == (st.st_size - 1) / BLOCKSIZE +1 )
	    files[swap].Created = 2;
	nxt = files[swap].DataNode;
	lseek(fd1, 0, SEEK_SET);
	if(files[swap].Created == 1)
	{
 	  do
	  {
		  read(fd1, &buf, BLOCKSIZE);
		  lseek(fd, OFFSET + nxt * BLOCKSIZE, SEEK_SET);
		  write(fd, &buf, BLOCKSIZE);
		  nxt = blocks[nxt].next;
	  } while(blocks[nxt].next != nxt);
	  prev = st.st_size  % BLOCKSIZE;
	  read(fd1, &buf, prev);
	  lseek(fd, OFFSET+nxt*BLOCKSIZE, SEEK_SET);
	  write(fd, &buf, prev);
	}
	else
	{
	  tab = malloc(sizeof(char) * st.st_size);
	  read(fd1, tab, st.st_size);
	  lseek(fd, OFFSET + nxt * BLOCKSIZE, SEEK_SET);
	  write(fd, tab, st.st_size);
	  free(tab);
	}
	printf("File copied to VirtualFileSystem.\n");
	close(fd1);
	return 0; 
}

int CopyToDisc( int argc, char ** argv)
{
	int i, check, nxt, fd1, lastdata;
	char buf[BLOCKSIZE];
	char *tab;
	check = 0;
	for(i=0; i<MAX_FILES; ++i)
		if( files[i].Created != 0 && strcmp( files[i].FileName, argv[argc] ) ==  0)
		{	
		 check = 1;
		 break;
		}
	if(check == 0)
	{
		printf("File not found.\n");
		return -1;
	}
	if( (fd1 = creat(argv[argc+1], S_IRWXU)) < 0 )
	{
		printf("Could not create file on Disc.\n");
		return -1;
	}	
	nxt = files[i].DataNode;
	if(files[i].Created == 1)
	{
	  while( blocks[nxt].next != nxt)
	  {
		  lseek(fd, OFFSET + nxt*BLOCKSIZE, SEEK_SET);
		  read(fd, &buf, BLOCKSIZE);
		  write(fd1, &buf, BLOCKSIZE);
		  nxt = blocks[nxt].next;
	  }
	  lastdata = files[i].FileSize % BLOCKSIZE;
	  lseek(fd, OFFSET + nxt*BLOCKSIZE, SEEK_SET);
	  read(fd, &buf, lastdata);
	  write(fd1, &buf, lastdata);
	}
	else
	{
	  tab = malloc(sizeof(char) * files[i].FileSize);
	  lseek(fd, OFFSET + nxt*BLOCKSIZE, SEEK_SET);
	  read(fd, tab, files[i].FileSize);
	  write(fd1, tab, files[i].FileSize);
	} 
	close(fd1);
	printf("File copied from VirtualDisc to given directory.\n");
	return 0;
}

int RemoveFromVDisc( int argc, char ** argv )
{
	int i, check;
	check = 0;
	for( i = 0; i < MAX_FILES; ++i)
	if( files[i].Created != 0 && strcmp( files[i].FileName , argv[argc] ) == 0 )
		{
		  check = 1;
		  break;
		}
	if(check == 0)
	{
		printf("File not found.\n");
		return -1;
	}
	files[i].Created = 0;
	--NUMBER_OF_FILES;
	FREESIZE += files[i].FileSize;
	i = files[i].DataNode;
	do
	{
		blocks[i].flags = 0;
		i = blocks[i].next;
		++FREE_BLOCKS;
	} while(i != blocks[i].next);
	blocks[i].flags = 0;
	++FREE_BLOCKS;
	Defragment();  
	printf("File deleted.\n");
	return 0;
}

void ls( void )
{
	int i, k;
	k=0;
	for(i = 0; i < MAX_FILES; ++i)
	{
		if(files[i].Created != 0)
		{
		  ++k;
		  printf("%s\t", files[i].FileName);
		  if(k%4 == 0)
		    printf("\n");
		}
	}
	printf("\n");
}

void MainInfo( void )
{
	printf("FREESIZE: %d \nNUMBER_OF_FILES: %d \nMAX_FILES: %d \nMAX_BLOCKS: %d \nOFFSET: %d \nFREE_BLOCKS: %d\n", FREESIZE, NUMBER_OF_FILES, MAX_FILES, MAX_BLOCKS, OFFSET, FREE_BLOCKS);
}

void Defragment( void )
{
	int i, k, j, g, lblock, numblocks, defcount, nxt, nonempty, tmp;
	int toswap;
	int *tab;
	char *from, *to;
	toswap = 0;
	for(i=0 ; i< MAX_FILES ; ++i)
	{
	 lblock = 0;
	 if(files[i].Created != 0 )
	 {
	   nonempty = 1;
	   numblocks = ( files[i].FileSize - 1 ) / BLOCKSIZE +1 ;
	   for(k=0 ; k<MAX_BLOCKS ; ++k)
	   {
	     if(blocks[k].File == i || blocks[k].flags == 0)
	     {
		++lblock;
		if(blocks[k].flags == 0)
		  nonempty = 0;
	     }
	     else
	     {
	        lblock = 0;
	     	nonempty = 1;
	     }
	     if(nonempty == 0 && lblock == numblocks)
	     {
		from = malloc(BLOCKSIZE * sizeof(char));
		to   = malloc(BLOCKSIZE * sizeof(char));
	        k -= numblocks-1;
	        tab = malloc(numblocks * sizeof(int));
	        nxt = files[i].DataNode;
		for(j=0 ; j<numblocks ; ++j)
		{
		  tab[j] = nxt;
		  nxt = blocks[nxt].next;
		}
		for(j=0 ; j< numblocks; ++j)
		{
		  if(tab[j] < k+numblocks  && tab[j] >= k)
		  {
		    blocks[j].flags = 0;
		    lseek(fd, OFFSET + BLOCKSIZE * tab[j], SEEK_SET);
		    read(fd, from, BLOCKSIZE);
		    lseek(fd, OFFSET + BLOCKSIZE * (k+j), SEEK_SET);
		    read(fd, to, BLOCKSIZE);
		    lseek(fd, OFFSET+BLOCKSIZE * tab[j], SEEK_SET);
	  	    write(fd, to, BLOCKSIZE);
		    lseek(fd, OFFSET+BLOCKSIZE*(k+j), SEEK_SET);
		    write(fd, from, BLOCKSIZE);
		    for(g=0; g<numblocks ; ++g)
		    {
		      if(tab[g] == j+k)
		      {
		        tmp = tab[j];
		        tab[j] = tab[g];
		        tab[g] = tmp;
		        blocks[g+k].flags = 0; 
		        break;
		      }
		    } 
		  }
		}
		for(j=0 ; j< numblocks; ++j)
		{
		  if(tab[j] != k+j)
		  {
		    blocks[tab[j]].flags = 0;
		    lseek(fd, OFFSET + BLOCKSIZE * tab[j], SEEK_SET);
		    read(fd, from, BLOCKSIZE);
		    lseek(fd, OFFSET + BLOCKSIZE * (j+k), SEEK_SET);
		    write(fd, from, BLOCKSIZE);
		  }
		}
		for(j=0; j<numblocks ; ++j)
		{
		   blocks[k+j].flags = 1;
		   blocks[k+j].File = i;
		   blocks[k+j].next = k+j+1;
		}
		blocks[k+j-1].next = k+j-1;
	        files[i].DataNode = k;
	        free(from);
	        free(to);
	        free(tab);
		toswap++;
	        break;
	      }
	      
	    }  
	  }
	 } 
	printf("Defragmentation done. %d file fragments have been merged.\n", toswap);
}

void Help( void )
{
	printf("Type ./e [Argument] to use following functions:\n");
	printf("[1] [NUMBER_OF_KILOBYTES] - create Disc\n");
	printf("[2] [FileName] - copy file from Disc to VirtualFileSystem.\n");
	printf("[3] [FileName] [NewName] - Copy file from VirtualFileSystem to Disc.\n");
	printf("[4] - use ls function in VFS.\n");
	printf("[5] [FileName] - remove file from VFS.\n");
	printf("[6] - remove whole VFS Disc.\n");
	printf("[7] - get detailed info about all the blocks in VFS memory.\n");
	printf("[8] - get info about Disc's size, number of files etc.\n");
	printf("[9] - manually defragment Disc.\n");
}

int main(int argc, char **argv)
{
	int isCreated;
	isCreated = Read_Metainfo(); 
	if( argc < 2 )
	{
	  printf("Need an argument!\n");
	  return -1;
	}
	if( isCreated == -1 )
	{
	 switch( argv[1][0] )
	 {
	  case '1':
	 CreateDisc( atoi( argv[2] ) ); break;
	 default: printf("You can only use 1 function until you create VDisc\n"); 
	 printf("Type ./e 1 [NUMBER_OF_KILOBYTES] in order to create a VDisc.\n"); break;
	 }
	}
	else
	{
	fd = open("/lab6/VirtualDisc", O_RDWR);
	switch( argv[1][0] )
	{
	  case '1':
    	CreateDisc( atoi( argv[2] ) ); break;
	  case '6': 
	if( RemoveDisc() == -1 )
		printf("Could not delete Virtual Disc.\n");
	else printf("Virtual Disc deleted succefuly.\n");
	break;
	  case '2': CopyToVDisc( 2, argv ); break;
	  case '3': CopyToDisc( 2, argv ); break;
	  case '4': ls(); break;
	  case '5': RemoveFromVDisc(2, argv); break;
	  case '7': Detailed_Info(); break;
	  case '8': MainInfo(); break;
	  case '9': Defragment(); break;
	  case 'h': Help(); break;
	default: printf("Type ./e h to get manual.\n"); break;
 	}
	}
	Write_Metainfo();
 	free(files);
	free(blocks);
	close(fd);
	return 0;
}
