#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define N_FRAMES 32
#define FRAME_SIZE 1024
#define true 1
#define false 0


typedef struct
{
	int startIndex;
	int endIndex;
	int filesize;
	char* filename;
	time_t lastUsed;
} PageTableEntry;

//globally define page table and server page memory structure
PageTableEntry pageTable[N_FRAMES];
char pages[N_FRAMES][FRAME_SIZE];

int main(int argc, char** argv)
{
	openSocket();
	while(true)
	{
		//listen for client connection
		//hand off connection to thread
	}
}

//handles client connection
void threadFunc(int socket)
{
	//reads in client message
	//checks if it is a valid command, sends back an error message otherwise
	//if valid read: read(file info)
	//if valid store: store(file info)
	//if valid delete: delete(file info)
	//if valid dir: dir() <--display all files in directory
}

char* readFile(char* filename, int offset)
{
	//TODO: Check if the specified file is in the stored server memory, if so, simply gather the
	//	    file contents from there in a string. If not, then
	//		read the filename specified out of the directory
	//		and store its contents in a string. return the string either way.
	//		format: <file-contents>\n
}

void storeFile(char* filename, char* contents, int filesize)
{
	//TODO: Make a new file in the storage directory with the passed in filename
	//		and write 'contents' into it
}

char* dir()
{
	//TODO: Return a string contaning the number of files in the directory followed by every filename
	//		format: <number-of-files>\n<filename1>\n<filename2>\netc.\n
}

int openSocket(){
  int sd, rc;
  struct sockaddr_in server;
  int length;
  sd = socket( AF_INET, SOCK_DGRAM, 0 );
  if ( sd == -1 ){
    perror( "socket() failed" );
    return EXIT_FAILURE;
  }
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl( INADDR_ANY );
  server.sin_port = htons( 8765 );
  rc=bind( sd, (struct sockaddr *)&server, sizeof( server ) );
  if ( rc == -1 ){
    perror( "bind() failed" );
    return EXIT_FAILURE;
  }
  length = sizeof( server );
  rc = getsockname( sd, (struct sockaddr *)&server, (socklen_t *)&length );
  if ( rc == -1 ){
    perror( "getsockname() failed" );
    return EXIT_FAILURE;
  }
  printf( "Listening on port %d\n", ntohs( server.sin_port ) );
  return rc;
}
