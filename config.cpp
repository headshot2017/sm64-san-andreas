#include "config.h"

#include <iostream>
#include <fstream>
#include <algorithm>

std::unordered_map<std::string, ConfigElement> config = {
    {"skip_sha1_checksum", {"Skips the ROM SHA-1 checksum. This allows using certain ROM hacks such as sound ROM hacks", 0}},
    {"use_wasapi_audio", {"Prefer WASAPI audio backend over SDL2. Only works on Windows Vista and later.", 0}},
    {"autospawn_mario_on_start", {"When loading a game, automatically spawn Mario.", 0}},
    {"mario_in_mocap_cutscenes", {"Replaces CJ with Mario in the 'motion-capture' cutscenes. NOTE: This feature is not finished!", 0}},
};


int getConfig(std::string value)
{
    return config.count(value) ? config[value].value : 0;
}

void saveConfig()
{
    std::ofstream configfile(CONFIG_FILENAME);
    for (auto& mapkey : config)
    {
        configfile << "# " << mapkey.second.desc << "\n";
        configfile << mapkey.first << ": " << mapkey.second.value << "\n\n";
    }

    std::cout << "sm64-san-andreas.cfg saved\n";
}

void loadConfig()
{
    std::ifstream configfile(CONFIG_FILENAME);
    if (!configfile.is_open())
    {
        std::cout << "sm64-san-andreas.cfg not found, creating...\n";
        saveConfig(); // saves default config
        return;
    }

    size_t totalCfg = 0; // if less than config.size(), save file to create new keys

    std::string line;
    while (getline(configfile, line))
    {
        // clear any whitespaces
        line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());

        if (line[0] == '#' || line.empty())
            continue; // ignore comments and empty lines

        std::size_t delimiterPos = line.find(":");
        if (delimiterPos == -1)
            continue; // invalid config syntax

        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);
        if (!config.count(key))
            continue; // invalid config key

        try
        {
            config[key].value = std::stoi(value);
            totalCfg++;
        }
        catch(...)
        {
            continue;
        }
    }

    if (totalCfg < config.size())
    {
        std::cout << "Updating sm64-san-andreas.cfg with new config keys...\n";
        saveConfig();
    }
}
