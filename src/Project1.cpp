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
    cin >> in;
    long sel = strtol(in.c_str(), NULL, 10);
    string this_address = addresses[(sel >= addresses.size()) ? 0 : sel];

    cout << "(running on " << this_address << ")" << endl;
    cout << "========== Menu ==========" << endl;
    cout << "[e]xit, [c]lient, [s]erver" << endl;
    cout << " [c2] client, [s2] server " << endl;
    while (exit) {
        cout << ">";
        cin >> in;

        int socket_id;
        socklen_t client_addr_len, server_addr_len;
        char buffer[256];
        struct sockaddr_in server_address, client_address;
        int n;

        for (int i = 0; in[i]; i++)
            in[i] = tolower(in[i]);
        if (in == "e") {
            exit = false;
            close(socket_id);
            cout << "Goodbye!" << endl;
            break;
        } else if (in == "s") {
            /*
             * SERVER
             */
            //make a new socket for Datagrams over the Internet
            socket_id = socket(AF_INET, SOCK_DGRAM, 0);
            if (socket_id < 0) {
                perror("ERROR opening socket");
                break;
            }
            //put address in memory
            memset(&server_address, NOTHING, sizeof(server_address));
            //set type, address, and port of this server.
            server_address.sin_family = AF_INET;
            cout << "Server on " << this_address << endl;
            server_address.sin_addr.s_addr = inet_addr(this_address.c_str()); //TODO: CHANGE to inet_aton!
            if (server_address.sin_addr.s_addr == INADDR_NONE) {
                perror("Invlaid address");
                break;
            }
            server_address.sin_port = htons(PORT_SERVER);
            server_addr_len = sizeof(server_address);
            //bind socket with addresses
            n = bind(socket_id, (sockaddr *) &server_address, server_addr_len);
            if (n < 0) {
                perror("ERROR on binding");
                break;
            }

            cout << "Waiting on connection.\n(send CTRL+Y on client to end)"
            << endl;
            bool exiting = false;
            while (!exiting) {
                memset(buffer, NOTHING, 256);
                client_addr_len = sizeof(client_address);
                n = recvfrom(socket_id, buffer, sizeof(buffer), 0,
                             (sockaddr *) &client_address, &client_addr_len);
                if (n < 0) {
                    perror("Error recvfrom");
                    break;
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
                    break;
                }
            }

        } else if (in == "c") {
            /*
             * CLIENT
             */
            try {
                struct hostent *server;

                socket_id = socket(AF_INET, SOCK_DGRAM, 0);
                if (socket_id < 0) {
                    perror("ERROR opening socket");
                    break;
                }
                printf("Socket opened\n");
                cout << "Enter host to connect to: ";
                string host;
                getline(cin, host);
                getline(cin, host);
                server = gethostbyname(host.c_str());
                if (server == NULL) {
                    fprintf(stderr, "ERROR, no such host\n");
                    break;
                }
                printf("Server name => %s\n", server->h_name);
                memset(&server_address, NOTHING, sizeof(server_address));
                server_address.sin_family = AF_INET;
                server_address.sin_addr.s_addr = inet_addr(host.c_str()); //TODO: CHANGE to inet_aton!
                if (server_address.sin_addr.s_addr == INADDR_NONE) {
                    perror("Invlaid address");
                    break;
                }
                server_address.sin_port = htons(PORT_SERVER);

                if (connect(socket_id, (sockaddr *) &server_address,
                            sizeof(server_address)) < 0) {
                    perror("ERROR connecting");
                    break;
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
                    break;
                }
                memset(buffer, NOTHING, 256);

                n = recv(socket_id, buffer, 255, 0);
                if (n < 0) {
                    perror("ERROR reading from socket");
                    break;
                }
                printf("%s\n", buffer);
            } catch (...) {
                cout << "program exited with error" << endl;
                close(socket_id);
                return -1;
            }
        } else if (in == "c2") { //******CLIENT CODE******//

            cout << "Enter server address: ";
            string host;
            getline(cin, host);
            getline(cin, host);
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
            RequestPacket requestPacket(ReqType::Fail, temp_msg);
            requestPacket.Send();

        } else if (in == "s2") { //******SERVER CODE******//

            cout << "Enter damage probability: ";
            string damage_prob;
            getline(cin, damage_prob);
            getline(cin, damage_prob);
            cout << "Enter loss probability: ";
            string loss_prob;
            getline(cin, loss_prob);
            Gremlin::instance()->initialize(atof(damage_prob.c_str()), atof(loss_prob.c_str()));

            Sockets::instance()->OpenServer(this_address, "127.0.0.1", PORT_SERVER, PORT_CLIENT);
            cout << "Success starting server." << endl;
            Sockets::instance()->TestRoundTrip(SERVER);
            cout << "end RTT test" << endl;
            cout << endl << " !!! Await starting (you must CTRL+C) !!! " << endl << endl;
            Packet *temp = new Packet();
            string temp_type;
            while (true) {
                dprintm("AWAIT RETURNED", Sockets::instance()->AwaitPacket(temp, temp_type));
                cout << endl;
            }
        } else if (in == "f") { /* FILE COPY TEST */
            cout << "Enter file name: ";
            string fname;
            getline(cin, fname);
            getline(cin, fname);
            FileManager mgr(CLIENT);
            mgr.ReadFile(fname);
            vector<DataPacket> packet_list;
            mgr.BreakFile(packet_list);
            cout << "File broken, putting back together" << endl;
            mgr.WriteFile("d_" + fname);
            mgr.JoinFile(packet_list);
        }
    }
    return 0;
}

