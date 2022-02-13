#pragma once

#include <string>
#include <unordered_map>

namespace sge {

/// A class assigning a unique id for every requested string.
/// The zero is reserved and no string will be assigned with that variable.
struct StringRegister {
	StringRegister() = default;

	int getIndex(const std::string& str)
	{
		int res = strings[str];

		if (res != 0)
			return res;

		strings[str] = (int)strings.size();
		return (int)strings.size();
	}

  private:
	std::unordered_map<std::string, int> strings;
};


} // namespace sge
