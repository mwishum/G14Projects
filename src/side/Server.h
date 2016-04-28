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

/**
 * Represents the Go Back N Protocol for the server.
 *
 * @param mgr FileManager instance
 * @param filename filename to send to client
 *
 * @return SR status of transmission
 */
inline SR GoBackNProtocol_Server(FileManager &mgr, string &filename) {
    UnknownPacket *received;
    string packet_type;
    SR result;

    mgr.ReadFile(filename);
    vector<DataPacket> packet_list;
    mgr.BreakFile(packet_list);

    uint8_t sequence_num = 0;
    uint8_t last_ack_num = 32;
    uint8_t window_start = 0;
    uint32_t sent_packet_num = 0;
    uint8_t window_roll_overs = 0;

    vector<TIME_METHOD::time_point> sent_times;

    while (1) {
        for (int i = sequence_num; i != ((window_start + WINDOW_SIZE - 1) % SEQUENCE_MAX); i = (i + 1) % SEQUENCE_MAX) {
            DataPacket packet = packet_list[sent_packet_num];
            packet.Sequence(sequence_num);
            packet.Send();
            sent_times.push_back(TIME_METHOD::now());
            sent_packet_num++;
            if (++sequence_num % 32 == 0) {
                window_roll_overs++;
            }
        }

        dprint("received address", &received);
        result = Sockets::instance()->AwaitPacket(&received, packet_type);
        dprintm("GBN Await", result);
        dprint("received address", &received);

        TIME_METHOD::time_point recv_time = TIME_METHOD::now();
        chrono::microseconds span = chrono::duration_cast<chrono::microseconds>(recv_time - sent_times.front());
        sent_times.pop_back();

        if (span.count() > Sockets::instance()->rtt_determined.tv_usec) {
            received->DecodePacket();
            if (sequence_num < received->Sequence()) {
                sent_packet_num = (uint32_t) SEQUENCE_MAX * (window_roll_overs - 1) + received->Sequence();
                window_roll_overs--;
                sequence_num = received->Sequence();
            } else {
                sent_packet_num = received->Sequence();
                sequence_num = received->Sequence();
            }
            continue;
        }

        if (packet_type == NO_ACK) { // Resend packets starting from sequence number
            received->DecodePacket();
            if (sequence_num < received->Sequence()) {
                sent_packet_num = (uint32_t) SEQUENCE_MAX * (window_roll_overs - 1) + received->Sequence();
                window_roll_overs--;
                sequence_num = received->Sequence();
            } else {
                sent_packet_num = received->Sequence();
                sequence_num = received->Sequence();
            }
            sent_times.clear();
            continue;
        } else if (packet_type == ACK) {
            received->DecodePacket();
            last_ack_num = received->Sequence();
            window_start = received->Sequence();
            continue;
        } else if (result == SR::Timeout) {
            if (sequence_num < (uint8_t) ((last_ack_num + 1) % 32)) {
                sent_packet_num = (uint32_t) SEQUENCE_MAX * (window_roll_overs - 1) + (uint8_t) ((last_ack_num + 1) % 32);
                window_roll_overs--;
                sequence_num = (uint8_t) ((last_ack_num + 1) % 32);
            } else {
                sequence_num = (uint8_t) ((last_ack_num + 1) % 32);
                sent_packet_num = sequence_num;
            }
            continue;
        } else if (packet_type == GET_SUCCESS) {
            break;
        } else {
            cerr << "UNEXPECTED TYPE " << packet_type << " " << StatusMessage[(int) result] << endl;
            continue;
        }
    } //END PACKET LOOP

    delete received;
    return SR::Success;
}

/**
 * Starts the main server loop.
 *
 * @param this_address string representing server address
 * @param command vector list of command arguments
 *
 */
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

    result = Gremlin::instance()->initialize(atof(damage_prob.c_str()), atof(loss_prob.c_str()),
                                             atof(delay_prob.c_str()), atoi(delay_time.c_str()));
    if (result != SR::Success) {
        cerr << "Could not start Gremlin." << endl;
        return true;
    }
    result = Sockets::instance()->OpenServer(this_address, PORT_CLIENT);
    if (result != SR::Success) {
        cerr << "Could not start server." << endl;
        return true;
    }

    FileManager mgr(SERVER);

    cout << "Success starting server." << endl;

    while (true) {
        UnknownPacket *received;
        //Wait for any packet, forever
        result = Sockets::instance()->AwaitPacketForever(&received, packet_type);

        if (result != SR::Success) continue;

        dprint("server received type", packet_type)
        if (packet_type == GREETING) {
            received->Sequence(0);
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

                result = GoBackNProtocol_Server(mgr, file_name_from_client);

                if (result == SR::Success) {
                    cout << "Finished file transmission successfully." << endl;
                }

            } //**RESULT == SUCCESS
            else { // NOT SUCCESS
                cout << "Sending File status Again" << endl;
                goto send_file_ack_again;
            }

        } //***ELSE IF GET INFO
        else {
        }
        if (loops++ >= MAX_LOOPS) {
            cout << "Server ran too long. (>" << MAX_LOOPS << ")\n";
            break;
        }
        delete received;
    } //***WHILE
    return true;
}


#endif //G14PROJECT2_SERVER_H
