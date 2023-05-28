#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#define CONFIG_FILENAME "sm64-san-andreas.cfg"

#include <string>
#include <unordered_map>

extern std::unordered_map<std::string, int> config;

void saveConfig();
void loadConfig();

#endif // CONFIG_H_INCLUDED
