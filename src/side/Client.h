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
    UnknownPacket* received;
    string packet_type = "D";

    uint8_t exp_sequence_num = 0;
    uint8_t last_seq_num = 32;
    vector<DataPacket> packet_list;

    while (true) {
        result = Sockets::instance()->AwaitPacket(&received, packet_type);

        // Checking the await result
        if (result != SR::Success) {
            continue;
        }

        received->Sequence(exp_sequence_num);
        result = received->DecodePacket();

        if (received->ContentSize() == 0) { // BREAK ON THIS
            RequestPacket suc(ReqType::Success, &file_name[0], file_name.size());
            suc.Send();
            break;
        }

        dprintm("Status Result (CLIENT)", result);

        if (result == SR::Success) {
            packet_list.push_back(*received);

            last_seq_num = exp_sequence_num;
            if (exp_sequence_num == SEQUENCE_MAX - 1) {
                exp_sequence_num = 0;
            } else exp_sequence_num++;

            AckPacket packet = AckPacket(exp_sequence_num);
            packet.Send();
        } else if (result == SR::ChecksumDoesNotMatch) {
            NakPacket packet = NakPacket(exp_sequence_num);
            packet.Send();
            //PRINTED IN DECODE
            cout << " Seq#:" << (int) exp_sequence_num << endl;
        } else if (result == SR::OutOfSequence) {
            AckPacket packet = AckPacket(last_seq_num);
            packet.Send();
            //PRINTED IN DECODE
            cout << " Seq#:" << (int) exp_sequence_num << endl;
        } else {
            dprintm("RECEIVE FILE LOOP ELSE", result)
        }
    }

    delete received;

    mgr.WriteFile(out_file_name);
    mgr.JoinFile(packet_list);
    cout << "File transferred to `" << out_file_name << "` successfully." << endl;
    Sockets::instance()->UseTimeout(TIMEOUT_SEC, TIMEOUT_MICRO_SEC);
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
        dprintm("client",result);
        return true;
    }
    cout << "Success starting client." << endl;
    cout << "Trying to contact server:" <<endl;
    bool connected = false;
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
            connected = true;
            break;
        }
    }

    //Exit client if could not contact server
    if(!connected) return false;

    while (true) {
        client:
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
                UnknownPacket *packet;
                result = Sockets::instance()->AwaitPacket(&packet, p_type);
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
            Sockets::instance()->Close();
            return true; //Back to main program loop
        } else {
            continue; //Continue client menu loop
        }
    }
}


#endif //G14PROJECT2_CLIENT_H
