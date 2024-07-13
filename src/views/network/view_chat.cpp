#include "core/scr_globals.hpp"
#include "fiber_pool.hpp"
#include "gta_util.hpp"
#include "views/view.hpp"
#include "util/chat.hpp"
#include "core/data/command_access_levels.hpp"
#include "services/api/api_service.hpp"

namespace big
{
	struct DeeplTargetLanguage
	{
		const char* TargetLanguageType;
		const char* TargetLanguageName;
	};
	struct BingTargetLanguage
	{
		const char* TargetLanguageType;
		const char* TargetLanguageName;
	};
	struct GoogleTargetLanguage
	{
		const char* TargetLanguageType;
		const char* TargetLanguageName;
	};
	struct ServiceProvider
	{
		int ProviderID;
		const char* ProviderName;
	};

	struct LibreTargetLanguage
	{
		const char* type;
		const char* name;
	};

	void view::chat()
	{
		static char msg[256];
		ImGui::Checkbox("USE_SPAM_TIMER"_T.data(), &g.session.use_spam_timer);
		if (g.session.use_spam_timer)
		{
			ImGui::SliderFloat("SPAM_TIMER"_T.data(), &g.session.spam_timer, 0.5f, 5.0f);
			ImGui::SliderInt("SPAM_LENGTH"_T.data(), &g.session.spam_length, 1, 256);
		}
		ImGui::Checkbox("LOG_CHAT_MSG"_T.data(), &g.session.log_chat_messages);
		ImGui::Checkbox("LOG_TXT_MSG"_T.data(), &g.session.log_text_messages);
		components::input_text_with_hint("##message", "VIEW_NET_CHAT_MESSAGE"_T, msg, sizeof(msg));

		ImGui::Checkbox("IS_TEAM"_T.data(), &g.session.is_team);
		ImGui::SameLine();
		components::button("SEND"_T, [] {
			if (const auto net_game_player = gta_util::get_network_player_mgr()->m_local_net_player; net_game_player)
			{
				chat::send_message(msg, nullptr, true, g.session.is_team);
			}
		});
		ImGui::SameLine();
		if (g.session.chat_translator.enabled)
		{
			components::button("翻译并发送", [] {
				chat_message messagetoadd{"me", msg, true, g.session.is_team};
				translate_queue.push(messagetoadd);
			});
		}
		else
		{
			components::button("开启聊天翻译以使用翻译并发送功能", [] {
			});
		}

		ImGui::Separator();

		ImGui::Checkbox("CHAT_COMMANDS"_T.data(), &g.session.chat_commands);
		if (g.session.chat_commands)
		{
			components::small_text("DEFAULT_CMD_PERMISSIONS"_T);
			if (ImGui::BeginCombo("##defualtchatcommands", COMMAND_ACCESS_LEVELS[g.session.chat_command_default_access_level]))
			{
				for (const auto& [type, name] : COMMAND_ACCESS_LEVELS)
				{
					if (ImGui::Selectable(name, type == g.session.chat_command_default_access_level))
					{
						g.session.chat_command_default_access_level = type;
					}

					if (type == g.session.chat_command_default_access_level)
					{
						ImGui::SetItemDefaultFocus();
					}
				}

				ImGui::EndCombo();
			}
		}

		components::command_checkbox<"translatechat">();
		if (g.session.chat_translator.enabled)
		{
			components::small_text("前三个提供商基于逆向分析,并非官方免费开放的接口,使用前请注意您所在国家和可能导致的法律问题!");

			static const auto Provider = std::to_array<ServiceProvider>({{0, "Microsoft(无门槛)"}, {1, "Google(需挂代理)"}, {2, "DeepLx(需要DeepLx程序)"}, {3, "OpenAI(需要token)"}, {4, "LibreTranslate(需要手动部署)"}});

			if (ImGui::BeginCombo("翻译服务提供商##ServiceProvider",
			        Provider[g.session.chat_translator.t_service_provider].ProviderName))
			{
				for (const auto& [id, name] : Provider)
				{
					components::selectable(name, false, [&id] {
						g.session.chat_translator.t_service_provider = id;
					});
				}
				ImGui::EndCombo();
			}
			ImGui::Checkbox("仅翻译上方翻译并发送功能的消息,忽略传入消息", &g.session.chat_translator.switch_send_only);


			ImGui::Checkbox("TRANSLATOR_HIDE_SAME_LANGUAGE"_T.data(), &g.session.chat_translator.bypass_same_language);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("TRANSLATOR_HIDE_SAME_LANGUAGE_DESC"_T.data());

			components::small_text("TRANSLATOR_OUTPUT"_T.data());
			ImGui::Checkbox("TRANSLATOR_SHOW_ON_CHAT"_T.data(), &g.session.chat_translator.draw_result);
			ImGui::Checkbox("TRANSLATOR_PRINT_TO_CONSOLE"_T.data(), &g.session.chat_translator.print_result);

			if (ImGui::CollapsingHeader("微软必应设置"))
			{
				static const auto BingTargetLang = std::to_array<BingTargetLanguage>({{"ar", "Arabic"}, {"az", "Azerbaijani"}, {"bn", "Bangla"}, {"bg", "Bulgarian"}, {"zh-Hans", "Chinese Simplified"}, {"zh-Hant", "Chinese Traditional"}, {"hr", "Croatian"}, {"cs", "Czech"}, {"da", "Danish"}, {"de", "German"}, {"el", "Greek"}, {"en", "English"}, {"es", "Spanish"}, {"et", "Estonian"}, {"fi", "Finnish"}, {"fr", "French"}, {"hu", "Hungarian"}, {"id", "Indonesian"}, {"it", "Italian"}, {"ja", "Japanese"}, {"ko", "Korean"}, {"lt", "Lithuanian"}, {"lv", "Latvian"}, {"nb", "Norwegian(Bokmål)"}, {"nl", "Dutch"}, {"pl", "Polish"}, {"pt", "Portuguese"}, {"pt-br", "Portuguese(Brazilian)"}, {"ro", "Romanian"}, {"ru", "Russian"}, {"sk", "Slovak"}, {"sl", "Slovenian"}, {"sv", "Swedish"}, {"th", "Thai"}, {"tr", "Turkish"}, {"uk", "Ukrainian"}, {"vi", "Vietnamese"}});

				if (ImGui::BeginCombo("目标语言(接收)##BingTargetLanguage", g.session.chat_translator.Bing_target_lang.c_str()))
				{
					for (const auto& [type, name] : BingTargetLang)
					{
						components::selectable(name, false, [&type] {
							g.session.chat_translator.Bing_target_lang = type;
						});
					}
					ImGui::EndCombo();
				}

				if (ImGui::BeginCombo("目标语言(发送)##BingTargetLanguagesend", g.session.chat_translator.Bing_target_lang_send.c_str()))
				{
					for (const auto& [type, name] : BingTargetLang)
					{
						components::selectable(name, false, [&type] {
							g.session.chat_translator.Bing_target_lang_send = type;
						});
					}
					ImGui::EndCombo();
				}

				components::button("重新获取token", [] {
					ms_token_str = "";
				});
			}
			if (ImGui::CollapsingHeader("谷歌翻译设置"))
			{
				static const auto GoogleTargetLang = std::to_array<GoogleTargetLanguage>({{"ar", "Arabic"}, {"az", "Azerbaijani"}, {"bg", "Bulgarian"}, {"zh-CN", "Chinese Simplified"}, {"zh-TW", "Chinese Traditional"}, {"hr", "Croatian"}, {"cs", "Czech"}, {"da", "Danish"}, {"de", "German"}, {"el", "Greek"}, {"en", "English"}, {"es", "Spanish"}, {"et", "Estonian"}, {"fi", "Finnish"}, {"fr", "French"}, {"hu", "Hungarian"}, {"id", "Indonesian"}, {"it", "Italian"}, {"ja", "Japanese"}, {"ko", "Korean"}, {"lt", "Lithuanian"}, {"lv", "Latvian"}, {"n0", "Norwegian"}, {"nl", "Dutch"}, {"pl", "Polish"}, {"pt", "Portuguese"}, {"ro", "Romanian"}, {"ru", "Russian"}, {"sk", "Slovak"}, {"sl", "Slovenian"}, {"sv", "Swedish"}, {"th", "Thai"}, {"tr", "Turkish"}, {"uk", "Ukrainian"}, {"vi", "Vietnamese"}});

				if (ImGui::BeginCombo("目标语言(接收)##GoogleTargetLangSwitcher", g.session.chat_translator.Google_target_lang.c_str()))
				{
					for (const auto& [type, name] : GoogleTargetLang)
					{
						components::selectable(name, false, [&type] {
							g.session.chat_translator.Google_target_lang = type;
						});
					}
					ImGui::EndCombo();
				}
				if (ImGui::BeginCombo("目标语言(发送)##GoogleTargetLangSwitchersend",
				        g.session.chat_translator.Google_target_lang_send.c_str()))
				{
					for (const auto& [type, name] : GoogleTargetLang)
					{
						components::selectable(name, false, [&type] {
							g.session.chat_translator.Google_target_lang_send = type;
						});
					}
					ImGui::EndCombo();
				}
			}

			if (ImGui::CollapsingHeader("DeepLx设置"))
			{
				static const auto DeepLTargetLang = std::to_array<DeeplTargetLanguage>({{"AR", "Arabic"}, {"BG", "Bulgarian"}, {"CS", "Czech"}, {"DA", "Danish"}, {"DE", "German"}, {"EL", "Greek"}, {"EN", "English"}, {"EN-GB", "English(British)"}, {"EN-US", "English(American)"}, {"ES", "Spanish"}, {"ET", "Estonian"}, {"FI", "Finnish"}, {"FR", "French"}, {"HU", "Hungarian"}, {"ID", "Indonesian"}, {"IT", "Italian"}, {"JA", "Japanese"}, {"KO", "Korean"}, {"LT", "Lithuanian"}, {"LV", "Latvian"}, {"NB", "Norwegian(Bokmål)"}, {"NL", "Dutch"}, {"PL", "Polish"}, {"PT", "Portuguese"}, {"PT-BR", "Portuguese(Brazilian)"}, {"PT-PT", "Portuguese(Others)"}, {"RO", "Romanian"}, {"RU", "Russian"}, {"SK", "Slovak"}, {"SL", "Slovenian"}, {"SV", "Swedish"}, {"TR", "Turkish"}, {"UK", "Ukrainian"}, {"ZH", "Chinese(simplified)"}});

				components::input_text_with_hint("DeepLx URL", "http://127.0.0.1:1188/translate", g.session.chat_translator.DeepLx_url);

				if (ImGui::BeginCombo("目标语言(接收)##DeepLTargetLangSwitcher", g.session.chat_translator.DeepL_target_lang.c_str()))
				{
					for (const auto& [type, name] : DeepLTargetLang)
					{
						components::selectable(name, false, [&type] {
							g.session.chat_translator.DeepL_target_lang = type;
						});
					}
					ImGui::EndCombo();
				}
				if (ImGui::BeginCombo("目标语言(发送)##DeepLTargetLangSwitchersend",
				        g.session.chat_translator.DeepL_target_lang_send.c_str()))
				{
					for (const auto& [type, name] : DeepLTargetLang)
					{
						components::selectable(name, false, [&type] {
							g.session.chat_translator.DeepL_target_lang_send = type;
						});
					}
					ImGui::EndCombo();
				}
			}

			if (ImGui::CollapsingHeader("OpenAI设置"))
			{
				components::input_text_with_hint("OpenAI端点", "https://api.openai.com/", g.session.chat_translator.OpenAI_endpoint);
				ImGui::Text("可使用兼容OpenAI格式的任何第三方站点,包括deepseek/Yi/GLM等国产API");
				components::input_text_with_hint("OpenAI token", "sk-*", g.session.chat_translator.OpenAI_key);
				components::input_text_with_hint("模型", "gpt-3.5-turbo", g.session.chat_translator.OpenAI_model);
				components::input_text_with_hint("目标语言(接收)##OpenAI", "Chinese", g.session.chat_translator.OpenAI_target_lang);
				components::input_text_with_hint("目标语言(发送)##OpenAIsend", "English", g.session.chat_translator.OpenAI_target_lang_send);
			}

			if (ImGui::CollapsingHeader("LibreTranslate设置"))
			{
				static const auto target_language = std::to_array<LibreTargetLanguage>({{"sq", "Albanian"}, {"ar", "Arabic"}, {"az", "Azerbaijani"}, {"bn", "Bengali"}, {"bg", "Bulgarian"}, {"ca", "Catalan"}, {"zh", "Chinese"}, {"zt", "Chinese(traditional)"}, {"cs", "Czech"}, {"da", "Danish"}, {"nl", "Dutch"}, {"en", "English"}, {"eo", "Esperanto"}, {"et", "Estonian"}, {"fi", "Finnish"}, {"fr", "French"}, {"de", "German"}, {"el", "Greek"}, {"he", "Hebrew"}, {"hi", "Hindi"}, {"hu", "Hungarian"}, {"id", "Indonesian"}, {"ga", "Irish"}, {"it", "Italian"}, {"ja", "Japanese"}, {"ko", "Korean"}, {"lv", "Latvian"}, {"lt", "Lithuanian"}, {"ms", "Malay"}, {"nb", "Norwegian"}, {"fa", "Persian"}, {"pl", "Polish"}, {"pt", "Portuguese"}, {"ro", "Romanian"}, {"ru", "Russian"}, {"sr", "Serbian"}, {"sk", "Slovak"}, {"sl", "Slovenian"}, {"es", "Spanish"}, {"sv", "Swedish"}, {"tl", "Tagalog"}, {"th", "Thai"}, {"tr", "Turkish"}, {"uk", "Ukrainian"}, {"ur", "Urdu"}, {"vi", "Vietnamese"}});

				components::input_text_with_hint("TRANSLATOR_ENDPOINT"_T.data(),
				    "http://localhost:5000/translate",
				    g.session.chat_translator.Libre_endpoint);

				if (ImGui::BeginCombo("目标语言(接收)##LibreTargetLangSwitcher",
				        g.session.chat_translator.Libre_target_lang.c_str()))
				{
					for (const auto& [type, name] : target_language)
					{
						components::selectable(name, false, [&type] {
							g.session.chat_translator.Libre_target_lang = type;
						});
					}
					ImGui::EndCombo();
				}
				if (ImGui::BeginCombo("目标语言(发送)##LibreTargetLangSwitchersend",
				        g.session.chat_translator.Libre_target_lang_send.c_str()))
				{
					for (const auto& [type, name] : target_language)
					{
						components::selectable(name, false, [&type] {
							g.session.chat_translator.Libre_target_lang_send = type;
						});
					}
					ImGui::EndCombo();
				}
			}

			components::button("发送英文测试消息", [] {
				chat_message messagetoadd{"testsender", "This is a test message", false, false};
				translate_queue.push(messagetoadd);
			});
		}	

		components::command_checkbox<"reportspam">();
	}
}
