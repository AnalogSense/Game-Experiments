#include <Windows.h>

#include <iostream>

#define export __declspec(dllexport)

using wooting_analog_read_analog_t = float(*)(unsigned short code);
static wooting_analog_read_analog_t read_analog;

struct ISteamInput_vtbl
{
	void* Init; // 0
	void* Shutdown; // 1
	void* SetInputActionManifestFilePath; // 2
	void* RunFrame; // 3
	void* BWaitForData; // 4
	void* BNewDataAvailable; // 5
	void* GetConnectedControllers; // 6
	void* EnableDeviceCallbacks; // 7
	void* EnableActionEventCallbacks; // 8
	void* GetActionSetHandle; // 9
	void* ActivateActionSet; // 10
	void* GetCurrentActionSet; // 11
	void* ActivateActionSetLayer; // 12
	void* DeactivateActionSetLayer; // 13
	void* DeactivateAllActionSetLayers; // 14
	void* GetActiveActionSetLayers; // 15
	void* GetDigitalActionHandle; // 16 (0x80)
	void* GetDigitalActionData; // 17
	void* GetDigitalActionOrigins; // 18
	void* GetStringForDigitalActionName; // 19
	void* GetAnalogActionHandle; // 20 (0xA0)
	void* GetAnalogActionData; // 21 (0xA8)
	void* GetAnalogActionOrigins; // 22
	void* GetGlyphPNGForActionOrigin; // 23
	void* GetGlyphSVGForActionOrigin; // 24
	void* GetGlyphForActionOrigin_Legacy; // 25
	void* GetStringForActionOrigin; // 26
	void* GetStringForAnalogActionName; // 27
	void* StopAnalogActionMomentum; // 28
	void* GetMotionData; // 29
	void* TriggerVibration; // 30
	void* TriggerVibrationExtended; // 31
	void* TriggerSimpleHapticEvent; // 32
	void* SetLEDColor; // 33
	void* Legacy_TriggerHapticPulse; // 34
	void* Legacy_TriggerRepeatedHapticPulse; // 35
	void* ShowBindingPanel; // 36
	void* GetInputTypeForHandle; // 37
	void* GetControllerForGamepadIndex; // 38
	void* GetGamepadIndexForController; // 39
	void* GetStringForXboxOrigin; // 40
	void* GetGlyphForXboxOrigin; // 41
	void* GetActionOriginFromXboxOrigin; // 42
	void* TranslateActionOrigin; // 43
	void* GetDeviceBindingRevision; // 44
	void* GetRemotePlaySessionID; // 45
	void* GetSessionInputConfigurationSettings; // 46
	void* SetDualSenseTriggerEffect; // 47
};

struct ISteamInput
{
	ISteamInput_vtbl* vtbl;
};

enum EInputSourceMode
{
	k_EInputSourceMode_None, // 0
	k_EInputSourceMode_Dpad, // 1
	k_EInputSourceMode_Buttons, // 2
	k_EInputSourceMode_FourButtons, // 3
	k_EInputSourceMode_AbsoluteMouse, // 4
	k_EInputSourceMode_RelativeMouse, // 5
	k_EInputSourceMode_JoystickMove, // 6
	k_EInputSourceMode_JoystickMouse, // 7
	k_EInputSourceMode_JoystickCamera, // 8
	k_EInputSourceMode_ScrollWheel, // 9
	k_EInputSourceMode_Trigger, // 10
	k_EInputSourceMode_TouchMenu, // 11
	k_EInputSourceMode_MouseJoystick, // 12
	k_EInputSourceMode_MouseRegion, // 13
	k_EInputSourceMode_RadialMenu, // 14
	k_EInputSourceMode_SingleButton, // 15
	k_EInputSourceMode_Switches // 16
};

#pragma pack(push, 1)
struct InputAnalogActionData_t
{
	// Type of data coming from this action, this will match what got specified in the action set
	EInputSourceMode eMode;
	
	// The current state of this action; will be delta updates for mouse actions
	float x, y;
	
	// Whether or not this action is currently available to be bound in the active action set
	bool bActive;
};

