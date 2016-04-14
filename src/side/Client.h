//============================================================================
// Project 2: Reliable FTP Using GBN over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// April 15, 2016
//============================================================================

#ifndef G14PROJECT1_CLIENT_H
#define G14PROJECT1_CLIENT_H

#include "../project.h"
#include "../Sockets.h"
#include "../FileManager.h"

using namespace std;

inline bool main_client(string this_address, vector<string> &command) {
    string remote_server_a, file_name, dest_file_name, in, primary;
    StatusResult result;
    char buffer[PACKET_SIZE];
    size_t buffer_len = PACKET_SIZE;
    if (command.size() == 1) {
        cout << "Enter server address: ";
        getline(cin, remote_server_a);
    } else {
        remote_server_a = command[1];
    }
    result = Sockets::instance()->OpenClient(remote_server_a, PORT_CLIENT);
    FileManager mgr(CLIENT);
    if (result != StatusResult::Success) {
        cerr << "Could not start Client." << endl;
        return true;
    }
    cout << "Success starting client." << endl;

    for (int tries = 5; tries >= 0; tries--) {
        GreetingPacket hello(&this_address[0], this_address.size());
        hello.Send();
        result = hello.Receive();
        if (result != StatusResult::Success) {
            cout << " [" << tries << "] Server unreachable (" << StatusMessage[(int) result] << ")." << endl;
        } else {
            cout << "Server ";
            fwrite(hello.Content(), hello.ContentSize(), 1, stdout);
            cout << " responded." << endl;
            break;
        }
    }
    while (true) {
        client:
        int loops = 0;
        cout << ">>";
        getline(cin, in);
        command.clear();
        file_name.clear();
        command = split(in, ' ');
        primary = command.empty() ? "" : command[0];
        if (primary == "get" || primary == "GET") {
            if (command.size() == 1) {
                cout << "Enter source file name: ";
                getline(cin, file_name);
            } else if (command.size() == 2) {
                file_name = command[1];
                cout << "Enter dest file name: ";
                getline(cin, dest_file_name);
            } else {
                file_name = command[1];
                dest_file_name = command[2];
            }

            //Find out if file exists on server
            RequestPacket *req_send = new RequestPacket(ReqType::Info, &file_name[0], file_name.size());
            result = req_send->Send();
            dprintm("Sending filename to server", result)
            //fwrite(req_send->Content(), req_send->ContentSize(), 1, stdout);

            string p_type;
            while (true) {
                result = Sockets::instance()->AwaitPacket(buffer, &buffer_len, p_type);
                if (result == StatusResult::Success) {
                    AckPacket next(0);
                    next.Send();
                } else cout << "Error communicating with server: " << StatusMessage[(int) result] << endl;
                if (p_type == GET_SUCCESS) {
                    cout << "Get success, file transmission in progress" << endl;
                    Sockets::instance()->TestRoundTrip(CLIENT);
                    break; //File exists
                } else if (p_type == GET_FAIL) {
                    cout << "File `" << file_name << "` does not exist on server." << endl;
                    goto client; //continue to client menu loop
                }
            }

            uint8_t alt_bit = 1;
            vector<DataPacket> packet_list;

            while (true) {
                if (alt_bit == 1) {
                    alt_bit = 0;
                } else alt_bit = 1;

                receive_more:
//                if (loops++ >= MAX_LOOPS) {
//                    cerr << "Client ran too long." << endl;
//                    break;
//                }

                DataPacket dataPacket;
                dataPacket.Sequence(alt_bit);
                result = dataPacket.Receive();

                if (dataPacket.ContentSize() == 0) { // BREAK ON THIS
                    RequestPacket suc(ReqType::Success, &file_name[0], file_name.size());
                    suc.Send();
                    break;
                }

                dprintm("Status Result (CLIENT)", result);

                if (result == StatusResult::Success) {
                    packet_list.push_back(dataPacket);
                    AckPacket packet = AckPacket(alt_bit);
                    packet.Send();
                    continue;
                } else if (result == StatusResult::ChecksumDoesNotMatch) {
                    NakPacket packet = NakPacket(alt_bit);
                    packet.Send();
                    //PRINTED IN DECODE
                    cout << " Seq#:" << (int) alt_bit << endl;
                    goto receive_more;
                } else if (result == StatusResult::OutOfSequence) {
                    NakPacket packet = NakPacket(alt_bit);
                    packet.Send();
                    //PRINTED IN DECODE
                    cout << " Seq#:" << (int) alt_bit << endl;
                    goto receive_more;
                } else {
                    dprintm("Status Result", result)
                    goto receive_more;
                }
            }

            mgr.WriteFile(dest_file_name);
            mgr.JoinFile(packet_list);
            cout << "File transfered to `" << dest_file_name << "` successfully." << endl;
            Sockets::instance()->use_manual_timeout = true;
            Sockets::instance()->ResetTimeout(TIMEOUT_SEC, TIMEOUT_MSEC);
            Sockets::instance()->use_manual_timeout = false;
        } else if (primary == "e") {
            cout << "[client closed]" << endl;
            return true; //Back to main program loop
        } else {
//            if (loops++ >= MAX_LOOPS) {
//                cerr << "Client ran too long." << endl;
//                break;
//            }
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
    string remove_server_address;
    getline(cin, remove_server_address);
    server = gethostbyname(remove_server_address.c_str());
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        //continue;
    }
    printf("Server name => %s\n", server->h_name);
    memset(&server_address, NOTHING, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(remove_server_address.c_str()); //TODO: CHANGE to inet_aton!
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
    inbuff.copy(buffer, inbuff.size());
    n = send(socket_id, buffer, inbuff.size(), 0);
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
