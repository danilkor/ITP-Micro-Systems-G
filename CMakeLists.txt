cmake_minimum_required(VERSION 4.1)
project(MQTTTClient LANGUAGES CXX)

add_executable(client main.cpp)
target_compile_features(client PRIVATE cxx_std_17)

include(FetchContent)
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v6.1.9
)
FetchContent_MakeAvailable(ftxui)

# Find Paho MQTT libraries
find_library(PAHO_MQTT_C paho-mqtt3as REQUIRED)
find_library(PAHO_MQTT_CPP paho-mqttpp3 REQUIRED)
find_package(Threads REQUIRED) # For pthread
find_package(nlohmann_json 3.12.0 REQUIRED)
target_link_libraries(client PRIVATE nlohmann_json::nlohmann_json)


target_link_libraries(client PRIVATE
    # Chose a submodule
    ftxui::component
    ftxui::dom
    ftxui::screen
    ${PAHO_MQTT_C}
    ${PAHO_MQTT_CPP}
    Threads::Threads
)
