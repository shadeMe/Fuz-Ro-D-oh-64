#include "Hooks.h"
#include "xbyak/xbyak.h"
#include "skse64_common/BranchTrampoline.h"
#include "skse64_common/Relocation.h"

namespace hookedAddresses
{
	// E8 ? ? ? ? 48 8B F8 EB 02 33 FF 48 85 FF
	RelocAddr<uintptr_t>	kCachedResponseData_Ctor(MAKE_RVA(0x0000000140591080));
	uintptr_t				kCachedResponseData_Ctor_Hook = kCachedResponseData_Ctor + 0xEC;
	uintptr_t				kCachedResponseData_Ctor_Ret = kCachedResponseData_Ctor + 0xF1;

	// E8 ? ? ? ? 8B 06 EB 09
	RelocAddr<uintptr_t>	kUIUtils_QueueDialogSubtitles(MAKE_RVA(0x00000001409184B0));
	uintptr_t				kUIUtils_QueueDialogSubtitles_Hook = kUIUtils_QueueDialogSubtitles + 0x4D;
	uintptr_t				kUIUtils_QueueDialogSubtitles_Show = kUIUtils_QueueDialogSubtitles + 0x5A;
	uintptr_t				kUIUtils_QueueDialogSubtitles_Exit = kUIUtils_QueueDialogSubtitles + 0x103;

	// E8 ? ? ? ? 84 C0 75 42 48 8B 35 ? ? ? ?
	RelocAddr<uintptr_t>	kASCM_DisplayQueuedNPCChatterData(MAKE_RVA(0x000000014090E800));
	uintptr_t				kASCM_DisplayQueuedNPCChatterData_DialogSubs_Hook = kASCM_DisplayQueuedNPCChatterData + 0x1CA;
	uintptr_t				kASCM_DisplayQueuedNPCChatterData_DialogSubs_Show = kASCM_DisplayQueuedNPCChatterData + 0x1D3;
	uintptr_t				kASCM_DisplayQueuedNPCChatterData_DialogSubs_Exit = kASCM_DisplayQueuedNPCChatterData + 0x1FD;

	uintptr_t				kASCM_DisplayQueuedNPCChatterData_GeneralSubs_Hook = kASCM_DisplayQueuedNPCChatterData + 0x99;
	uintptr_t				kASCM_DisplayQueuedNPCChatterData_GeneralSubs_Show = kASCM_DisplayQueuedNPCChatterData + 0xA6;
	uintptr_t				kASCM_DisplayQueuedNPCChatterData_GeneralSubs_Exit = kASCM_DisplayQueuedNPCChatterData + 0x1CA;

	// E8 ? ? ? ? F3 0F 10 35 ? ? ? ? 48 8D 4E 28
	RelocAddr<uintptr_t>	kASCM_QueueNPCChatterData(MAKE_RVA(0x000000014090E150));
	uintptr_t				kASCM_QueueNPCChatterData_Hook = kASCM_QueueNPCChatterData + 0x85;
	uintptr_t				kASCM_QueueNPCChatterData_Show = kASCM_QueueNPCChatterData + 0x92;
	uintptr_t				kASCM_QueueNPCChatterData_Exit = kASCM_QueueNPCChatterData + 0xCA;
}


