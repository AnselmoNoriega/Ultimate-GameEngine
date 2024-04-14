#pragma once

namespace NR
{
	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

		operator uint64_t() const { return mUUID; }

	private:
		uint64_t mUUID;
	};

}

namespace std 
{
	template <typename T> 
	struct hash;

	template<>
	struct hash<NR::UUID>
	{
		std::size_t operator()(const NR::UUID& uuid) const
		{
			return (uint64_t)uuid;
		}
	};
}