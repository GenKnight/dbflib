#include "DBaseFile.h"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <bitset>
#include <ctime>

using namespace std;

DBaseFile::DBaseFile()
{
    //ctor
}

DBaseFile::~DBaseFile()
{
    //dtor
}

bitset<8> DBaseFile::ToBits(char* byte)
{
    return bitset<8>(*byte);
}

bool DBaseFile::openFile(const std::string fileName)
{
	fstream iFile;
	//iFile.exceptions(ifstream::failbit | ifstream::badbit);

	try{
		iFile.open(fileName.c_str(), ios::binary | ios::in);
		if(!iFile){
			throw std::runtime_error("File not found.");
		}

		//Temporary variables
		char* currentByte = new char[1];
		unsigned int i = 0;
		struct tm fileLastUpdated = {0,0,0,0,0,0,0,0,0};

		while(!iFile.eof()){
			//Read 1 byte at a time
			iFile.read(currentByte, 1);

			//Read file header. Bit by bit. Spec of DBF files available at:
			//https://en.wikipedia.org/wiki/.dbf
			switch(i){
				case 0:{
					bitset<8> firstBitset = ToBits(currentByte);

					versionNr[0] = firstBitset[0];
					versionNr[1] = firstBitset[1];
					firstBitset[3] == 1 ? memoFilePresent = true : memoFilePresent = false;
					firstBitset[4] == 1 ? sqlFilePresent = true : sqlFilePresent = false;
					firstBitset[5] == 1 ? sqlFilePresent = true : sqlFilePresent = false;
					firstBitset[6] == 1 ? sqlFilePresent = true : sqlFilePresent = false;
					firstBitset[6] == 1 ? anyMemoFilePresent = true : anyMemoFilePresent = false;
					break;
				}
				case 1:{	//Last opened: year
					fileLastUpdated.tm_year = ((int)*currentByte + 100);
					break;
				}case 2:{	//Last opened: month
					fileLastUpdated.tm_mon = (((int)*currentByte) - 1);
					break;
				}case 3:{	//Last opened: day
					fileLastUpdated.tm_mday = (int)*currentByte;
					break;
				}case 4: case 5: case 6: case 7:{
					numRecordsInDB |= *currentByte;
					break;
				}
			}

			i++;
		}

		//Postfix: convert fileLastUpdated to lastUpdated time type
		lastUpdated = mktime(&fileLastUpdated);

		cout << "Version Nr: " << (int)versionNr[0] << "." << (int)versionNr[1] << endl;
		memoFilePresent ? cout << "Memo present!"  : cout << "Memo not present!" << endl;
		sqlFilePresent ? cout << "SQL present!"  : cout << "SQL not present!" << endl;
		anyMemoFilePresent ? cout << "Any memo present!"  : cout << "Any memo not present!" << endl;
		cout << "Last updated: " << ctime(&lastUpdated) << endl;
		cout << "Number of records: " << (int)numRecordsInDB << endl;

	}catch(const std::runtime_error& e){
		cout << "Exeption opening file: " << e.what() << endl;
		return false;
	}catch(const std::ifstream::failure& f){
		cout << "Exception when reading file: " << f.what() << endl;
		return false;
	}

	iFile.close();
    return true;
}

