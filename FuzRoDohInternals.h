#pragma once
#include "skse64/PluginAPI.h"
#include "skse64/skse64_common/skse_version.h"
#include "skse64/GameAPI.h"
#include "skse64/GameTypes.h"
#include "skse64/GameForms.h"
#include "skse64/GameEvents.h"
#include "skse64/GameData.h"
#include "skse64/GameSettings.h"

#include <SME_Prefix.h>
#include <INIManager.h>
#include <StringHelpers.h>
#include <MiscGunk.h>

#include "common/ICriticalSection.h"
#include <thread>
#include <chrono>
#include <unordered_set>

extern IDebugLog						gLog;

namespace interfaces
{
	extern PluginHandle					kPluginHandle;
	extern SKSEMessagingInterface*		kMsgInterface;
}


extern SME::INI::INISetting				kWordsPerSecondSilence;
extern SME::INI::INISetting				kSkipEmptyResponses;

#define MAKE_RVA(addr)		addr - 0x140000000i64


class FuzRoDohINIManager : public SME::INI::INIManager
{
public:
	void								Initialize(const char* INIPath, void* Paramenter) override;

	static FuzRoDohINIManager			Instance;
};

class SubtitleHasher
{
	static const double					kPurgeInterval;		// in ms

	using HashT = unsigned long;
	using HashListT = std::unordered_set<HashT>;

	mutable ICriticalSection			Lock;
	HashListT							Store;
	SME::MiscGunk::ElapsedTimeCounter	TickCounter;
	double								TickReminder;

	static HashT						CalculateHash(const char* String);

	void								Purge(void);
public:
	SubtitleHasher() : Lock(), Store(), TickCounter(), TickReminder(kPurgeInterval) {}

	void								Add(const char* Subtitle);
	bool								HasMatch(const char* Subtitle);
	void								Tick(void);

	static SubtitleHasher				Instance;
};

// 20
class BSIStream
{
public:
	MEMBER_FN_PREFIX(BSIStream);

	// E8 ? ? ? ? 90 33 DB 38 5C 24 38
	DEFINE_MEMBER_FN(Ctor, BSIStream*, MAKE_RVA(0x0000000140D927F0), const char* FilePath, void* ParentLocation);

	// members
	///*00*/ void**					vtbl;
	/*08*/ BSTSmartPointer<void>	unk04;		// actual file stream
	/*10*/ UInt8					valid;		// set to 1 if the stream's valid
	/*11*/ UInt8					pad09[7];
	/*18*/ StringCache::Ref			filePath;	// relative to the Data directory when no BSResource::Location's passed to the ctor (the game uses a static instance)
												// otherwise, use its location

	virtual void*					Dtor(bool FreeMemory = true);

	static BSIStream*				CreateInstance(const char* FilePath, void* ParentLocation = nullptr);		// BSResource::Location* ParentLocation
};
STATIC_ASSERT(sizeof(BSIStream) == 0x20);

// 10
template <typename NodeT>
class BSSimpleList
{
public:
	template <typename NodeT>
	struct ListNode
	{
		// members
		/*00*/ NodeT*					Data;
		/*08*/ ListNode<NodeT>*			Next;
	};

	// members
	/*00*/ ListNode<NodeT>				Head;
};
STATIC_ASSERT(sizeof(BSSimpleList<void>) == 0x10);

// arbitrary name, final cache that gets passed to the dialog playback subsystem
// 40
class CachedResponseData
{
public:
	// members
	/*00*/ BSString					responseText;
	/*10*/ UInt32					emotionType;
	/*14*/ UInt32					emotionLevel;
	/*18*/ StringCache::Ref			voiceFilePath;		// relative path to the voice file
	/*20*/ UInt64					unk20;				// speaker idle anim?
	/*28*/ UInt64					unk28;				// listener idle anim?
	/*30*/ UInt64					unk30;
	/*38*/ UInt8					useEmotionAnim;
	/*39*/ UInt8					hasLipFile;
	/*3A*/ UInt8					pad22[6];
};
STATIC_ASSERT(sizeof(CachedResponseData) == 0x40);