struct InputDigitalActionData_t
{
	// The current state of this action; will be true if currently pressed
	bool bState;
	
	// Whether or not this action is currently available to be bound in the active action set
	bool bActive;
};
#pragma pack(pop)

using InputHandle_t = uint64_t;
using InputDigitalActionHandle_t = uint64_t;
using InputAnalogActionHandle_t = uint64_t;

static InputDigitalActionHandle_t dhCrouch = -1;

using GetDigitalActionHandle_t = InputDigitalActionHandle_t(*)(ISteamInput* _this, const char* pszActionName);
static GetDigitalActionHandle_t og_GetDigitalActionHandle;
static InputDigitalActionHandle_t GetDigitalActionHandle_detour(ISteamInput* _this, const char* pszActionName)
{
	auto ret = og_GetDigitalActionHandle(_this, pszActionName);
	std::cout << "Digital action '" << pszActionName << "' got handle " << ret << "\n";
	if (strcmp(pszActionName, "crouch") == 0)
	{
		dhCrouch = ret;
	}
	return ret;
}

using GetDigitalActionData_t = void(*)(ISteamInput* _this, InputDigitalActionData_t* out, InputHandle_t inputHandle, InputDigitalActionHandle_t digitalActionHandle);
static GetDigitalActionData_t og_GetDigitalActionData;
static void GetDigitalActionData_detour(ISteamInput* _this, InputDigitalActionData_t* out, InputHandle_t inputHandle, InputDigitalActionHandle_t digitalActionHandle)
{
	og_GetDigitalActionData(_this, out, inputHandle, digitalActionHandle);
	if (digitalActionHandle == dhCrouch)
	{
		out->bState = (GetAsyncKeyState(VK_CONTROL) & 0x8000);
	}
}

static InputAnalogActionHandle_t ahMove = -1;
static InputAnalogActionHandle_t ahThrottle = -1;
static InputAnalogActionHandle_t ahBrake = -1;

using GetAnalogActionHandle_t = InputAnalogActionHandle_t(*)(ISteamInput* _this, const char* pszActionName);
static GetAnalogActionHandle_t og_GetAnalogActionHandle;
static InputAnalogActionHandle_t GetAnalogActionHandle_detour(ISteamInput* _this, const char* pszActionName)
{
	auto ret = og_GetAnalogActionHandle(_this, pszActionName);
	std::cout << "Analogue action '" << pszActionName << "' got handle " << ret << "\n";
	if (strcmp(pszActionName, "Move") == 0)
	{
		ahMove = ret;
	}
	else if (strcmp(pszActionName, "throttle") == 0)
	{
		ahThrottle = ret;
	}
	else if (strcmp(pszActionName, "brake") == 0)
	{
		ahBrake = ret;
	}
	return ret;
}

using GetAnalogActionData_t = void(*)(ISteamInput* _this, InputAnalogActionData_t* out, InputHandle_t inputHandle, InputAnalogActionHandle_t analogActionHandle);
static GetAnalogActionData_t og_GetAnalogActionData;
static void GetAnalogActionData_detour(ISteamInput* _this, InputAnalogActionData_t* out, InputHandle_t inputHandle, InputAnalogActionHandle_t analogActionHandle)
{
	og_GetAnalogActionData(_this, out, inputHandle, analogActionHandle);
	if (analogActionHandle == ahMove)
	{
		//std::cout << out->x << ", " << out->y << ", " << out->bActive << "\n";
		out->y = read_analog('W') - read_analog('S');
		out->x = read_analog('D') - read_analog('A');
	}
	else if (analogActionHandle == ahThrottle)
	{
		out->x = read_analog('W');
	}
	else if (analogActionHandle == ahBrake)
	{
		out->x = read_analog('S');
	}
}

static HWND g_hwnd{};

