#ifndef DBASEFILE_H
#define DBASEFILE_H

#include <string>
#include <vector>
#include <stdint.h>
#include <bitset>
#include <ctime>
#include "DBaseRecord.h"

class DBaseFile
{
    public:
        DBaseFile();
        ~DBaseFile();

        bool openFile(const std::string fileName);

    protected:

    private:
    	//Helper function: converts char to bitset
    	std::bitset<8> ToBits(char* byte);
        //--------------------------------------------------
        //Level 5 DOS Header for dBase III - dBase VII files
        //--------------------------------------------------
        char firstByte;                     //BYPTE 0 temporary storage, c++ can only read bytes
            char versionNr[2];                    //BYTE 0 bit 0-2 version number of file
            bool memoFilePresent;               //BYTE 0 bit 3 is a memo present or not
            bool sqlFilePresent;                //BYTE 0 bit 4-6 is a .sql file present or not
            bool anyMemoFilePresent;            //BYTE 0 bit 7 is one of the above true
        time_t lastUpdated = time(0);       //BYTE 1-3 YYMMDD format
        uint32_t numRecordsInDB = 0;        //BYTE 4-7 32-bit nr of records
        uint16_t numBytesInHeader = 0;      //BYTE 8-9 16 bit nr of bytes in header
        uint16_t numBytesInRecord = 0;      //BYTE 10-11 16 bit nr of bytes in record
                                            //BYTE 12-13: 0, ignore
        bool incompleteTransaction;         //BYTE 14, set by BEGIN TRANSACTION
        bool enycrypted;                    //BYTE 15, encryption, used by FoxPro
                                            //BYTE 16-27 dBase DOS in multi-user environment
        bool productionMDX;                 //BYTE 28 0x01 = production .mdx available, else not
        char languageID;                    //BYTE 29 language driver ID
                                            //BYTE 30-31:0, ignore
        std::vector<DBaseRecord> records;   //BYTE 32-n: Field descriptor
                                            //BYTE n+1: 0x0D (13) field descriptor terminator
};

#endif // DBASEFILE_H
