#define YYSDK_PLUGIN
#include "CoinTools.hpp"	// Include our header
#include "ModInfo.h"		// Include our info file
#include <Windows.h>		// Include Windows's mess.
#include <vector>			// Include the STL vector.
#include <unordered_map>
#include <functional>
#include <fstream>
#include <chrono>
#include <iostream>
#include "json.hpp"
using json = nlohmann::json;

static struct Version {
	int major = VERSION_MAJOR;
	int minor = VERSION_MINOR;
	int build = VERSION_BUILD;
} version;

static struct Mod {
	Version version;
	const char* name = MOD_NAME;
} mod;

// CallBuiltIn is way too slow to use per frame. Need to investigate if there's a better way to call in built functions.

// We save the CodeCallbackHandler attributes here, so we can unregister the callback in the unload routine.
static CallbackAttributes_t* g_pFrameCallbackAttributes = nullptr;
static CallbackAttributes_t* g_pCodeCallbackAttributes = nullptr;

static uint32_t FrameNumber = 0;
static uint32_t enemiesKilled = 0;
static int gameTimer = 0;
static int currentRunCoins = 0;
static int haluBonusCoins = 0;
static int totalRunCoins = 0;
static int currentMinCoinsGained = 0;
static int prevCoinsGained = 0;
static int scoreFuncIndex = 0;
static const char* moddedVerStr;
static bool versionChanged = false;
static bool gameEnded = false;
static std::vector<const char*> currentStickers;
static std::vector<const char*> availableStickers;
static std::unordered_map<int, int> stickerVarMap;
static std::ofstream runLogFile;

// Config vars
static struct Config {
	bool logRuns = true;
} config;

void to_json(json& j, const Config& c) {
	j = json{ 
		{"logRuns", c.logRuns}
	};
}

void from_json(const json& j, Config& c) {
	j.at("logRuns").get_to(c.logRuns);
}

static std::unordered_map<int, const char*> codeIndexToName;
static std::unordered_map<int, std::function<void(YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags)>> codeFuncTable;

static const char* playStr = "Play Modded!";
RefString tempVar = RefString(playStr, strlen(playStr), false);
static bool versionTextChanged = false;

std::string GenerateRunFileName() {
    const auto now = std::chrono::system_clock::now();
    const auto timePoint = std::chrono::system_clock::to_time_t(now);
    const std::tm* currentTime = std::localtime(&timePoint);
    const int day = currentTime->tm_mday;
    const int month = currentTime->tm_mon + 1;
    const int year = currentTime->tm_year + 1900;
    const int hour = currentTime->tm_hour;
    const int minute = currentTime->tm_min;
    const int second = currentTime->tm_sec;

    char formattedDateTime[30];
    std::sprintf(formattedDateTime, "%04d-%02d-%02d_%02d.%02d.%02d", year, month, day, hour, minute, second);

    return std::string(formattedDateTime);
}

std::string formatString(const std::string& input) {
    std::string formattedString = input;

    // Convert all characters to lowercase
    for (char& c : formattedString) {
        c = std::tolower(c);
    }

    // Replace spaces with hyphens
    for (char& c : formattedString) {
        if (c == ' ') {
            c = '-';
        }
    }

    return formattedString;
}

void GenerateConfig(std::string fileName) {
	json data = config;

	std::ofstream configFile("modconfigs/" + fileName);
	if (configFile.is_open()) {
		PrintMessage(CLR_DEFAULT, "Config file \"%s\" created!", fileName.c_str());
		configFile << std::setw(4) << data << std::endl;
		configFile.close();
	} else {
		PrintError(__FILE__, __LINE__, "Error opening config file \"%s\"", fileName.c_str());
	}
}

// This callback is registered on EVT_PRESENT and EVT_ENDSCENE, so it gets called every frame on DX9 / DX11 games.
YYTKStatus FrameCallback(YYTKEventBase* pEvent, void* OptionalArgument)
{
	FrameNumber++;

	// Tell the core the handler was successful.
	return YYTK_OK;
}

