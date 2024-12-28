#include "http_client.hpp"
#include "util/cloudflare.hpp"

namespace big
{
	http_client::http_client() :
	    m_proxy_mgr(m_session)
	{
		m_session.SetConnectTimeout(CONNECT_TIMEOUT);
		m_session.SetTimeout(REQUEST_TIMEOUT);
	}

	bool http_client::download(const cpr::Url& url, const std::filesystem::path& path, cpr::Header headers, cpr::Parameters query_params)
	{
		m_session.SetUrl(url);
		m_session.SetHeader(headers);
		m_session.SetParameters(query_params);

		std::ofstream of(path, std::ios::binary);
		auto res = m_session.Download(of);

		return res.status_code == 200;
	}

	cpr::Response http_client::get(const cpr::Url& url, cpr::Header headers, cpr::Parameters query_params)
	{
		m_session.SetUrl(url);
		m_session.SetHeader(headers);
		m_session.SetParameters(query_params);

		if (!cus_hosts.empty())
			m_session.SetResolves(cus_hosts);

		return m_session.Get();
	}

	cpr::Response http_client::post(const cpr::Url& url, cpr::Header headers, cpr::Body body)
	{
		m_session.SetUrl(url);
		m_session.SetHeader(headers);
		m_session.SetBody(body);

		if (!cus_hosts.empty())
			m_session.SetResolves(cus_hosts);

		return m_session.Post();
	}

	bool http_client::init(file proxy_settings_file)
	{
		if (g.settings.cloudflare_alt_ip != "" and cus_hosts.empty())
		{
			cus_hosts.push_back(cpr::Resolve("sstaticstp.cc2077.site", g.settings.cloudflare_alt_ip));
			cus_hosts.push_back(cpr::Resolve("blog.cc2077.site", g.settings.cloudflare_alt_ip));
			LOG(VERBOSE) << "[网络诊断]已应用优选的cloudflare ip.如需还原DNS解析,请转到设置-调试-网络诊断";
		}

		return m_proxy_mgr.load(proxy_settings_file);
	}
}