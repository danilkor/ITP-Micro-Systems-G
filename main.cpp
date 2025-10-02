#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "mqtt/async_client.h"
#include <nlohmann/json.hpp>

#include "ftxui/component/captured_mouse.hpp"     // for ftxui
#include "ftxui/component/component.hpp"          // for Button, Horizontal, Renderer
#include "ftxui/component/component_base.hpp"     // for ComponentBase
#include "ftxui/component/screen_interactive.hpp" // for ScreenInteractive
#include "ftxui/dom/elements.hpp"                 // for separator, gauge, text, Element, operator|, vbox, border

using namespace std;
using namespace std::chrono;
using namespace ftxui;

// for convenience
using json = nlohmann::json;

/////////////////////////////  ////////////////////////////////////////////////
const std::string DFLT_SERVER_ADDRESS{"tcp://eu1.cloud.thethings.network:1883"};
const std::string CLIENT_ID{"itp-team-1b0123314213124123"};
const string mqqt_username = "itp-project-1@ttn";
const string device_name = "uno-0004a30b001c1b03";

/////////////////////////////////////////////////////////////////////////////
const string downlink_topic = "v3/" + mqqt_username + "/devices/" + device_name + "/down/push";
const string uplink_topic = "v3/" + mqqt_username + "/devices/" + device_name + "/up";
atomic<float> last_temp = 0;
atomic_int led_status = 0;

// This is a helper function to create a button with a custom style.
// The style is defined by a lambda function that takes an EntryState and
// returns an Element.
// We are using `center` to center the text inside the button, then `border` to
// add a border around the button, and finally `flex` to make the button fill
// the available space.
ButtonOption Style()
{
    auto option = ButtonOption::Animated();
    option.transform = [](const EntryState &s)
    {
        auto element = text(s.label);
        if (s.focused)
        {
            element |= bold;
        }
        return element | center | borderEmpty | flex;
    };
    return option;
}

void publish(mqtt::async_client_ptr client, const string topic, const string payload)
{
    auto msg = mqtt::make_message(topic, payload);
    msg->set_qos(1);
    client->publish(msg);
}

void set_led_status(mqtt::async_client_ptr client, bool status)
{
    json json_payload;
    json_payload["downlinks"][0] = {{"f_port", 15},
                                  {"decoded_payload", {{"app", "building"}, {"type", "ledcontrol"}, {"led", (int)status}}},
                                  {"priority", "NORMAL"}};
    publish(client, downlink_topic, json_payload.dump());
}

void analyse_msg(string msg)
{
    auto message = json::parse(msg);
    // LED status
    if (message["uplink_message"]["decoded_payload"]["type"] == "ledstatus")
    {
        led_status = message["uplink_message"]["decoded_payload"]["led"];
    } else if (message["uplink_message"]["decoded_payload"]["type"] == "temp")
    {
        last_temp = message["uplink_message"]["decoded_payload"]["value"];
    }
}

const char *api_key;
int main(int argc, char *argv[])
{
    // Get the API key
    api_key = getenv("TTN_API_KEY");
    if (!api_key)
    {
        cerr << "Error: Please set the environment variable TTN_API_KEY";
        exit(1);
    }
    else
    {
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

    auto TOPICS = mqtt::string_collection::create({uplink_topic});
    const vector<int> QOS{0};

    try
    {
        auto screen = ScreenInteractive::FitComponent();

        // Start consuming _before_ connecting, because we could get a flood
        // of stored messages as soon as the connection completes since
        // we're using a persistent (non-clean) session with the broker.
        client->start_consuming();

        cout << "Connecting to the MQTT server at " << address << "..." << flush;
        auto rsp = client->connect(connOpts)->get_connect_response();
        cout << "OK\n"
             << endl;

        // Subscribe if this is a new session with the server
        if (!rsp.is_session_present())
            client->subscribe(TOPICS, QOS);

        // Start another thread to read incoming topics
        std::thread{[client, &screen]
                    {
                        while (true)
                        {
                            auto msg = client->consume_message();
                            if (!msg)
                            {
                                // Exit if the consumer was shut down
                                if (client->consumer_closed())
                                    break;
                                continue;
                            }
                            analyse_msg(msg->to_string());
                            screen.PostEvent(ftxui::Event::Custom);
                        }
                    }}
            .detach();

        // clang-format off
        auto btn_led_on = Button("ON", [&] { std::thread(set_led_status, client, true).detach(); }, Style());
        auto btn_led_off = Button("OFF", [&] { std::thread(set_led_status, client, false).detach(); }, Style());
        // clang-format on

        // The tree of components. This defines how to navigate using the keyboard.
        // The selected `row` is shared to get a grid layout.
        int row = 0;
        auto buttons = Container::Vertical({
            Container::Horizontal({btn_led_on, btn_led_off}, &row) | flex,
        });

        // Modify the way to render them on screen:
        auto ui_renderer = Renderer(buttons, [&]
                                  { return vbox({
                                               text("TEMP = " + std::to_string(std::round(last_temp*10)/10).substr(0, std::to_string(std::round(last_temp*10)/10).find('.') + 2) + "°C"),
                                               text(std::string("LED = ") + (led_status == 0 ? "OFF" : "ON")),
                                               separator(),
                                               buttons->Render() | flex,
                                           }) |
                                           flex | border; });

        
        screen.Loop(ui_renderer);

        client->stop_consuming();
        // Close the counter and wait for the publisher thread to complete
        cout << "\nShutting down..." << flush;

        // Disconnect
        cout << "OK\nDisconnecting..." << flush;
        client->disconnect();
        cout << "OK" << endl;
    }
    catch (const mqtt::exception &exc)
    {
        cerr << exc.what() << endl;
        return 1;
    }

    return 0;
}