#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#define N_FRAMES 32
#define FRAME_SIZE 1024
#define FRAMES_PER_FILE 4
#define true 1
#define false 0
#define N_CLIENTS 10
#define BUFFER_SIZE 1024

enum {ACTIVE, EMPTY, FINISHED};

typedef struct
{
	int serverIndices[FRAMES_PER_FILE];
	int filesize;
	char filename[BUFFER_SIZE];
	time_t lastused;
} PageTableEntry;

typedef struct{
  int socketDescriptor;
  int index;
} threadArgs;

int openSocket();
void* threadFunc(void * args);
void storeFile(char* filename, int socket, int filesize);
void del(char* filename, int socket);
void dir(int socket);
void readFile(char* filename, int offset, int length, int socket);

//globally define page table and server page memory structure
PageTableEntry pageTable[N_FRAMES];
char pages[N_FRAMES][FRAME_SIZE];
pthread_t threads[N_CLIENTS];
volatile int status[N_CLIENTS];

int main(int argc, char** argv)
{
  mkdir(".storage", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

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
        //printf("Thread joined\n");
        connections--;
      }
	  }
	  if(connections<N_CLIENTS){
      int sockd = accept( sd, (struct sockaddr *)&client,(socklen_t*)&fromlen );
      if(sockd<0) perror("accept() failed");
	  char* address = inet_ntoa(client.sin_addr);
	  printf("Received incoming connection from %s\n", address);
      for(i=0;i<N_CLIENTS;i++){
        if(status[i]==EMPTY){
          threadArgs args;
          args.socketDescriptor = sockd;
          args.index=i;
          pthread_create(&(threads[i]), NULL, threadFunc, &args);
          //printf("Thread created\n");
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
  entry->filename[0] = '\0';
  entry->filesize = -1;
  entry->lastused = -1;
  int i;
  for(i = 0; i < FRAMES_PER_FILE; i++)
  {
	entry->serverIndices[i] = -1;
  }
}

void sendEntry(int socket, int offset, int length, PageTableEntry entry)
{	
    int total = 0;
	int nwritten;

	char * output;
	int first = true;
	int i = 0;
    while (total < length && i < FRAMES_PER_FILE)
    {
		if(offset > (i+1) * FRAME_SIZE)
		{
			continue;
		}
		else if(first == true)
		{
			output = pages[entry.serverIndices[i]];
			output += offset - (i*FRAME_SIZE);
		}
		else
		{
			output = pages[entry.serverIndices[i]];
		}

        if(strlen(output)+total <= length)
        {
			char ok_msg[100];
		    sprintf(ok_msg, "ACK %d\n", (int)strlen(output));
			write(socket, ok_msg, (int)strlen(ok_msg));
			printf("[thread %u] Sent: ACK %d\n", (int)pthread_self(),(int)strlen(output));
			printf("[thread %u] Transferred %d bytes from offset %d\n", (int)pthread_self(), (int)strlen(output), total+offset);
        	nwritten = write(socket, output, strlen(output));  
        }
        else
        {
			char ok_msg[100];
			  sprintf(ok_msg, "ACK %d\n", length-total);
			  write(socket, ok_msg, strlen(ok_msg));
			  printf("[thread %u] Sent: ACK %d\n", (int)pthread_self(),length-total);
			  printf("[thread %u] Transferred %d bytes from offset %d\n", (int)pthread_self(), length-total, total+offset);
            nwritten = write(socket, output, length-total);
        }

        if (nwritten >= 0)
        {
        	total += nwritten;
        }
		i++;
    }

    if (total >= length)
    {
	  write(socket,"\n",1);
	  write(socket,"ACK\n",strlen("ACK\n"));
	  return;

      /* Success! */
    }
	write(socket,"\n",1);
	write(socket,"ACK\n",strlen("ACK\n"));
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

    lseek(fd_from, offset, SEEK_SET);

    total = 0;
    while (nread = read(fd_from, buf, sizeof buf), nread > 0 && total < length)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            if(nread+total <= length)
            {
			  char ok_msg[100];
			  sprintf(ok_msg, "ACK %d\n", (int)nread);
			  write(socket, ok_msg, strlen(ok_msg));
			  printf("[thread %u] Sent: ACK %d\n", (int)pthread_self(),(int) nread);
			  printf("[thread %u] Transferred %d bytes from offset %d\n", (int)pthread_self(), (int)nread, total+offset);
              nwritten = write(socket, out_ptr, nread);  
            }
            else
            {
			  char ok_msg[100];
			  sprintf(ok_msg, "ACK %d\n", length-total);
			  write(socket, ok_msg, strlen(ok_msg));
			  printf("[thread %u] Sent: ACK %d\n", (int)pthread_self(), length-total);
			  printf("[thread %u] Transferred %d bytes from offset %d\n", (int)pthread_self(), length-total, total+offset);
              nwritten = write(socket, out_ptr, length-total);
            }

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
                total += nwritten;
            }
        } while (nread > 0 && total < length);
    }

    if (nread == 0)
    {
	  write(socket, "\n", 1);
  	  write(socket,"ACK\n",strlen("ACK\n"));
	  printf("[thread %u] Sent: ACK\n", (int)pthread_self());
      close(fd_from);
	  return;

      /* Success! */
    }
	write(socket, "\n", 1);
	write(socket,"ACK\n",strlen("ACK\n"));
	printf("[thread %u] Sent: ACK\n", (int)pthread_self());
	close(fd_from);
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

