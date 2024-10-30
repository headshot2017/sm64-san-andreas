#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#define CONFIG_FILENAME "sm64-san-andreas.cfg"

#include <string>
#include <unordered_map>

struct ConfigElement
{
	const std::string desc;
	int value;
};

extern std::unordered_map<std::string, ConfigElement> config;

int getConfig(std::string value);
void saveConfig();
void loadConfig();

#endif // CONFIG_H_INCLUDED
