//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================


#ifndef G14PROJECT1_SERVER_H
#define G14PROJECT1_SERVER_H

#include "project.h"
#include "Sockets.h"
#include "FileManager.h"
#include "Gremlin.h"

using namespace std;

inline bool main_server(string this_address, vector<string> &command) {
    string damage_prob, loss_prob;
    StatusResult result;
    if (command.size() < 3) {
        cout << "Enter damage probability: ";
        getline(cin, damage_prob);
        cout << "Enter loss probability: ";
        getline(cin, loss_prob);
    } else {
        damage_prob = command[1];
        loss_prob = command[2];
    }

    Gremlin::instance()->initialize(atof(damage_prob.c_str()), atof(loss_prob.c_str()));
    result = Sockets::instance()->OpenServer(this_address, PORT_SERVER, PORT_CLIENT);
    FileManager mgr(SERVER);
    if (result != StatusResult::Success) {
        cerr << "Could not start server." << endl;
        return true;
    }
    cout << "Success starting server." << endl;

    string packet_type;
    char *pack_buffer_a = new char[PACKET_SIZE];
    size_t size = PACKET_SIZE;
    int loops = 0;

    while (true) {
         size = PACKET_SIZE;
        dprintm("server await returned", result = Sockets::instance()->AwaitPacket(pack_buffer_a, &size, packet_type));
        if (loops++ >= MAX_LOOPS) {
            cout << "OUT OF LOOPS" << endl;
            break;
        }
        if (result != StatusResult::Success) continue;
        dprint("Packet type", packet_type)
        if (packet_type == GREETING) {
            GreetingPacket hello(NO_CONTENT, 1);
            hello.DecodePacket(pack_buffer_a, size);
            hello = GreetingPacket(&this_address[0], this_address.size());
            hello.Send();
        } else if (packet_type == GET_INFO) { // GET FILE INFO
            bool file_exists;
            RequestPacket req_info(ReqType::Info, NO_CONTENT, 1);
            req_info.DecodePacket(pack_buffer_a, size);

            struct stat stat_buff;
            string file_name_from_client;
            file_name_from_client.insert(0, req_info.Content(), req_info.ContentSize());
            file_exists = (stat(file_name_from_client.c_str(), &stat_buff) == 0);
            cout << "FILE `" << file_name_from_client << "` EXISTS ?";

            send_file_ack_again:
            if (file_exists) {
                RequestPacket suc(ReqType::Success, req_info.Content(), req_info.ContentSize());
                cout << " TRUE" << endl;
                suc.Send();
            } else {
                RequestPacket fail(ReqType::Fail, req_info.Content(), req_info.ContentSize());
                cout << " FALSE" << endl;
                fail.Send();
            }
            //////
            if (file_exists) {
                AckPacket ack_file_exists(0);
                dprintm("Client ack file exists", result = ack_file_exists.Receive());
                if (result == StatusResult::Success) {
                    cout << "\n\nRTT Test\n";
                    Sockets::instance()->TestRoundTrip(SERVER);
                    cout << "\nEnd RTT Test\n\n";

                    mgr.ReadFile(file_name_from_client);
                    vector<DataPacket> packet_list;
                    mgr.BreakFile(packet_list);
                    uint8_t alt_bit = 1;

                    for (DataPacket packet : packet_list) {
                        if (alt_bit == 1) {
                            alt_bit = 0;
                        } else alt_bit = 1;

                        send_again:
                        cout << endl;
                        packet.Sequence(alt_bit);
                        packet.Send();

                        dprintm("server await returned",
                                result = Sockets::instance()->AwaitPacket(pack_buffer_a, &size, packet_type));

                        if (packet_type == NO_ACK) {
                            NakPacket nakPacket(0);
                            nakPacket.DecodePacket(pack_buffer_a, size);
                            int seq = nakPacket.Sequence();

                            if (seq != alt_bit) {
                                cout << "Packet Status: Lost (NAK)" << endl;
                                cout << "Sequence number: " << seq << endl;
                                cout << "Expected number: " << (int) alt_bit << endl;
                            }

                            goto send_again;
                        } else if (packet_type == ACK) {
                            AckPacket ackPacket(0);
                            ackPacket.DecodePacket(pack_buffer_a, size);
                            int seq = ackPacket.Sequence();

                            if (seq != alt_bit) {
                                cout << "Packet Status: Lost/Tampered (ACK)" << endl;
                                cout << "Sequence number: " << seq << endl;
                                cout << "Expected number: " << (int) alt_bit << endl;

                                goto send_again;
                            }

                            continue;
                        } else if (result == StatusResult::Timeout) {
                            goto send_again;
                        } else if (packet_type == GET_SUCCESS) {
                            break;
                        } else {
                            dprintm("Something Happened", result);
                        }
                    }
                    cout << "Finished file transmission." << endl;
                } else if (result == StatusResult::Timeout) {
                    goto send_file_ack_again;
                }
            }
        } //***ELSE IF GET INFO
        else {
        }
        if (loops++ >= MAX_LOOPS) {
            cout << "OUT OF LOOPS" << endl;
            break;
        }
    } //***WHILE
    return true;
}


inline void proof_server(string this_address) {
    int socket_id;
    socklen_t client_addr_len, server_addr_len;
    char buffer[256];
    struct sockaddr_in server_address, client_address;
    ssize_t n;
    /*
     *  OLD SERVER
     */
    //make a new socket for Datagrams over the Internet
    socket_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_id < 0) {
        perror("ERROR opening socket");
        //continue;
    }
    //put address in memory
    memset(&server_address, NOTHING, sizeof(server_address));
    //set type, address, and port of this server.
    server_address.sin_family = AF_INET;
    cout << "Server on " << this_address << endl;
    server_address.sin_addr.s_addr = inet_addr(this_address.c_str()); //TODO: CHANGE to inet_aton!
    if (server_address.sin_addr.s_addr == INADDR_NONE) {
        perror("Invalid address");
        //continue;
    }
    server_address.sin_port = htons(PORT_SERVER);
    server_addr_len = sizeof(server_address);
    //bind socket with addresses
    n = bind(socket_id, (sockaddr *) &server_address, server_addr_len);
    if (n < 0) {
        perror("ERROR on binding");
        //continue;
    }

    cout << "Waiting on connection.\n(send CTRL+Y on client to end)" << endl;
    bool exiting = false;
    while (!exiting) {
        memset(buffer, NOTHING, 256);
        client_addr_len = sizeof(client_address);
        n = recvfrom(socket_id, buffer, sizeof(buffer), 0, (sockaddr *) &client_address, &client_addr_len);
        if (n < 0) {
            perror("Error recvfrom");
            continue;
        }
        printf("Here is the message :%s:\n", buffer);
        buffer[n] = 0;
        if (!strcasecmp(buffer, "\x19")) { //CTRL+Y
            cout << "Exit received" << endl;
            exiting = true;
        }
        string message = "echo:";
        message.copy(buffer, message.size(), 0);
        n = sendto(socket_id, buffer, sizeof(buffer), 0,
                   (sockaddr *) &client_address, client_addr_len);
        if (n < 0) {
            perror("Error sendto");
            continue;
        }
    }

}

#endif //G14PROJECT1_SERVER_H