void saveToMemory(char* filename)
{
	int fd_from;
    char buffer[FRAME_SIZE];
    ssize_t nread;

    char from[strlen(".storage/")+strlen(filename)];
    strcpy(from, ".storage/");
    strcat(from, filename);
    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
    {
      perror("ERR: PROBLEM OPENING FILE\n");
        return;
    }

    PageTableEntry newEntry;
	resetEntry(&newEntry);
	strcpy(newEntry.filename, filename);
	int inUse[N_FRAMES];

	char workingDirectory[1024];
	getcwd(workingDirectory,1024);
	char * storageDirectory = malloc(sizeof(workingDirectory) + sizeof("/.storage")); 
	strcpy(storageDirectory, workingDirectory);
	strcat(storageDirectory, "/.storage");
	
	struct stat buf;
	char storageFile[BUFFER_SIZE];
	strcpy(storageFile, storageDirectory);
	strcat(storageFile, "/");
	strcat(storageFile, filename);
	int rc = lstat(storageFile, &buf);
	if(rc < 0)
	{
		perror("ERR: LSTAT FAILED\n");
		return;
	}
	newEntry.filesize = (int) buf.st_size;
	
	int framesNeeded = newEntry.filesize/FRAME_SIZE;
	if(newEntry.filesize % FRAME_SIZE != 0) framesNeeded++;

	if(framesNeeded > FRAMES_PER_FILE)
	{
		perror("ERROR: file too large\n");
		return;
	}

	int i, j;
	int availFrames = N_FRAMES;
	for(i = 0; i < N_FRAMES; i++)
	{
		inUse[i] = false;
	}

	for(i = 0; i < N_FRAMES; i++)
	{
		for(j = 0; j < FRAMES_PER_FILE; j++)
		{
			if(strlen(pageTable[i].filename) != 0 && pageTable[i].serverIndices[j] != -1)
			{
				inUse[pageTable[i].serverIndices[j]] = true;
				availFrames--;
			}
		}
	}

	time_t rawtime;
	time ( &rawtime );
	newEntry.lastused = rawtime;
	time_t oldest = -1;
	int oldestIndex = -1;
	if(availFrames < framesNeeded)
	{
		for(i = 0; i < N_FRAMES; i++)
		{
			double diff;
			if(strlen(pageTable[i].filename) != 0)
			{
				diff = difftime(pageTable[i].lastused, oldest);
				if(oldestIndex == -1 || diff < 0)
				{
					oldest = pageTable[i].lastused;
					oldestIndex = i;
				}
			}
		}
	}

	if(oldestIndex != -1)
	{
		resetEntry(&pageTable[oldestIndex]);
		printf("[thread %u] Page %d freed for extra memory (frames", (int)pthread_self(), oldestIndex);
		for(i = 0; i < FRAMES_PER_FILE; i++)
		{
			if(pageTable[oldestIndex].serverIndices[i] != -1)
			{
				printf(" %d", pageTable[oldestIndex].serverIndices[i]);
			}
		}
		printf("\n");
	}
	
	int pageIndex = 0;
	for(i = 0; N_FRAMES; i++)
	{
		if(strlen(pageTable[i].filename) == 0)
		{
			pageIndex = i;
			break;
		}
	}

    while (nread = read(fd_from, buffer, FRAME_SIZE), nread > 0)
    {
		for(i = 0; i < N_FRAMES; i++)
		{
			if(inUse[i] == false)
			{
				strcpy(pages[i], buffer);
				printf("[thread %u] Allocated page %d to frame %d\n", (int)pthread_self(), pageIndex, i);
				for(j = 0; j < FRAMES_PER_FILE; j++)
				{
					if(newEntry.serverIndices[j] == -1)
					{
						newEntry.serverIndices[j] = i;
						break;
					}
				}
				break;
			}
		}
    }


    if (nread == 0)
    {
      close(fd_from);
	  pageTable[pageIndex] = newEntry;
	  return;

      /* Success! */
    }
	pageTable[pageIndex] = newEntry;
	close(fd_from);
}

