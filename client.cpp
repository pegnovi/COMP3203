//for binary file streaming, make a new connection with server
// disconnect is used as delimiter (FTP does this - it makes a new connection for every file)

//for part 3, can be things like change dir (cd)

#include <iostream>
#include <fstream>
#include <vector>
#include <stdio.h>

#include <string.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SERVER_PORT 5432
#define MAX_LINE 1000

using namespace std;

//This clears the character buffer passed into it setting all values to 0
void clearBuffer(char* buf);

//used when expecting data as chars
void receive(vector<char>& buffer, int socketHandle);

//used when expecting data as bytes with binary data
void receiveBinary(vector<char>& buffer, int socketHandle);

//copies the contents of buffer (which are bytes with binary data) into a file (named filename)
void copyBufferIntoFileBinary(vector<char>& buffer, string filename);

//connects to the specified server and gives a handle to socketHandle
void connectToServer(int& socketHandle, string ipAddress);


int main(int argc, char* argv[]) {
	cout << "I am Client" << endl;

	int socketHandle;

	char buf[MAX_LINE]; //only used for sending messages
	vector<char> buffer;
	buffer.reserve(MAX_LINE);

	int len;

	if(argc > 1) {
		for(int i=1; i<argc; i++) {
			cout << argv[i] << endl;	
		}
	}
	else {
		cout << "No Server specified in arguments" << endl;
		exit(1);
	}

	connectToServer(socketHandle, argv[1]);
	
	//main loop: get and send lines of text
	while(fgets(buf, sizeof(buf), stdin)) {
		buf[MAX_LINE-1] = '\0';
		len = strlen(buf) + 1;
		
		send(socketHandle, buf, len, 0);

		//if the user sent a quit message, quit the program
		if(strcmp(buf, "q\n") == 0) { 
			cout << "Client exiting" << endl;
			close(socketHandle);
			exit(0);
		}
		//if the user sent a get command
		else if(buf[0] == 'g' && buf[1] == 'e' && buf[2] == 't' && buf[3] == ' ') { 

			//take out the "get " from the string to get the filename
			string filename = buf;
			filename = filename.substr(4, len-6);
			filename += "2"; //append a 2 at the end of the file (since client and server code are on same dir)
			

			//establish new connection to server just for this file transfer
			int tempSocketHandle;
			connectToServer(tempSocketHandle, argv[1]);


			receiveBinary(buffer, tempSocketHandle);

			copyBufferIntoFileBinary(buffer, filename);
			
			close(tempSocketHandle);
		}
		//otherwise, wait to recieve a message from the server
		else { 

			receive(buffer, socketHandle);

			for(int i=0; i<buffer.size(); i++) {
				cout << buffer.at(i);
			}
			cout << endl;

		}
		//*/
		clearBuffer(buf);
	}

	return 0;
}


void receiveBinary(vector<char>& buffer, int socketHandle) {
	buffer.clear();

	char buf[100]; //buffer used for recv'ing data

	while(1) {
		
		socklen_t length = recv(socketHandle, buf, sizeof(buf), 0);

		//cout << "Length so far = " << length << endl;

		if(length == 0) { //server has disconnected
			break;
		}

		//copy the contents of buf into the string buffer
		for(int i=0; i<length; i++) {
			buffer.push_back(buf[i]); 
		}

	}
}


void receive(vector<char>& buffer, int socketHandle) {

	buffer.clear();

	//this is used to check if we should stop recv'ing
	bool endDelimFound = false;

	char buf[100]; //buffer used for recv'ing data

	while(!endDelimFound) {
		
		socklen_t length = recv(socketHandle, buf, sizeof(buf), 0);
		//copy the contents of buf into the string buffer
		for(int i=0; i<length; i++) {
			if(buf[i] == -1) { //if the delimiter has been found, we're done recv'ing data
				endDelimFound = true;
				break;
			}
			buffer.push_back(buf[i]); 
		}

	}

}

void clearBuffer(char* buf) {
	memset(buf, 0, sizeof(char)*MAX_LINE);
}

void copyBufferIntoFileBinary(vector<char>& buffer, string filename) {
	ofstream file(filename.c_str(), ios::out|ios::binary);
	file.write(buffer.data(), buffer.size());
	file.close();
}


void connectToServer(int& socketHandle, string ipAddress) {
	
	struct sockaddr_in sin;	

	//Build address data structure
	memset((char*)&sin, 0, sizeof(sin));	
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ipAddress.c_str());
	sin.sin_port = htons(SERVER_PORT);

	//Active open
	if((socketHandle = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("simplex-talk: socket");
		exit(1);
	}

	cout << "Connecting..." << endl;
	while(connect(socketHandle, (struct sockaddr*)&sin, sizeof(sin)) < 0) {}
	cout << "=== Connected!!! ===" << endl;
}

