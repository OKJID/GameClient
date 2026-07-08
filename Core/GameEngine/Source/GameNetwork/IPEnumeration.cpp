/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "GameNetwork/IPEnumeration.h"
#include "GameNetwork/networkutil.h"
#include "GameClient/ClientInstance.h"

#ifdef __APPLE__
#include <ifaddrs.h>
#include <net/if.h>
#endif

IPEnumeration::IPEnumeration()
{
	m_IPlist = nullptr;
	m_isWinsockInitialized = false;
}

IPEnumeration::~IPEnumeration()
{
	if (m_isWinsockInitialized)
	{
#ifndef __APPLE__
		WSACleanup();
#endif
		m_isWinsockInitialized = false;
	}

	EnumeratedIP *ip = m_IPlist;
	while (ip)
	{
		ip = ip->getNext();
		deleteInstance(m_IPlist);
		m_IPlist = ip;
	}
}

EnumeratedIP * IPEnumeration::getAddresses()
{
	if (m_IPlist)
		return m_IPlist;

	if (!m_isWinsockInitialized)
	{
#ifndef __APPLE__
		WORD verReq = MAKEWORD(2, 2);
		WSADATA wsadata;

		int err = WSAStartup(verReq, &wsadata);
		if (err != 0) {
			return nullptr;
		}

		if ((LOBYTE(wsadata.wVersion) != 2) || (HIBYTE(wsadata.wVersion) !=2)) {
			WSACleanup();
			return nullptr;
		}
#endif
		m_isWinsockInitialized = true;
	}

	// TheSuperHackers @feature Add one unique local host IP address for each multi client instance.
	if (rts::ClientInstance::isMultiInstance())
	{
		const UnsignedInt id = rts::ClientInstance::getInstanceId();
		addNewIP(
			127,
			(UnsignedByte)(id >> 16),
			(UnsignedByte)(id >> 8),
			(UnsignedByte)(id));
	}

#ifndef __APPLE__
	// get the local machine's host name
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)))
	{
		DEBUG_LOG(("Failed call to gethostname; WSAGetLastError returned %d", WSAGetLastError()));
		return nullptr;
	}
	DEBUG_LOG(("Hostname is '%s'", hostname));

	// get host information from the host name
	HOSTENT* hostEnt = gethostbyname(hostname);
	if (hostEnt == nullptr)
	{
		DEBUG_LOG(("Failed call to gethostbyname; WSAGetLastError returned %d", WSAGetLastError()));
		return nullptr;
	}

	// sanity-check the length of the IP adress
	if (hostEnt->h_length != 4)
	{
		DEBUG_LOG(("gethostbyname returns oddly-sized IP addresses!"));
		return nullptr;
	}

	// construct a list of addresses
	int numAddresses = 0;
	char *entry;
	while ( (entry = hostEnt->h_addr_list[numAddresses++]) != nullptr )
	{
		addNewIP(
			(UnsignedByte)entry[0],
			(UnsignedByte)entry[1],
			(UnsignedByte)entry[2],
			(UnsignedByte)entry[3]);
	}
#else // __APPLE__
	struct ifaddrs *ifaddrList = nullptr;
	if (getifaddrs(&ifaddrList) == 0)
	{
		for (struct ifaddrs *ifa = ifaddrList; ifa != nullptr; ifa = ifa->ifa_next)
		{
			if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET)
			{
				continue;
			}
			if ((ifa->ifa_flags & IFF_UP) == 0 || (ifa->ifa_flags & IFF_LOOPBACK) != 0)
			{
				continue;
			}

			// Skip virtual/tunnel interfaces (ZeroTier, Hamachi, utun, etc.)
			const char *ifname = ifa->ifa_name;
			if (strncmp(ifname, "feth", 4) == 0 ||
				strncmp(ifname, "utun", 4) == 0 ||
				strncmp(ifname, "zt", 2) == 0 ||
				strncmp(ifname, "awdl", 4) == 0 ||
				strncmp(ifname, "llw", 3) == 0 ||
				strncmp(ifname, "bridge", 6) == 0 ||
				strncmp(ifname, "ap", 2) == 0)
			{
				const struct sockaddr_in *sin = (const struct sockaddr_in *)ifa->ifa_addr;
				const UnsignedInt addr = ntohl(sin->sin_addr.s_addr);
				DEBUG_LOG(("IPEnumeration: skipping virtual interface %s (IP: %d.%d.%d.%d)",
					ifname,
					(int)(addr >> 24), (int)((addr >> 16) & 0xFF),
					(int)((addr >> 8) & 0xFF), (int)(addr & 0xFF)));
				continue;
			}

			const struct sockaddr_in *sin = (const struct sockaddr_in *)ifa->ifa_addr;
			const UnsignedInt addr = ntohl(sin->sin_addr.s_addr);
			addNewIP(
				(UnsignedByte)(addr >> 24),
				(UnsignedByte)(addr >> 16),
				(UnsignedByte)(addr >> 8),
				(UnsignedByte)(addr));
		}
		freeifaddrs(ifaddrList);
	}
#endif

	return m_IPlist;
}

void IPEnumeration::addNewIP( UnsignedByte a, UnsignedByte b, UnsignedByte c, UnsignedByte d )
{
	EnumeratedIP *newIP = newInstance(EnumeratedIP);

	AsciiString str;
	str.format("%d.%d.%d.%d", (int)a, (int)b, (int)c, (int)d);

	UnsignedInt ip = AssembleIp(a, b, c, d);

	newIP->setIPstring(str);
	newIP->setIP(ip);

	DEBUG_LOG(("IP: 0x%8.8X (%s)", ip, str.str()));

	// Add the IP to the list in ascending order
	if (!m_IPlist)
	{
		m_IPlist = newIP;
		newIP->setNext(nullptr);
	}
	else
	{
		if (newIP->getIP() < m_IPlist->getIP())
		{
			newIP->setNext(m_IPlist);
			m_IPlist = newIP;
		}
		else
		{
			EnumeratedIP *p = m_IPlist;
			while (p->getNext() && p->getNext()->getIP() < newIP->getIP())
			{
				p = p->getNext();
			}
			newIP->setNext(p->getNext());
			p->setNext(newIP);
		}
	}
}

AsciiString IPEnumeration::getMachineName()
{
	if (!m_isWinsockInitialized)
	{
#ifndef __APPLE__
		WORD verReq = MAKEWORD(2, 2);
		WSADATA wsadata;

		int err = WSAStartup(verReq, &wsadata);
		if (err != 0) {
			return "";
		}

		if ((LOBYTE(wsadata.wVersion) != 2) || (HIBYTE(wsadata.wVersion) !=2)) {
			WSACleanup();
			return "";
		}
#endif
		m_isWinsockInitialized = true;
	}

	// get the local machine's host name
	char hostname[256];
	if (gethostname(hostname, sizeof(hostname)))
	{
		DEBUG_LOG(("Failed call to gethostname"));
		return "";
	}

	return AsciiString(hostname);
}


