#include "pch.h"
#include "Settings.h"
#include <fstream>

#include "Connection.h"
#include "UI.h"
#include "Utils.h"

#include "inih/cpp/INIReader.h"

#define settings_file_name "p2vp_settings.ini"
#define msg(x, title) MessageBoxA(NULL, x, title, MB_OK);

extern P2PConnection g_connection;
extern bool g_is_log;

Settings g_settings;

Settings::Settings()
{
}

Settings::~Settings()
{
	// Clean up ini_reader
	delete ini_reader;
	ini_reader = nullptr;
}

bool Settings::SettingsExists()
{
	std::ifstream f(settings_file_name);
	return f.good();
}

void Settings::LoadSettings()
{
	if (!this->SettingsExists())
	{
		// Settings do not exist, ignore loading
		LogInfo("Setting file p2vp_settings.ini does not exist, ignoring");
		return;
	}

	LogInfo("Loading setting file p2vp_settings.ini");
	ini_reader = new INIReader(settings_file_name);

	// Parse error
	if (this->ini_reader->ParseError() < 0) {
		LogInfo("Cannot load p2vp_settings.ini");
		msg("Cannot load p2vp_settings.ini", "Settings error")
		return;
	}

	// Utils settings
	g_is_log = GetLogEnabled();

	if (ShowConsole())
	{
		AllocAndShowConsole();
	}

	LogInfo("Setting file p2vp_settings.ini was loaded"); // Print log message that will be shown in console if it is enabled
	
	// Connection settings
	g_connection.LoadSettings();
	SetDataFromSettingsIntoUI(); // Also set settings to UI to visually sync them with data in P2PConnection
}


std::string Settings::GetHostIp()
{
	if (this->ini_reader == nullptr)
	{
		return ""; // Empty address
	}
	
	return this->ini_reader->Get("Connection", "host_ip", "");
}

int Settings::GetDelayCompensation()
{
	if (this->ini_reader == nullptr)
	{
		return -1; // Default delay
	}

	return this->ini_reader->GetInteger("Connection", "delay_compensation", -1);
}

int Settings::GetHostPort()
{
	if (this->ini_reader == nullptr)
	{
		return 18000; // Default port
	}

	int port = this->ini_reader->GetInteger("Connection", "host_port", 18000);

	if (port > 65535 || port < 0)
	{
		return 18000; // Default port
	}

	return port;
}

int Settings::GetGuestPort()
{
	if (this->ini_reader == nullptr)
	{
		return 18001; // Default port
	}

	int port = this->ini_reader->GetInteger("Connection", "guest_port", 18001);

	if (port > 65535 || port < 0)
	{
		return 18001; // Default port
	}

	return port;
}

bool Settings::GetLogEnabled()
{
	if (this->ini_reader == nullptr)
	{
		return false;
	}

	return this->ini_reader->GetBoolean("Utils", "log_enabled", false);
}

bool Settings::ShowConsole()
{
	if (this->ini_reader == nullptr)
	{
		return false;
	}

	return this->ini_reader->GetBoolean("Utils", "show_console", false);
}
