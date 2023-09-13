#pragma once
#include <string>

#include "inih/cpp/INIReader.h"

struct Settings
{
    INIReader* ini_reader = nullptr;

    Settings();
    ~Settings();
    
    bool SettingsExists();
    void LoadSettings();

    std::string GetHostIp();
    int GetDelayCompensation();
    int GetHostPort();
    int GetGuestPort();
    bool GetLogEnabled();
    bool ShowConsole();
};
