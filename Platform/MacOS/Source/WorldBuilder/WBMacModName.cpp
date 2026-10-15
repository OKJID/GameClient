#include "StdAfx.h"
#include "WBMacModName.h"

#include "Common/GlobalData.h"

std::string WBMacActiveModName()
{
#if defined(WORLDBUILDER_MODS)
	if (TheGlobalData == nullptr || TheGlobalData->m_modDir.isEmpty())
	{
		return std::string();
	}
	if (TheGlobalData->m_modDisplayName.isNotEmpty())
	{
		return TheGlobalData->m_modDisplayName.str();
	}
	return TheGlobalData->m_modId.str();
#else
	return std::string();
#endif
}