//handles client connection
void* threadFunc(void * args)
{
  threadArgs* arguments = (threadArgs*) args;
  char* buffer;
  int ret;
  while(true)
  {
    ret = sgetline(arguments->socketDescriptor, &buffer);
    if(ret <= 0)
    {
      printf("[thread %u] Client closed its socket....terminating\n", (int)pthread_self());
      break; //error / disconnect
    }
    char cmd[7];
    char filename[BUFFER_SIZE];
    int off, len;
    off=len=0;
	printf("[thread %u] Rcvd: %s",(int)pthread_self(), buffer);
    sscanf(buffer, "%6s", cmd);
    if(strcmp(cmd, "STORE")==0){
      //printf("hit store\n");
      sscanf(buffer, "%6s %s %d", cmd, filename, &len);
      storeFile(filename, arguments->socketDescriptor, len);
    }
    else if(strcmp(cmd, "READ")==0){
      sscanf(buffer, "%6s %s %d %d", cmd, filename, &off, &len);
      //printf("hit read\n");
	  readFile(filename, off, len, arguments->socketDescriptor);
    }
    else if(strcmp(cmd, "DEL")==0){
      sscanf(buffer, "%6s %s", cmd, filename);
      //printf("hit delete\n");
      del(filename, arguments->socketDescriptor);
    }
    else if(strcmp(cmd, "DIR")==0){
      //printf("hit dir\n");
	  dir(arguments->socketDescriptor);
    }
    else{
      //send error message to user
      char * err_msg = "ERROR: Unknown command\n";
	  write(arguments->socketDescriptor, err_msg, strlen(err_msg));
	  printf("[thread %u] Sent: %s\n", (int)pthread_self(), err_msg);
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
	//if valid delete: del(file info)
	//if valid dir: dir() <--display all files in directory
}

void readFile(char* filename, int offset, int length, int socket)
{
	//TODO: Check if the specified file is in the stored server memory, if so, simply gather the
	//	    file contents from there in a string. If not, then
	//		  read the filename specified out of the directory
	//		  and store its contents in a string. return the string either way.
  //      Also, if the file was not in server memory, overwrite the least recently used file in server memory with it.

  int found = false;
  int i;
  for(i = 0; i < N_FRAMES; i++)
  {
    if(strlen(pageTable[i].filename) == 0)
    {
      continue;
    }
    else if(strcmp(pageTable[i].filename, filename) == 0)
    {
      found = true;
      if(length > pageTable[i].filesize - offset)
      {
        write(socket, "ERROR: INVALID BYTE RANGE\n", strlen("ERROR: INVALID BYTE RANGE\n"));
		printf("[thread %u] Sent: ERROR: INVALID BYTE RANGE\n", (int) pthread_self());
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
	//	Make a new file in the storage directory with the passed in filename
	//		and write 'contents' into it

  char to[strlen(".storage/")+strlen(filename)];
  strcpy(to, ".storage/");
  strcat(to, filename);
	int file = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (errno == EEXIST)
	{
		char* message = "ERROR: FILE EXISTS\n";
		write(socket,message,strlen(message));
		return;
	}

	char* buffer = calloc(BUFFER_SIZE, sizeof(char));
	int total = 0;
	while(true && total < filesize)
	{
		int n, w;
		n = read(socket, buffer, BUFFER_SIZE);
		if (n==0) break;
		if (n<0)
		{
			char* fail_message = "ERROR: read failed\n";
			write(socket,fail_message,strlen(fail_message));
			printf("[thread %u] Sent: %s", (int)pthread_self(), fail_message);
			return;
		}
		
		if (total+n <= filesize)
		{
			w = write(file,buffer,n);
			total += n;
		}
		else
		{
			w = write(file,buffer,filesize-total);
			break;
		}		

		if (w<0)
		{
			char* fail_message = "ERROR: write failed\n";
			write(socket,fail_message,strlen(fail_message));
			printf("[thread %u] Sent: %s", (int)pthread_self(), fail_message);
			return;
		}
  	}

	free(buffer);
	close(file);

	char* success_message = "ACK\n";
	write(socket,success_message,strlen(success_message));
	printf("[thread %u] Transferred file (%d bytes)\n",(int)pthread_self(), filesize);
	printf("[thread %u] Sent: ACK\n",(int)pthread_self());
	return;
}

void del(char* filename, int socket)
{
	//      -- delete file <filename> from the storage server
	//		-- if the file does not exist, return an "ERROR: NO SUCH FILE\n" error
	//		-- return "ACK\n" if successful
	//		-- return "ERROR: <error-description>\n" if unsuccessful
	struct stat st;
	char to[strlen(".storage/")+strlen(filename)];
  	strcpy(to, ".storage/");
  	strcat(to, filename);
	int result = lstat(to, &st);
	if (result != 0)
	{
		char* message = "ERROR: NO SUCH FILE\n";
		write(socket,message,strlen(message));
		return;
	}

	// Check if file is in the page table, if so delete it
	int i, j;
	for (i=0; i<N_FRAMES; i++)
	{
		if (strcmp(pageTable[i].filename,filename)==0)
		{
			for(j = 0; j < FRAMES_PER_FILE; j++)
			{
				if(pageTable[i].serverIndices[j] != -1)
				{

					printf("[thread %u] Deallocated frame %d\n", (int)pthread_self(), pageTable[i].serverIndices[j]);
				}
			}
			resetEntry(&pageTable[i]);
			break;
		}
	}

	// Delete file from directory
	int status = remove(to);
	if (status != 0)
	{
		char* message = "ERROR: remove file failed\n";
		write(socket,message,strlen(message));
		return;
	}
	else
	{
		printf("[thread %u] Deleted %s file\n", (int)pthread_self(), filename);
		char* message = "ACK\n";
		write(socket,message,strlen(message));
		printf("[thread %u] Sent: ACK\n",(int)pthread_self());
		return;
	}

}

void dir(int socket)
{
	//TODO: Return a string contaning the number of files in the directory followed by every filename
	//		format: <number-of-files>\n<filename1>\n<filename2>\netc.\n

	char workingDirectory[1024];
	getcwd(workingDirectory,1024);
	char * storageDirectory = malloc(sizeof(workingDirectory) + sizeof("/.storage")); 
	strcpy(storageDirectory, workingDirectory);
	strcat(storageDirectory, "/.storage");
	DIR* dir = opendir(storageDirectory);

	if (dir == NULL)
	{
		char* fail_message = "ERROR: opendir failed\n";
 		write(socket,fail_message,strlen(fail_message));
		return;
	}
	

	char filelist[BUFFER_SIZE*10];
	filelist[0] = '\0';
	int num_files = 0;
	struct dirent* file;
	while((file = readdir(dir)) != NULL)
	{
		struct stat buf;
		char storageFile[BUFFER_SIZE];
		strcpy(storageFile, storageDirectory);
		strcat(storageFile, "/");
		strcat(storageFile, file->d_name);
		int rc = lstat(storageFile, &buf);
		
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

	char output[BUFFER_SIZE*10+5];
    sprintf(output,"%d",num_files);
	strcat(output,filelist);
	strcat(output,"\n\0");

	write(socket,output,strlen(output));
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
