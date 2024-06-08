#include "api_service.hpp"

#include "http_client/http_client.hpp"
#include "pointers.hpp"
#include "services/creator_storage/creator_storage_service.hpp"


namespace big
{

	api_service::api_service()
	{
		g_api_service = this;
	}

	api_service::~api_service()
	{
		g_api_service = nullptr;
	}

	std::vector<std::string> api_service::get_ad_list()
	{
		std::string url = "https://blog.cc2077.site/https://raw.githubusercontent.com/sch-lda/yctest2/main/ad.json";
		const auto response = g_http_client.get(url,
		    {},
		    {});

		if (response.status_code != 200)
		{
			LOG(INFO) << "HTTP status_code:" << response.status_code;
			try
			{
				if (big::g_file_manager.get_project_file("./ad.json").exists())
				{
					std::time_t now = std::time(nullptr);

					static std::ifstream ad_file(g_file_manager.get_project_file("./ad.json").get_path(), std::ios::app);
					nlohmann::json json_obj;
					ad_file >> json_obj;
					ad_file.close();
					if (!json_obj.contains("ts"))
						return {};
					int p_ts = json_obj["ts"];
					if (now - p_ts > 259200)
					{
						LOG(WARNING) << "previous Ad list exist but too old";

						return {};
					}
					std::vector<std::string> ad_word_list = json_obj["ad_word_list"].get<std::vector<std::string>>();
					LOG(WARNING) << "Update failed. Use previous Ad list";
					return ad_word_list;
				}
			}
			catch (std::exception& e)
			{
				LOG(INFO) << e.what();

				return {};
			}
			return {};
		}

		try
		{
			std::time_t now = std::time(nullptr);
			nlohmann::json json_obj2;
			json_obj2       = nlohmann::json::parse(response.text);
			json_obj2["ts"] = now;
			static std::ofstream ad_file(g_file_manager.get_project_file("./ad.json").get_path());
			ad_file << json_obj2;
			ad_file.close();

			std::vector<std::string> spam_list = nlohmann::json::parse(response.text)["ad_word_list"].get<std::vector<std::string>>();
			return spam_list;
		}
		catch (std::exception& e)
		{
			LOG(INFO) << e.what();

			return {};
		}
	}

	std::vector<uint64_t> api_service::get_ad_rid_list()
	{
		std::string url     = "https://blog.cc2077.site/https://raw.githubusercontent.com/sch-lda/yctest2/main/ad_rid.json";
		const auto response = g_http_client.get(url, {}, {});

		if (response.status_code != 200)
		{
			LOG(INFO) << "HTTP status_code:" << response.status_code;
			try
			{
				if (big::g_file_manager.get_project_file("./ad_rid.json").exists())
				{
					std::time_t now = std::time(nullptr);

					static std::ifstream ad_file(g_file_manager.get_project_file("./ad_rid.json").get_path(), std::ios::app);
					nlohmann::json json_obj;
					ad_file >> json_obj;
					ad_file.close();
					if (!json_obj.contains("ts"))
						return {};
					int p_ts = json_obj["ts"];
					if (now - p_ts > 259200)
					{
						LOG(WARNING) << "previous Ad RID list exist but too old";

						return {};
					}
					std::vector<uint64_t> ad_word_list = json_obj["ad_rid_list"].get<std::vector<uint64_t>>();
					LOG(WARNING) << "Update failed. Use previous Ad RID list";
					return ad_word_list;
				}
			}
			catch (std::exception& e)
			{
				LOG(INFO) << e.what();

				return {};
			}
			return {};
		}

		try
		{
			std::time_t now = std::time(nullptr);
			nlohmann::json json_obj2;
			json_obj2       = nlohmann::json::parse(response.text);
			json_obj2["ts"] = now;
			static std::ofstream ad_file(g_file_manager.get_project_file("./ad_rid.json").get_path());
			ad_file << json_obj2;
			ad_file.close();

			std::vector<uint64_t> spam_list = nlohmann::json::parse(response.text)["ad_rid_list"].get<std::vector<uint64_t>>();
			return spam_list;
		}
		catch (std::exception& e)
		{
			LOG(INFO) << e.what();

			return {};
		}
	}

