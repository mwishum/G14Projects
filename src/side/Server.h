//============================================================================
// Project 2: Reliable FTP Using GBN over UDP
// Author: Group 14
// Mason Wishum (mlw0032), Harrison Kinchler (hdk0002),
// Michael Pearce (mtp0013)
// April 15, 2016
//============================================================================


#ifndef G14PROJECT2_SERVER_H
#define G14PROJECT2_SERVER_H

#include "../project.h"
#include "../Sockets.h"
#include "../FileManager.h"
#include "../Gremlin.h"

using namespace std;

inline bool main_server(string this_address, vector<string> &command) {
    string damage_prob, loss_prob, delay_prob, delay_time;
    SR result;
    string packet_type;
    int loops = 0;

    //Decode command
    if (command.size() < 5) {
        cout << "Enter damage probability: ";
        getline(cin, damage_prob);
        cout << "Enter loss probability: ";
        getline(cin, loss_prob);
        cout << "Enter delay probability: ";
        getline(cin, delay_prob);
        cout << "Enter delay time: ";
        getline(cin, delay_time);
    } else {
        damage_prob = command[1];
        loss_prob = command[2];
        delay_prob = command[3];
        delay_time = command[4];
    }

    Gremlin::instance()->initialize(atof(damage_prob.c_str()), atof(loss_prob.c_str()),
    		atof(delay_prob.c_str()), atof(delay_time.c_str()));
    result = Sockets::instance()->OpenServer(this_address, PORT_CLIENT);
    if (result != SR::Success) {
        cerr << "Could not start server." << endl;
        return true;
    }

    FileManager mgr(SERVER);

    cout << "Success starting server." << endl;

    while (true) {
        UnknownPacket *received = nullptr;
        result = Sockets::instance()->AwaitPacketForever(&received, packet_type);
        assert(received != nullptr);

        if (result != SR::Success) continue;

        dprint("Packet type", packet_type)
        if (packet_type == GREETING) {
            received->Sequence(1);
            received->DecodePacket();

            GreetingPacket hello = GreetingPacket(&this_address[0], this_address.size());
            hello.Send();
        } else if (packet_type == GET_INFO) { // GET FILE INFO
            bool file_exists;
            //Get File Exists Request
            //RequestPacket req_info(ReqType::Info, NO_CONTENT, 1);
            received->Sequence(1);
            received->DecodePacket();

            //Check actual file exists
            struct stat stat_buff;
            string file_name_from_client;
            file_name_from_client.insert(0, received->Content(), received->ContentSize());
            file_exists = (stat(file_name_from_client.c_str(), &stat_buff) == 0);
            cout << "FILE `" << file_name_from_client << "` EXISTS ?";

            //Send status of file (Success or Fail)
            send_file_ack_again:
            if (file_exists) {
                RequestPacket suc(ReqType::Success, received->Content(), received->ContentSize());
                cout << " TRUE" << endl;
                suc.Send();
            } else {
                RequestPacket fail(ReqType::Fail, received->Content(), received->ContentSize());
                cout << " FALSE" << endl;
                fail.Send();
            }

            //Make sure client ack the file status
            AckPacket ack_file_exists(0);
            result = ack_file_exists.Receive();
            dprintm("Client ack file exists", result);
            if (result == SR::Success) {
                //Back to loop if file doesnt exist.
                if (!file_exists) continue;

                //If file exists and in sync with client, check RTT
                Sockets::instance()->TestRoundTrip(SERVER);

                mgr.ReadFile(file_name_from_client);
                vector<DataPacket> packet_list;
                mgr.BreakFile(packet_list);
                uint8_t alt_bit = 1;

                for (int i = 0; i < packet_list.size(); i++) {
                    DataPacket packet = packet_list[i];

                    if (alt_bit == 1) {
                        alt_bit = 0;
                    } else alt_bit = 1;

                    send_again:

                    packet.Sequence(alt_bit);
                    packet.Send();

                    //Wait for response from server

                    result = Sockets::instance()->AwaitPacket(&received, packet_type);
                    assert(&received != nullptr);

                    if (packet_type == NO_ACK) {
                        received->Sequence(0);
                        received->DecodePacket();
                        int seq = received->Sequence();

                        if (seq != alt_bit) {
                            cout << "Packet Status: Lost (NAK)" << endl;
                            cout << "Sequence number: " << seq << endl;
                            cout << "Expected number: " << (int) alt_bit << endl;
                        }

                        goto send_again;
                    } else if (packet_type == ACK) {
                        received->Sequence(0);
                        received->DecodePacket();
                        int seq = received->Sequence();

                        if (seq != alt_bit) {
                            cout << "Packet Status: Lost/Tampered (ACK)" << endl;
                            cout << "Sequence number: " << seq << endl;
                            cout << "Expected number: " << (int) alt_bit << endl;
                            goto send_again;
                        }

                        continue;
                    } else if (result == SR::Timeout) {
                        goto send_again;
                    } else if (packet_type == GET_SUCCESS) {
                        break;
                    } else {
                        cerr << "UNEXPECTED TYPE " << packet_type << " " << StatusMessage[(int) result] << endl;
                        goto send_again;
                    }
                } //END PACKET LOOP
                cout << "Finished file transmission successfully." << endl;
            } //**RESULT == SUCCESS
            else { // NOT SUCCESS
                cout << "Sending File status Again" << endl;
                goto send_file_ack_again;
            }

        } //***ELSE IF GET INFO
        else {
        }
        if (loops++ >= MAX_LOOPS) {
            cout << "Server ran too long. (" << MAX_LOOPS << ")\n";
            break;
        }
        delete received;
    } //***WHILE
    return true;
}


#endif //G14PROJECT2_SERVER_H
