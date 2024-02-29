#include <iostream>
#include <thread>

#include <DelayedCtor.hpp>
#include <Fiber.hpp>
#include <joaat.hpp>
#include <kbRgbWooting.hpp>
#include <Module.hpp>
#include <Pattern.hpp>
#include <Pointer.hpp>
#include <Process.hpp>
#include <Window.hpp>
#include <wooting_enums.hpp>

using namespace soup;

static DWORD prev_focus_pid = -1;
static UniquePtr<Process> proc;
static std::shared_ptr<Module> mod;
static DelayedCtor<Fiber> fib;
static bool fib_ret;

#define YIELD_VALUE(x) fib_ret = x; Fiber::current()->yield();

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
				auto func = mod->externalScan(Pattern("48 89 5C 24 10 4C 89 74 24 18 48 89 4C 24 08 55"));
				auto player_info = func.add((0x0000000140F886F9 - 0x0000000140F88678) + 3).externalRip(*mod);
				while (true)
				{
					if (Pointer player_ptr = mod->externalRead<void*>(player_info.add(0x10)))
					{
						if (Pointer player_ped_ptr = mod->externalRead<void*>(player_ptr.add(0x5B0)))
						{
							if (Pointer vehicle_ptr = mod->externalRead<void*>(player_ped_ptr.add(0x90)))
							{
								YIELD_VALUE(true);
								continue;
							}
						}
					}
					YIELD_VALUE(false);
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
			mod = proc->open();
			std::cout << "Appropriate game process detected: " << proc->name << "\n";
		}
		else
		{
			if (mod)
			{
				mod.reset();
				fib.reset();
				std::cout << "Game process no longer active/focused\n";
			}
		}
	}

	if (mod)
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
