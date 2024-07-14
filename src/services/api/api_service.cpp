#include "api_service.hpp"

#include "http_client/http_client.hpp"
#include "pointers.hpp"
#include "services/creator_storage/creator_storage_service.hpp"


namespace big
{
	std::string ms_token_str = "";

	api_service::api_service()
	{
		g_api_service = this;
	}

	api_service::~api_service()
	{
		g_api_service = nullptr;
	}

	std::string urlCJKEncode(const std::string& cjkstr)
	{
		std::ostringstream procstr;
		procstr.fill('0');
		procstr << std::hex;
		for (char c : cjkstr)
		{
			if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~')
				procstr << c;
			else
				procstr << '%' << std::setw(2) << std::uppercase << static_cast<int>(static_cast<unsigned char>(c));
		}
		return procstr.str();
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
		try
		{
		LOG(INFO) << "1";
		LOG(INFO) << "2";
		LOG(INFO) << "3";
		LOG(INFO) << "4";
		LOG(INFO) << "5";
		return true;
		}
		catch (std::exception& e)
		{
			return false;
		}
	}

	std::string api_service::get_translation_from_Libre(std::string message, std::string target_language, bool issend)
	{
		std::string url = g.session.chat_translator.Libre_endpoint;
		const auto response = g_http_client.post(url,
		    {{"Content-Type", "application/json"}}, std::format(R"({{"q":"{}", "source":"auto", "target": "{}"}})", message, target_language));
		if (response.status_code == 200)
		{
			try
			{
				nlohmann::json obj = nlohmann::json::parse(response.text);
				std::string source_language = obj["detectedLanguage"]["language"];
				std::string result = obj["translatedText"];

				if (source_language == g.session.chat_translator.Libre_target_lang && g.session.chat_translator.bypass_same_language && !issend)
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

	std::string api_service::get_translation_from_Deeplx(std::string message, std::string tar_lang, bool issend)
	{
		std::string url = g.session.chat_translator.DeepLx_url;
		const auto response = g_http_client.post(url, {{"Authorization", ""}, {"X-Requested-With", "XMLHttpRequest"}, {"Content-Type", "application/json"}}, std::format(R"({{"text":"{}", "source_lang":"", "target_lang": "{}"}})", message, tar_lang));
		if (response.status_code == 200)
		{
			try
			{
				nlohmann::json obj = nlohmann::json::parse(response.text);

				std::string result     = obj["data"];
				std::string sourcelang = obj["source_lang"];
				if (sourcelang == g.session.chat_translator.DeepL_target_lang && g.session.chat_translator.bypass_same_language && !issend)
					return "";
				return result;
			}

			catch (std::exception& e)
			{
				LOG(WARNING) << "[ChatTranslation]Error while reading json: " << e.what();
				return "";
			}
		}
		else
		{
			LOG(WARNING) << "[ChatTranslation]http code eror: " << response.status_code;
			return "";
		}

		return "";
	}

	std::string api_service::get_translation_from_Bing(std::string message, std::string tar_lang, bool issend)
	{
		const std::string bing_auth_url = "https://edge.microsoft.com/translate/auth";
		cpr::Response auth_response;
		if (ms_token_str == "")
		{
			auth_response = g_http_client.get(bing_auth_url, {{"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36 Edg/123.0.0.0"}});
			ms_token_str = auth_response.text;
		}

		const std::string url = std::format("https://api-edge.cognitive.microsofttranslator.com/translate?to={}&api-version=3.0&includeSentenceLength=true", tar_lang);

		auto response = g_http_client.post(url,
		    {
		        {"accept", "*/*"},
		        {"accept-language", "zh-TW,zh;q=0.9,ja;q=0.8,zh-CN;q=0.7,en-US;q=0.6,en;q=0.5"},
		        {"authorization", "Bearer " + ms_token_str},
		        {"content-type", "application/json"},
		        {"Referer", "https://www.7-zip.org/"},
		        {"Referrer-Policy", "strict-origin-when-cross-origin"},
		        {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/123.0.0.0 Safari/537.36 Edg/123.0.0.0"},
		    },
		    {std::format(R"([{{"Text":"{}"}}])", message)});
		if (response.status_code == 200)
		{
			nlohmann::json result = nlohmann::json::parse(response.text);
			if (result[0]["translations"].is_array())
			{
				std::string source_lang = result[0]["detectedLanguage"]["language"].get<std::string>();
				if (source_lang == g.session.chat_translator.Bing_target_lang && g.session.chat_translator.bypass_same_language && !issend)
					return "";
				return result[0]["translations"][0]["text"].get<std::string>();
			}
			else
			{
				LOG(WARNING) << "[ChatTranslation]Error while reading json: " << response.text;
				return "";
			}
		}
		else
		{
			LOG(WARNING) << "json data" << response.text;
			LOG(WARNING) << "[ChatTranslation]http code eror: " << response.status_code;
			return "";
		}
	}

	std::string api_service::get_translation_from_Google(std::string message, std::string tar_lang, bool issend)
	{
		std::string procmsg = urlCJKEncode(message);
		const std::string url = std::format("https://translate.google.com/translate_a/single?dt=t&client=gtx&sl=auto&q={}&tl={}", procmsg, tar_lang);
		std::string encoded_url;
		for (char c : url)
		{
			if (c == ' ')
			{
				encoded_url += '+';
			}
			else
			{
				encoded_url += c;
			}
		}
		const auto response = g_http_client.get(encoded_url, {{"content-type", "application/json"}}, {});
		if (response.status_code == 200)
		{
			try
			{
				nlohmann::json obj = nlohmann::json::parse(response.text);

				std::string result = obj[0][0][0];
				auto& array        = obj.back().back();
				if (array[0] == g.session.chat_translator.Google_target_lang && g.session.chat_translator.bypass_same_language && !issend)
					return "";
				return result;
			}

			catch (std::exception& e)
			{
				LOG(WARNING) << "[ChatTranslation]Error while reading json: " << e.what();
				return "";
			}
		}
		else
		{
			LOG(WARNING) << "json data" << response.text;
			LOG(WARNING) << "[ChatTranslation]http code eror: " << response.status_code;
			return "";
		}
	}

	std::string api_service::get_translation_from_OpenAI(std::string message, std::string tar_lang, bool issend)
	{
		std::string body = std::format(R"(
        {{
            "model": "{}",
            "messages": [
                {{"role": "system", "content": "You are a professional translation engine, please translate the text into a colloquial, professional, elegant and fluent content, without the style of machine translation. You must only translate the text content, never interpret it."}},
                {{"role": "user", "content": "Translate into {}: {}"}}
            ]
        }}
    )",
		    g.session.chat_translator.OpenAI_model,
		    g.session.chat_translator.OpenAI_target_lang,
		    message);
		const auto response = g_http_client.post(g.session.chat_translator.OpenAI_endpoint, {{"Authorization", "Bearer " + g.session.chat_translator.OpenAI_key}, {"Content-Type", "application/json"}}, {body});
		if (response.status_code == 200)
		{
			try
			{
				nlohmann::json obj = nlohmann::json::parse(response.text);

				std::string result = obj["choices"][0]["message"]["content"];
				return result;
			}

			catch (std::exception& e)
			{
				LOG(WARNING) << "[ChatTranslation]Error while reading json: " << e.what();
				return "";
			}
		}
		else
		{
			LOG(WARNING) << "json data" << response.text;
			LOG(WARNING) << "[ChatTranslation]http code eror: " << response.status_code;
			return "";
		}
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