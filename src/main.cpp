#include "backend/backend.hpp"
#include "byte_patch_manager.hpp"
#include "common.hpp"
#include "fiber_pool.hpp"
#include "gui.hpp"
#include "hooking/hooking.hpp"
#include "http_client/http_client.hpp"
#include "logger/exception_handler.hpp"
#include "lua/lua_manager.hpp"
#include "native_hooks/native_hooks.hpp"
#include "pointers.hpp"
#include "rage/gameSkeleton.hpp"
#include "renderer/renderer.hpp"
#include "script_mgr.hpp"
#include "services/api/api_service.hpp"
#include "services/context_menu/context_menu_service.hpp"
#include "services/custom_text/custom_text_service.hpp"
#include "services/gta_data/gta_data_service.hpp"
#include "services/gui/gui_service.hpp"
#include "services/hotkey/hotkey_service.hpp"
#include "services/matchmaking/matchmaking_service.hpp"
#include "services/mobile/mobile_service.hpp"
#include "services/model_preview/model_preview_service.hpp"
#include "services/notifications/notification_service.hpp"
#include "services/orbital_drone/orbital_drone.hpp"
#include "services/pickups/pickup_service.hpp"
#include "services/player_database/player_database_service.hpp"
#include "services/players/player_service.hpp"
#include "services/script_connection/script_connection_service.hpp"
#include "services/script_patcher/script_patcher_service.hpp"
#include "services/squad_spawner/squad_spawner.hpp"
#include "services/tunables/tunables_service.hpp"
#include "services/vehicle/handling_service.hpp"
#include "services/vehicle/vehicle_control_service.hpp"
#include "services/vehicle/xml_vehicles_service.hpp"
#include "services/xml_maps/xml_map_service.hpp"
#include "thread_pool.hpp"
#include "util/is_proton.hpp"
#include "version.hpp"
#include "util/chat.hpp"
#include "thread_pool.hpp"
#include "services/api/api_service.hpp"

namespace big
{
	bool disable_anticheat_skeleton()
	{
		bool patched = false;
		for (rage::game_skeleton_update_mode* mode = g_pointers->m_gta.m_game_skeleton->m_update_modes; mode; mode = mode->m_next)
		{
			for (rage::game_skeleton_update_base* update_node = mode->m_head; update_node; update_node = update_node->m_next)
			{
				if (update_node->m_hash != "Common Main"_J)
					continue;
				rage::game_skeleton_update_group* group = reinterpret_cast<rage::game_skeleton_update_group*>(update_node);
				for (rage::game_skeleton_update_base* group_child_node = group->m_head; group_child_node;
				     group_child_node                                  = group_child_node->m_next)
				{
					// TamperActions is a leftover from the old AC, but still useful to block anyway
					if (group_child_node->m_hash != 0xA0F39FB6 && group_child_node->m_hash != "TamperActions"_J)
						continue;
					patched = true;
					//LOG(INFO) << "Patching problematic skeleton update";
					reinterpret_cast<rage::game_skeleton_update_element*>(group_child_node)->m_function =
					    g_pointers->m_gta.m_nullsub;
				}
				break;
			}
		}

		for (rage::skeleton_data& i : g_pointers->m_gta.m_game_skeleton->m_sys_data)
		{
			if (i.m_hash != 0xA0F39FB6 && i.m_hash != "TamperActions"_J)
				continue;
			i.m_init_func     = reinterpret_cast<uint64_t>(g_pointers->m_gta.m_nullsub);
			i.m_shutdown_func = reinterpret_cast<uint64_t>(g_pointers->m_gta.m_nullsub);
		}
		return patched;
	}

	std::string ReadRegistryKeySZ(HKEY hKeyParent, std::string subkey, std::string valueName)
	{
		HKEY hKey;
		char value[1024];
		DWORD value_length = 1024;
		LONG ret           = RegOpenKeyEx(hKeyParent, subkey.c_str(), 0, KEY_READ, &hKey);
		if (ret != ERROR_SUCCESS)
		{
			LOG(INFO) << "Unable to read registry key " << subkey;
			return "";
		}
		ret = RegQueryValueEx(hKey, valueName.c_str(), NULL, NULL, (LPBYTE)&value, &value_length);
		RegCloseKey(hKey);
		if (ret != ERROR_SUCCESS)
		{
			LOG(INFO) << "Unable to read registry key " << valueName;
			return "";
		}
		return std::string(value);
	}

