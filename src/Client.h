//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================


#ifndef G14PROJECT1_CLIENT_H
#define G14PROJECT1_CLIENT_H

#include "project.h"
#include "Sockets.h"
#include "FileManager.h"

using namespace std;

inline bool main_client(string this_address, vector<string> &command) {
    string host, file_name, in, primary;
    StatusResult result;
    char buffer[PACKET_SIZE];
    size_t buffer_len = PACKET_SIZE;
    if (command.size() == 1) {
        cout << "Enter server address: ";
        getline(cin, host);
    } else {
        host = command[1];
    }
    result = Sockets::instance()->OpenClient(this_address, host, PORT_CLIENT, PORT_SERVER);
    FileManager mgr(CLIENT);
    if (result != StatusResult::Success) {
        cerr << "Could not start Client." << endl;
        return true;
    }
    cout << "Success starting client." << endl;

    while (true) {
        client:
        cout << ">>";
        getline(cin, in);
        command.clear();
        command = split(in, ' ');
        primary = command.empty() ? "" : command[0];
        if (primary == "get" || primary == "GET") {
            if (command.size() == 1) {
                cout << "Enter file name: ";
                getline(cin, file_name);
            } else file_name = command[1];

            //Find out if file exists on server
            RequestPacket req_send(ReqType::Info, &file_name[0], file_name.length());
            dprintm("Sending filename to server", result = req_send.Send())
            fwrite(req_send.Content(), req_send.ContentSize(), 1, stdout);
            cout << endl;
            string p_type;
            while (true) {
                result = Sockets::instance()->AwaitPacket(buffer, &buffer_len, p_type);
                if (result == StatusResult::Success) {
                    AckPacket next(0);
                    next.Send();
                } else dprintm("receive server file status error", result)
                if (p_type == GET_SUCCESS) {
                    cout << "Get success, file transmission in progress" <<endl;
                    break; //File exists
                }
                if (p_type == GET_FAIL) {
                    cout << "File `" << file_name << "` does not exist on server." << endl;
                    goto client;//continue to client menu loop
                }
            }

            uint8_t alt_bit = 1;
            vector<DataPacket> packet_list;

            while (true) {
                if (alt_bit == 1) {
                    alt_bit = 0;
                } else alt_bit = 1;

                receive_more:
                DataPacket dataPacket;
                result = dataPacket.Receive();

                if (dataPacket.Content() == "") { // BREAK ON THIS
                    break;
                }

                uint8_t seq = dataPacket.Sequence();

                if (result == StatusResult::Success) {
                    packet_list.push_back(dataPacket);
                    AckPacket packet = AckPacket(seq);
                    packet.Send();
                } else if (result == StatusResult::ChecksumDoesNotMatch) {
                    NakPacket packet = NakPacket(seq);
                    packet.Send();
                    cout << "Packet has errors! - Sequence Number: " << seq << endl;
                } else {
                    dprintm("Status Result", result)
                    goto receive_more;
                }
            }

            mgr.WriteFile("d_" + file_name);
            mgr.JoinFile(packet_list);
        } else if (primary == "e") {
            cout << "[client closed]" << endl;
            return true; //Back to main program loop
        } else {
            continue; //Continue client menu loop
        }
    }
}


inline void proof_client(string this_address) {
    /*
     * OLD CLIENT
     */
    struct hostent *server;
    int socket_id;
    socklen_t client_addr_len, server_addr_len;
    char buffer[256];
    struct sockaddr_in server_address, client_address;
    ssize_t n;
    socket_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_id < 0) {
        perror("ERROR opening socket");
        //continue;
    }
    printf("Socket opened\n");
    cout << "Enter host to connect to: ";
    string host;
    getline(cin, host);
    server = gethostbyname(host.c_str());
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        //continue;
    }
    printf("Server name => %s\n", server->h_name);
    memset(&server_address, NOTHING, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(host.c_str()); //TODO: CHANGE to inet_aton!
    if (server_address.sin_addr.s_addr == INADDR_NONE) {
        perror("Invlaid address");
        //continue;
    }
    server_address.sin_port = htons(PORT_SERVER);

    if (connect(socket_id, (sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("ERROR connecting");
        //continue;
    }

    memset(buffer, NOTHING, 256);
    string inbuff = "", trash;

    cout << "Enter message: ";
    getline(cin, inbuff);
    inbuff.copy(buffer, inbuff.length());
    n = send(socket_id, buffer, inbuff.length(), 0);
    cout << "=|" << buffer << "|=" << endl;
    if (n < 0) {
        perror("ERROR writing to socket");
        //continue;
    }
    memset(buffer, NOTHING, 256);

    n = recv(socket_id, buffer, 255, 0);
    if (n < 0) {
        perror("ERROR reading from socket");
        //continue;
    }
    printf("%s\n", buffer);

}

#endif //G14PROJECT1_CLIENT_H
