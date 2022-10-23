#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <array>
#include <map>
#include "lib/discord.h"
using namespace std;


#define CLIENTID _ID


const discord::ClientId client_id = CLIENTID;
// Global vars for discord Core
struct DiscordState {
    discord::User currentUser;

    std::unique_ptr<discord::Core> core;
};
discord::Activity activity{};
DiscordState state{};
discord::Core* core_raw{};


// Simple wrapper for executing shell command
std::string execCommand(const char* cmd) {
    std::array<char, 128> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}


// Parse AppleScript output to a dictionary
map<string,string> parseAppleScript(string rawOutput) {
    // Result
    map<string, string> result;
    // Temporarily store Key (which is split by delimiter ":")
    string tmp_key;
    // First set the delimiter to ":"
    string delimiter = ":";

    size_t pos = 0;
    string token;
    while ((pos = rawOutput.find(delimiter)) != string::npos) {
        token = rawOutput.substr(0, pos);
        rawOutput.erase(0, pos + delimiter.length());
        if (delimiter == ":") {
            tmp_key = token;
            delimiter = ", ";
        } else {
            result[tmp_key] = token;
            delimiter = ":";
        }
    }
    result[tmp_key] = rawOutput;
    return result;
}

int discord_rpc_thread() {
    auto result = discord::Core::Create(client_id, DiscordCreateFlags_Default, &core_raw);
    state.core.reset(core_raw);
    if (!state.core) {
        cout << "Failed to instantiate discord core! (err " << static_cast<int>(result)
                  << ")\n";
        exit(-1);
    };
    state.core->SetLogHook(
            discord::LogLevel::Debug, [](discord::LogLevel level, const char* message) {
                std::cerr << "Log(" << static_cast<uint32_t>(level) << "): " << message << "\n";
            });

    activity.GetAssets().SetLargeImage("apple-music");
    activity.SetType(discord::ActivityType::Playing);
    activity.SetDetails("Hi Detail!");
    activity.GetTimestamps().SetStart(chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count());
    activity.SetState("hello");
    activity.SetDetails("Detail");
    while(true){
        state.core->ActivityManager().UpdateActivity(activity, nullptr);
        state.core->RunCallbacks();
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    return 0;
}

// Check the Now Playing music every 100ms. If changed, edit.
void music_check() {
    string songname;
    char const *script = "osascript -e 'tell application \"Music\" to get properties of current track'\n";
    while (true) {
        map<string,string> songObj = parseAppleScript(execCommand(script));
        if (songObj["name"] != songname) {
            songname = songObj["name"];
            activity.GetTimestamps().SetStart(chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count());
            activity.SetState(&(songObj["artist"]+" | "+songObj["album"])[0]);
            activity.SetDetails(&songObj["name"][0]);
            cout << "🎵 \033[32mNow Playing: \033[0m" << songname << " | " << songObj["artist"]+" | "+songObj["album"] << endl;
        };
        this_thread::sleep_for(chrono::milliseconds(100));
    }

}

int main(int argc, char **argv) {
    // If Discord is not running, exit.
    if (execCommand("ps -A | grep Discord").empty()) {
        cerr << "Discord is not running." << endl;
        return 1;
    }

    // Start Discord RPC thread
    thread t1(discord_rpc_thread);
    thread t2(music_check);

    t1.join();
    t2.join();

    return 0;
}
