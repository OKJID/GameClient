#pragma once

class WBMacIterationPool
{
public:
	WBMacIterationPool() = default;
	WBMacIterationPool(const WBMacIterationPool&) = delete;
	WBMacIterationPool& operator=(const WBMacIterationPool&) = delete;
	~WBMacIterationPool();

	void renew();

private:
	void drain();

	void* m_pool = nullptr;
};
