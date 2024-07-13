#pragma once
#include "core/enums.hpp"
#include "core/scr_globals.hpp"
#include "fiber_pool.hpp"
#include "file_manager/file.hpp"
#include "natives.hpp"
#include "packet.hpp"
#include "script.hpp"
#include "services/players/player_service.hpp"

#include <network/CNetGamePlayer.hpp>
#include <network/ChatData.hpp>
#include <script/HudColor.hpp>
#include <script/globals/GPBD_FM_3.hpp>


namespace
{
	static void gamer_handle_serialize(rage::rlGamerHandle& hnd, rage::datBitBuffer& buf)
	{
		buf.Write<uint8_t>(hnd.m_platform, sizeof(hnd.m_platform) * 8);
		if (hnd.m_platform == rage::rlPlatforms::PC)
		{
			buf.WriteRockstarId(hnd.m_rockstar_id);
			buf.Write<uint8_t>(hnd.m_padding, sizeof(hnd.m_padding) * 8);
		}
	}

}

namespace big
{
	struct chat_message
	{
		std::string sender;
		std::string content;
		bool issend;
		bool isteam;
	};

	inline std::vector<std::string> spam_texts;
	inline std::vector<uint64_t> spam_rid;

	inline std::queue<chat_message> translate_queue;
}

namespace big::chat
{
	inline SpamReason is_text_spam(const char* text, player_ptr player)
	{
		if ((player->is_trusted || (g.session.trust_friends && player->is_friend())))
			return SpamReason::NOT_A_SPAMMER; // don't filter messages from friends

		if (g.session.use_spam_timer)
		{
			if (player->last_message_time.has_value())
			{
				auto currentTime = std::chrono::steady_clock::now();
				auto diff = std::chrono::duration_cast<std::chrono::seconds>(currentTime - player->last_message_time.value());
				player->last_message_time.emplace(currentTime);

				if (strlen(text) > g.session.spam_length && diff.count() <= g.session.spam_timer)
					return SpamReason::TIMER_DETECTION;
			}
			else
			{
				player->last_message_time.emplace(std::chrono::steady_clock::now());
			}
		}
		for (auto e : spam_texts)
		{
			std::string e_str(e);
			std::transform(e_str.begin(), e_str.end(), e_str.begin(), ::tolower);
			std::string text_str(text);
			std::transform(text_str.begin(), text_str.end(), text_str.begin(), ::tolower);
			if (strstr(text_str.c_str(), e_str.c_str()) != 0)
				return SpamReason::STATIC_DETECTION;
		}
		return SpamReason::NOT_A_SPAMMER;
	}

	inline void log_chat(char* msg, player_ptr player, SpamReason spam_reason, bool is_team)
	{
		std::ofstream log(
		    g_file_manager.get_project_file(spam_reason != SpamReason::NOT_A_SPAMMER ? "./spam.log" : "./chat.log").get_path(),
		    std::ios::app);

		auto rockstar_id = player->get_rockstar_id();
		auto ip          = player->get_ip_address();

		auto now        = std::chrono::system_clock::now();
		auto ms         = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
		auto timer      = std::chrono::system_clock::to_time_t(now);
		auto local_time = *std::localtime(&timer);

		std::string spam_reason_str = "";

		switch (spam_reason)
		{
		case SpamReason::STATIC_DETECTION: spam_reason_str = "(Static Detection) "; break;
		case SpamReason::TIMER_DETECTION: spam_reason_str = "(Timer Detection) "; break;
		}

		log << spam_reason_str << "[" << std::put_time(&local_time, "%m/%d/%Y %I:%M:%S") << ":" << std::setfill('0') << std::setw(3) << ms.count() << " " << std::put_time(&local_time, "%p") << "] ";

		if (ip)
			log << player->get_name() << " (" << rockstar_id << ") <" << (int)ip.value().m_field1 << "."
			    << (int)ip.value().m_field2 << "." << (int)ip.value().m_field3 << "." << (int)ip.value().m_field4 << "> " << ((is_team == true) ? "[TEAM]: " : "[ALL]: ") << msg << std::endl;
		else
			log << player->get_name() << " (" << rockstar_id << ") <UNKNOWN> " << ((is_team == true) ? "[TEAM]: " : "[ALL]: ") << msg << std::endl;

		log.close();
	}