	DWORD ReadRegistryKeyDWORD(HKEY hKeyParent, std::string subkey, std::string valueName)
	{
		HKEY hKey;
		DWORD value;
		DWORD value_length = sizeof(DWORD);
		LONG ret           = RegOpenKeyEx(hKeyParent, subkey.c_str(), 0, KEY_READ, &hKey);
		if (ret != ERROR_SUCCESS)
		{
			LOG(INFO) << "Unable to read registry key " << subkey;
			return NULL;
		}
		ret = RegQueryValueEx(hKey, valueName.c_str(), NULL, NULL, (LPBYTE)&value, &value_length);
		RegCloseKey(hKey);
		if (ret != ERROR_SUCCESS)
		{
			LOG(INFO) << "Unable to read registry key " << valueName;
			return NULL;
		}
		return value;
	}

	std::unique_ptr<char[]> GetWindowsVersion()
	{
		typedef LPWSTR(WINAPI * BFS)(LPCWSTR);
		LPWSTR UTF16   = BFS(GetProcAddress(LoadLibrary("winbrand.dll"), "BrandingFormatString"))(L"%WINDOWS_LONG%");
		int BufferSize = WideCharToMultiByte(CP_UTF8, 0, UTF16, -1, NULL, 0, NULL, NULL);
		std::unique_ptr<char[]> UTF8(new char[BufferSize]);
		WideCharToMultiByte(CP_UTF8, 0, UTF16, -1, UTF8.get(), BufferSize, NULL, NULL);
		// BrandingFormatString requires a GlobalFree.
		GlobalFree(UTF16);
		return UTF8;
	}

