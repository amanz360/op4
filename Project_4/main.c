#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
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

  /*char* cmd[7];
  int off, len;
  off=len=0;
  sscanf(buffer, "$6s %d %d", cmd, &off, &len)
  if(strcmp(cmd, "STORE")==0){

  }
  else if(strcmp(cmd, "READ")==0){

  }
  else if(strcmp(cmd, "DEL")==0){

  }
  else if(strcmp(cmd, "DIR")==0){

  }
  else{
    //send error message to user
  }*/

	//reads in client message
	//checks if it is a valid command, sends back an error message otherwise
	//if valid read: read(file info)
	//if valid store: store(file info)
	//if valid delete: del(file info)
	//if valid dir: dir() <--display all files in directory
}

char* readFile(char* filename, int offset)
{
	//TODO: Check if the specified file is in the stored server memory, if so, simply gather the
	//	    file contents from there in a string. If not, then
	//		read the filename specified out of the directory
	//		and store its contents in a string. return the string either way.
	//		format: <file-contents>\n

	return "ACK\n";
}

void storeFile(char* filename, int socket, int filesize)
{
	//TODO: Make a new file in the storage directory with the passed in filename
	//		and write 'contents' into it

	int file = open(filename, O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (errno == EEXIST)
	{
		char* message = "ERROR: FILE EXISTS\n";
		write(socket,message,strlen(message));
		return;
	}

	char* buffer = calloc(BUFFER_SIZE, sizeof(char));
	
	while(true)
	{
		int n, w;
		n = read(socket, buffer, BUFFER_SIZE);
		if (n==0) break;
		if (n<0)
		{
			char* fail_message = "ERROR: read failed\n";
			write(socket,fail_message,strlen(fail_message));
			return;
		}
		w = write(file, buffer, n);
		if (w<0)
		{
			char* fail_message = "ERROR: write failed\n";
			write(socket,fail_message,strlen(fail_message));
			return;
		}
  	}

	free(buffer);
	close(socket);
	close(file);

	char* success_message = "ACK\n";
	write(socket,success_message,strlen(success_message));
	return;
}

void del(char* filename, int socket)
{
	//TODO: -- delete file <filename> from the storage server
	//		-- if the file does not exist, return an "ERROR: NO SUCH FILE\n" error
	//		-- return "ACK\n" if successful
	//		-- return "ERROR: <error-description>\n" if unsuccessful
	struct stat st;
	int result = stat(filename, &st);
	if (result != 0)
	{
		char* message = "ERROR: NO SUCH FILE\n";
		write(socket,message,strlen(message));
		return;
	}

	// Check if file is in the page table, if so delete it
	int i;
	for (i=0; i<N_FRAMES; i++)
	{
		if (strcmp(pageTable[i].filename,filename)==0)
		{
			resetEntry(&pageTable[i]);
		}
	}

	// Delete file from directory
	int status = remove(filename);
	if (status != 0)
	{
		char* message = "ERROR: remove file failed\n";
		write(socket,message,strlen(message));
		return;
	}
	else
	{
		char* message = "ACK\n";
		write(socket,message,strlen(message));
		return;
	}

}

void dir(int socket)
{
	//TODO: Return a string contaning the number of files in the directory followed by every filename
	//		format: <number-of-files>\n<filename1>\n<filename2>\netc.\n

	//TODO: Unsure if BUFFER_SIZE gives enough room

	DIR* dir = opendir(".");
	if (dir == NULL)
	{
		char* fail_message = "ERROR: opendir failed\n";
 		write(socket,fail_message,strlen(fail_message));
		return;
	}

	char filelist[BUFFER_SIZE];
	int num_files = 0;
	struct dirent* file;
	while((file = readdir(dir)) != NULL)
	{
		struct stat buf;
		int rc = lstat(file->d_name, &buf);
		if (rc==-1)
		{
			char* fail_message = "ERROR: lstat failed\n";
 			write(socket,fail_message,strlen(fail_message));
			return;
		}
		if (S_ISREG(buf.st_mode))
		{
			char tmp[BUFFER_SIZE];
			strcpy(tmp,"\n");
			strcat(tmp,file->d_name);
			strcat(filelist,tmp);
			num_files++;
		}
	}

	char output[BUFFER_SIZE];
	strcpy(output,(char*)num_files);
	strcat(output,filelist);

	char* success_message = "ACK\n";
	write(socket,success_message,strlen(success_message));
	return;
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
