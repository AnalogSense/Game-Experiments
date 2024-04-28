#include <iostream>
#include <thread>

#include <AssemblyBuilder.hpp>
#include <AllocRaiiRemote.hpp>
#include <DelayedCtor.hpp>
#include <Fiber.hpp>
#include <joaat.hpp>
#include <kbRgbWooting.hpp>
#include <Pattern.hpp>
#include <Pointer.hpp>
#include <Process.hpp>
#include <ProcessHandle.hpp>
#include <Window.hpp>
#include <wooting_enums.hpp>

using namespace soup;

static DWORD prev_focus_pid = -1;
static UniquePtr<Process> proc;
static std::shared_ptr<ProcessHandle> h;
static DelayedCtor<Fiber> fib;
static bool fib_ret;

static void YIELD_VALUE(bool x)
{
	fib_ret = x;
	Fiber::current()->yield();
}

[[nodiscard]] static bool isAppropriateGameProcess(const std::string& name)
{
	switch (joaat::hash(name))
	{
	case joaat::hash("Cyberpunk2077.exe"):
	case joaat::hash("Warframe.x64.exe"):
		return true;
	}
	return false;
}

[[nodiscard]] static bool isInVehicleContext()
{
	if (proc->name == "Cyberpunk2077.exe")
	{
		if (!fib.isConstructed())
		{
			fib.construct([](Capture&&)
			{
				Pointer pMountingFacility;
				for (; !pMountingFacility; YIELD_VALUE(false))
				{
					Pointer pGameEngine = h->externalRead<void*>(h->externalScan(Pattern("48 8B 0D ? ? ? ? BA BE FF FF FF")).add(3).externalRip(*h));
					Pointer pGameFramework = h->externalRead<void*>(pGameEngine.add(0x308));
					Pointer pGameInstance = h->externalRead<void*>(pGameFramework.add(0x10));
					if (!pGameInstance)
					{
						continue;
					}
					Pointer pGameInstance_vft = h->externalRead<void*>(pGameInstance);
					Pointer pGameInstance_QuerySystem = h->externalRead<void*>(pGameInstance_vft.add(8));
					Pointer pMountingFacilityClassDesc = h->externalRead<void*>(h->externalScan(Pattern("48 8B 15 ? ? ? ? 48 89 5D 98 48 89 45 90 48 8B 01 FF 50 08")).add(3).externalRip(*h)); // Pattern is in PhotoModeSystem::InitializePhotoModeContext, found via "BaseStatusEffect.CyberspacePresence"

					auto retvalarea = h->allocate(8);
					AssemblyBuilder ab;

					ab.funcBegin();

					// RAX = pGameInstance_QuerySystem(pGameInstance, pMountingFacilityClassDesc)
					ab.setA(pGameInstance_QuerySystem.as<uintptr_t>());
					ab.setC(pGameInstance.as<uintptr_t>());
					ab.setD(pMountingFacilityClassDesc.as<uintptr_t>());
					ab.callA();

					// *retvalarea->p = RAX
					ab.setC(retvalarea->p.as<uintptr_t>());
					ab.setU64fromCtoA();

					ab.funcEnd();

					auto remote_bytecode = h->copyInto(ab.data(), ab.size());
					h->executeSync(remote_bytecode->p.as<void*>(), 0xDEADDEADDEADDEAD);

					pMountingFacility = h->externalRead<void*>(retvalarea->p);
				}

				Pointer pChildIds = pMountingFacility.add(0x60);
				Pointer pChildIds_size = pMountingFacility.add(0x6C);
				while (true)
				{
					Pointer pChildIdsArr = h->externalRead<void*>(pChildIds);
					constexpr uint64_t PLAYER_PED_ID = 1;
					bool player_is_mount_child = false;
					for (uint32_t i = 0; i != h->externalRead<uint32_t>(pChildIds_size); ++i)
					{
						if (h->externalRead<uint64_t>(pChildIdsArr.add(i * 8)) == PLAYER_PED_ID)
						{
							player_is_mount_child = true;
							break;
						}
					}
					YIELD_VALUE(player_is_mount_child);
				}
			});
		}
	}

	if (fib)
	{
		fib->run();
		return fib_ret;
	}
	return false;
}

[[nodiscard]] static uint8_t getAppropriateProfile()
{
	DWORD focus_pid = Window::getFocused().getOwnerPid();
	if (focus_pid != prev_focus_pid)
	{
		prev_focus_pid = focus_pid;
		proc = Process::get(focus_pid);
		if (proc && isAppropriateGameProcess(proc->name))
		{
			h = proc->open();
			std::cout << "Appropriate game process detected: " << proc->name << "\n";
		}
		else
		{
			if (h)
			{
				h.reset();
				fib.reset();
				std::cout << "Game process no longer active/focused\n";
			}
		}
	}

	if (h)
	{
		if (isInVehicleContext())
		{
			return 2;
		}
		return 3;
	}
	return 0;
}

static void sendProfileSwitchToHardware(uint8_t profile)
{
	for (auto& _kbd : kbRgb::getAll(false))
	{
		if (_kbd->isWooting())
		{
			auto kbd = _kbd->asWooting();
			if (!kbd->isUwu())
			{
				std::cout << "Sending report to " << kbd->name << "...";
				{
					Buffer buf(8);
					buf.push_back(/* 0 */ 0); // HID report index
					buf.push_back(/* 1 */ 0xD0); // Magic word
					buf.push_back(/* 2 */ 0xDA); // Magic word
					buf.push_back(/* 3 */ (uint8_t)WootingCommand::ActivateProfile);
					buf.push_back(/* 4 */ profile);
					buf.push_back(/* 5 */ 0);
					buf.push_back(/* 6 */ 0);
					buf.push_back(/* 7 */ 0);
					kbd->hid.sendFeatureReport(std::move(buf));
				}
				SOUP_UNUSED(kbd->hid.receiveReport());
				std::cout << " Success.\n";
			}
		}
	}
}

int main()
{
	uint8_t profile = -1;
	while (true)
	{
		auto appropriate_profile = getAppropriateProfile();
		if (appropriate_profile != profile)
		{
			profile = appropriate_profile;
			std::cout << "Switching to profile " << (int)(profile + 1) << "\n";
			sendProfileSwitchToHardware(profile);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