// arbitrary name, used to queue subtitles for gamemode conversations (outside the standard dialog menu; NPC-NPC or NPC-PC)
// 20
class NPCChatterData
{
public:
	// members
	/*00*/ UInt32					speaker;				// the BSHandleRefObject handle to the speaker
	/*08*/ BSString					title;
	/*18*/ float					subtitleDistance;		// init to float::MAX
	/*1C*/ UInt8					forceSubtitles;
	/*1D*/ UInt8					pad11[3];
};
STATIC_ASSERT(sizeof(NPCChatterData) == 0x20);

class PlayerDialogData;

using CachedResponseListT = BSSimpleList<CachedResponseData>;
using PlayerTopicListT = BSSimpleList<PlayerDialogData>;
using DialogBranchArrayT = tArray<BGSDialogueBranch>;
using TopicArrayT = tArray<TESTopic>;

// arbitrary name, the actual class is probably a member of MenuTopicManager and not limited to the player
// 58
class PlayerDialogData
{
public:
	// members
	/*00*/ BSString					title;
	/*10*/ UInt8					unk08;
	/*11*/ UInt8					unk09;
	/*12*/ UInt8					unk0A;
	/*13*/ UInt8					pad0B[5];
	/*18*/ CachedResponseListT		responses;
	/*28*/ TESQuest*				parentQuest;
	/*30*/ TESTopicInfo*			parentTopicInfo;
	/*38*/ TESTopic*				parentTopic;
	/*40*/ CachedResponseListT*		unk20;				// seen pointing to this::unk0C
	/*48*/ UInt8					unk24;
	/*49*/ UInt8					pad25;
	/*4A*/ UInt8					unk26;
	/*4B*/ UInt8					pad27[5];
	/*50*/ TESTopic*				unk28;				// seen caching parentTopic
};
STATIC_ASSERT(offsetof(PlayerDialogData, responses) == 0x18);
STATIC_ASSERT(offsetof(PlayerDialogData, unk26) == 0x4A);
STATIC_ASSERT(sizeof(PlayerDialogData) == 0x58);

namespace override
{
	// E0
	class MenuTopicManager
	{
	public:
		// members
		///*00*/ void**					vtbl;
		/*08*/ BSTEventSink<void*>		unk04;					// BSTEventSink<PositionPlayerEvent>
		/*10*/ UInt64					unk08;
		/*18*/ PlayerTopicListT*		selectedResponseNode;	// points to the ListNode that refers to the PlayerDialogData instance of the selected topicinfo
		/*20*/ PlayerTopicListT*		availableResponses;
		/*28*/ TESTopicInfo*			unk14;
		/*30*/ TESTopicInfo*			rootTopicInfo;
		/*38*/ PlayerDialogData*		lastSelectedResponse;
		/*40*/ CRITICAL_SECTION			topicManagerCS;
		/*68*/ UInt32					speaker;				// a BSHandleRefObject handle to the speaker
		/*6C*/ UInt32					refHandle3C;			// same as above
		/*70*/ UInt64					unk40;
		/*78*/ UInt64					unk44;
		/*80*/ DialogBranchArrayT		unk48;
		/*98*/ DialogBranchArrayT		unk54;
		/*B0*/ UInt8					unk60;
		/*B1*/ UInt8					unk61;
		/*B2*/ UInt8					unk62;
		/*B3*/ UInt8					unk63;
		/*B4*/ UInt8					unk64;
		/*B5*/ UInt8					unk65;
		/*B6*/ UInt8					unk66;
		/*B7*/ UInt8					unk67;
		/*B8*/ UInt8					unk68;
		/*B9*/ UInt8					unk69;
		/*BA*/ UInt8					unk6A;					// init to 1
		/*BB*/ UInt8					unk6B;
		/*C0*/ TopicArrayT				unk6C;
		/*D8*/ UInt64					unkD8;

		// methods
		virtual void*					Dtor(bool FreeMemory = true);

		static MenuTopicManager*		GetSingleton(void);
	};
	STATIC_ASSERT(sizeof(MenuTopicManager) == 0xE0);
}

std::string			MakeSillyName();
bool				CanShowDialogSubtitles();
bool				CanShowGeneralSubtitles();