static WNDPROC og_wndproc;
static LRESULT CALLBACK WndProc_detour(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN || msg == WM_CHAR || msg == WM_KEYUP || msg == WM_SYSKEYUP)
	{
		const UINT scancode = ((lparam >> 16) & 0b11111111);
		unsigned int vk = MapVirtualKeyExA(scancode, MAPVK_VSC_TO_VK, GetKeyboardLayout(0));
		if (vk == 'W' || vk == 'A' || vk == 'S' || vk == 'D' || vk == VK_CONTROL)
		{
			return 0;
		}
	}
	return CallWindowProcW(og_wndproc, hwnd, msg, wparam, lparam);
}

static void do_vtbl_hook(void** pp, void* detour)
{
	DWORD oldProtect;
	VirtualProtect((void*)pp, sizeof(void*), PAGE_READWRITE, &oldProtect);
	*pp = detour;
	VirtualProtect((void*)pp, sizeof(void*), oldProtect, &oldProtect);
}

extern "C"
{
	using SteamInternal_FindOrCreateUserInterface_t = void*(*)(unsigned int HSteamUser, const char* ifname);
	static SteamInternal_FindOrCreateUserInterface_t og_SteamInternal_FindOrCreateUserInterface;
	export void* SteamInternal_FindOrCreateUserInterface(unsigned int HSteamUser, const char* ifname)
	{
		auto ret = og_SteamInternal_FindOrCreateUserInterface(HSteamUser, ifname);
		std::cout << "User interface '" << ifname << "' allocated at " << (void*)ret << "\n";
		if (strcmp(ifname, "SteamInput006") == 0)
		{
			g_hwnd = GetForegroundWindow();

			og_GetDigitalActionHandle = (GetDigitalActionHandle_t)reinterpret_cast<ISteamInput*>(ret)->vtbl->GetDigitalActionHandle;
			do_vtbl_hook(&reinterpret_cast<ISteamInput*>(ret)->vtbl->GetDigitalActionHandle, (void*)&GetDigitalActionHandle_detour);
			og_GetDigitalActionData = (GetDigitalActionData_t)reinterpret_cast<ISteamInput*>(ret)->vtbl->GetDigitalActionData;
			do_vtbl_hook(&reinterpret_cast<ISteamInput*>(ret)->vtbl->GetDigitalActionData, (void*)&GetDigitalActionData_detour);
			og_GetAnalogActionHandle = (GetAnalogActionHandle_t)reinterpret_cast<ISteamInput*>(ret)->vtbl->GetAnalogActionHandle;
			do_vtbl_hook(&reinterpret_cast<ISteamInput*>(ret)->vtbl->GetAnalogActionHandle, (void*)&GetAnalogActionHandle_detour);
			og_GetAnalogActionData = (GetAnalogActionData_t)reinterpret_cast<ISteamInput*>(ret)->vtbl->GetAnalogActionData;
			do_vtbl_hook(&reinterpret_cast<ISteamInput*>(ret)->vtbl->GetAnalogActionData, (void*)&GetAnalogActionData_detour);
			std::cout << "Hooked ISteamInput vtbl.\n";

			og_wndproc = (WNDPROC)SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&WndProc_detour));
			std::cout << "Hooked WndProc.\n";
		}
		return ret;
	}

	// The compiler doesn't touch any registers (other than RSP) so this "perfectly forwards" arguments & return values.
	static FARPROC og_SteamAPI_GetHSteamUser; export void SteamAPI_GetHSteamUser() { og_SteamAPI_GetHSteamUser(); }
	static FARPROC og_SteamInternal_ContextInit; export void SteamInternal_ContextInit() { og_SteamInternal_ContextInit(); }
	static FARPROC og_SteamAPI_RunCallbacks; export void SteamAPI_RunCallbacks() { og_SteamAPI_RunCallbacks(); }
	static FARPROC og_SteamInternal_CreateInterface; export void SteamInternal_CreateInterface() { og_SteamInternal_CreateInterface(); }
	static FARPROC og_SteamAPI_RegisterCallback; export void SteamAPI_RegisterCallback() { og_SteamAPI_RegisterCallback(); }
	static FARPROC og_SteamAPI_UnregisterCallback; export void SteamAPI_UnregisterCallback() { og_SteamAPI_UnregisterCallback(); }
	static FARPROC og_SteamAPI_RegisterCallResult; export void SteamAPI_RegisterCallResult() { og_SteamAPI_RegisterCallResult(); }
	static FARPROC og_SteamAPI_UnregisterCallResult; export void SteamAPI_UnregisterCallResult() { og_SteamAPI_UnregisterCallResult(); }
	static FARPROC og_SteamAPI_Init; export void SteamAPI_Init() { og_SteamAPI_Init(); }
	static FARPROC og_SteamAPI_Shutdown; export void SteamAPI_Shutdown() { og_SteamAPI_Shutdown(); }
}

