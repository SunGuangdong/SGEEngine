#pragma once

namespace sge {

/// The following function will reqest a Visual Studio debugger to get attached
/// to this process and start debugging it. It will bring a dialogue from vsjitdebugger
/// asking you to create new Visual Studio instance or use and existing one.
///
/// The function will return immendatley when finished, the debugger minght not be attached yet.
/// if you want to do so use IsDebuggerPresent.
bool startVisualStudioDebugging();

bool isVSDebuggerPresent();

} // namespace sge
