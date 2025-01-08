#pragma once

namespace KZ::API
{
	enum class Mode : u8
	{
		Vanilla = 1,
		Classic = 2,
	};

	static bool DecodeModeString(const std::string &modeString, Mode &mode)
	{
		if (modeString == "vanilla")
		{
			mode = Mode::Vanilla;
			return true;
		}
		else if (modeString == "classic")
		{
			mode = Mode::Classic;
			return true;
		}
		else
		{
			return false;
		}
	}
} // namespace KZ::API