static HMODULE og_lib;
static HMODULE wooting_lib;

using wooting_analog_initialise_t = int(*)();
using wooting_analog_uninitialise_t = int(*)();
using wooting_analog_set_keycode_mode_t = int(*)(int mode);

#define LOGGING false

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hInstance);
#if LOGGING
		AllocConsole();
		{
			FILE* f;
			freopen_s(&f, "CONIN$", "r", stdin);
			freopen_s(&f, "CONOUT$", "w", stderr);
			freopen_s(&f, "CONOUT$", "w", stdout);
		}
#endif
		og_lib = LoadLibraryA("steam_api64_og");
		std::cout << "steam_api64_og loaded at " << (void*)og_lib << "\n";
		if (!og_lib)
		{
			MessageBoxA(0, "Failed to load steam_api64_og", 0, 0);
			return FALSE;
		}
		og_SteamInternal_FindOrCreateUserInterface = (SteamInternal_FindOrCreateUserInterface_t)GetProcAddress(og_lib, "SteamInternal_FindOrCreateUserInterface");
		og_SteamAPI_GetHSteamUser = GetProcAddress(og_lib, "SteamAPI_GetHSteamUser");
		og_SteamInternal_ContextInit = GetProcAddress(og_lib, "SteamInternal_ContextInit");
		og_SteamAPI_RunCallbacks = GetProcAddress(og_lib, "SteamAPI_RunCallbacks");
		og_SteamInternal_CreateInterface = GetProcAddress(og_lib, "SteamInternal_CreateInterface");
		og_SteamAPI_RegisterCallback = GetProcAddress(og_lib, "SteamAPI_RegisterCallback");
		og_SteamAPI_UnregisterCallback = GetProcAddress(og_lib, "SteamAPI_UnregisterCallback");
		og_SteamAPI_RegisterCallResult = GetProcAddress(og_lib, "SteamAPI_RegisterCallResult");
		og_SteamAPI_UnregisterCallResult = GetProcAddress(og_lib, "SteamAPI_UnregisterCallResult");
		og_SteamAPI_Init = GetProcAddress(og_lib, "SteamAPI_Init");
		og_SteamAPI_Shutdown = GetProcAddress(og_lib, "SteamAPI_Shutdown");
		wooting_lib = LoadLibraryA("wooting_analog_sdk");
		if (!wooting_lib)
		{
			MessageBoxA(0, "Failed to load Wooting Analog SDK", 0, 0);
			return FALSE;
		}
		std::cout << "wooting_analog_sdk loaded at " << (void*)wooting_lib << "\n";
		{
			auto ret = ((wooting_analog_initialise_t)GetProcAddress(wooting_lib, "wooting_analog_initialise"))();
			std::cout << "wooting_analog_initialise returned " << ret << "\n";
			if (ret < 0)
			{
				MessageBoxA(0, "Failed to initialise Wooting Analog SDK", 0, 0);
				return FALSE;
			}
		}
		((wooting_analog_set_keycode_mode_t)GetProcAddress(wooting_lib, "wooting_analog_set_keycode_mode"))(2); // VirtualKey
		read_analog = (wooting_analog_read_analog_t)GetProcAddress(wooting_lib, "wooting_analog_read_analog");
		break;

	case DLL_PROCESS_DETACH:
		((wooting_analog_uninitialise_t)GetProcAddress(wooting_lib, "wooting_analog_uninitialise"))();
		FreeLibrary(og_lib);
		FreeLibrary(wooting_lib);
#if LOGGING
		FreeConsole();
#endif
		break;
	}
	return TRUE;
}
