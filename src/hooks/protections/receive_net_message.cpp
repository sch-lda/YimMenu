#include "backend/command.hpp"
#include "backend/context/chat_command_context.hpp"
#include "backend/player_command.hpp"
#include "core/data/packet_types.hpp"
#include "gta/enums.hpp"
#include "gta/net_game_event.hpp"
#include "gta_util.hpp"
#include "hooking/hooking.hpp"
#include "lua/lua_manager.hpp"
#include "natives.hpp"
#include "script/scriptIdBase.hpp"
#include "services/players/player_service.hpp"
#include "util/chat.hpp"
#include "util/session.hpp"

#include <network/Network.hpp>
#include <network/netTime.hpp>


inline void gamer_handle_deserialize(rage::rlGamerHandle& hnd, rage::datBitBuffer& buf)
{
	if ((hnd.m_platform = buf.Read<uint8_t>(sizeof(hnd.m_platform) * 8)) != rage::rlPlatforms::PC)
		return;

	buf.ReadRockstarId(&hnd.m_rockstar_id);
	hnd.m_padding = buf.Read<uint8_t>(sizeof(hnd.m_padding) * 8);
}

inline bool is_kick_instruction(rage::datBitBuffer& buffer)
{
	rage::rlGamerHandle sender, receiver;
	char name[18];
	gamer_handle_deserialize(sender, buffer);
	gamer_handle_deserialize(receiver, buffer);
	buffer.ReadString(name, 17);
	int instruction = buffer.Read<int>(32);
	return instruction == 8;
}

namespace big
{
	bool get_msg_type(rage::eNetMessage& msgType, rage::datBitBuffer& buffer)
	{
		uint32_t pos;
		uint32_t magic;
		uint32_t length;
		uint32_t extended{};
		if ((buffer.m_flagBits & 2) != 0 || (buffer.m_flagBits & 1) == 0 ? (pos = buffer.m_curBit) : (pos = buffer.m_maxBit),
		    buffer.m_bitsRead + 15 > pos || !buffer.ReadDword(&magic, 14) || magic != 0x3246 || !buffer.ReadDword(&extended, 1))
		{
			msgType = rage::eNetMessage::MsgInvalid;
			return false;
		}
		length = extended ? 16 : 8;
		if ((buffer.m_flagBits & 1) == 0 ? (pos = buffer.m_curBit) : (pos = buffer.m_maxBit),
		    length + buffer.m_bitsRead <= pos && buffer.ReadDword((uint32_t*)&msgType, length))
			return true;
		else
			return false;
	}

	static void script_id_deserialize(CGameScriptId& id, rage::datBitBuffer& buffer)
	{
		id.m_hash      = buffer.Read<uint32_t>(32);
		id.m_timestamp = buffer.Read<uint32_t>(32);

		if (buffer.Read<bool>(1))
			id.m_position_hash = buffer.Read<uint32_t>(32);

		if (buffer.Read<bool>(1))
			id.m_instance_id = buffer.Read<int32_t>(8);
	}

