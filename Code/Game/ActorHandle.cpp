#include "Game/ActorHandle.hpp"

const ActorHandle ActorHandle::INVALID(0x0000ffffu, 0x0000ffffu);

ActorHandle::ActorHandle() // this could be wrong.
{
	unsigned int maskedIndex = 0x0000ffffu & MAX_ACTOR_INDEX;
	unsigned int shiftedUid = MAX_ACTOR_UID << 16;
	m_data = maskedIndex | shiftedUid;
}

ActorHandle::ActorHandle(unsigned int uid, unsigned int index) // The index points to an array of actors that the map owns.
{
	// Mask out the left most 16 bits on the index
	// Shift the uid left 16 bits
	// bitwise or the masked index and shifted uid and set it as out data

	unsigned int maskedIndex = 0x0000ffffu & index;
	unsigned int shiftedUid = uid << 16;
	m_data = maskedIndex | shiftedUid;
}

bool ActorHandle::IsValid() const
{
	return true;
}

unsigned int ActorHandle::GetIndex() const
{
	return 0x0000ffffu & m_data;
}

unsigned int ActorHandle::GetData() const
{
	return m_data;
}

bool ActorHandle::operator!=(ActorHandle const& other) const
{
	if (other.m_data == m_data)
	{
		false;
	}
	return true;
}

bool ActorHandle::operator==(ActorHandle const& other) const
{
	if (other.m_data == m_data)
	{
		true;
	}
	return false;
}

