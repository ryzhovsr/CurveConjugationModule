#pragma once

#include "Common/RPLM.Base.Framework.String.h"
#include "Common/RPLM.Base.Framework.ResourceDefs.h"

namespace RPLM::CAD::DimensionChain::UI
{
	const RPLM::Base::Framework::String& GetModuleName();
}

#define  _STRING(key) _RESOURCE(_STR(key))
#define	 _RESOURCE(key) Base::Framework::GetModuleResource(key, CAD::DimensionChain::UI::GetModuleName())
#define  _WSTRING(key) QString::fromStdWString(_RESOURCE(_STR(key)))