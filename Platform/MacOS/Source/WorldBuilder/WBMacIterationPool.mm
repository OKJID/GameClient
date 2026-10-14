#import <Foundation/Foundation.h>

#include "WBMacIterationPool.h"

#if __has_feature(objc_arc)
#error WBMacIterationPool.mm manages NSAutoreleasePool by hand and must be compiled without ARC
#endif

WBMacIterationPool::~WBMacIterationPool()
{
	drain();
}

void WBMacIterationPool::renew()
{
	drain();
	m_pool = [[NSAutoreleasePool alloc] init];
}

void WBMacIterationPool::drain()
{
	if (m_pool == nullptr)
	{
		return;
	}

	[static_cast<NSAutoreleasePool*>(m_pool) drain];
	m_pool = nullptr;
}
