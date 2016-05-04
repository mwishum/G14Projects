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
    string packet_type;
    char buffer[PACKET_SIZE];
    size_t buffer_len = PACKET_SIZE;
    SR result;

    mgr.ReadFile(filename);
    vector<DataPacket> packet_list;
    mgr.BreakFile(packet_list);

    uint8_t sequence_num = 0;
    uint32_t last_ack_num = 32;
    uint32_t window_start = 0;
    uint32_t sent_packet_num = 0;
    uint32_t window_roll_overs = 0;

    while (1) {
        int window_end;
        for (int i = sequence_num; ; i = (i + 1) % SEQUENCE_MAX) {
            window_end = (window_start + WINDOW_SIZE) % SEQUENCE_MAX;
            if (window_end < window_start) {
                if (i < window_start && i >= window_end) {
                    break;
                }
            }
            else if (i >= window_end || i < window_start) {
                break;
            }
            if (sent_packet_num >= packet_list.size()) {
                cout << color_text("43", "[Server]SENT ALL PACKETS!") << endl;
                break;
            }
            DataPacket packet = packet_list.at(sent_packet_num);
            packet.Sequence(sequence_num);
            packet.Send();
            sent_packet_num++;
            if (++sequence_num % 32 == 0) {
                window_roll_overs++;
                sequence_num = 0;
            }
        }

        result = Sockets::instance()->AwaitPacket(buffer, buffer_len, packet_type);
        dprint("  [SERVER]Await", StatusMessage[(int) result] + ", " + packet_type)

        if (result == SR::Timeout) {
            dprint("", color_text("44", " -> Timeout"))

            if (sequence_num < last_ack_num % SEQUENCE_MAX) {
                window_roll_overs--;
            }

            sent_packet_num = SEQUENCE_MAX * window_roll_overs + (last_ack_num % 32);
            sequence_num = (uint8_t) (last_ack_num % SEQUENCE_MAX);
            usleep(90);
        } else if (packet_type == NO_ACK) { // Resend packets starting from sequence number
            dprint("", color_text("45", " -> NO ACK"))
            NakPacket received(0);
            received.DecodePacket(buffer, buffer_len);
            received.DecodePacket();

            if (sequence_num < received.Sequence()) {
                window_roll_overs--;
            }

            sent_packet_num = SEQUENCE_MAX * window_roll_overs + received.Sequence();
            sequence_num = received.Sequence();
        } else if (packet_type == ACK) {
            dprint("", color_text("46", " -> ACK"))
            AckPacket received(0);
            received.DecodePacket(buffer, buffer_len);
            received.DecodePacket();
            if (received.Sequence() == 32) { // Special case - first packet out of sequence
                sent_packet_num = 0;
                sequence_num = 0;
                continue;
            }
            if (last_ack_num != received.Sequence() + 1) {
                last_ack_num = received.Sequence();
                window_start = received.Sequence();
            }
        } else if (packet_type == GET_SUCCESS) {
            ///////////////////////////////////////////////
            string s = "Success received, transfer done. (" + to_string((long long int) packet_list.size() - 1) + ")";
            cout << endl << endl << endl << endl << endl << "=====================" << endl;
            cout << color_text("42", s) << endl;
            cout << "=====================" << endl << endl << endl << endl;
            break;
        } else {
            cerr << "UNEXPECTED TYPE " << packet_type << " " << StatusMessage[(int) result] << endl;
        }
        cout << "Server successfully sent" << color_text("40", to_string((long long int) sent_packet_num)) << endl;
    } //END PACKET LOOP
    Sockets::instance()->UseTimeout(TIMEOUT_SEC, TIMEOUT_MICRO_SEC);
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
    char buffer[PACKET_SIZE];
    size_t buffer_len;

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
        Gremlin::instance()->Close();
        Sockets::instance()->Close();
        return true;
    }
    result = Sockets::instance()->OpenServer(this_address, PORT_CLIENT);
    if (result != SR::Success) {
        cerr << "Could not start server." << endl;
        Gremlin::instance()->Close();
        Sockets::instance()->Close();
        return true;
    }

    FileManager mgr(SERVER);

    cout << "Success starting server." << endl;

    while (true) {
        //Wait for any packet, forever
        buffer_len = PACKET_SIZE;
        result = Sockets::instance()->AwaitPacketForever(buffer, buffer_len, packet_type);

        if (result != SR::Success) continue;

        dprint("server received type", packet_type)
        if (packet_type == GREETING) {
            GreetingPacket received(NO_CONTENT, 1);
            received.Sequence(0);
            received.DecodePacket(buffer, buffer_len);

            GreetingPacket hello = GreetingPacket(&this_address[0], this_address.size());
            hello.Send();
        } else if (packet_type == GET_INFO) { // GET FILE INFO
            bool file_exists;
            //Get File Exists Request
            RequestPacket received(ReqType::Info, NO_CONTENT, 1);
            received.Sequence(1);
            received.DecodePacket(buffer, buffer_len);

            //Check actual file exists
            struct stat stat_buff;
            string file_name_from_client;
            file_name_from_client.insert(0, received.Content(), received.ContentSize());
            file_exists = (stat(file_name_from_client.c_str(), &stat_buff) == 0);
            cout << "FILE `" << file_name_from_client << "` EXISTS ?";

            //Send status of file (Success or Fail)
            send_file_ack_again:
            if (file_exists) {
                RequestPacket suc(ReqType::Success, received.Content(), received.ContentSize());
                cout << " TRUE" << endl;
                suc.Send();
            } else {
                RequestPacket fail(ReqType::Fail, received.Content(), received.ContentSize());
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
                    cout << "TOTAL PACKETS SENT " << Sockets::instance()->TOTAL_SENT << endl;
                }

            } //**RESULT == SUCCESS
            else { // NOT SUCCESS
                cout << "Sending File status Again" << endl;
                goto send_file_ack_again;
            }

        } //***ELSE IF GET INFO
        else {

        }
    } //***WHILE
}


#endif //G14PROJECT2_SERVER_H
