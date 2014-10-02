#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
using namespace std;

void fillBufferWithStreamBinary(vector<char>& buf, string filename);
void copyBufferIntoFileBinary(vector<char>& buffer, string filename);


int main() {
	//string buffer;

	vector<char> buffer;

	fillBufferWithStreamBinary(buffer, "dragon.jpg");
	copyBufferIntoFileBinary(buffer, "dragon2.jpg");

	return 0;
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

void copyBufferIntoFileBinary(vector<char>& buffer, string filename) {
	ofstream file(filename.c_str(), ios::out|ios::binary);
	file.write(buffer.data(), buffer.size());
	file.close();
}
