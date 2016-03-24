//============================================================================
// Project 1: Reliable FTP over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// March 9, 2016
//============================================================================

#include "project.h"
#include "packets.h"
#include "FileManager.h"
#include "Gremlin.h"
#include "Server.h"
#include "Client.h"

using namespace std;

int main(int argc, char *argv[]) {
    ///////////////////////////////////////////////
    cout << "Ver " << __DATE__ << " " << __TIME__ << endl;
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
    cout << " Server: s <dmg%> <loss%> " << endl;
    cout << " Client: c <address> " << endl;
    cout << "         get <filename> " << endl;
    while (exit) {
        cout << ">";
        getline(cin, in);
        for (int i = 0; in[i]; i++)
            in[i] = (char) tolower(in[i]);
        vector<string> command = split(in, ' ');

        string primary;
        if (command.empty()) {
            primary = " ";
        } else {
            primary = command[0];
        }
        if (primary == "e") { //******EXIT******//
            exit = false;
            cout << "Goodbye!" << endl;

        } else if (primary == "c") { //******CLIENT CODE******//

            main_client(this_address, command);

        } else if (primary == "s") { //******SERVER CODE******//

            main_server(this_address, command);

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
        } else if (primary == "_c") { /////////////////////////////////////////////// previously client
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
            Sockets::instance()->OpenServer(this_address, PORT_SERVER, PORT_CLIENT);
            cout << "Success starting server." << endl;

            string file_name;
            Packet first_packet = Packet();
            string type;
            StatusResult res = Sockets::instance()->AwaitPacket(&first_packet, type);
            file_name = first_packet.Content();

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
                res = Sockets::instance()->AwaitPacket(&received, type);
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
        } else { //******NO COMMAND******//
            continue;
        }
    } //***END MAIN WHILE
    return 0;
}