	inline void render_chat(const char* msg, const char* player_name, bool is_team)
	{
		int scaleform = GRAPHICS::REQUEST_SCALEFORM_MOVIE("MULTIPLAYER_CHAT");

		while (!GRAPHICS::HAS_SCALEFORM_MOVIE_LOADED(scaleform))
			script::get_current()->yield();

		GRAPHICS::BEGIN_SCALEFORM_MOVIE_METHOD(scaleform, "ADD_MESSAGE");
		GRAPHICS::SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING(player_name); // player name
		GRAPHICS::SCALEFORM_MOVIE_METHOD_ADD_PARAM_LITERAL_STRING(msg);             // content
		GRAPHICS::SCALEFORM_MOVIE_METHOD_ADD_PARAM_TEXTURE_NAME_STRING(HUD::GET_FILENAME_FOR_AUDIO_CONVERSATION(is_team ? "MP_CHAT_TEAM" : "MP_CHAT_ALL")); // scope
		GRAPHICS::SCALEFORM_MOVIE_METHOD_ADD_PARAM_BOOL(false);                               // teamOnly
		GRAPHICS::SCALEFORM_MOVIE_METHOD_ADD_PARAM_INT((int)HudColor::HUD_COLOUR_PURE_WHITE); // eHudColour
		GRAPHICS::END_SCALEFORM_MOVIE_METHOD();

		GRAPHICS::BEGIN_SCALEFORM_MOVIE_METHOD(scaleform, "SET_FOCUS");
		GRAPHICS::SCALEFORM_MOVIE_METHOD_ADD_PARAM_INT(1);                                    // VISIBLE_STATE_DEFAULT
		GRAPHICS::SCALEFORM_MOVIE_METHOD_ADD_PARAM_INT(0);                                    // scopeType (unused)
		GRAPHICS::SCALEFORM_MOVIE_METHOD_ADD_PARAM_INT(0);                                    // scope (unused)
		GRAPHICS::SCALEFORM_MOVIE_METHOD_ADD_PARAM_PLAYER_NAME_STRING(player_name);           // player
		GRAPHICS::SCALEFORM_MOVIE_METHOD_ADD_PARAM_INT((int)HudColor::HUD_COLOUR_PURE_WHITE); // eHudColour
		GRAPHICS::END_SCALEFORM_MOVIE_METHOD();

		GRAPHICS::DRAW_SCALEFORM_MOVIE_FULLSCREEN(scaleform, 255, 255, 255, 255, 0);

		// fix broken scaleforms, when chat alrdy opened
		if (const auto chat_data = *g_pointers->m_gta.m_chat_data; chat_data && (chat_data->m_chat_open || chat_data->m_timer_two))
			HUD::CLOSE_MP_TEXT_CHAT();
	}

	inline void draw_chat(const std::string& message, const std::string& sender, bool is_team)
	{
		if (rage::tlsContext::get()->m_is_script_thread_active)
			render_chat(message.c_str(), sender.c_str(), is_team);
		else
			g_fiber_pool->queue_job([message, sender, is_team] {
				render_chat(message.c_str(), sender.c_str(), is_team);
			});
	}

	inline bool is_on_same_team(CNetGamePlayer* player)
	{
		auto target_id = player->m_player_id;

		if (NETWORK::NETWORK_IS_ACTIVITY_SESSION())
		{
			// mission
			return PLAYER::GET_PLAYER_TEAM(target_id) == PLAYER::GET_PLAYER_TEAM(self::id);
		}
		else
		{
			auto boss_goon = &scr_globals::gpbd_fm_3.as<GPBD_FM_3*>()->Entries[self::id].BossGoon;

			if (boss_goon->Boss == target_id)
				return true;

			if (boss_goon->Boss == -1)
				return false;

			if (boss_goon->Boss != self::id)
				boss_goon = &scr_globals::gpbd_fm_3.as<GPBD_FM_3*>()->Entries[boss_goon->Boss].BossGoon; // get their structure

			// bypass some P2Cs
			for (int i = 0; i < boss_goon->Goons.Size; i++)
			{
				if (boss_goon->Goons[i] == target_id)
				{
					return true;
				}
			}

			return false;
		}
	}

	// set target to send to a specific player
	inline void send_message(const std::string& message, player_ptr target = nullptr, bool draw = true, bool is_team = false)
	{
		if (!*g_pointers->m_gta.m_is_session_started)
			return;

		packet msg{};
		msg.write_message(rage::eNetMessage::MsgTextMessage);
		msg.m_buffer.WriteString(message.c_str(), 256);
		gamer_handle_serialize(g_player_service->get_self()->get_net_data()->m_gamer_handle, msg.m_buffer);
		msg.write<bool>(is_team, 1);


		for (auto& player : g_player_service->players())
		{
			if (player.second && player.second->is_valid())
			{
				if (target && player.second != target)
					continue;

				if (!target && is_team && !is_on_same_team(player.second->get_net_game_player()))
					continue;

				msg.send(player.second->get_net_game_player()->m_msg_id);
			}
		}

		if (draw)
			draw_chat(message, g_player_service->get_self()->get_name(), is_team);
	}
}