// This callback is registered on EVT_CODE_EXECUTE, so it gets called every game function call.
YYTKStatus CodeCallback(YYTKEventBase* pEvent, void* OptionalArgument)
{
	YYTKCodeEvent* pCodeEvent = dynamic_cast<decltype(pCodeEvent)>(pEvent);

	std::tuple<CInstance*, CInstance*, CCode*, RValue*, int> args = pCodeEvent->Arguments();

	CInstance* Self = std::get<0>(args);
	CInstance* Other = std::get<1>(args);
	CCode* Code = std::get<2>(args);
	RValue* Res = std::get<3>(args);
	int			Flags = std::get<4>(args);

	if (!Code->i_pName)
	{
		return YYTK_INVALIDARG;
	}

	if (codeFuncTable.count(Code->i_CodeIndex) != 0) {
		codeFuncTable[Code->i_CodeIndex](pCodeEvent, Self, Other, Code, Res, Flags);
	}
	else // Haven't cached the function in the table yet. Run the if statements and assign the function to the code index
	{
		codeIndexToName[Code->i_CodeIndex] = Code->i_pName;
		if (_strcmpi(Code->i_pName, "gml_Object_obj_TitleScreen_Create_0") == 0) {
			auto TitleScreen_Create_0 = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				if (versionTextChanged == false) {
					YYRValue yyrv_version;
					CallBuiltin(yyrv_version, "variable_global_get", Self, Other, { "version" });
					std::string moddedVerStr = yyrv_version.operator std::string() + " (Modded)";
					CallBuiltin(yyrv_version, "variable_global_set", Self, Other, { "version", moddedVerStr.c_str() });
					versionTextChanged = true;
				}

				pCodeEvent->Call(Self, Other, Code, Res, Flags);
			};
			TitleScreen_Create_0(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = TitleScreen_Create_0;
		}
		else if (_strcmpi(Code->i_pName, "gml_Object_obj_TextController_Create_0") == 0) {
			auto TextController_Create_0 = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				YYRValue Result;
				pCodeEvent->Call(Self, Other, Code, Res, Flags);
				CallBuiltin(Result, "variable_global_get", Self, Other, { "TextContainer" });
				YYRValue tempResult;
				CallBuiltin(tempResult, "struct_get", Self, Other, { Result, "titleButtons" });
				YYRValue tempResultOne;
				CallBuiltin(tempResultOne, "struct_get", Self, Other, { tempResult, "eng" });

				tempResultOne.RefArray->m_Array[0].String = &tempVar;
			};
			TextController_Create_0(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = TextController_Create_0;
		}
		else if (_strcmpi(Code->i_pName, "gml_Object_obj_EnemyDead_Create_0") == 0)
		{
			auto EnemyDead_Create_0 = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				enemiesKilled++;
				pCodeEvent->Call(Self, Other, Code, Res, Flags);
			};
			EnemyDead_Create_0(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = EnemyDead_Create_0;
		}
		else if (_strcmpi(Code->i_pName, "gml_Object_obj_StageManager_Create_0") == 0) {
			auto StageManager_Create_0 = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				pCodeEvent->Call(Self, Other, Code, Res, Flags);
				enemiesKilled = 0;
				currentMinCoinsGained = 0;
				prevCoinsGained = 0;
				FrameNumber = 0;
				gameTimer = 0;
				gameEnded = false;
				PrintMessage(CLR_DEFAULT, "============================");
				PrintMessage(CLR_DEFAULT, "New run started - good luck!");
			};
			StageManager_Create_0(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = StageManager_Create_0;
		}
		else if (_strcmpi(Code->i_pName, "gml_Object_obj_StageManager_Step_0") == 0) {
			auto StageManager_Step_0 = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				// Block only runs if game timer is not paused (pause menu, level up, stamp, anvil, etc.)

				// Get global bool timePause
				YYRValue yyrv_timePause;
				pCodeEvent->Call(Self, Other, Code, Res, Flags);
				CallBuiltin(yyrv_timePause, "variable_global_get", Self, Other, { "timePause" });

				// Get global int currentRunMoneyGained
				YYRValue yyrv_currentRunMoneyGained;
				CallBuiltin(yyrv_currentRunMoneyGained, "variable_global_get", Self, Other, { "currentRunMoneyGained" });

				// Set current run money
				currentRunCoins = static_cast<int>(yyrv_currentRunMoneyGained);

				// Increment game timer
				gameTimer++;

				if (gameTimer % (60 * 10) == 0) {
					PrintMessage(CLR_RED, "KPS@%02d:%02d - %d", (gameTimer / (60 * 60)), (gameTimer / 60) % 60, enemiesKilled / 10);
					enemiesKilled = 0;
				}
				if (gameTimer % (60 * 60) == 0) {
					currentMinCoinsGained = (int) yyrv_currentRunMoneyGained - prevCoinsGained;
					prevCoinsGained = (int) yyrv_currentRunMoneyGained;

					// Turn currentMinCoinsGained into comma separated string
					std::string strValue = std::to_string(currentMinCoinsGained);
					std::string curMinCoins_string;
					int commaCount = 0;
					for (int i = strValue.length() - 1; i >= 0; i--) {
						curMinCoins_string = strValue[i] + curMinCoins_string;
						commaCount++;
						if (commaCount % 3 == 0 && i > 0) {
							curMinCoins_string = "," + curMinCoins_string;
						}
					}

					PrintMessage(CLR_YELLOW, "CPM@%02d:00 - %s", (gameTimer / (60 * 60)), curMinCoins_string);
				}
			};
			StageManager_Step_0(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = StageManager_Step_0;
		}
		else if (_strcmpi(Code->i_pName, "gml_Object_obj_Sticker_Collision_obj_Player") == 0) {
			auto Sticker_Collision_obj_Player = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				pCodeEvent->Call(Self, Other, Code, Res, Flags);
				YYRValue yyrv_collectedSticker;
				CallBuiltin(yyrv_collectedSticker, "variable_global_get", Self, Other, { "collectedSticker" });
				YYRValue yyrv_optionName;
				CallBuiltin(yyrv_optionName, "struct_get", Self, Other, { yyrv_collectedSticker, "optionName" });
				PrintMessage(CLR_BRIGHTPURPLE, "Sticker Name: %s", static_cast<const char*>(yyrv_optionName));
			};
			Sticker_Collision_obj_Player(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = Sticker_Collision_obj_Player;
		}
		else if (_strcmpi(Code->i_pName, "gml_Object_obj_PlayerManager_Create_0") == 0) {
			auto PlayerManager_Create_0 = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				pCodeEvent->Call(Self, Other, Code, Res, Flags);
				YYRValue yyrv_currentStickers;
				CallBuiltin(yyrv_currentStickers, "variable_global_get", Self, Other, { "currentStickers" });
				const RefDynamicArrayOfRValue* array = reinterpret_cast<const RefDynamicArrayOfRValue*>(yyrv_currentStickers.RefArray);
				std::vector<int> result;
				for (int i = 0; i < array->length; i++) {
					const RValue &element = array->m_Array[i];
					if (element.Kind == VALUE_REAL) {
						result.push_back(element.Real);
					}
				}

				for (int i = 0; i < result.size(); i++) {
					PrintMessage(CLR_BRIGHTPURPLE, "currentStickers[%d] = %d", i, result[i]);
				}
			};
			PlayerManager_Create_0(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = PlayerManager_Create_0;
		}
		// gml_Object_obj_PlayerManager_Create_0 - Line 1520
		// function Confirmed_gml_Object_obj_PlayerManager_Create_0() - Line 965
		else if (_strcmpi(Code->i_pName, "gml_Script_Confirmed_gml_Object_obj_PlayerManager_Create_0") == 0) {
			auto Confirmed_gml_Object_obj_PlayerManager_Create_0 = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				pCodeEvent->Call(Self, Other, Code, Res, Flags);
				PrintMessage(CLR_BRIGHTPURPLE, "Confirmed Triggered!");
			};
			Confirmed_gml_Object_obj_PlayerManager_Create_0(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = Confirmed_gml_Object_obj_PlayerManager_Create_0;
		}
		else if (_strcmpi(Code->i_pName, "gml_Object_obj_PlayerManager_Draw_64") == 0) {
			auto PlayerManager_Draw_64 = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				YYRValue yyrv_gameOverTime;
				pCodeEvent->Call(Self, Other, Code, Res, Flags);
				CallBuiltin(yyrv_gameOverTime, "variable_instance_get", Self, Other, { (long long)Self->i_id, "gameOverTime" });
				YYRValue yyrv_haluBonusCoins;
				CallBuiltin(yyrv_haluBonusCoins, "variable_instance_get", Self, Other, { (long long)Self->i_id, "haluBonusCoins" });
				if (static_cast<int>(yyrv_gameOverTime) >= 330 && gameEnded == false) {
					PrintMessage(CLR_DEFAULT, "Run Ended!");
					YYRValue yyrv_scriptIndex;
					CallBuiltin(yyrv_scriptIndex, "asset_get_index", Self, Other, { "gml_Script_CalculateScore" });
					scoreFuncIndex = static_cast<int>(yyrv_scriptIndex);
					YYRValue yyrv_score;
					CallBuiltin(yyrv_score, "script_execute", Self, Other, { (double) scoreFuncIndex, 0.0, 0.0, 0.0 });
					PrintMessage(CLR_DEFAULT, "Score: %d", static_cast<int>(yyrv_score));
					YYRValue yyrv_haluBonusCoins;
					CallBuiltin(yyrv_haluBonusCoins, "variable_instance_get", Self, Other, { (long long)Self->i_id, "haluBonusCoins" });
					haluBonusCoins = floor(static_cast<int>(yyrv_haluBonusCoins));
					totalRunCoins = floor(currentRunCoins + haluBonusCoins);
					PrintMessage(CLR_YELLOW, "Run Coins: %d + %d (HALU BONUS)", currentRunCoins, haluBonusCoins);
					PrintMessage(CLR_YELLOW, "Total:     %d", totalRunCoins);
					gameEnded = true;

					if (config.logRuns == true) {
						const wchar_t* dirName = L"runlogs";

						if (GetFileAttributes(dirName) == INVALID_FILE_ATTRIBUTES) {
							if (CreateDirectory(dirName, NULL)) {
								std::wcout << L"Directory \"runlogs\" created!" << std::endl;
							} else {
								std::cerr << "Failed to create the Run Logs directory. Error code: " << GetLastError() << std::endl;
								return;
							}
						}

						std::string fileName = GenerateRunFileName();

						runLogFile.open("runlogs/" + fileName + ".txt");
						if (runLogFile.is_open()) {
							runLogFile << "Run Data - " << fileName << std::endl;
							runLogFile << "Score: " << static_cast<int>(yyrv_score) << std::endl;
							runLogFile << "Run Coins: " << currentRunCoins << " + " << haluBonusCoins << " (HALU BONUS)" << std::endl;
							runLogFile << "Total:     " << totalRunCoins << std::endl;
							PrintMessage(CLR_DEFAULT, "Run log \"%s.txt\" created!", fileName.c_str());
							runLogFile.close();
						} else {
							PrintMessage(CLR_RED, "Error opening file \"%s.txt\"", fileName.c_str());
							return;
						}
					}
				}
			};
			PlayerManager_Draw_64(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = PlayerManager_Draw_64;
		}
		else // Not using this code function, so just quickly ignore it when it's called next time
		{
			auto UnmodifiedFunc = [](YYTKCodeEvent* pCodeEvent, CInstance* Self, CInstance* Other, CCode* Code, RValue* Res, int Flags) {
				pCodeEvent->Call(Self, Other, Code, Res, Flags);
			};
			UnmodifiedFunc(pCodeEvent, Self, Other, Code, Res, Flags);
			codeFuncTable[Code->i_CodeIndex] = UnmodifiedFunc;
		}
	}
	// Tell the core the handler was successful.
	return YYTK_OK;
}