	bool api_service::report_spam(std::string_view message, uint64_t rid, int type)
	{
		LOG(INFO) << "1";
		LOG(INFO) << "2";
		LOG(INFO) << "3";
		LOG(INFO) << "4";
		LOG(INFO) << "5";
		return true;
	}
	std::string api_service::get_translation(std::string message, std::string target_language)
	{
		std::string url = g.session.chat_translator.endpoint;
		const auto response = g_http_client.post(url,
		    {{"Content-Type", "application/json"}}, std::format(R"({{"q":"{}", "source":"auto", "target": "{}"}})", message, target_language));
		if (response.status_code == 200)
		{
			try
			{
				nlohmann::json obj = nlohmann::json::parse(response.text);
				std::string source_language = obj["detectedLanguage"]["language"];
				std::string result = obj["translatedText"];

				if (source_language == g.session.chat_translator.target_language && g.session.chat_translator.bypass_same_language)
					return "";
				return result;
			}
			catch (std::exception& e)
			{
				LOG(WARNING) << "[Chat Translator]Error when parse JSON data: " << e.what();

				return "";
			}
		}
		else if (response.status_code == 0)
		{
			g.session.chat_translator.enabled = false;
			g_notification_service.push_error("TRANSLATOR_TOGGLE"_T.data(), "TRANSLATOR_FAILED_TO_CONNECT"_T.data());
			LOG(WARNING) << "[Chat Translator]Unable to connect to LibreTranslate server. Follow the guide in Yimmenu Wiki to setup LibreTranslate server on your computer.";
		}
		else
		{
			LOG(WARNING) << "[Chat Translator]Error when sending request. Status code: " << response.status_code << " Response: " << response.text;
		}
		
		return "";
	}

	bool api_service::get_rid_from_username(std::string_view username, uint64_t& result)
	{
		const auto response = g_http_client.post("https://scui.rockstargames.com/api/friend/accountsearch", {{"Authorization", AUTHORIZATION_TICKET}, {"X-Requested-With", "XMLHttpRequest"}}, {std::format("searchNickname={}", username)});
		if (response.status_code == 200)
		{
			try
			{
				nlohmann::json obj = nlohmann::json::parse(response.text);

				if (obj["Total"] > 0 && username.compare(obj["Accounts"].at(0)["Nickname"]) == 0)
				{
					result = obj["Accounts"].at(0)["RockstarId"];
					return true;
				}
			}
			catch (std::exception& e)
			{
				return false;
			}
		}

		return false;
	}

	bool api_service::get_username_from_rid(uint64_t rid, std::string& result)
	{
		const auto response = g_http_client.post("https://scui.rockstargames.com/api/friend/getprofile", {{"Authorization", AUTHORIZATION_TICKET}, {"X-Requested-With", "XMLHttpRequest"}, {"Content-Type", "application/json"}}, std::format(R"({{"RockstarId":"{}"}})", rid));
		if (response.status_code == 200)
		{
			try
			{
				nlohmann::json obj = nlohmann::json::parse(response.text);
				result             = obj["Accounts"].at(0)["RockstarAccount"]["Name"];
				return true;
			}
			catch (std::exception& e)
			{
				return false;
			}
		}

		return false;
	}

	// Ratelimit: 10 per Minute, if exceeded than 5 min cooldown
	bool api_service::send_socialclub_message(uint64_t rid, std::string_view message)
	{
		const auto response = g_http_client.post("https://scui.rockstargames.com/api/messaging/sendmessage", {{"Authorization", AUTHORIZATION_TICKET}, {"X-Requested-With", "XMLHttpRequest"}, {"Content-Type", "application/json"}}, {std::format(R"({{"env":"prod","title":"gta5","version":11,"recipientRockstarId":"{}","messageText":"{}"}})", rid, message)});

		return response.status_code == 200;
	}

	bool api_service::get_job_details(std::string_view content_id, nlohmann::json& result)
	{
		const auto response = g_http_client.get("https://scapi.rockstargames.com/ugc/mission/details",
		    {{"X-AMC", "true"}, {"X-Requested-With", "XMLHttpRequest"}},
		    {{"title", "gtav"}, {"contentId", content_id.data()}});

		if (response.status_code != 200)
			return false;

		try
		{
			result = nlohmann::json::parse(response.text);
			return true;
		}
		catch (std::exception& e)
		{
			return false;
		}
	}

	bool api_service::download_job_metadata(std::string_view content_id, int f1, int f0, int lang)
	{
		const auto response = g_http_client.get(std::format("https://prod.cloud.rockstargames.com/ugc/gta5mission/{}/{}_{}_{}.json",
		    content_id,
		    f1,
		    f0,
		    languages.at(lang)));

		if (response.status_code == 200)
		{
			const auto of = creator_storage_service::create_file(std::string(content_id) + ".json");

			return g_http_client.download(response.url, of);
		}

		return false;
	}
}