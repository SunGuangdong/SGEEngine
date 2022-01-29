#pragma once

#include <string>
#include <vector>

namespace sge {

/// Option permutator generates all possible combinations, of the spcified
/// options.
struct OptionPermuataor {
  public:
	typedef int OptionIDType;
	struct OptionDesc {
		OptionIDType optionId;
		std::string name;
		std::vector<std::string> possibleValues;
	};

	struct OptionChoice {
		OptionIDType optionId;
		int optionChoice;
	};

  public:
	OptionPermuataor() = default;

	OptionPermuataor(const std::vector<OptionDesc>& options) {
		build(options);
	}

	void build(const std::vector<OptionDesc>& options);
	int computePermutationIndex(const OptionChoice* const optionChoices, const int numOptions) const;
	const std::vector<std::vector<int>>& getAllPermunations() const {
		return allPermutations;
	}
	const std::vector<OptionDesc>& getAllOptions() const {
		return allOptions;
	}

  private:
	int numAllPermutations = 0;
	std::vector<OptionDesc> allOptions;
	std::vector<std::vector<int>> allPermutations;
};

} // namespace sge
