#pragma once

#include "sge_core/typelib/typeLib.h"

#define ReflAddActor(T) ReflAddType(T) ReflInherits(T, sge::Actor)
#define ReflAddObject(T) ReflAddType(T) ReflInherits(T, sge::GameObject)
#define ReflAddScript(T) ReflAddType(T) ReflInherits(T, sge::Script)
