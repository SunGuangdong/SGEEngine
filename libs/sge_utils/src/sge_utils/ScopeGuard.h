#pragma once

#include <functional>

#include "sge_utils/types.h"

namespace sge {

struct ScopeGuard : public NoCopyNoMove {
	ScopeGuard() = default;
	ScopeGuard(std::function<void()> scopeExitProc)
	    : scopeExitProc(std::move(scopeExitProc))
	{
	}

	~ScopeGuard()
	{
		if (scopeExitProc) {
			scopeExitProc();
		}
	}

	void dismiss() { scopeExitProc = nullptr; }

  private:
	std::function<void()> scopeExitProc;
};
} // namespace sge
