#include <random>

#include "cloudflare.hpp"
#include "http_client/http_client.hpp"
#include "services/api/api_service.hpp"

using asio::ip::tcp;

namespace big
{
	std::mutex output_mutex;

	int tested_count = 0;
	int connected_count     = 0;

	std::vector<std::string> cips;


	static uint32_t ipToUint32(const std::string& ip)
	{
		uint32_t result = 0;
		size_t start = 0, end = ip.find('.');
		for (int i = 3; i >= 0; --i)
		{
			result |= (std::stoul(ip.substr(start, end - start)) << (i * 8));
			start = end + 1;
			end   = ip.find('.', start);
		}
		return result;
	}

	static std::string uint32ToIp(uint32_t ip)
	{
		return std::to_string((ip >> 24) & 0xFF) + "." + std::to_string((ip >> 16) & 0xFF) + "." + std::to_string((ip >> 8) & 0xFF) + "." + std::to_string(ip & 0xFF);
	}

	static std::tuple<uint32_t, uint32_t> parseCIDR(const std::string& cidr)
	{
		size_t slashPos    = cidr.find('/');
		std::string ipPart = cidr.substr(0, slashPos);
		int prefixLength   = std::stoi(cidr.substr(slashPos + 1));

		uint32_t startIp      = ipToUint32(ipPart);
		uint32_t numAddresses = 1 << (32 - prefixLength); // 2^(32 - prefixLength)

		return {startIp, numAddresses};
	}

	static std::vector<std::string> selectRandomIPs(const std::vector<std::string>& cidrRanges, int numIPs)
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::vector<std::string> selectedIPs;

		while (selectedIPs.size() < numIPs)
		{
			for (const auto& cidr : cidrRanges)
			{
				auto [startIp, numAddresses] = parseCIDR(cidr);

				if (numAddresses == 0)
					continue;

				std::uniform_int_distribution<uint32_t> dis(0, numAddresses - 1);
				uint32_t randomOffset = dis(gen);

				selectedIPs.push_back(uint32ToIp(startIp + randomOffset));

				if (selectedIPs.size() >= numIPs)
					break;
			}
		}

		return selectedIPs;
	}

	static bool tcp_ping(const std::string& host, unsigned short port, int timeout)
	{
		asio::io_context io_context;
		if (host.size() < 4)
			return false;

		try
		{
			asio::io_context io_context;
			tcp::resolver resolver(io_context);
			auto endpoints = resolver.resolve(host, std::to_string(port));
			tcp::socket socket(io_context);

			asio::steady_timer timer(io_context);
			bool connected = false;

			timer.expires_after(std::chrono::seconds(timeout));
			timer.async_wait([&](const asio::error_code& ec) {
				if (!ec)
				{
					socket.close();
				}
			});

			asio::async_connect(socket, endpoints, [&](const asio::error_code& ec, const tcp::endpoint&) {
				if (!ec)
				{
					connected = true;
				}
			});

			io_context.run();
			tested_count++;

			std::lock_guard<std::mutex> guard(output_mutex);
			if (connected)
			{
				connected_count++;
				cips.push_back(host);
				return true;
			}
		}
		catch (const std::exception& e)
		{
			std::lock_guard<std::mutex> guard(output_mutex);
			LOG(INFO) << "[网络诊断]捕获到异常: " << e.what();
		}
		return false;
	}

	static void segment_batch_job(const std::vector<std::string>& batch, unsigned short port, int timeout)
	{
		std::vector<std::thread> threads;
		for (const auto& ip : batch)
		{
			threads.emplace_back([=]() {
				tcp_ping(ip, port, timeout);
			});
		}

		for (auto& thread : threads)
		{
			thread.join();
		}
	}


	int cd_ip_auto_test()
	{
		unsigned short port = 443;

		std::vector<std::string> known_cf_ip_ranges = {"173.245.48.0/20", "103.21.244.0/22", "103.22.200.0/22", "103.31.4.0/22", "141.101.64.0/18", "108.162.192.0/18", "190.93.240.0/20", "188.114.96.0/20", "197.234.240.0/22", "198.41.128.0/17", "162.158.0.0/15", "104.16.0.0/12", "172.64.0.0/17", "172.64.128.0/18", "172.64.192.0/19", "172.64.224.0/22", "172.64.229.0/24", "172.64.230.0/23", "172.64.232.0/21", "172.64.240.0/21", "172.64.248.0/21", "172.65.0.0/16", "172.66.0.0/16", "172.67.0.0/16", "131.0.72.0/22"};

		std::vector<std::string> test_queue_ip = selectRandomIPs(known_cf_ip_ranges, pre_test_ip_limit);

		LOG(INFO) << "[网络诊断]已从Cloudflare公开的IP地址段中随机选择 " << pre_test_ip_limit << " 个ip";

		for (size_t i = 0; i < test_queue_ip.size(); i += thread_counts)
		{
			if (cips.size() > retest_ip_limite)
			{
				LOG(INFO) << "[网络诊断]已获得超过" << retest_ip_limite << "个可用ip,结束tcping预筛选,即将进行丢包测试和文件下载验证"
				          << "\n预筛选ip总数:" << tested_count << " 连接成功数:" << connected_count;
				break;
			}
			std::vector<std::string> batch(test_queue_ip.begin() + i,
			    test_queue_ip.begin() + std::min(i + thread_counts, test_queue_ip.size()));
			segment_batch_job(batch, port, timeout);
		}

			for (const auto& ip : cips)
			{
			    if (ip.size() < 4)
				    continue;

			    bool isok = true;
				for (int i = 0; i < 5; i++)
				{
					if (!tcp_ping(ip, port, 2))
					{
					    LOG(INFO) << "[网络诊断] " << ip << " 未通过丢包测试,测试下一个ip";
					    isok = false;
					    break;
					}
				}
			    if (isok)
			    {
				    g.settings.cloudflare_alt_ip = "";
				    if (!cus_hosts.empty())
					    cus_hosts.clear();

				    cus_hosts.push_back(cpr::Resolve("sstaticstp.cc2077.site", ip));
				    cus_hosts.push_back(cpr::Resolve("blog.cc2077.site", ip));

				    LOG(INFO) << "[网络诊断] " << ip << "通过丢包测试,即将进行文件下载验证";
				    std::vector<std::string> spam_texts_tmp = g_api_service->get_ad_list();

				    if (!spam_texts_tmp.empty())
				    {
					    LOG(INFO) << "[网络诊断]文件下载验证成功,已存储可用ip:" << ip;
					    g.settings.cloudflare_alt_ip = ip;
					    return 0;
				    }
				    else
					    LOG(INFO) << "[网络诊断]文件下载验证失败,测试下一个ip";
			    }
			}

		return 1;
	}
}