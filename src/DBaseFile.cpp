#include <iostream>
#include <fstream>
#include <stdexcept>
#include <bitset>
#include <ctime>
#include <iomanip>
#include <sstream>

#include "DBaseFile.h"
#include "DBaseFieldDescArray.h"

using namespace std;

/** \brief Constructs dBase structure from given file.
 *  \param Name of the dBase file
 *  \return True if succeded. Otherwise throws exception
 */
bool DBaseFile::openFile(const std::string fileName){

	ifstream iFile;

    iFile.open(fileName.c_str(), ios::binary | ios::in);

    if(!iFile){
            //throw fileNotFoundEx();
    }

    iFile.seekg(0, iFile.end);
    fileSizeBytes = iFile.tellg();
    iFile.seekg(endOfHeader, iFile.beg);


    if(fileSizeBytes <= 0){
        //throw unexpectedHeaderEndEx(nullptr, fileSizeBytes);
    }

    //Read file contents into memory
    m_rawData = getFileContents(iFile);

    unsigned char* rawData = new unsigned char[fileSizeBytes];

    iFile.close();

    //Temporary variables
    unsigned char currentByte;
    char fieldDescArray[blockSize-1];
    struct tm fileLastUpdated = {0,0,0,0,0,0,0,0,0};

    //Loop through file
    for(unsigned int i = 0; i <= m_rawData.size(); i++){

        currentByte = m_rawData.at(i);

        //Read file header bit by bit. Spec of DBF files available at:
        //http://www.dbf2002.com/dbf-file-format.html
        if(currentByte == 0x0D){break;}

        if(i < blockSize){
            switch(i){
                case 0:{
                    bitset<8> firstBitset = ToBits(currentByte);

                    versionNr = (firstBitset[0] | (firstBitset[1] << 1));
                    firstBitset[3] == 1 ? memoFilePresent = true : memoFilePresent = false;
                    firstBitset[4] == 1 ? sqlFilePresent = true : sqlFilePresent = false;
                    firstBitset[5] == 1 ? sqlFilePresent = true : sqlFilePresent = false;
                    firstBitset[6] == 1 ? sqlFilePresent = true : sqlFilePresent = false;
                    firstBitset[6] == 1 ? anyMemoFilePresent = true : anyMemoFilePresent = false;

                    //There is some system to this (the memo and SQL part), but the version numbers are rather
                    //random. DO NOT BELIEVE the "official" dBase documentation. This information is based on
                    //http://stackoverflow.com/questions/3391525/which-header-format-can-be-assumed-by-reading-an-initial-dbf-byte
                    if(currentByte == 0x01){fileType = "not used";};
                    if(currentByte == 0x02){fileType = "FoxBASE";};
                    if(currentByte == 0x03){fileType = "FoxBASE+/dBASE III PLUS, no memo";};
                    if(currentByte == 0x04){fileType = "dBASE 7";};
                    if(currentByte == 0x05){fileType = "dBASE 5, no memo";};
                    if(currentByte == 0x30){fileType = "Visual FoxPro";};
                    if(currentByte == 0x31){fileType = "Visual FoxPro, autoincrement enabled";};
                    if(currentByte == 0x32){fileType = "Visual FoxPro, Varchar, Varbinary, or Blob-enabled";};
                    if(currentByte == 0x43){fileType = "dBASE IV SQL table files, no memo";};
                    if(currentByte == 0x63){fileType = "dBASE IV SQL system files, no memo";};
                    if(currentByte == 0x7B){fileType = "dBASE IV, with memo";};
                    if(currentByte == 0x83){fileType = "FoxBASE+/dBASE III PLUS, with memo";};
                    if(currentByte == 0x8B){fileType = "dBASE IV, with memo";};
                    if(currentByte == 0x8E){fileType = "dBASE IV with SQL table";};
                    if(currentByte == 0xCB){fileType = "dBASE IV SQL table files, with memo";};
                    if(currentByte == 0xE5){fileType = "Clipper SIX driver, with SMT memo";};
                    if(currentByte == 0xF5){fileType = "FoxPro 2.x (or earlier) with memo";};
                    if(currentByte == 0xFB){fileType = "FoxBASE (with memo?)";};
                    break;
                }
                case 1:{	//Last opened: year
                    fileLastUpdated.tm_year = ((int)currentByte + 100);
                    break;
                }case 2:{	//Last opened: month
                    fileLastUpdated.tm_mon = (((int)currentByte) - 1);
                    break;
                }case 3:{	//Last opened: day
                    fileLastUpdated.tm_mday = (int)currentByte;
                    break;
                }case 4: case 5: case 6: case 7:{ //Number of records in table
                    numRecordsInDB += (currentByte << (8*(i-4)));
                    break;
                }case 8: case 9:{ //Position of first data record
                    numBytesInHeader += (currentByte << (8*(i-8)));
                    break;
                }case 10: case 11:{ //Length of one data record incl. deleting flag
                    numBytesInRecord += (currentByte << (8*(i-10)));
                    break;
                }case 14:{
                    currentByte = 1 ? incompleteTransaction = true : incompleteTransaction = false;
                }case 15:{
                    currentByte = 1 ? encrypted = true : encrypted = false;
                }case 28:{
                    //FUCKING HELL, WHAT IS IT WITH ALL THIS BIT SHIFTING
                    bitset<8> bit28 = ToBits(currentByte);
                    if(bit28[0]){hasStructuralCDX = true;};
                    if(bit28[1]){hasMemoField = true;};
                    if(bit28[2]){isDatabase = true;};
                }case 29:{
                    codePageMark = (uint8_t)currentByte;
                }
            }

        }else if(i >= blockSize){
            fieldDescArray[((i-blockSize) % blockSize)] = currentByte;
            if((i-blockSize) % blockSize == blockSize-1){
                //One block is full
                fieldDescriptors.push_back(DBaseFieldDescArray(fieldDescArray));
            }
        }//end if

    }//end for loop

    fieldDescArrayNum = ((endOfHeader-blockSize) / blockSize);

    if(((endOfHeader-blockSize) % blockSize) != 0){
        throw runtime_error("Field descriptors do not line up with header byte alignment!");
    }

    //Postfix: convert fileLastUpdated to lastUpdated time type
    lastUpdated = mktime(&fileLastUpdated);

    delete rawData;
    //TODO: Exceptions, block size handling
    return true;
}

