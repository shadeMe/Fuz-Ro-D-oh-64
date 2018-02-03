#include "FuzRoDohInternals.h"
#include "Hooks.h"
#include "VersionInfo.h"
#include <ShlObj.h>

extern "C"
{
	bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\Fuz Ro D-oh.log");
		_MESSAGE("%s Initializing...", MakeSillyName().c_str());

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "Fuz Ro D'oh";
		info->version = PACKED_SME_VERSION;

		g_pluginHandle = skse->GetPluginHandle();

		if (skse->isEditor)
			return false;
		else if (skse->runtimeVersion != RUNTIME_VERSION_1_5_23)
		{
			_MESSAGE("Unsupported runtime version %08X", skse->runtimeVersion);
			return false;
		}

		// supported runtime version
		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse)
	{
		while (IsDebuggerPresent() == false)
			Sleep(100);

		_MESSAGE("Initializing INI Manager");
		FuzRoDohINIManager::Instance.Initialize("Data\\SKSE\\Plugins\\Fuz Ro D'oh.ini", nullptr);

		if (InstallHooks())
		{
			_MESSAGE("%s Initialized!", MakeSillyName().c_str());
			return true;
		}
		else
			return false;
	}

};