	bool hooks::receive_net_message(void* netConnectionManager, void* a2, rage::netConnection::InFrame* frame)
	{
		if (frame->get_event_type() != rage::netConnection::InFrame::EventType::FrameReceived)
			return g_hooking->get_original<hooks::receive_net_message>()(netConnectionManager, a2, frame);

		if (frame->m_data == nullptr || frame->m_length == 0)
			return g_hooking->get_original<hooks::receive_net_message>()(netConnectionManager, a2, frame);

		rage::datBitBuffer buffer(frame->m_data, frame->m_length);
		buffer.m_flagBits = 1;

		rage::eNetMessage msgType;
		player_ptr player = nullptr;

		for (uint32_t i = 0; i < gta_util::get_network()->m_game_session_ptr->m_player_count; i++)
		{
			if (auto player_iter = gta_util::get_network()->m_game_session_ptr->m_players[i])
			{
				if (frame && player_iter->m_player_data.m_peer_id_2 == frame->m_peer_id)
				{
					player = g_player_service->get_by_host_token(
					    gta_util::get_network()->m_game_session_ptr->m_players[i]->m_player_data.m_host_token);
					break;
				}
			}
		}

		if (!get_msg_type(msgType, buffer))
			return g_hooking->get_original<hooks::receive_net_message>()(netConnectionManager, a2, frame);


		if (msgType == rage::eNetMessage::MsgRoamingJoinBubbleAck)
			return true;

		if (player)
		{
			switch (msgType)
			{
			case rage::eNetMessage::MsgTextMessage:
			case rage::eNetMessage::MsgTextMessage2:
			{
				char message[256];
				rage::rlGamerHandle handle{};
				bool is_team;
				buffer.ReadString(message, sizeof(message));
				gamer_handle_deserialize(handle, buffer);
				buffer.ReadBool(&is_team);

				if (player->is_spammer)
					return true;

				if (!(player->is_trusted || (player->is_friend() && g.session.trust_friends) || g.session.trust_session))
				{
					for (auto rid : spam_rid)
					{
						if (player->get_rockstar_id() == rid)
						{
							if (g.session.log_chat_messages)
								LOG(INFO) << "Known AD rid: " << player->get_name() << " (" << player->get_rockstar_id() << ")";
							player->is_spammer = true;
							session::add_infraction(player, Infraction::CHAT_SPAM);
							g.reactions.chat_spam.process(player);
							return true;
						}
					}
				}

				if (auto spam_reason = chat::is_text_spam(message, player))
				{
					if (g.session.log_chat_messages)
						chat::log_chat(message, player, spam_reason, is_team);

					if ((g.session.spam_timer <= 5.0f && g.session.spam_length > 20 && g.session.auto_report_spam) || spam_reason == SpamReason::STATIC_DETECTION)
					{
						g_thread_pool->push([message, player, spam_reason] {
							bool isok = g_api_service->report_spam(message, player->get_rockstar_id(), spam_reason);
						});
					}

					player->is_spammer = true;
					if (!(player->is_trusted || (player->is_friend() && g.session.trust_friends) || g.session.trust_session))
					{
						session::add_infraction(player, Infraction::CHAT_SPAM);
						g.reactions.chat_spam.process(player);
					}
					return true;
				}
				else
				{
					if (g.session.log_chat_messages)
						chat::log_chat(message, player, SpamReason::NOT_A_SPAMMER, is_team);
					if (g.session.chat_translator.enabled)
					{
						chat_message new_message{player->get_name(), message};
						translate_queue.push(new_message);
					}

					if (g.session.chat_commands && message[0] == g.session.chat_command_prefix)
						command::process(std::string(message + 1), std::make_shared<chat_command_context>(player));
					else
						g_lua_manager->trigger_event<menu_event::ChatMessageReceived>(player->id(), message);

					if (msgType == rage::eNetMessage::MsgTextMessage && g_pointers->m_gta.m_chat_data && player->get_net_data())
					{
						buffer.Seek(0);
						return g_hooking->get_original<hooks::receive_net_message>()(netConnectionManager, a2, frame); // Call original function since we can't seem to handle it
					}
				}
				break;
			}
			case rage::eNetMessage::MsgScriptMigrateHost:
			{
				if (player->m_host_migration_rate_limit.process())
				{
					if (player->m_host_migration_rate_limit.exceeded_last_process())
					{
						session::add_infraction(player, Infraction::TRIED_KICK_PLAYER);
						auto p_name = player->get_name();

						g_notification_service.push_error("PROTECTIONS"_T.data(), std::vformat("OOM_KICK"_T, std::make_format_args(p_name)));
					}
					return true;
				}
				break;
			}
			case rage::eNetMessage::MsgScriptHostRequest:
			{
				CGameScriptId script;
				script_id_deserialize(script, buffer);

				if (script.m_hash == "freemode"_J && g.session.force_script_host)
					return true;

				break;
			}
			case rage::eNetMessage::MsgNetTimeSync:
			{
				int action         = buffer.Read<int>(2);
				uint32_t counter   = buffer.Read<uint32_t>(32);
				uint32_t token     = buffer.Read<uint32_t>(32);
				uint32_t timestamp = buffer.Read<uint32_t>(32);
				uint32_t time_diff = (*g_pointers->m_gta.m_network_time)->m_time_offset + frame->m_timestamp;

				if (action == 0)
				{
					player->player_time_value = timestamp;
					player->player_time_value_received_time = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
					if (!player->time_difference || time_diff > player->time_difference.value())
						player->time_difference = time_diff;
				}
				break;
			}
			case rage::eNetMessage::MsgKickPlayer:
			{
				KickReason reason = buffer.Read<KickReason>(3);

				if (!player->is_host())
					return true;

				if (reason == KickReason::VOTED_OUT)
				{
					g_notification_service.push_warning("PROTECTIONS"_T.data(), "YOU_HAVE_BEEN_KICKED"_T.data());
					return true;
				}

				break;
			}
			case rage::eNetMessage::MsgRadioStationSyncRequest:
			{
				if (player->block_radio_requests)
					return true;

				if (player->m_radio_request_rate_limit.process())
				{
					if (player->m_radio_request_rate_limit.exceeded_last_process())
					{
						session::add_infraction(player, Infraction::TRIED_KICK_PLAYER);
						auto p_name = player->get_name();

						g_notification_service.push_error("PROTECTIONS"_T.data(), std::vformat("OOM_KICK"_T, std::make_format_args(p_name)));
						player->block_radio_requests = true;
					}
					return true;
				}

				break;
			}
			case rage::eNetMessage::MsgRoamingInitialBubble:
			{
				LOG(WARNING) << "Shouldn't get this again";
				return true;
			}
			}
		}
		else
		{
			switch (msgType)
			{
			case rage::eNetMessage::MsgScriptMigrateHost: return true;
			case rage::eNetMessage::MsgRadioStationSyncRequest:
			{
				static rate_limiter unk_player_radio_requests{1s, 6};

				if (unk_player_radio_requests.process())
				{
					if (unk_player_radio_requests.exceeded_last_process())
					{
						// Make a translation for this new OOM kick protection
						g_notification_service.push_error("PROTECTIONS"_T.data(), "OOM_KICK_UNK"_T.data());
					}
					return true;
				}
				break;
			}
			case rage::eNetMessage::MsgNonPhysicalData:
			{
				buffer.Read<int>(7); // size
				int bubble = buffer.Read<int>(4);
				int player = buffer.Read<int>(6);


				if (g_player_service->get_self() && g_player_service->get_self()->id() == player)
				{
					LOG(WARNING) << "We're being replaced";
					return true;
				}

				if (bubble != 0)
				{
					LOG(WARNING) << "Wrong bubble: " << bubble;
				}
				break;
			}
			}
		}

		if (g.debug.logs.packet_logs) [[unlikely]]
		{
			if (g.debug.logs.packet_logs == 1 || //ALL
			    (g.debug.logs.packet_logs == 2 && msgType != rage::eNetMessage::MsgCloneSync && msgType != rage::eNetMessage::MsgPackedCloneSyncACKs && msgType != rage::eNetMessage::MsgPackedEvents && msgType != rage::eNetMessage::MsgPackedReliables && msgType != rage::eNetMessage::MsgPackedEventReliablesMsgs && msgType != rage::eNetMessage::MsgNetArrayMgrUpdate && msgType != rage::eNetMessage::MsgNetArrayMgrSplitUpdateAck && msgType != rage::eNetMessage::MsgNetArrayMgrUpdateAck && msgType != rage::eNetMessage::MsgScriptHandshakeAck && msgType != rage::eNetMessage::MsgScriptHandshake && msgType != rage::eNetMessage::MsgScriptJoin && msgType != rage::eNetMessage::MsgScriptJoinAck && msgType != rage::eNetMessage::MsgScriptJoinHostAck && msgType != rage::eNetMessage::MsgRequestObjectIds && msgType != rage::eNetMessage::MsgInformObjectIds && msgType != rage::eNetMessage::MsgNetTimeSync)) //FILTERED
			{
				const char* packet_type = "<UNKNOWN>";
				for (const auto& p : packet_types)
				{
					if (p.second == (int)msgType)
					{
						packet_type = p.first;
						break;
					}
				}

				auto now        = std::chrono::system_clock::now();
				auto ms         = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
				auto timer      = std::chrono::system_clock::to_time_t(now);
				auto local_time = *std::localtime(&timer);

				static std::ofstream log(g_file_manager.get_project_file("./packets.log").get_path(), std::ios::app);
				log << "[" << std::put_time(&local_time, "%m/%d/%Y %I:%M:%S") << ":" << std::setfill('0') << std::setw(3) << ms.count() << " " << std::put_time(&local_time, "%p") << "] "
				    << "RECEIVED PACKET | Type: " << packet_type << " | Length: " << frame->m_length << " | Sender: "
				    << (player ? player->get_name() :
				                 std::format("<M:{}>, <C:{:X}>, <P:{}>",
				                     (int)frame->m_msg_id,
				                     frame->m_connection_identifier,
				                     frame->m_peer_id)
				                     .c_str())
				    << " | " << HEX_TO_UPPER((int)msgType) << std::endl;
				log.flush();
			}
		}

		return g_hooking->get_original<hooks::receive_net_message>()(netConnectionManager, a2, frame);
	}
}
