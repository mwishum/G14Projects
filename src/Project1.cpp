//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#include "project.h"
#include "packets.h"
#include "Sockets.h"
#include "FileManager.h"
#include "Gremlin.h"

using namespace std;

int main(int argc, char *argv[]) {
    ///////////////////////////////////////////////
    cout << "Ver " << __DATE__ << " " << __TIME__ << endl;
//    for (int i = 0; i <= 100; ++i) {
//        usleep(30000);
//        printf("\r[%3d%%]", i);
//        cout << flush;
//    }
//    cout << endl;
    ///////////////////////////////////////////////
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa = NULL;
    void *tmpAddrPtr = NULL;
    bool volatile exit = true;
    string in;
    locale loc;

    //get list of adapters and addresses
    getifaddrs(&ifAddrStruct);
    cout << "Pick an adapter to use:" << endl;
    vector<string> addresses;
    int index = 0;
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            printf(" [%d] %s (%s)\n", index++, ifa->ifa_name, addressBuffer);
            addresses.push_back(addressBuffer);
        }
    }
    if (ifAddrStruct != NULL)
        freeifaddrs(ifAddrStruct);

    cout << "Adapter #:";
    getline(cin, in);
    long sel = strtol(in.c_str(), NULL, 10);
    string this_address = addresses[(sel >= addresses.size()) ? 0 : sel];

    cout << "(running on " << this_address << ")" << endl;
    cout << "========== Menu ==========" << endl;
    cout << "[e]xit, [_c]lient, [_s]erver" << endl;
    cout << " [c] client, [s] server " << endl;
    while (exit) {
        cout << ">";
        getline(cin, in);
        for (int i = 0; in[i]; i++)
            in[i] = tolower(in[i]);
        vector<string> command = split(in, ' ');
        int socket_id;
        socklen_t client_addr_len, server_addr_len;
        char buffer[256];
        struct sockaddr_in server_address, client_address;
        ssize_t n;
        string primary;
        if (command.empty()) {
            primary = " ";
        } else {
            primary = command[0];
        }

        if (primary == "e") {
            exit = false;
            close(socket_id);
            cout << "Goodbye!" << endl;
            break;
        } else if (primary == "_s") {
            /*
             *  OLD SERVER
             */
            //make a new socket for Datagrams over the Internet
            socket_id = socket(AF_INET, SOCK_DGRAM, 0);
            if (socket_id < 0) {
                perror("ERROR opening socket");
                continue;
            }
            //put address in memory
            memset(&server_address, NOTHING, sizeof(server_address));
            //set type, address, and port of this server.
            server_address.sin_family = AF_INET;
            cout << "Server on " << this_address << endl;
            server_address.sin_addr.s_addr = inet_addr(this_address.c_str()); //TODO: CHANGE to inet_aton!
            if (server_address.sin_addr.s_addr == INADDR_NONE) {
                perror("Invlaid address");
                continue;
            }
            server_address.sin_port = htons(PORT_SERVER);
            server_addr_len = sizeof(server_address);
            //bind socket with addresses
            n = bind(socket_id, (sockaddr *) &server_address, server_addr_len);
            if (n < 0) {
                perror("ERROR on binding");
                continue;
            }

            cout << "Waiting on connection.\n(send CTRL+Y on client to end)"
            << endl;
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
                message.copy(buffer, message.length(), 0);
                n = sendto(socket_id, buffer, sizeof(buffer), 0,
                           (sockaddr *) &client_address, client_addr_len);
                if (n < 0) {
                    perror("Error sendto");
                    continue;
                }
            }

        } else if (primary == "_c") {
            /*
             * OLD CLIENT
             */
            try {
                struct hostent *server;

                socket_id = socket(AF_INET, SOCK_DGRAM, 0);
                if (socket_id < 0) {
                    perror("ERROR opening socket");
                    continue;
                }
                printf("Socket opened\n");
                cout << "Enter host to connect to: ";
                string host;
                getline(cin, host);
                server = gethostbyname(host.c_str());
                if (server == NULL) {
                    fprintf(stderr, "ERROR, no such host\n");
                    continue;
                }
                printf("Server name => %s\n", server->h_name);
                memset(&server_address, NOTHING, sizeof(server_address));
                server_address.sin_family = AF_INET;
                server_address.sin_addr.s_addr = inet_addr(host.c_str()); //TODO: CHANGE to inet_aton!
                if (server_address.sin_addr.s_addr == INADDR_NONE) {
                    perror("Invlaid address");
                    continue;
                }
                server_address.sin_port = htons(PORT_SERVER);

                if (connect(socket_id, (sockaddr *) &server_address,
                            sizeof(server_address)) < 0) {
                    perror("ERROR connecting");
                    continue;
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
                    continue;
                }
                memset(buffer, NOTHING, 256);

                n = recv(socket_id, buffer, 255, 0);
                if (n < 0) {
                    perror("ERROR reading from socket");
                    continue;
                }
                printf("%s\n", buffer);
            } catch (...) {
                cout << "program exited with error" << endl;
                close(socket_id);
                return -1;
            }
        } else if (primary == "c") { //******CLIENT CODE******//
            string host;
            if (command.size() < 2) {
                cout << "Enter server address: ";
                getline(cin, host);
            } else host = command[1];
            Sockets::instance()->OpenClient(this_address, host, PORT_CLIENT, PORT_SERVER);
            cout << "Success starting client." << endl;
            Sockets::instance()->TestRoundTrip(CLIENT);
            cout << "end RTT test" << endl;

            cout << endl << " !!! Testing Await function !!! " << endl << endl;
            char *temp_msg = (char *) "Hello!";
            DataPacket d(temp_msg, strlen(temp_msg));
            d.Sequence(1);
            d.Send();
            AckPacket ackPacket(0);
            ackPacket.Send();
            RequestPacket requestPacket(ReqType::Fail, temp_msg, strlen(temp_msg));
            requestPacket.Send();

        } else if (primary == "s") { //******SERVER CODE******//
            string damage_prob, loss_prob;
            if (command.size() < 3) {
                cout << "Enter damage probability: ";
                getline(cin, damage_prob);
                cout << "Enter loss probability: ";
                getline(cin, loss_prob);
            } else {
                damage_prob = command[1];
                damage_prob = command[2];
            }

            Gremlin::instance()->initialize(atof(damage_prob.c_str()), atof(loss_prob.c_str()));
            Sockets::instance()->OpenServer(this_address, "127.0.0.1", PORT_SERVER, PORT_CLIENT);
            cout << "Success starting server." << endl;

            string temp_type;
            Packet *temp = new Packet();
            int loops = 0;
            StatusResult result;
            while (true) {
                FileManager mgr(SERVER);
                dprintm("Await Returned", Sockets::instance()->AwaitPacket(temp, temp_type));
                if (temp_type == GREETING) {

                } else if (temp_type == GET_INFO) { // GET FILE INFO
                    bool file_exists = false;
                    ////// file exists check

                    struct stat buffer;
                    file_exists = (stat(temp->Content(), &buffer) == 0);

                    if (file_exists) {
                        RequestPacket suc(ReqType::Success, temp->Content(), temp->ContentSize());
                        cerr << "FILE EXISTS" << endl;
                        fwrite(temp->Content(), temp->ContentSize(), 1, stdout);
                        suc.Send();
                    } else {
                        RequestPacket fail(ReqType::Fail, temp->Content(), temp->ContentSize());
                        cerr << "FILE DNE" << endl;
                        fwrite(temp->Content(), temp->ContentSize(), 1, stdout);
                        fail.Send();
                        continue;
                    }

                    //////
                    if (file_exists) {
                        AckPacket ack_file_exists(0);
                        dprintm("Client ack file exists", result = ack_file_exists.Receive());
                        if (result == StatusResult::Success) {
                            string file_name;

                            if (command.size() < 2) {
                                cout << "Enter file name: ";
                                getline(cin, file_name);
                            } else {
                                file_name = command[1];
                            }

                            FileManager mgr(CLIENT);
                            mgr.ReadFile(file_name);
                            vector<DataPacket> packet_list;
                            mgr.BreakFile(packet_list);
                            uint8_t alt_bit = 1;

                            for (DataPacket packet : packet_list) {
                                if (alt_bit == 1) {
                                    alt_bit = 0;
                                } else alt_bit = 1;
                                packet.Sequence(alt_bit);

                                Send:
                                packet.Send();
                                Packet received = Packet();
                                string type;
                                StatusResult res = Sockets::instance()->AwaitPacket(&received, type);
                                uint8_t seq = received.Sequence();

                                if (seq != alt_bit) {
                                    cout << "Packet Status" << "LOST!" << endl;
                                    cout << "Sequence number: " << seq << endl;
                                    cout << "Expected number: " << alt_bit << endl;
                                }

                                if (type == NO_ACK) {
                                    goto Send;
                                } else if (type == ACK) {
                                    continue;
                                } else {
                                    dprint("Something Happened", "")
                                }
                            }
                        }
                    }
//                    cout << endl;
//
//                    Sockets::instance()->TestRoundTrip(SERVER);
//                    cout << "end RTT test" << endl;
//                    cout << endl << " !!! Await starting (you must CTRL+C) !!! " << endl << endl;
                    if (loops++ >= 30000) {
                        cout << "bye" << endl;
                        break;
                    }
                }
            }
        } else if (primary == "f") { /* FILE COPY TEST */
            string file_name;
            if (command.size() < 2) {
                cout << "Enter file name: ";
                getline(cin, file_name);
            } else {
                file_name = command[1];
            }
            FileManager mgr(CLIENT);
            mgr.FileExists(file_name);
            mgr.ReadFile(file_name);
            vector<DataPacket> packet_list;
            mgr.BreakFile(packet_list);
            cout << "File broken, putting back together" << endl;
            mgr.WriteFile("d_" + file_name);
            mgr.JoinFile(packet_list);
        } else if (primary == "fc") { ///////////////////////////////////////////////
            string host, file_name;

            if (command.size() == 1) {
                cout << "Enter server address: ";
                getline(cin, host);
                cout << "Enter file name: ";
                getline(cin, file_name);
            } else if (command.size() == 2) {
                host = command[1];
                cout << "Enter file name: ";
                getline(cin, file_name);
            } else if (command.size() >= 3) {
                host = command[1];
                file_name = command[2];
            }

            Sockets::instance()->OpenClient(this_address, host, PORT_CLIENT, PORT_SERVER);
            cout << "Success starting client." << endl;
            FileManager mgr(CLIENT);

            ///////
            bool file_exists = false;
            RequestPacket req_send(ReqType::Info, &file_name[0], file_name.length());
            StatusResult res;
            dprintm("Sending filename to server", res = req_send.Send())
            fwrite(req_send.Content(), req_send.ContentSize(), 1, stdout);
            cout << endl;
            Packet unknown;
            string type;
            res = Sockets::instance()->AwaitPacket(&unknown, type);
            if (type == ACK) {
                cerr << "GET FILE SUCCESS" << endl;
                file_exists = true;
            } else if (type == GET_FAIL && res == StatusResult::Success) {
                cerr << "GET FILE FAILED" << endl;
                file_exists = false;
            } else {
                dprintm("Response from server get info - error", res)
            }

            ///////

            if (!file_exists) {
                cout << "File " << file_name << " does not exist on server." << endl;
                continue;
            }

            uint8_t alt_bit = 1;
            vector<DataPacket> packet_list;

            while (true) {
                if (alt_bit == 1) {
                    alt_bit = 0;
                } else alt_bit = 1;

                Receive:
                DataPacket received;
                StatusResult res = received.Receive();

                if (received.Content()) { // BREAK ON THIS
                    break;
                }

                uint8_t seq = received.Sequence();

                if (res == StatusResult::Success) {
                    packet_list.push_back(received);
                    AckPacket packet = AckPacket(seq);
                    packet.Send();
                } else if (res == StatusResult::ChecksumDoesNotMatch) {
                    NakPacket packet = NakPacket(seq);
                    packet.Send();
                    cout << "Packet has errors!\n" << "Sequence Number: " << seq << endl;
                } else {
                    dprintm("Status Result", res)
                    goto Receive;
                }
            }

            mgr.WriteFile("d_" + file_name);
            mgr.JoinFile(packet_list);

        } else if (primary == "fs") { ////////////////////////////////////
            string damage_prob, loss_prob;

            if (command.size() < 3) {
                cout << "Enter damage probability: ";
                getline(cin, damage_prob);
                cout << "Enter loss probability: ";
                getline(cin, loss_prob);
            } else {
                damage_prob = command[1];
                damage_prob = command[2];
            }

            Gremlin::instance()->initialize(atof(damage_prob.c_str()), atof(loss_prob.c_str()));
            Sockets::instance()->OpenServer(this_address, "127.0.0.1", PORT_SERVER, PORT_CLIENT);
            cout << "Success starting server." << endl;

            string file_name;

            if (command.size() < 2) {
                cout << "Enter file name: ";
                getline(cin, file_name);
            } else {
                file_name = command[1];
            }

            FileManager mgr(SERVER);
            mgr.ReadFile(file_name);
            vector<DataPacket> packet_list;
            mgr.BreakFile(packet_list);
            uint8_t alt_bit = 1;

            for (DataPacket packet : packet_list) {
                if (alt_bit == 1) {
                    alt_bit = 0;
                } else alt_bit = 1;
                packet.Sequence(alt_bit);

                Send_fs:
                packet.Send();
                Packet received = Packet();
                string type;
                StatusResult res = Sockets::instance()->AwaitPacket(&received, type);
                uint8_t seq = received.Sequence();

                if (seq != alt_bit) {
                    cout << "Packet Status" << "LOST!" << endl;
                    cout << "Sequence number: " << seq << endl;
                    cout << "Expected number: " << alt_bit << endl;
                }

                if (type == NO_ACK) {
                    goto Send_fs;
                } else if (type == ACK) {
                    continue;
                } else {
                    dprint("Something Happened", "")
                }
            }


        } else {
            continue;
        }
    }
    return 0;
}

