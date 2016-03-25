//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#ifndef FILEMANAGER_H_
#define FILEMANAGER_H_

#include "project.h"
#include "packets.h"
#include "Sockets.h"
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
    StatusResult ReadFile(const string &path);
    StatusResult WriteFile(const string &path);
    StatusResult BreakFile(vector<DataPacket> &packs);
    StatusResult JoinFile(vector<DataPacket> &packs);
    virtual ~FileManager();
};

#endif /* FILEMANAGER_H_ */
