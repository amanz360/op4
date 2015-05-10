#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define N_FRAMES 32
#define FRAME_SIZE 1024
#define true 1
#define false 0
#define N_CLIENTS 10

enum {ACTIVE, EMPTY, FINISHED};

typedef struct
{
	int startIndex;
	int endIndex;
	int filesize;
	char* filename;
	time_t lastUsed;
} PageTableEntry;

typedef struct{
  int socketDescriptor;
  int index;
} threadArgs;

int openSocket();
void* threadFunc(void * args);

//globally define page table and server page memory structure
PageTableEntry pageTable[N_FRAMES];
char pages[N_FRAMES][FRAME_SIZE];
pthread_t threads[N_CLIENTS];
volatile int status[N_CLIENTS];

int main(int argc, char** argv)
{
  int sd=openSocket();
	struct sockaddr_in client;
  int fromlen = sizeof( client );
  int connections=0;
  int i;
  for(i=0;i<N_CLIENTS;i++) status[i]=EMPTY;
  int rc=listen(sd, N_CLIENTS);
  if(rc<0) perror("accept() failed");
	while(true)
	{
	  void** rc = NULL;
	  for(i=0;i<N_CLIENTS;i++){
      if(status[i]==FINISHED){
        pthread_join(threads[i], rc);
        status[i]=EMPTY;
        printf("Thread joined\n");
        connections--;
      }
	  }
	  if(connections<N_CLIENTS){
      int sockd = accept( sd, (struct sockaddr *)&client,(socklen_t*)&fromlen );
      if(sockd<0) perror("accept() failed");
      for(i=0;i<N_CLIENTS;i++){
        if(status[i]==EMPTY){
          threadArgs args;
          args.socketDescriptor = sockd;
          args.index=i;
          pthread_create(&(threads[i]), NULL, threadFunc, &args);
          printf("Thread created\n");
          status[i]=ACTIVE;
          connections++;
          break;
        }
      }
    }
	}
}

//handles client connection
void* threadFunc(void * args)
{
  threadArgs* arguments = (threadArgs*) args;
  char* buffer = calloc(100, sizeof(char));
  while(true){
    int n;
    n=read(arguments->socketDescriptor, buffer, 99);
    if(n==0) break;
    write(arguments->socketDescriptor, buffer, n);
  }
  free(buffer);
  close(arguments->socketDescriptor);
  status[arguments->index]=FINISHED;
  return NULL;
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
  sd = socket( AF_INET, SOCK_STREAM, 0 );
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
  return sd;
}
