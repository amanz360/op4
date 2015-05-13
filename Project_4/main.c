#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#define N_FRAMES 32
#define FRAME_SIZE 1024
#define true 1
#define false 0
#define N_CLIENTS 10
#define BUFFER_SIZE 1024

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

void resetEntry(PageTableEntry* entry)
{
  entry->filename = NULL;
  entry->startIndex = -1;
  entry->endIndex = -1;
  entry->filesize = -1;
  entry->lastUsed = -1;
}

void sendEntry(int socket, int offset, int length, PageTableEntry entry)
{

}

void sendFromDirectory(int socket, int offset, int length, char* filename)
{
    int fd_from;
    char buf[4096];
    ssize_t nread;
    int total = 0;

    char from[strlen(".storage/")+strlen(filename)];
    strcpy(from, ".storage/");
    strcat(from, filename);
    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
    {
      perror("ERR: PROBLEM OPENING FILE\n");
        return;
    }

    while(nread = read(fd_from, buf, 1), total < offset)
    {
      total++;
    }

    total = 0;
    while (nread = read(fd_from, buf, sizeof buf), nread > 0 && total < length)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            if(nread+total <= length)
            {
              nwritten = write(socket, out_ptr, nread);  
            }
            else
            {
              nwritten = write(socket, out_ptr, length-total);
            }

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
                total += nwritten;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
  
      close(fd_from);

      /* Success! */
    }
}

int sgetline(int fd, char ** out) 
{ 
    int buf_size = 128; 
    int bytesloaded = 0; 
    int ret;
    char buf; 
    char * buffer = malloc(buf_size); 
    char * newbuf; 

    if (NULL == buffer)
        return -1;

    do
    {
        // read a single byte
        ret = read(fd, &buf, 1);
        if (ret < 1)
        {
            // error or disconnect
            free(buffer);
            return -1;
        }

        buffer[bytesloaded] = buf; 
        bytesloaded++; 

        // has end of line been reached?
        if (buf == '\n') 
            break; // yes

        // is more memory needed?
        if (bytesloaded >= buf_size) 
        { 
            buf_size += 128; 
            newbuf = realloc(buffer, buf_size); 

            if (NULL == newbuf) 
            { 
                free(buffer);
                return -1;
            } 

            buffer = newbuf; 
        } 
    } while(true);

    // if the line was terminated by "\r\n", ignore the
    // "\r". the "\n" is not in the buffer
    if ((bytesloaded) && (buffer[bytesloaded-1] == '\r'))
        bytesloaded--;

    *out = buffer; // complete line
    return bytesloaded; // number of bytes in the line, not counting the line break
}


//handles client connection
void* threadFunc(void * args)
{
  printf("Handling client with socket descriptor: %d", arguments->socketDescriptor);
  threadArgs* arguments = (threadArgs*) args;
  char* buffer;
  int ret;
  while(true)
  {
    ret = sgetline(arguments->socketDescriptor, &buffer);
    printf("%s", buffer);
    if(ret < 0)
    {
      break; //error / disconnect
    }
    char cmd[7];
    int off, len;
    off=len=0;
    sscanf(buffer, "%6s %d %d", cmd, &off, &len);
    printf("%s", buffer);
    if(strcmp(cmd, "STORE")==0){
      printf("hit store\n");
    }
    else if(strcmp(cmd, "READ")==0){
      printf("hit read\n");
    }
    else if(strcmp(cmd, "DEL")==0){
      printf("hit delete\n");
    }
    else if(strcmp(cmd, "DIR")==0){
      printf("hit dir\n");
    }
    else{
      //send error message to user
      printf("hit error\n");
    }
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

void readFile(char* filename, int offset, int length, int socket)
{
	//TODO: Check if the specified file is in the stored server memory, if so, simply gather the
	//	    file contents from there in a string. If not, then
	//		  read the filename specified out of the directory
	//		  and store its contents in a string. return the string either way.
  //      Also, if the file was not in server memory, overwrite the least recently used file in server memory with it.
	//		  format: <file-contents>\n

  int found = false;
  int i;
  for(i = 0; i < N_FRAMES; i++)
  {
    if(pageTable[i].filename == NULL)
    {
      continue;
    }
    else if(strcmp(pageTable[i].filename, filename) == 0)
    {
      found = true;
      if(length > pageTable[i].filesize - offset)
      {
        write(socket, "ERROR: INVALID BYTE RANGE", strlen("ERROR: INVALID BYTE RANGE"));
        return;
      }
      else
      {
        sendEntry(socket, offset, length, pageTable[i]);
      }
      break;
    }
  }

  if(!found)
  {
    sendFromDirectory(socket, offset, length, filename);
    saveToMemory(filename);
  }
}

void storeFile(char* filename, int socket, int filesize)
{
	//TODO: Make a new file in the storage directory with the passed in filename
	//		and write 'contents' into it
}

void dir()
{
	//TODO: Return a string contaning the number of files in the directory followed by every filename
	//		  format: <number-of-files>\n<filename1>\n<filename2>\netc.\n
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
