// async_subscribe.cpp
//
// This is a Paho MQTT C++ client, sample application.
//
// This application is an MQTT publisher/subscriber using the C++
// asynchronous client interface, demonstrating how you can share a client
// between multiple threads.
//
// The app will count the number of "data" messages arriving at the broker
// and then emit "events" with updated counts. A data message is any on a
// "data/#" topic, and counts are emitted on the "events/count" topic. It
// emits an event count around once every ten data messages.
//
// Note that this is a fairly contrived example, and it could be done much
// more easily in a single thread. It is meant to demonstrate how you can
// share a client amongst threads if and when that's a proper thing to do.
//
// At this time, there is a single callback or consumer queue for all
// incoming messages, so you would typically only have one thead receiving
// messages, although it _could_ send messages to multiple threads for
// processing, perhaps based on the topics. It could be common, however, to
// want to have multiple threads for publishing.
//
// This example demonstrates:
//  - Creating a client and sharing it across threads using a shared_ptr<>
//  - Using one thread to receive incoming messages from the broker and
//    another thread to publish messages to it.
//  - Connecting to an MQTT se./asrver/broker.
//  - Automatic reconnect
//  - Publishing messages
//  - Subscribing to multiple topics
//  - Using the asynchronous message consumer
//  - Signaling consumer from another thread
//

/*******************************************************************************
 * Copyright (c) 2020-2025 Frank Pagliughi <fpagliughi@mindspring.com>
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Frank Pagliughi - initial implementation and documentation
 *******************************************************************************/

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "mqtt/async_client.h"

using namespace std;
using namespace std::chrono;

const std::string DFLT_SERVER_ADDRESS{"tcp://eu1.cloud.thethings.network:1883"};
const std::string CLIENT_ID{"itp-team-1b0123314213124123"};
const string mqqt_username = "itp-project-1@ttn";
const string device_name = "uno-0004a30b001c1b03";

/////////////////////////////////////////////////////////////////////////////

const char* api_key;
int main(int argc, char* argv[])
{
    // Get the API key
    api_key = getenv("TTN_API_KEY");
    if (!api_key) {
        cerr << "Error: Please set the environment variable TTN_API_KEY";
        exit(1);
    } else {
        cout << "Using API key: " << string(api_key).substr(0, 8) << "..." << endl;
    }

    string address = DFLT_SERVER_ADDRESS;

    // Create an MQTT client using a smart pointer to be shared among threads.
    auto client = std::make_shared<mqtt::async_client>(address, CLIENT_ID);

    // Connect options for a persistent session and automatic reconnects.
    auto connOpts = mqtt::connect_options_builder()
                        .clean_session(false)
                        .user_name(mqqt_username)
                        .password(api_key)
                        .automatic_reconnect(seconds(2), seconds(30))
                        .finalize();

    auto TOPICS = mqtt::string_collection::create({"#","data"});
    const vector<int> QOS{0, 1};

    try {
        // Start consuming _before_ connecting, because we could get a flood
        // of stored messages as soon as the connection completes since
        // we're using a persistent (non-clean) session with the broker.
        client->start_consuming();

        cout << "Connecting to the MQTT server at " << address << "..." << flush;
        auto rsp = client->connect(connOpts)->get_connect_response();
        cout << "OK\n" << endl;

        // Subscribe if this is a new session with the server
        if (!rsp.is_session_present())
            client->subscribe(TOPICS, QOS);


        // Start another thread to read incoming topics

        std::thread{[client] {
            while (true) {
            cout << "Waiting for a message\n";
            auto msg = client->consume_message();
            cout << "Message recieved\n";
            if (!msg) {
                // Exit if the consumer was shut down
                if (client->consumer_closed())
                    break;

                // Otherwise let auto-reconnect deal with it.
                cout << "Disconnect detected. Attempting an auto-reconnect." << endl;
                continue;
            }

            cout << msg->get_topic() << ": " << msg->to_string() << endl;
        }
        }}.detach();
        
        string user_input;

        while (user_input != "exit") {
            
        }

        client->stop_consuming();
        // Close the counter and wait for the publisher thread to complete
        cout << "\nShutting down..." << flush;

        // Disconnect
        cout << "OK\nDisconnecting..." << flush;
        client->disconnect();
        cout << "OK" << endl;
    }
    catch (const mqtt::exception& exc) {
        cerr << exc.what() << endl;
        return 1;
    }

    return 0;
}