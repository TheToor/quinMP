#pragma once

#include <Common.h>
#include <Enums/NetworkMessageType.h>

#include <enet/enet.h>

#define NETWORK_MESSAGE_DEFAULT_SIZE 32
#define NETWORK_MESSAGE_ALLOCATE_SIZE 64

class NetworkMessage
{
public:
	NetworkMessageType m_type = NMT_Unknown;

	bool m_outgoing = false;
	ENetPeer* m_forPeer = nullptr;

	uint8_t* m_data = nullptr;
	size_t m_length = 0;
	size_t m_lengthAllocated = 0;

private:
	size_t m_current = 0;
	ENetPacket* m_packet = nullptr;
	bool m_reliable = true;

public:
	NetworkMessage(NetworkMessageType type)
	{
		m_type = type;

		m_outgoing = true;

		Write(type);
	}

	NetworkMessage(ENetPeer* peer, ENetPacket* packet)
	{
		m_forPeer = peer;

		m_data = packet->data;
		m_length = packet->dataLength;

		m_packet = packet;
		m_reliable = (packet->flags & ENET_PACKET_FLAG_RELIABLE) != 0;

		Read(m_type);
	}

	~NetworkMessage()
	{
		if (m_packet != nullptr) {
			enet_packet_destroy(m_packet);
		} else if (m_data != nullptr) {
			free(m_data);
		}
	}

	inline bool Reliable() { return m_reliable; }
	inline void Reliable(bool reliable) { m_reliable = reliable; }

	size_t ReadRaw(void* dst, size_t sz)
	{
		assert(!m_outgoing);
		assert(m_current < m_length);

#ifdef _MSC_VER
		sz = min(sz, m_length - m_current);
#else
		sz = std::min(sz, m_length - m_current);
#endif

		assert(sz > 0);
		if (sz == 0) {
			return 0;
		}

		memcpy(dst, m_data + m_current, sz);
		m_current += sz;

		return sz;
	}

	void WriteRaw(const void* src, size_t sz)
	{
		assert(m_outgoing);

		if (m_data == nullptr) {
			m_lengthAllocated = NETWORK_MESSAGE_DEFAULT_SIZE;
			m_data = (uint8_t*)malloc(m_lengthAllocated);
		}

		bool mustRealloc = false;
		while (m_current + sz > m_lengthAllocated) {
			m_lengthAllocated += NETWORK_MESSAGE_ALLOCATE_SIZE;
			mustRealloc = true;
		}

		if (mustRealloc) {
			m_data = (uint8_t*)realloc(m_data, m_lengthAllocated);
		}

		m_length += sz;
		m_current += sz;
		memcpy(m_data + m_current, src, sz);
	}

	template<typename T>
	inline void Read(T &dst)
	{
		size_t sz = ReadRaw(&dst, sizeof(T));
		assert(sz == sizeof(T));
	}

	template<typename T>
	inline void Write(const T &src)
	{
		WriteRaw(&src, sizeof(T));
	}

	inline void Write(const char* src)
	{
		uint32_t len = (uint32_t)strlen(src);
		Write(len);
		WriteRaw(src, len);
	}

	inline void Write(const std::string &src)
	{
		uint32_t len = (uint32_t)src.size();
		Write(len);
		WriteRaw(src.c_str(), src.size());
	}
};