void SneakAtackVoicePath(CachedResponseData* Data, char* VoicePathBuffer)
{
	// overwritten code
	CALL_MEMBER_FN(&Data->voiceFilePath, Set)(VoicePathBuffer);

	if (strlen(VoicePathBuffer) < 17)
		return;

	std::string FUZPath(VoicePathBuffer), WAVPath(VoicePathBuffer), XWMPath(VoicePathBuffer);
	WAVPath.erase(0, 5);

	FUZPath.erase(0, 5);
	FUZPath.erase(FUZPath.length() - 3, 3);
	FUZPath.append("fuz");

	XWMPath.erase(0, 5);
	XWMPath.erase(XWMPath.length() - 3, 3);
	XWMPath.append("xwm");

	BSIStream* WAVStream = BSIStream::CreateInstance(WAVPath.c_str());
	BSIStream* FUZStream = BSIStream::CreateInstance(FUZPath.c_str());
	BSIStream* XWMStream = BSIStream::CreateInstance(XWMPath.c_str());

#if 0
	_MESSAGE("Expected: %s", VoicePathBuffer);
	gLog.Indent();
	_MESSAGE("WAV Stream [%s] Validity = %d", WAVPath.c_str(), WAVStream->valid);
	_MESSAGE("FUZ Stream [%s] Validity = %d", FUZPath.c_str(), FUZStream->valid);
	_MESSAGE("XWM Stream [%s] Validity = %d", XWMPath.c_str(), XWMStream->valid);
	gLog.Outdent();
#endif

#ifndef NDEBUG
	if (!(WAVStream->valid == 0 && FUZStream->valid == 0 && XWMStream->valid == 0))
#else
	if (WAVStream->valid == 0 && FUZStream->valid == 0 && XWMStream->valid == 0)
#endif
	{
		static const int kWordsPerSecond = kWordsPerSecondSilence.GetData().i;
		static const int kCharacterPerWord = kWideCharacterPerWord.GetData().i;
		static const int kMaxSeconds = 10;

		int SecondsOfSilence = 2;
		char ShimAssetFilePath[0x104] = { 0 };
		std::string ResponseText(Data->responseText.Get());

		if (ResponseText.length() > 4 && strncmp(ResponseText.c_str(), "<ID=", 4))
		{
			int WordCount = 0;
			int WideCharCount = 0;
			int CharOver = 0;
			for (char ch : ResponseText) // check each character
			{
				if (CharOver > 0) // this char is part of a wide-character, pass it
				{
					CharOver --;
					continue;
				}
				if (ch & 0x80 && ch & 0x40 && ch & 0x20)
				{
					if (ch & 0x10)
					{
						CharOver = 3; // a 4 wide-character, 3 bytes left
					}
					else
					{
						CharOver = 2; // a 3 wide-character, 2 bytes left
					}
					WideCharCount ++;
					// What about 2 wide-character? These "2 wide-character languages" basically use spaces to separate words by my google
				}
				else
				{
					WordCount += (ch == ' ');
				}
			}
			WordCount += (WideCharCount / kCharacterPerWord);

			SecondsOfSilence = WordCount / ((kWordsPerSecond > 0) ? kWordsPerSecond : 2) + 1;

			if (SecondsOfSilence <= 0)
				SecondsOfSilence = 2;
			else if (SecondsOfSilence > kMaxSeconds)
				SecondsOfSilence = kMaxSeconds;

			// calculate the response text's hash and stash it for later lookups
			SubtitleHasher::Instance.Add(Data->responseText.Get());
		}

		if (ResponseText.length() > 1 || (ResponseText.length() == 1 && ResponseText[0] == ' ' && kSkipEmptyResponses.GetData().i == 0))
		{
			FORMAT_STR(ShimAssetFilePath, "Data\\Sound\\Voice\\Fuz Ro Doh\\Stock_%d.xwm", SecondsOfSilence);
			CALL_MEMBER_FN(&Data->voiceFilePath, Set)(ShimAssetFilePath);
#ifndef NDEBUG
			_MESSAGE("Missing Asset - Switching to '%s'", ShimAssetFilePath);
#endif
		}
	}

	WAVStream->Dtor();
	FUZStream->Dtor();
	XWMStream->Dtor();
}

bool ShouldForceSubs(NPCChatterData* ChatterData, UInt32 ForceRegardless, const char* Subtitle)
{
	bool Result = false;

	if (Subtitle && SubtitleHasher::Instance.HasMatch(Subtitle))		// force if the subtitle is for a voiceless response
	{
#ifndef NDEBUG
		_MESSAGE("Found a match for %s - Forcing subs", Subtitle);
#endif

		Result = true;
	}
	else if (ForceRegardless || (ChatterData && ChatterData->forceSubtitles))
		Result = true;
	else
	{
		TESTopicInfo* CurrentTopicInfo = nullptr;
		PlayerDialogData* Selection = nullptr;

		if (override::MenuTopicManager::GetSingleton()->selectedResponseNode)
			Selection = override::MenuTopicManager::GetSingleton()->selectedResponseNode->Head.Data;
		else
			Selection = override::MenuTopicManager::GetSingleton()->lastSelectedResponse;

		if (Selection)
			CurrentTopicInfo = Selection->parentTopicInfo;
		else if (override::MenuTopicManager::GetSingleton()->rootTopicInfo)
			CurrentTopicInfo = override::MenuTopicManager::GetSingleton()->rootTopicInfo;
		else
			CurrentTopicInfo = override::MenuTopicManager::GetSingleton()->unk14;

		if (CurrentTopicInfo)
		{
			if ((CurrentTopicInfo->dialogFlags >> 9) & 1)		// force subs flag's set
				Result = true;
		}
	}

	return Result;
}

#define PUSH_VOLATILE		push(rcx); push(rdx); push(r8); sub(rsp, 0x20);
#define POP_VOLATILE		add(rsp, 0x20); pop(r8); pop(rdx); pop(rcx); 

