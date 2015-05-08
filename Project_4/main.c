#include <time.h>
#define N_FRAMES 32
#define FRAME_SIZE 1024


struct PageTableEntry
{
	int startIndex;
	int endIndex;
	int filesize;
	char* filename;
	time_t lastUsed;
};

//globally define page table and server page memory structure
PageTableEntry pageTable[N_FRAMES];
char pages[N_FRAMES][FRAME_SIZE];

int main(int argc, char** argv)
{
	//open socket on port
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

char* read(char* filename, int offset)
{
	//TODO: Check if the specified file is in the stored server memory, if so, simply gather the
	//	    file contents from there in a string. If not, then
	//		read the filename specified out of the directory
	//		and store its contents in a string. return the string either way.
	//		format: <file-contents>\n
}

void store(char* filename, char* contents, int filesize)
{
	//TODO: Make a new file in the storage directory with the passed in filename
	//		and write 'contents' into it
}

char* dir()
{
	//TODO: Return a string contaning the number of files in the directory followed by every filename
	//		format: <number-of-files>\n<filename1>\n<filename2>\netc.\n
}