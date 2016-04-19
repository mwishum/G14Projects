//============================================================================
// Project 2: Reliable FTP Using GBN over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// April 15, 2016
//============================================================================

#ifndef FILEMANAGER_H_
#define FILEMANAGER_H_

#include "project.h"
#include "packets.h"
#include <fstream>
#include <iterator>
#include <algorithm>
#include <sys/stat.h>

using namespace std;

class FileManager {
    fstream in_file;
    fstream out_file;
    char *file_buffer;
    int man_side;

public:
    FileManager(int side);
    SR ReadFile(const string &path);
    SR WriteFile(const string &path);
    SR BreakFile(vector<DataPacket> &packs);
    SR JoinFile(vector<DataPacket> &packs);
    virtual ~FileManager();
};

#endif /* FILEMANAGER_H_ */
