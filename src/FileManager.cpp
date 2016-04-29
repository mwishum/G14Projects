//============================================================================
// Project 2: Reliable FTP Using GBN over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// April 15, 2016
//============================================================================

#include "FileManager.h"

/**
 * Starts file manager
 *
 * @param side Int representing client/server
 */
FileManager::FileManager(int side) :
        file_buffer(NULL), man_side(side) {
}

/**
 * Opens a file for reading and initializes buffers (SERVER)
 *
 * @param path filename/path of file to open for reading.
 *
 * @return Status of opening (FatalError if not readable)
 */
SR FileManager::ReadFile(const string &path) {
    in_file.open(path, ios::in | ios::binary | ios::ate);
    file_buffer = new char[Packet::max_content()];
    if (in_file.is_open() && in_file.good())
        return SR::Success;
    else
        perror("File Read Error");
    return SR::FatalError;
}

/**
 * Opens a file for writing and initializes buffers (CLIENT)
 *
 * @param path filename/path of file to open for reading.
 *
 * @return Status of opening (FatalError if not readable)
 */
SR FileManager::WriteFile(const string &path) {
    out_file.open(path, ios::out | ios::binary);
    file_buffer = new char[Packet::max_content()];
    if (out_file.is_open() && out_file.good())
        return SR::Success;
    else
        perror("File Write Error");
    return SR::FatalError;
}

/**
 * Breaks the file specified in OpenFile into however many packets are needed
 * to transmit it and returns it.
 *
 * @param &packs NON-NULL vector of DataPackets to write to
 *
 * @return status of breaking (Success or NotInitialized)
 */
SR FileManager::BreakFile(vector<DataPacket> &packs) {
    if (in_file == NULL || !in_file.is_open()) {
        cerr << "FILE NOT OPEN/VALID" << endl;
        return SR::NotInitialized;
    }
    in_file.seekg(0, ios::end);
    streamoff file_size = in_file.tellg();
    dprint("File Size", file_size)
    in_file.seekg(0, ios::beg);
    while (in_file.tellg() < file_size) {
        size_t rem = (size_t) file_size - in_file.tellg();
        //dprint("get pos", in_file.tellg())
        memset(file_buffer, NOTHING, Packet::max_content());

        if (rem <= Packet::max_content()) {
            dprint("END OF FILE", "making small packet")
            in_file.read(file_buffer, rem);
            packs.push_back(DataPacket(file_buffer, rem));
            //dprint("pack #", packs.size())
            break;
        }

        in_file.read(file_buffer, Packet::max_content());
        packs.push_back(DataPacket(file_buffer, Packet::max_content()));
        //dprint("pack #", packs.size())
    }
    in_file.close();
    DataPacket final_packet = DataPacket(NO_CONTENT, 0);
    cout << "Created " << packs.size() << " packets."<< endl;
    packs.push_back(final_packet);
    return SR::Success;
}

/**
 * Joins all packets from the vector into a file specified by WriteFile.
 *
 * @param &packs NON-NULL vector of DataPackets to read into file
 *
 * @return status of joining (Success or NotInitialized)
 */
SR FileManager::JoinFile(vector<DataPacket> &packs) {
    if (out_file == NULL || !out_file.is_open() || packs.size() < 1) {
        cerr << "FILE NOT OPEN/VALID/NO PACKETS" << endl;
        return SR::NotInitialized;
    }
    cout << "Joining " << packs.size() << " packets." << endl;
    out_file.seekg(0, ios::beg);
    for (uint p_i = 0; p_i < packs.size(); p_i++) {
        out_file.write(packs[p_i].Content(), packs[p_i].ContentSize());
    }
    out_file.close();
    return SR::Success;
}
/**
 * Closes streams (if open)
 */
FileManager::~FileManager() {
    delete[] file_buffer;
    if (in_file.is_open())
        in_file.close();
    if (out_file.is_open())
        out_file.close();
}

