#include <iostream>
#include <fstream>
#include <vector>

#include <string.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/types.h>
#include <unistd.h>


using namespace std;

#define SERVER_PORT 5432
#define MAX_PENDING 5
#define MAX_LINE 1000

void clearBuffer(char* buf);

//Puts the contents of the ls command into buf and calls fillBuffersWithStream
void lsCommand(vector<char>& buf);

//Opens the file specified in buf
//If the file exists, fillBufferWithStream is called
void getFileCommand(char* command, vector<char>& buf, int commandLength);


//Fills buf with the contents of fp
//appends -1 to the end of the buffer
void fillBufferWithStream(vector<char>& buf, FILE* fp);

//like the above except it takes in a filename and reads from a file in binary
void fillBufferWithStreamBinary(vector<char>& buf, string filename);

//Fills buf with the contents of message
//appends -1 to the end of the buffer
void fillBufferWithMessage(vector<char>& buf, string message);

//Creates a server socket with the specified port number and gives a handle to the socket through socketHandle
void createServerSocket(int& socketHandle, struct sockaddr_in& sin, socklen_t& len, int port);


int main(int argc, char* argv[]) {
	cout << "I am Server" << endl;

	int socketHandle;
	struct sockaddr_in sin;
	socklen_t len;

	char buf[MAX_LINE]; //only used for recieving requests
	vector<char> buffer;
	buffer.reserve(MAX_LINE);	

	int clientSocketHandle;

	
	createServerSocket(socketHandle, sin, len, SERVER_PORT);
	
	cout << "Accepting..." << endl;
	if((clientSocketHandle = accept(socketHandle, (struct sockaddr*)&sin, &len)) < 0) {
		perror("simplex-talk: accept");
		exit(1);
	}
	cout << "=== Accepted!!! ===" << endl;

	//receive and do processes
	while(1) {


		while(len = recv(clientSocketHandle, buf, sizeof(buf), 0)) {
			
			//If client has sent the quit command or has disconnected
			if(strcmp(buf, "q\n") == 0 || len == 0) { 
				cout << "Server exiting" << endl;
				close(clientSocketHandle);
				close(socketHandle);
				exit(0);
			}

			//1. ls: print on the client window a listing of the contents of the current directory on the server machine.
			if(strcmp(buf, "ls\n") == 0) { //ls command

				lsCommand(buffer);
				send(clientSocketHandle, buffer.data(), buffer.size(), 0);
				buffer.clear();

			}
			//2. Another file transfer command of your choice.
			//cd command
			else if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ' && len > 3) {

				//trim "cd " from the command to get the path
				string command(buf);
				command = command.substr(3, len);
				//take out any newlines
				string path;				
				for(int i=0; i<command.length(); i++) {
					if(command.at(i) != '\n') {
						path += command.at(i);
					}			
				}
				cout << "path = " << path << endl;
				int result = chdir(path.c_str());
	
				if(result == 0) {
					fillBufferWithMessage(buffer, "Changed directory");
				}
				else {
					fillBufferWithMessage(buffer, "Unable to change directory");
				}

				send(clientSocketHandle, buffer.data(), buffer.size(), 0);
				buffer.clear();
			}
			//2. get remote-file: retrieve the remote-­‐file on the server and store it on the client machine.
			//It is given the same name it has on the server machine.
			else if(buf[0] == 'g' && buf[1] == 'e' && buf[2] == 't' && buf[3] == ' ' && len > 4) { //get remote-file

				//create new connection with client just for this file transfer
				int tempSocketHandle;
				int tempClientSocketHandle;
				struct sockaddr_in tempSin;
				socklen_t tempLen;
				createServerSocket(tempSocketHandle, tempSin, tempLen, 5431);
				while((tempClientSocketHandle = accept(socketHandle, (struct sockaddr*)&sin, &len)) < 0) {}

				getFileCommand(buf, buffer, len);
				send(tempClientSocketHandle, buffer.data(), buffer.size(), 0);
				buffer.clear();

				close(tempClientSocketHandle);
				close(tempSocketHandle);
			}
			else {
				fillBufferWithMessage(buffer, "Invalid Command");
				send(clientSocketHandle, buffer.data(), buffer.size(), 0);
				buffer.clear();
			}

			clearBuffer(buf);
		}
		
	}

	

	return 0;
}

void lsCommand(vector<char>& buf) {

	FILE *fp = popen("ls", "r");

	fillBufferWithStream(buf, fp);

	pclose(fp);
}

void getFileCommand(char* command, vector<char>& buf, int commandLength) {		
	
	string theCommand(command);

	//take out the "get " from the command char array
	theCommand = theCommand.substr(4, commandLength);
	
	string filename;
	//take out the newline at the end
	for(int i=0; i<theCommand.length(); i++) {
		if(theCommand[i] != '\n') {
			filename += theCommand[i];
		}
	}
	
	fillBufferWithStreamBinary(buf, filename); 
}

void fillBufferWithStreamBinary(vector<char>& buf, string filename) {
	ifstream file(filename.c_str(), ios::in|ios::binary|ios::ate); //ios::ate sets the initial position at the end of the file

	streampos size;
	char* memblock;

	if(file.is_open()) {

		size = file.tellg();

		memblock = new char[size];		

		file.seekg(0, ios::beg);
		file.read(memblock, size);
		file.close();

		buf.assign(memblock, memblock+size-1);

		
		delete[] memblock;	
			
	}
	else {
		cout << "Unable to open the file" << endl;
		
	}
}

void fillBufferWithStream(vector<char>& buffer, FILE* fp) {
	buffer.clear();

	char c;
	while((c = fgetc(fp)) != EOF) {
		buffer.push_back(c);	
	}
	buffer.push_back(-1);
}

void fillBufferWithMessage(vector<char>& buffer, string message) {	
	buffer.assign(message.c_str(), message.c_str()+message.length());
	buffer.push_back(-1);
}


void clearBuffer(char* buf) {
	memset(buf, 0, sizeof(char)*MAX_LINE);
}


void createServerSocket(int& socketHandle, struct sockaddr_in& sin, socklen_t& len, int port) {

	//Build address data structure
	memset((char*)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	//By not specifying an IP address, 
	//	the application program is willing to accept connections on any of the local host’s IP addresses
	sin.sin_addr.s_addr = INADDR_ANY;

	sin.sin_port = htons(port);
	len = sizeof(sin);

	//setup passive open
	if((socketHandle = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("simplex-talk: socket");
		exit(1);
	}
	if((bind(socketHandle, (struct sockaddr*)&sin, sizeof(sin))) < 0) {
		perror("simplex-talk: bind");
		exit(1);
	}

	if(listen(socketHandle, MAX_PENDING) < 0) {
		perror("simplex-talk: listen");
		exit(1);
	}

}