/** \brief Helper function to safely get
 *
 * \param Input File to get File from
 * \throw unexpectedHeaderEx
 * \throw noMemoryAvailableEx
 * \return File contents as std::string
 */
std::string DBaseFile::getFileContents(ifstream& iFile){
	std::stringstream tempContentsFileStream;

		if(iFile.peek() == std::ifstream::traits_type::eof()){
			//throw unexpectedHeaderEndEx("Empty file", 0);
		}

		iFile.exceptions(std::ios::failbit);
		//operator << returns an ostream&, but in reality it's a stringstream&
		tempContentsFileStream << iFile.rdbuf();

		if(!(tempContentsFileStream.good())){
			//throw noMemoryAvailableEx();
		}

	return tempContentsFileStream.str();
}


/** \brief Helper function: convert byte to bitset
 * \param {char} 8-Bit char
 * \return {bitset} exploded byte into arrays
 */
bitset<8> DBaseFile::ToBits(const char& byte){
    return bitset<8>(byte);
}

/** \brief Outputs debug information to cout
 */
void DBaseFile::stat()
{
    ///DEBUG STATEMENTS BEGIN
    cout << endl;
    cout << fileType << endl;
    cout << "Header with length " << endOfHeader << " bytes contains " << fieldDescArrayNum << " field descriptor arrays!"  << endl;

    cout << "Has memo field:\t\t\t\t";
    cout << (hasMemoField ? "YES" : "NO") << endl;
    cout << "Has structural .cdx file:\t\t";
    cout << (hasStructuralCDX ? "YES" : "NO") << endl;
    cout << "File is a database:\t\t\t";
    cout << (isDatabase ? "YES" : "NO") << endl;
    cout << "SQL file is present:\t\t\t";
    cout << (sqlFilePresent ? "YES" : "NO") << endl;
    cout << "Any memo file present:\t\t\t";
    cout << (anyMemoFilePresent ? "YES" : "NO") << endl;
    cout << "Has code page mark:\t\t\t";
    cout << ((codePageMark != 0) ? "YES" : "NO") << endl;

    cout << "Last updated:\t\t\t\t" << ctime(&lastUpdated);
    cout << "Number of records:\t\t\t" << (int)numRecordsInDB << endl;
    cout << "Number of bytes in header:\t\t" << (int)numBytesInHeader << endl;
    cout << "Number of bytes per record:\t\t" << (int)numBytesInRecord << endl;
}
