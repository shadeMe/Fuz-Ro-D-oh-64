#include "FuzrodohInternals.h"

IDebugLog				gLog;

namespace interfaces
{
	PluginHandle				kPluginHandle = kPluginHandle_Invalid;
	SKSEMessagingInterface*		kMsgInterface = nullptr;
}

FuzRoDohINIManager		FuzRoDohINIManager::Instance;
SubtitleHasher			SubtitleHasher::Instance;
const double			SubtitleHasher::kPurgeInterval = 1000.0 * 60.0f;

SME::INI::INISetting	kWordsPerSecondSilence("WordsPerSecondSilence",
											   "General",
											   "Number of words a second of silent voice can \"hold\"",
											   (SInt32)2);

SME::INI::INISetting	kSkipEmptyResponses("SkipEmptyResponses",
											"General",
											"Don't play back silent dialog for empty dialog responses",
											(SInt32)1);

std::string MakeSillyName()
{
	std::string Out("Fuz Ro ");
	for (int i = 0; i < 64; i++)
		Out += "D'oh";
#ifdef VR_BUILD
	Out += " (for Skyrim VR)";
#endif
	return Out;
}

bool CanShowDialogSubtitles()
{
	return GetINISetting("bDialogueSubtitles:Interface")->data.u8 != 0;
}

bool CanShowGeneralSubtitles()
{
	return GetINISetting("bGeneralSubtitles:Interface")->data.u8 != 0;
}

void FuzRoDohINIManager::Initialize(const char* INIPath, void* Paramenter)
{
	this->INIFilePath = INIPath;
	_MESSAGE("INI Path: %s", INIPath);

	std::fstream INIStream(INIPath, std::fstream::in);
	bool CreateINI = false;

	if (INIStream.fail())
	{
		_MESSAGE("INI File not found; Creating one...");
		CreateINI = true;
	}

	INIStream.close();
	INIStream.clear();

	RegisterSetting(&kWordsPerSecondSilence);
	RegisterSetting(&kSkipEmptyResponses);

	if (CreateINI)
		Save();
}

SubtitleHasher::HashT SubtitleHasher::CalculateHash(const char* String)
{
	SME_ASSERT(String);

	// Uses the djb2 string hashing algorithm
	// http://www.cse.yorku.ca/~oz/hash.html

	HashT Hash = 0;
	int i;

	while (i = *String++)
		Hash = ((Hash << 5) + Hash) + i; // Hash * 33 + i

	return Hash;
}

void SubtitleHasher::Add(const char* Subtitle)
{
	IScopedCriticalSection Guard(&Lock);
	if (Subtitle && strlen(Subtitle) > 1 && HasMatch(Subtitle) == false)
		Store.insert(CalculateHash(Subtitle));
}

bool SubtitleHasher::HasMatch(const char* Subtitle)
{
	IScopedCriticalSection Guard(&Lock);
	HashT Current = CalculateHash(Subtitle);
	return Store.find(Current) != Store.end();
}

void SubtitleHasher::Purge(void)
{
	IScopedCriticalSection Guard(&Lock);
	Store.clear();
}

void SubtitleHasher::Tick(void)
{
	IScopedCriticalSection Guard(&Lock);

	TickCounter.Update();
	TickReminder -= TickCounter.GetTimePassed();

	if (TickReminder <= 0.0f)
	{
		TickReminder = kPurgeInterval;

#ifndef NDEBUG
		_MESSAGE("SubtitleHasher::Tick - Tock!");
#endif
		// we need to periodically purge the hash store as we can't differentiate b'ween topic responses with the same dialog text but different voice assets
		// for instance, there may be two responses with the text "Hello there!" but only one with a valid voice file
		Purge();
	}
}

BSIStream* BSIStream::CreateInstance(const char* FilePath, void* ParentLocation)
{
	auto Instance = (BSIStream*)Heap_Allocate(0x20);		// standard bucket
	return CALL_MEMBER_FN(Instance, Ctor)(FilePath, ParentLocation);
}

override::MenuTopicManager* override::MenuTopicManager::GetSingleton(void)
{
	return (override::MenuTopicManager*)::MenuTopicManager::GetSingleton();
}
