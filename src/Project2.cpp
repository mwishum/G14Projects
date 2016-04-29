//============================================================================
// Project 2: Reliable FTP Using GBN over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// April 15, 2016
//============================================================================

#include "project.h"
#include "FileManager.h"
#include "side/Server.h"
#include "side/Client.h"

using namespace std;

int main(int argc, char *argv[]) {
    ///////////////////////////////////////////////
    cout << getenv("TERM") <<endl;
    dprint("\033[1;31m[Test]\033[0m","")
    cout << "Ver " << __DATE__ << " " << __TIME__ << endl;
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa = NULL;
    void *tmpAddrPtr = NULL;
    bool volatile exit = true;
    string in;

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
    cout << "Project 2: FTP using GBN" <<endl;
    cout << "=========== Menu ===========" << endl;
    cout << "Exit: e " << endl;
    cout << "Server:\n s <dmg%> <loss%> <delay%> <delay ms> " << endl;
    cout << "Client:\n c <address>     " << endl;
    cout << " get <src file> <out file> " << endl;
    cout << "============================" << endl << endl;
    while (exit) {
        cout << ">";
        getline(cin, in);
        for (int i = 0; in[i]; i++)
            in[i] = (char) tolower(in[i]);
        //Vector of user input (split by spaces, in lowercase)
        vector<string> command = split(in, ' ');

        string primary;
        //primary is the FIRST element in the vector (the main command)
        if (command.empty()) {
            primary = " ";
        } else {
            primary = command[0];
        }
        if (primary == "e") {
            //******EXIT******//
            exit = false;
            cout << "Goodbye!" << endl;
        } else if (primary == "c") {
            //******CLIENT CODE******//

            main_client(this_address, command);

        } else if (primary == "s") {
            //******SERVER CODE******//

            main_server(this_address, command);
        } else if (primary == "t") {
            Sockets::instance()->OpenClient(this_address, PORT_CLIENT);
            char *c = (char *) "Hello";
            DataPacket dp(c, strlen(c));
            dp.SendDelayed();
        } else {
            //******NO COMMAND******//
            continue;
        }

    } //***END MAIN WHILE
    return 0;
}