// Create an entry routine - it must be named exactly this, and must accept these exact arguments.
// It must also be declared DllExport (notice how the other functions are not).
DllExport YYTKStatus PluginEntry(YYTKPlugin* PluginObject)
{
	// Set the unload routine
	PluginObject->PluginUnload = PluginUnload;

	// Print a message to the console
	PrintMessage(CLR_DEFAULT, "[%s v%d.%d.%d] - Hello from PluginEntry!", mod.name, mod.version.major, mod.version.minor, mod.version.build);

	PluginAttributes_t* PluginAttributes = nullptr;

	// Get the attributes for the plugin - this is an opaque structure, as it may change without any warning.
	// If Status == YYTK_OK (0), then PluginAttributes is guaranteed to be valid (non-null).
	if (YYTKStatus Status = PmGetPluginAttributes(PluginObject, PluginAttributes))
	{
		PrintError(__FILE__, __LINE__, "[%s v%d.%d.%d] - PmGetPluginAttributes failed with 0x%x", mod.name, mod.version.major, mod.version.minor, mod.version.build, Status);
		return YYTK_FAIL;
	}

	// Register a callback for frame events
	YYTKStatus Status = PmCreateCallback(
		PluginAttributes,					// Plugin Attributes
		g_pFrameCallbackAttributes,				// (out) Callback Attributes
		FrameCallback,						// The function to register as a callback
		static_cast<EventType>(EVT_PRESENT | EVT_ENDSCENE), // Which events trigger this callback
		nullptr								// The optional argument to pass to the function
	);

	if (Status)
	{
		PrintError(__FILE__, __LINE__, "[%s v%d.%d.%d] - PmCreateCallback failed with 0x%x", mod.name, mod.version.major, mod.version.minor, mod.version.build, Status);
		return YYTK_FAIL;
	}

	// Register a callback for frame events
	Status = PmCreateCallback(
		PluginAttributes,					// Plugin Attributes
		g_pCodeCallbackAttributes,			// (out) Callback Attributes
		CodeCallback,						// The function to register as a callback
		static_cast<EventType>(EVT_CODE_EXECUTE), // Which events trigger this callback
		nullptr								// The optional argument to pass to the function
	);

	if (Status)
	{
		PrintError(__FILE__, __LINE__, "[%s v%d.%d.%d] - PmCreateCallback failed with 0x%x", mod.name, mod.version.major, mod.version.minor, mod.version.build, Status);
		return YYTK_FAIL;
	}

	if (HAS_CONFIG == true) {
		// Load mod config file or create one if there isn't one already.
		const wchar_t* dirName = L"modconfigs";

		if (GetFileAttributes(dirName) == INVALID_FILE_ATTRIBUTES) {
			if (CreateDirectory(dirName, NULL)) {
				std::wcout << L"Directory \"modconfigs\" created!" << std::endl;
			} else {
				std::cerr << "Failed to create the modconfigs directory. Error code: " << GetLastError() << std::endl;
				return YYTK_FAIL;
			}
		}
		
		std::string fileName = formatString(std::string(mod.name)) + "-config.json";
		std::ifstream configFile("modconfigs/" + fileName);
		json data;
		if (configFile.is_open() == false) {	// no config file
			GenerateConfig(fileName);
		} else {
			try {
				data = json::parse(configFile);
			} catch (json::parse_error& e) {
				PrintError(__FILE__, __LINE__, "Message: %s\nException ID: %d\nByte Position of Error: %u", e.what(), e.id, (unsigned)e.byte);
				return YYTK_FAIL;
			}
			
			config = data.template get<Config>();
		}
		PrintMessage(CLR_WHITE, "%s loaded successfully!", fileName.c_str());
	}

	// Off it goes to the core.
	return YYTK_OK;
}

// The routine that gets called on plugin unload.
// Registered in PluginEntry - you should use this to release resources.
YYTKStatus PluginUnload()
{
	YYTKStatus Removal = PmRemoveCallback(g_pFrameCallbackAttributes);

	// If we didn't succeed in removing the callback.
	if (Removal != YYTK_OK)
	{
		PrintError(__FILE__, __LINE__, "[%s v%d.%d.%d] PmRemoveCallback failed with 0x%x", mod.name, mod.version.major, mod.version.minor, mod.version.build, Removal);
	}

	Removal = PmRemoveCallback(g_pCodeCallbackAttributes);

	// If we didn't succeed in removing the callback.
	if (Removal != YYTK_OK)
	{
		PrintError(__FILE__, __LINE__, "[%s v%d.%d.%d] PmRemoveCallback failed with 0x%x", mod.name, mod.version.major, mod.version.minor, mod.version.build, Removal);
	}

	PrintMessage(CLR_DEFAULT, "[%s v%d.%d.%d] - Goodbye!", mod.name, mod.version.major, mod.version.minor, mod.version.build);

	return YYTK_OK;
}

// Boilerplate setup for a Windows DLL, can just return TRUE.
// This has to be here or else you get linker errors (unless you disable the main method)
BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	return 1;
}