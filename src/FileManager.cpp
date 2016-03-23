//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#include "FileManager.h"

FileManager::FileManager(int side) :
        file_buffer(NULL), man_side(side) {
}

StatusResult FileManager::ReadFile(string path) {
    in_file.open(path, ios::in | ios::binary | ios::ate);
    file_buffer = new char[Packet::max_content()];
    if (in_file.is_open() && in_file.good())
        return StatusResult::Success;
    else
        perror("open file error");
    return StatusResult::FatalError;
}

StatusResult FileManager::WriteFile(string path) {
    out_file.open(path, ios::out | ios::binary);
    return StatusResult::FatalError;
}

StatusResult FileManager::BreakFile(vector<DataPacket> &packs) {
    if (in_file == NULL && !in_file.is_open()) {
        cerr << "FILE NOT OPEN/VALID" << endl;
        return StatusResult::NotInitialized;
    }
    in_file.seekg(0, ios::end);
    streamoff file_size = in_file.tellg();
    dprint("file size", file_size)
    in_file.seekg(0, ios::beg);
    while (in_file.tellg() < file_size) {
        size_t rem = (size_t) file_size - in_file.tellg();
        dprint("get pos", in_file.tellg())
        memset(file_buffer, NOTHING, Packet::max_content());

        if (rem <= Packet::max_content()) {
            dprint("END OF FILE", "making small packet")
            in_file.read(file_buffer, rem);
            packs.push_back(DataPacket(file_buffer, rem));
            dprint("pack #", packs.size())
            char temp[rem + 1];
            temp[rem] = '\0';
            memcpy(temp, file_buffer, rem);
            dprint("data", temp)
            break;
        }

        in_file.read(file_buffer, Packet::max_content());
        packs.push_back(DataPacket(file_buffer, Packet::max_content()));
        char temp[Packet::max_content() + 1];
        temp[Packet::max_content()] = '\0';
        memcpy(temp, file_buffer, Packet::max_content());
        dprint("pack #", packs.size())
        dprint("data", temp)
    }
    in_file.close();
    return StatusResult::Success;
}

StatusResult FileManager::JoinFile(vector<DataPacket> &packs) {
    if (out_file == NULL && !out_file.is_open() && packs.size() < 1) {
        cerr << "FILE NOT OPEN/VALID/NO PACKETS" << endl;
        return StatusResult::NotInitialized;
    }
    dprint("Joining packets. Count", packs.size())
    out_file.seekg(0, ios::beg);
    for (uint p_i = 0; p_i < packs.size(); p_i++) {
        out_file.write(packs[p_i].Content(), packs[p_i].ContentSize());
    }
    out_file.close();
    return StatusResult::Success;
}

FileManager::~FileManager() {
    delete[] file_buffer;
    if (in_file.is_open())
        in_file.close();
    if (out_file.is_open())
        out_file.close();
}
