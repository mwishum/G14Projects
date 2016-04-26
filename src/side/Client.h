//============================================================================
// Project 2: Reliable FTP Using GBN over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// April 15, 2016
//============================================================================

#ifndef G14PROJECT2_CLIENT_H
#define G14PROJECT2_CLIENT_H

#include "../project.h"
#include "../Sockets.h"
#include "../FileManager.h"

using namespace std;

/**
 * Represents the Go Back N Protocol for the client.
 *
 * @param mgr FileManager instance
 * @param filename name of file requesting FROM server
 * @param out_file_name file name to write to
 *
 * @return SR status of transmission
 */
inline SR GoBackNProtocol_Client(FileManager &mgr, string &file_name, string &out_file_name) {
    SR result;

    uint8_t alt_bit = 1;
    vector<DataPacket> packet_list;

    //TODO: Write the GBN Protocol

    while (true) {
        if (alt_bit == 1) {
            alt_bit = 0;
        } else alt_bit = 1;

        receive_more:

        DataPacket pack(NO_CONTENT, 0);
        pack.Sequence(alt_bit);
        result = pack.Receive();

        if (pack.ContentSize() == 0) { // BREAK ON THIS
            RequestPacket suc(ReqType::Success, &file_name[0], file_name.size());
            suc.Send();
            break;
        }

        dprintm("Status Result (CLIENT)", result);

        if (result == SR::Success) {
            packet_list.push_back(pack);
            AckPacket packet = AckPacket(alt_bit);
            packet.Send();
            continue;
        } else if (result == SR::ChecksumDoesNotMatch) {
            NakPacket packet = NakPacket(alt_bit);
            packet.Send();
            //PRINTED IN DECODE
            cout << " Seq#:" << (int) alt_bit << endl;
            goto receive_more;
        } else if (result == SR::OutOfSequence) {
            NakPacket packet = NakPacket(alt_bit);
            packet.Send();
            //PRINTED IN DECODE
            cout << " Seq#:" << (int) alt_bit << endl;
            goto receive_more;
        } else {
            dprintm("RECEIVE FILE LOOP ELSE", result)
            goto receive_more;
        }
    }

    mgr.WriteFile(out_file_name);
    mgr.JoinFile(packet_list);
    cout << "File transferred to `" << out_file_name << "` successfully." << endl;
    //TODO: Check logic of timeouts
    Sockets::instance()->use_manual_timeout = true;
    Sockets::instance()->ResetTimeout(TIMEOUT_SEC, TIMEOUT_MICRO_SEC);
    Sockets::instance()->use_manual_timeout = false;
    return SR::Success;
}

/**
 * Starts the main client loop.
 *
 * @param this_address string representing server address
 * @param command vector list of command arguments
 *
 */
inline bool main_client(string this_address, vector<string> &command) {
    string remote_server_a, file_name, out_file_name, in, primary;
    SR result;

    if (command.size() == 1) {
        cout << "Enter server address: ";
        getline(cin, remote_server_a);
    } else {
        remote_server_a = command[1];
    }
    result = Sockets::instance()->OpenClient(remote_server_a, PORT_CLIENT);
    FileManager mgr(CLIENT);
    if (result != SR::Success) {
        cerr << "Could not start Client." << endl;
        return true;
    }
    cout << "Success starting client." << endl;

    for (int tries = 5; tries >= 0; tries--) {
        GreetingPacket hello(&this_address[0], this_address.size());
        hello.Send();
        result = hello.Receive();
        if (result != SR::Success) {
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
        //int loops = 0;
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
                getline(cin, out_file_name);
            } else {
                file_name = command[1];
                out_file_name = command[2];
            }

            //Find out if file exists on server
            RequestPacket *req_send = new RequestPacket(ReqType::Info, &file_name[0], file_name.size());
            result = req_send->Send();
            dprintm("Sending filename to server", result)
            //fwrite(req_send->Content(), req_send->ContentSize(), 1, stdout);

            string p_type;
            while (true) {
                //result = Sockets::instance()->AwaitPacket(buffer, &buffer_len, p_type);
                UnknownPacket *packet = nullptr;
                result = Sockets::instance()->AwaitPacket(&packet, p_type);
                assert(packet != nullptr);
                delete packet;
                if (result == SR::Success) {
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

            GoBackNProtocol_Client(mgr, file_name, out_file_name);

        } else if (primary == "e") {
            cout << "[client closed]" << endl;
            return true; //Back to main program loop
        } else {
            continue; //Continue client menu loop
        }
    }
}


#endif //G14PROJECT2_CLIENT_H
