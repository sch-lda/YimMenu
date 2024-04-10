#include "backend/looped/looped.hpp"
#include "gta_util.hpp"
#include "script_local.hpp"
#include "util/chat.hpp"
#include "services/api/api_service.hpp"
#include "thread_pool.hpp"

namespace big
{
	inline std::atomic_bool translate_lock{false};

	void looped::chat_translate()
	{
		while (!MsgQueue.empty() and !translate_lock and g.session.translatechat)
		{
			if (MsgQueue.size() >= 3)
			{
				LOG(WARNING) << "消息队列过长,正在清理.打开广告机计时限速规则可能缓解";
				while (!MsgQueue.empty())
					MsgQueue.pop();
				continue;
			}

			try
			{
				auto& first_message     = MsgQueue.front();
				translate_lock = true;
				g_thread_pool->push([first_message] {
					std::string translate_result;
					std::string sender = "[T]" + first_message.sender;
					switch (g.session.t_service_provider)
					{
						case 0:
							translate_result = g_api_service->get_translation_from_Bing(first_message.content, g.session.Bing_target_lang);
							break;
						case 1: 
							translate_result = g_api_service->get_translation_from_Google(first_message.content, g.session.Google_target_lang);
							break;
					    case 2:
						    translate_result = g_api_service->get_translation_from_Deeplx(first_message.content, g.session.DeepL_target_lang);
						    break;
					    case 3:
							translate_result = g_api_service->get_translation_from_OpenAI(first_message.content, g.session.OpenAI_target_lang);
						    break;
					}
					
					translate_lock   = false;
					if (translate_result != "Error" && translate_result != "None")
					{
						if (g.session.chat_translator_send)
							chat::send_message(std::format("[{}]", first_message.sender) + translate_result,
							    nullptr,
							    true,
							    g.session.chat_translator_send_team);
						if (g.session.chat_translator_draw)
						{
							if (rage::tlsContext::get()->m_is_script_thread_active)
								chat::draw_chat(translate_result.c_str(), sender.c_str(), false);
							else
								g_fiber_pool->queue_job([translate_result, sender] {
									chat::draw_chat(translate_result.c_str(), sender.c_str(), false);
								});
						}
						if (g.session.chat_translator_print)
							LOG(INFO) << "[" << first_message.sender << "]" << first_message.content << " --> " << translate_result;
					}
				});
				MsgQueue.pop();
			}

			catch (std::exception& e)
			{
				LOG(WARNING) << "Error: " << e.what();
			}
		}
	}
}