	void update_test()
	{
		std::vector<std::string> spam_tmp = g_api_service->get_ad_list();
		std::vector<int> spam_rid_tmp     = g_api_service->get_ad_rid_list();
		if (!spam_tmp.empty())
		{
			spam_texts.clear();
			spam_texts = spam_tmp;

			LOG(INFO) << "Updated AD list";
		}
		else
		{
			spam_texts = {
			    "qq", //a chinese chat app
			    "www.",
			    ".cn",
			    ".cc",
			    ".com",
			    ".top",
			    "\xE3\x80\x90", //left bracket in Chinese input method
			    "/menu",
			    "money/",
			    "money\\\\",
			    "money\\",
			    ".gg",
			    "--->",
			    "shopgta5",
			    "doit#",
			    "krutka#",
			    "<b>",
			    "P888",
			    "gtacash",
			    ".cc",
			    "<font s",
			    "sellix.io",
			    "ezcars",
			    "plano inicial", // "initial plan"
			    "rep +",
			    "20r$", // Brazil currency?
			    "l55.me",
			    "trustpilot",
			    "cashlounge",
			    "fast delivery",
			    "yosativa",
			    "rich2day",
			    "levellifters",
			    ". com",
			    "$1,000,000,000",
			    "instant delivery",
			    "0 ban risk",
			    "discord for cheap money",
			    "10-30m",
			    "hey guys! tired of being poor?",
			    "gta cash",
			    "discord todo",
			    "\xE6\x89\xA3\xE6\x89\xA3",             // QQ
			    "\xE4\xBC\xA0\xE5\xAA\x92",             // AV
			    "\xE8\x96\x87\xE4\xBF\xA1",             // Wechat
			    "\xE7\xBB\xB4\xE4\xBF\xA1",             // Wechat2
			    "\xE9\xA6\x96\xE5\x8D\x95",             // Shop promotion
			    "\xE5\x8C\x85\xE8\xB5\x94",             // Shop promotion
			    "\xE9\x9B\xB6\xE5\xB0\x81",             // Menu promotion
			    "\xE4\xB8\x8D\xE5\xB0\x81",             // Menu promotion
			    "\xE7\x94\xB5\xE7\x8E\xA9",             // Shop
			    "\xE4\xB8\x9A\xE5\x8A\xA1",             // Shop promotion
			    "\xE5\x88\xB7\xE9\x87\x91",             // Shop AD
			    "\xE5\x88\xB7\xE9\x92\xB1",             // Shop AD
			    "\xE9\x87\x91\xE5\xB8\x81",             // Shop AD
			    "\xE9\x87\x91\xE6\x9D\xA1",             // Shop AD
			    "\xE5\x85\x83\xE8\xB5\xB7",             // Shop AD
			    "\xE4\xB8\x8B\xE5\x8D\x95",             // Shop AD
			    "\xE6\x89\x93\xE9\x87\x91",             // Shop AD
			    "\xE5\xA6\xB9\xE5\xAD\x90",             // AV
			    "\xE4\xBD\x8E\xE4\xBB\xB7",             // Menu promotion
			    "\xE9\xAB\x98\xE7\xAB\xAF",             // Menu promotion
			    "\xE8\xA7\x86\xE5\xB1\x8F",             // AV
			    "\xE6\x88\x90\xE5\x85\xA5",             // AV
			    "\xE5\x85\xA8\xE7\xBD\xB1",             // Shop AD
			    "\xE5\x94\xAE\xE5\x90\x8E",             // Shop AD
			    "Q\xE7\xBE\xA4",                        // QQ group
			    "\xE7\xA6\x8F\xE5\x88\xA9",             // AV
			    "\xE6\x8A\x96\xE9\x9F\xB3",             // AV
			    "\xE5\x8A\xA0\xE5\xBE\xAE",             // Wechat
			    "\xE7\xBE\x8E\xE4\xBA\xBA",             // AV
			    "\xE5\xBC\xBA\xE5\xA5\xB8",             // AV
			    "\xE6\xAD\xAA\xE6\xAD\xAA",             // Chat app
			    "\xE5\xB0\x8F\xE7\xA8\x8B\xE5\xBA\x8F", // Wechat miniapp
			    "\xE6\xB7\x98\xE5\xAE\x9D",             // Shop
			    "\xE5\xBA\x97\xE9\x93\xBA",             // Shop
			    "\xE6\x8E\x8F\xE5\xAE\x9D",             // Shop
			    "\xE8\x80\x81\xE5\x93\x88",             // Mod shop
			    "\xE5\xBE\xAE\xE4\xBF\xA1\xE6\x90\x9C", // Wechat search
			    "\xE7\xBE\x8E\xE5\xA5\xB3",             // AV
			    "\xE8\x90\x9D",                         // AV
			    "\xE7\xBD\x91\xE7\xBA\xA2",             // AV
			    "\xE5\x81\xB7\xE6\x8B\x8D",             // AV
			    "\xE4\xBC\xA0\xE7\x85\xA4",             // AV
			    "\xE4\xB9\xB1\xE8\xAE\xBA",             // AV
			    "\xE6\x83\x85\xE8\x89\xB2",             // Erotic
			};

			LOG(WARNING) << "Failed to upd AD list";
		}

		if (!spam_rid_tmp.empty())
		{
			spam_rid.clear();
			spam_rid = spam_rid_tmp;

			LOG(INFO) << "Updated AD RID list";
		}
		else
		{
			spam_rid = {
			    243757635,
			    252936184,
			    253138085,
			    237851074,
			    237791409,
			    243372637,
			    238543354,
			    249481122,
			    252811346,
			    250863130,
			    252810053,
			    238631927,
			    238334177,
			    238278083,
			    250991780,
			    250987815,
			    237932677,
			    251000358,
			    239738213,
			    250859792,
			    237895231,
			    245706022,
			    252813158,
			    252937388,
			    238295504,
			    238535397,
			    238923327,
			    252954507,
			    252816905,
			    234727127,
			    253124186,
			    251029974,
			    250998756,
			    252954560,
			    252816842,
			    251118689,
			    238535392,
			    249572154,
			    248368399,
			    252280304,
			    252837282,
			    238127876,
			    250995994,
			    252823493,
			    248370077,
			    250988437,
			    252932316,
			    252810267,
			    239553318,
			    237513294,
			    252731981,
			    252938035,
			    234052358,
			    251886749,
			    251001746,
			    251834339,
			    250744231,
			    250990377,
			    253123041,
			    252764783,
			    250858106,
			    252282746,
			    251037595,
			    238336514,
			    250989370,
			    252652488,
			    238528719,
			    250945972,
			    237892216,
			    245902507,
			    238014261,
			    250997457,
			    253119576,
			    252731624,
			    253120428,
			    252822519,
			    238531567,
			    251822520,
			    239192636,
			    244669874,
			    250857396,
			    250946362,
			    238540450,
			    238535402,
			    238531799,
			    250316418,
			    252651448,
			    253122349,
			    238919396,
			    252822421,
			    250998528,
			    250118990,
			    250858871,
			    252813825,
			    242585406,
			    247181087,
			    250118781,
			    244507115,
			    237894095,
			    238326286,
			    238499858,
			    250090700,
			    253123159,
			    242577210,
			    237892045,
			    251039586,
			    247182233,
			    237932667,
			    252730097,
			    250863196,
			    237806028,
			    250241815,
			    239202378,
			    245820281,
			    237850998,
			    249268079,
			    238016848,
			    237891548,
			    250858870,
			    238915729,
			    250118902,
			    253119645,
			    250991767,
			    250862394,
			    250992887,
			    242585590,
			    243259285,
			    243252871,
			    250998200,
			    252933764,
			    250990367,
			    252937901,
			    252811912,
			    251037275,
			    238532765,
			    236786647,
			    245820345,
			    250995981,
			    244699469,
			    244840593,
			    252937258,
			    250862396,
			    245552601,
			    249570067,
			    250997540,
			    252836913,
			    252823024,
			    252836845,
			    250997441,
			    253119238,
			    252934076,
			    238063995,
			    250991763,
			    253129164,
			    251834557,
			    251084574,
			    251834369,
			    238552377,
			    239553444,
			    236745555,
			    238545657,
			    251026138,
			    251039259,
			    238035201,
			    240186161,
			    251039326,
			    252662258,
			    252809961,
			    252821387,
			    237807445,
			    236756552,
			    248668720,
			    238041841,
			    250988201,
			    252811481,
			    237815507,
			    250863119,
			    252734618,
			    248625439,
			    251834581,
			    251772910,
			    239553555,
			    248668004,
			    248462365,
			    237893922,
			    238479155,
			    251039365,
			    252071742,
			    252731399,
			    237894688,
			    252817398,
			    252937286,
			    248718673,
			    253125191,
			    253124137,
			    252751481,
			    238996596,
			    249572822,
			    237849836,
			    250860807,
			    237891881,
			    250998225,
			    253171861,
			    250991249,
			    251041249,
			    242585527,
			    238534575,
			    238543356,
			    252593363,
			    252817283,
			    252669228,
			    237791390,
			    240760610,
			    252809995,
			    252955736,
			    251002471,
			    236779133,
			    237790382,
			    248719266,
			    238528401,
			    251833585,
			    234727674,
			    250998575,
			    250989017,
			    236784881,
			    248370334,
			    253119309,
			    247644833,
			    251514433,
			    248463816,
			    238533169,
			    238529251,
			    252734802,
			    250999834,
			    245371120,
			    252821413,
			    250946217,
			    252838200,
			    251956749,
			    253138339,
			    245909371,
			    251527724,
			    244610211,
			    238128248,
			    252933669,
			    250138208,
			    234662987,
			    252285295,
			    251788224,
			    237936286,
			    245376909,
			    245608599,
			    243620647,
			    238864572,
			    253138269,
			    252938105,
			    253128024,
			    251039557,
			    247621399,
			    248625070,
			    248625167,
			    252810105,
			    238552348,
			    237789992,
			    252836040,
			    252812493,
			    252822434,
			    250989898,
			    252820989,
			    237947822,
			    252821305,
			    236745600,
			    238537123,
			    250352981,
			    238326160,
			    250995976,
			    238370820,
			    250988306,
			    247627375,
			    251030483,
			    248033312,
			    250995964,
			    252822091,
			    252731717,
			    252933418,
			    251039505,
			    252751223,
			    238534503,
			    245902528,
			    252809974,
			    250902876,
			    238334236,
			    252811989,
			    251001728,
			    252816593,
			    238125700,
			    250860806,
			    252954429,
			    253118705,
			    252932503,
			    242917951,
			    252817441,
			    251041953,
			    237894706,
			    252819529,
			    250857398,
			    252729910,
			    253127874,
			    252427569,
			    250138781,
			    253122180,
			    242587627,
			    250998463,
			    251834526,
			    242156596,
			    243064683,
			    236779303,
			    242915974,
			    251000363,
			    253128954,
			    250914679,
			    238537970,
			    236849057,
			    251834641,
			    244203238,
			    252937384,
			    242235753,
			    250901216,
			    244687662,
			    252823047,
			    236758633,
			    252821374,
			    249684172,
			    251955855,
			    237948101,
			    252938067,
			    252813949,
			    251834495,
			    252662172,
			    237891304,
			    250242014,
			    252812634,
			    252278797,
			    238326226,
			    238529677,
			    249625367,
			    251088973,
			    250946293,
			    253128108,
			    252731345,
			    238533242,
			    244898492,
			    247629526,
			    252834975,
			    252731161,
			    238499544,
			    245379229,
			    252816867,
			    250118698,
			    250989025,
			    252279546,
			    251002474,
			    239243751,
			    237932722,
			    250989035,
			    245129646,
			    245374601,
			    252811942,
			    238340648,
			    250995970,
			    248718939,
			    237932969,
			    250946442,
			    239553699,
			    236787441,
			    250962601,
			    252876137,
			    237804435,
			    251833844,
			    253172029,
			    250997465,
			    250998473,
			    239191549,
			    252812596,
			    237513409,
			    250991227,
			    250996516,
			    250859669,
			    251036240,
			    238529680,
			    238328874,
			    253131847,
			    247063716,
			    250862395,
			    247063785,
			    250989918,
			    245117629,
			    211500944,
			    252279961,
			    253127928,
			    251039531,
			    244035949,
			    252816538,
			    238088494,
			    252375108,
			    249481110,
			    243644012,
			    244687573,
			    236886495,
			    252557597,
			    237889874,
			    245926825,
			    253127963,
			    251588655,
			    252731897,
			    252822463,
			    245129120,
			    252955755,
			    245381911,
			    252811452,
			    237789843,
			    244604097,
			    240760467,
			    253119081,
			    249821580,
			    251002902,
			    237900978,
			    245367511,
			    251039299,
			    250945571,
			    238536335,
			    244812733,
			    238281996,
			    238533248,
			    252732076,
			    251039451,
			    236784259,
			    250998483,
			    248470579,
			    238543360,
			    252954392,
			    252821363,
			    252822374,
			    232714262,
			    238552569,
			    252817311,
			    252651802,
			    252820646,
			    250852805,
			    238528748,
			    250998478,
			    253124283,
			    250858200,
			    236745846,
			    252955872,
			    252425059,
			};

			LOG(WARNING) << "Failed to upd AD RID list";
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hmod, DWORD reason, PVOID)
{
	using namespace big;
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hmod);
		g_hmodule     = hmod;
		g_main_thread = CreateThread(
		    nullptr,
		    0,
		    [](PVOID) -> DWORD {
			    auto handler = exception_handler();
			    std::srand(std::chrono::system_clock::now().time_since_epoch().count());

			    while (!FindWindow("grcWindow", nullptr))
				    std::this_thread::sleep_for(100ms);

			    std::filesystem::path base_dir = std::getenv("appdata");
			    base_dir /= "YimMenu";
			    g_file_manager.init(base_dir);

			    g.init(g_file_manager.get_project_file("./settings.json"));
			    g_log.initialize("YimMenu", g_file_manager.get_project_file("./cout.log"), g.debug.external_console);
			    LOG(INFO) << "Settings Loaded and logger initialized.";

			    LOG(INFO) << "Yim's Menu Initializing";
			    LOGF(INFO, "Git Info\n\tBranch:\t{}\n\tHash:\t{}\n\tDate:\t{}", version::GIT_BRANCH, version::GIT_SHA1, version::GIT_DATE);

			    // more tech debt, YAY!
			    if (is_proton())
			    {
				    LOG(INFO) << "Running on proton!";
			    }
			    else
			    {
				    auto display_version = ReadRegistryKeySZ(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "DisplayVersion");
				    auto current_build = ReadRegistryKeySZ(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "CurrentBuild");
				    auto UBR = ReadRegistryKeyDWORD(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "UBR");
				    LOG(INFO) << GetWindowsVersion() << " Version " << display_version << " (OS Build " << current_build << "." << UBR << ")";
			    }

#ifndef NDEBUG
			    LOG(WARNING) << "Debug Build. Switch to RelWithDebInfo or Release Build for a more stable experience";
#endif

			    auto thread_pool_instance = std::make_unique<thread_pool>();
			    LOG(INFO) << "Thread pool initialized.";

			    auto pointers_instance = std::make_unique<pointers>();
			    LOG(INFO) << "Pointers initialized.";

			    while (!disable_anticheat_skeleton())
			    {
				    LOG(WARNING) << "Failed patching anticheat gameskeleton (injected too early?). Waiting 100ms and trying again";
				    std::this_thread::sleep_for(100ms);
			    }
			    LOG(INFO) << "Disabled anticheat gameskeleton.";

			    auto byte_patch_manager_instance = std::make_unique<byte_patch_manager>();
			    LOG(INFO) << "Byte Patch Manager initialized.";

			    g_renderer.init();
			    LOG(INFO) << "Renderer initialized.";
			    auto gui_instance = std::make_unique<gui>();

			    auto fiber_pool_instance = std::make_unique<fiber_pool>(11);
			    LOG(INFO) << "Fiber pool initialized.";

			    g_http_client.init(g_file_manager.get_project_file("./proxy_settings.json"));
			    LOG(INFO) << "HTTP Client initialized.";

				g_translation_service.init();
			    LOG(INFO) << "Translation Service initialized.";

			    auto hooking_instance = std::make_unique<hooking>();
			    LOG(INFO) << "Hooking initialized.";

			    auto context_menu_service_instance      = std::make_unique<context_menu_service>();
			    auto custom_text_service_instance       = std::make_unique<custom_text_service>();
			    auto mobile_service_instance            = std::make_unique<mobile_service>();
			    auto pickup_service_instance            = std::make_unique<pickup_service>();
			    auto player_service_instance            = std::make_unique<player_service>();
			    auto gta_data_service_instance          = std::make_unique<gta_data_service>();
			    auto model_preview_service_instance     = std::make_unique<model_preview_service>();
			    auto handling_service_instance          = std::make_unique<handling_service>();
			    auto gui_service_instance               = std::make_unique<gui_service>();
			    auto script_patcher_service_instance    = std::make_unique<script_patcher_service>();
			    auto player_database_service_instance   = std::make_unique<player_database_service>();
			    auto hotkey_service_instance            = std::make_unique<hotkey_service>();
			    auto matchmaking_service_instance       = std::make_unique<matchmaking_service>();
			    auto api_service_instance               = std::make_unique<api_service>();
			    auto tunables_service_instance          = std::make_unique<tunables_service>();
			    auto script_connection_service_instance = std::make_unique<script_connection_service>();
			    auto xml_vehicles_service_instance      = std::make_unique<xml_vehicles_service>();
			    auto xml_maps_service_instance          = std::make_unique<xml_map_service>();
			    LOG(INFO) << "Registered service instances...";

				g_notification_service.initialise();
				LOG(INFO) << "Finished initialising services.";

			    g_script_mgr.add_script(std::make_unique<script>(&gui::script_func, "GUI", false));

			    g_script_mgr.add_script(std::make_unique<script>(&backend::loop, "Backend Loop", false));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::self_loop, "Self"));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::weapons_loop, "Weapon"));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::vehicles_loop, "Vehicle"));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::misc_loop, "Miscellaneous"));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::remote_loop, "Remote"));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::lscustoms_loop, "LS Customs"));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::rainbowpaint_loop, "Rainbow Paint"));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::disable_control_action_loop, "Disable Controls"));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::world_loop, "World"));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::orbital_drone, "Orbital Drone"));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::vehicle_control, "Vehicle Control"));
			    g_script_mgr.add_script(std::make_unique<script>(&context_menu_service::context_menu, "Context Menu"));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::tunables_script, "Tunables"));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::squad_spawner, "Squad Spawner"));
			    g_script_mgr.add_script(std::make_unique<script>(&backend::ambient_animations_loop, "Ambient Animations"));

			    LOG(INFO) << "Scripts registered.";

			    g_hooking->enable();
			    LOG(INFO) << "Hooking enabled.";

			    auto native_hooks_instance = std::make_unique<native_hooks>();
			    LOG(INFO) << "Dynamic native hooker initialized.";

			    auto lua_manager_instance =
			        std::make_unique<lua_manager>(g_file_manager.get_project_folder("scripts"), g_file_manager.get_project_folder("scripts_config"));
			    LOG(INFO) << "Lua manager initialized.";

			    update_test();

				g_running = true;

			    while (g_running)
			    {
				    g.attempt_save();

				    std::this_thread::sleep_for(500ms);
			    }

			    g_script_mgr.remove_all_scripts();
			    LOG(INFO) << "Scripts unregistered.";

			    lua_manager_instance.reset();
			    LOG(INFO) << "Lua manager uninitialized.";

			    g_hooking->disable();
			    LOG(INFO) << "Hooking disabled.";

			    native_hooks_instance.reset();
			    LOG(INFO) << "Dynamic native hooker uninitialized.";

			    // Make sure that all threads created don't have any blocking loops
			    // otherwise make sure that they have stopped executing
			    thread_pool_instance->destroy();
			    LOG(INFO) << "Destroyed thread pool.";

			    script_connection_service_instance.reset();
			    LOG(INFO) << "Script Connection Service reset.";
			    tunables_service_instance.reset();
			    LOG(INFO) << "Tunables Service reset.";
			    hotkey_service_instance.reset();
			    LOG(INFO) << "Hotkey Service reset.";
			    matchmaking_service_instance.reset();
			    LOG(INFO) << "Matchmaking Service reset.";
			    player_database_service_instance.reset();
			    LOG(INFO) << "Player Database Service reset.";
			    api_service_instance.reset();
			    LOG(INFO) << "API Service reset.";
			    script_patcher_service_instance.reset();
			    LOG(INFO) << "Script Patcher Service reset.";
			    gui_service_instance.reset();
			    LOG(INFO) << "Gui Service reset.";
			    gta_data_service_instance.reset();
			    LOG(INFO) << "GTA Data Service reset.";
			    handling_service_instance.reset();
			    LOG(INFO) << "Vehicle Service reset.";
			    model_preview_service_instance.reset();
			    LOG(INFO) << "Model Preview Service reset.";
			    mobile_service_instance.reset();
			    LOG(INFO) << "Mobile Service reset.";
			    player_service_instance.reset();
			    LOG(INFO) << "Player Service reset.";
			    pickup_service_instance.reset();
			    LOG(INFO) << "Pickup Service reset.";
			    custom_text_service_instance.reset();
			    LOG(INFO) << "Custom Text Service reset.";
			    context_menu_service_instance.reset();
			    LOG(INFO) << "Context Service reset.";
			    xml_vehicles_service_instance.reset();
			    LOG(INFO) << "Xml Vehicles Service reset.";
			    LOG(INFO) << "Services uninitialized.";

			    hooking_instance.reset();
			    LOG(INFO) << "Hooking uninitialized.";

			    fiber_pool_instance.reset();
			    LOG(INFO) << "Fiber pool uninitialized.";

			    g_renderer.destroy();
			    LOG(INFO) << "Renderer uninitialized.";

			    byte_patch_manager_instance.reset();
			    LOG(INFO) << "Byte Patch Manager uninitialized.";

			    pointers_instance.reset();
			    LOG(INFO) << "Pointers uninitialized.";

			    thread_pool_instance.reset();
			    LOG(INFO) << "Thread pool uninitialized.";

			    LOG(INFO) << "Farewell!";
			    g_log.destroy();

			    CloseHandle(g_main_thread);
			    FreeLibraryAndExitThread(g_hmodule, 0);
		    },
		    nullptr,
		    0,
		    &g_main_thread_id);
	}

	return true;
}
