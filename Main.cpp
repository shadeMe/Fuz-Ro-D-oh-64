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

	bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
	{
		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\Fuz Ro D-oh.log");
		_MESSAGE("%s Initializing...", MakeSillyName().c_str());

		// populate info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "Fuz Ro D'oh";
		info->version = PACKED_SME_VERSION;

		interfaces::kPluginHandle = skse->GetPluginHandle();
		interfaces::kMsgInterface = (SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);

		if (skse->isEditor)
			return false;
		else if (skse->runtimeVersion != RUNTIME_VERSION_1_5_53)
		{
			_MESSAGE("Unsupported runtime version %08X", skse->runtimeVersion);
			return false;
		}
		else if (!interfaces::kMsgInterface)
		{
			_MESSAGE("Couldn't initialize messaging interface");
			return false;
		}
		else if (interfaces::kMsgInterface->interfaceVersion < 2)
		{
			_MESSAGE("Messaging interface too old (%d expected %d)", interfaces::kMsgInterface->interfaceVersion, 2);
			return false;
		}
		// supported runtime version
		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse)
	{
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

};
