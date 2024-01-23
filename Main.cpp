#include "FuzRoDohInternals.h"
#include "Hooks.h"
#include "VersionInfo.h"
#include <ShlObj.h>


extern "C"
{
	void MessageHandler(SKSEMessagingInterface::Message * msg)
	{
		switch (msg->type)
		{
		case SKSEMessagingInterface::kMessage_InputLoaded:
			{
				// schedule a cleanup thread for the subtitle hasher
				std::thread CleanupThread([]() {
					while (true)
					{
						std::this_thread::sleep_for(std::chrono::seconds(2));
						SubtitleHasher::Instance.Tick();
					}
				});
				CleanupThread.detach();

				_MESSAGE("Scheduled cleanup thread");
				_MESSAGE("%s Initialized!", MakeSillyName().c_str());
			}
			break;
		}
	}

	__declspec(dllexport) bool SKSEPlugin_Load(const SKSEInterface * skse)
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\Fuz Ro D-oh.log");

		_MESSAGE("%s Initializing...", MakeSillyName().c_str());

		interfaces::kPluginHandle = skse->GetPluginHandle();
		interfaces::kMsgInterface = (SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);

		if (!interfaces::kMsgInterface)
		{
			_MESSAGE("Couldn't initialize messaging interface");
			return false;
		}
		else if (interfaces::kMsgInterface->interfaceVersion < 2)
		{
			_MESSAGE("Messaging interface too old (%d expected %d)", interfaces::kMsgInterface->interfaceVersion, 2);
			return false;
		}

		_MESSAGE("Initializing INI Manager");
		FuzRoDohINIManager::Instance.Initialize("Data\\SKSE\\Plugins\\Fuz Ro D'oh.ini", nullptr);

		if (interfaces::kMsgInterface->RegisterListener(interfaces::kPluginHandle, "SKSE", MessageHandler) == false)
		{
			_MESSAGE("Couldn't register message listener");
			return false;
		}
		else if (InstallHooks() == false)
			return false;

		return true;
	}

	__declspec(dllexport) SKSEPluginVersionData SKSEPlugin_Version =
	{
		SKSEPluginVersionData::kVersion,

		PACKED_SME_VERSION,
		"Fuz Ro D'oh",
		"shadeMe",
		"",
		0,	// Version-dependent
		0,
		{ RUNTIME_VERSION_1_6_1170, 0 },
		0,
	};
};