bool InstallHooks()
{
	if (!g_branchTrampoline.Create(1024 * 1))
	{
		_ERROR("Couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
		return false;
	}

	if (!g_localTrampoline.Create(1024 * 1, nullptr))
	{
		_ERROR("Couldn't create codegen buffer. this is fatal. skipping remainder of init process.");
		return false;
	}

	{
		struct HotswapReponseAssetPath_Code : Xbyak::CodeGenerator
		{
			HotswapReponseAssetPath_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label RetnLabel;

				push(rcx);
				push(rdx);		// asset path
				mov(rcx, rbx);	// cached response
				mov(rax, (uintptr_t)SneakAtackVoicePath);
				call(rax);
				pop(rdx);
				pop(rcx);
				jmp(ptr[rip + RetnLabel]);

			L(RetnLabel);
				dq(hookedAddresses::kCachedResponseData_Ctor_Ret);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		HotswapReponseAssetPath_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kCachedResponseData_Ctor_Hook, uintptr_t(Code.getCode()));
	}

	{
		struct UIUtilsQueueDialogSubs_Code : Xbyak::CodeGenerator
		{
			UIUtilsQueueDialogSubs_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label ShowLabel;

				mov(rax, (uintptr_t)CanShowDialogSubtitles);
				PUSH_VOLATILE;
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				PUSH_VOLATILE;
				xor(rcx, rcx);
				xor(rdx, rdx);
				mov(r8, r14);	// subtitle
				mov(rax, (uintptr_t)ShouldForceSubs);
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				jmp(ptr[rip]);
				dq(hookedAddresses::kUIUtils_QueueDialogSubtitles_Exit);

			L(ShowLabel);
				jmp(ptr[rip]);
				dq(hookedAddresses::kUIUtils_QueueDialogSubtitles_Show);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		UIUtilsQueueDialogSubs_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kUIUtils_QueueDialogSubtitles_Hook, uintptr_t(Code.getCode()));
	}

	{
		struct ASCMDisplayQueuedNPCChatterData_DialogSubs_Code : Xbyak::CodeGenerator
		{
			ASCMDisplayQueuedNPCChatterData_DialogSubs_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label ShowLabel;

				mov(rax, (uintptr_t)CanShowDialogSubtitles);
				PUSH_VOLATILE;
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				PUSH_VOLATILE;
				mov(rcx, rsi);
				xor(rdx, rdx);
				mov(r8, ptr[rsi + 0x8]);	// subtitle
				mov(rax, (uintptr_t)ShouldForceSubs);
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_DisplayQueuedNPCChatterData_DialogSubs_Exit);

			L(ShowLabel);
				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_DisplayQueuedNPCChatterData_DialogSubs_Show);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		ASCMDisplayQueuedNPCChatterData_DialogSubs_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kASCM_DisplayQueuedNPCChatterData_DialogSubs_Hook, uintptr_t(Code.getCode()));
	}

	{
		struct ASCMDisplayQueuedNPCChatterData_GeneralSubs_Code : Xbyak::CodeGenerator
		{
			ASCMDisplayQueuedNPCChatterData_GeneralSubs_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label ShowLabel;

				mov(rax, (uintptr_t)CanShowGeneralSubtitles);
				PUSH_VOLATILE;
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				PUSH_VOLATILE;
				mov(rcx, rsi);
				xor (rdx, rdx);
				mov(r8, ptr[rsi + 0x8]);	// subtitle
				mov(rax, (uintptr_t)ShouldForceSubs);
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_DisplayQueuedNPCChatterData_GeneralSubs_Exit);

			L(ShowLabel);
				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_DisplayQueuedNPCChatterData_GeneralSubs_Show);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		ASCMDisplayQueuedNPCChatterData_GeneralSubs_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kASCM_DisplayQueuedNPCChatterData_GeneralSubs_Hook, uintptr_t(Code.getCode()));
	}

	{
		struct ASCMQueueNPCChatterData_Code : Xbyak::CodeGenerator
		{
			ASCMQueueNPCChatterData_Code(void * buf) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label ShowLabel;

				mov(rax, (uintptr_t)CanShowDialogSubtitles);
				PUSH_VOLATILE;
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				PUSH_VOLATILE;
				xor(rcx, rcx);
				mov(rdx, r15d);
				mov(r8, rbp);	// subtitle
				mov(rax, (uintptr_t)ShouldForceSubs);
				call(rax);
				POP_VOLATILE;
				test(al, al);
				jnz(ShowLabel);

				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_QueueNPCChatterData_Exit);

			L(ShowLabel);
				jmp(ptr[rip]);
				dq(hookedAddresses::kASCM_QueueNPCChatterData_Show);
			}
		};

		void* CodeBuf = g_localTrampoline.StartAlloc();
		ASCMQueueNPCChatterData_Code Code(CodeBuf);
		g_localTrampoline.EndAlloc(Code.getCurr());

		g_branchTrampoline.Write5Branch(hookedAddresses::kASCM_QueueNPCChatterData_Hook, uintptr_t(Code.getCode()));
	}

	return true;
}
