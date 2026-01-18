#include "counter.h"

#include <climits>
#include <cstdint>
#include <time.h>

#define LOCK_AT_MICROSECONDS

static uint64_t g_CountDiv = 1;
static constexpr uint64_t kCounterTicksPerSecond = 1000000000ULL;

static uint64_t counter_get_time_ns()
{
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return static_cast<uint64_t>(ts.tv_sec) * kCounterTicksPerSecond + static_cast<uint64_t>(ts.tv_nsec);
}

static void counter_store_time(unsigned long data[2], uint64_t value)
{
#if ULONG_MAX >= 0xFFFFFFFFFFFFFFFFULL
	data[0] = static_cast<unsigned long>(value);
	data[1] = 0;
#else
	data[0] = static_cast<unsigned long>(value & 0xFFFFFFFFULL);
	data[1] = static_cast<unsigned long>(value >> 32);
#endif
}

static uint64_t counter_load_time(const unsigned long data[2])
{
#if ULONG_MAX >= 0xFFFFFFFFFFFFFFFFULL
	return static_cast<uint64_t>(data[0]);
#else
	return static_cast<uint64_t>(data[0]) | (static_cast<uint64_t>(data[1]) << 32);
#endif
}

class CountDivSetter
{
public:
	CountDivSetter()
	{
#ifdef LOCK_AT_MICROSECONDS
		g_CountDiv = kCounterTicksPerSecond / 1000000ULL;
		if (g_CountDiv == 0)
			g_CountDiv = 1;
#else
		g_CountDiv = 1;
#endif
	}
};
static CountDivSetter __g_CountDivSetter;


unsigned long cnt_NumTicksPerSecond()
{
	return static_cast<unsigned long>(kCounterTicksPerSecond / g_CountDiv);
}


CounterFinal::CounterFinal(unsigned long startMode)
{
	if(startMode == CSTART_MICRO)
		StartMicro();
	else if(startMode == CSTART_MILLI)
		StartMS();
}


void CounterFinal::StartMS()
{
	counter_store_time(m_Data, counter_get_time_ns());
}

unsigned long CounterFinal::EndMS()
{
	const uint64_t start = counter_load_time(m_Data);
	const uint64_t now = counter_get_time_ns();
	const uint64_t elapsed = now - start;
	return static_cast<unsigned long>((elapsed * 1000ULL) / kCounterTicksPerSecond);
}

unsigned long CounterFinal::CountMS()
{
	return EndMS();
}


void CounterFinal::StartMicro()
{
	counter_store_time(m_Data, counter_get_time_ns());
}

unsigned long CounterFinal::EndMicro()
{
	const uint64_t start = counter_load_time(m_Data);
	const uint64_t now = counter_get_time_ns();
	const uint64_t elapsed = now - start;
	return static_cast<unsigned long>(elapsed / g_CountDiv);
}

unsigned long CounterFinal::CountMicro()
{
	return EndMicro();
}


void cnt_StartCounterFinal(CounterFinal &pCounter)
{
	pCounter.StartMicro();
}


unsigned long cnt_EndCounterFinal(CounterFinal &pCounter)
{
	return pCounter.EndMicro();
}

#ifndef _FINAL

float CountPercent::CalcPercent()
{
	const uint64_t total_out = counter_load_time(m_TotalOut);
	if ((total_out | counter_load_time(m_TotalIn)) == 0)
		return 0.0f;

	const uint64_t total_in = counter_load_time(m_TotalIn);
	const uint64_t denom = total_in + total_out;
	if (denom == 0)
		return 0.0f;

	const uint64_t result = (total_in * 10000ULL) / denom;
	return static_cast<float>(result) / 10000.0f;
}

void CountPercent::Clear()
{
	counter_store_time(m_Finger, 0);
	counter_store_time(m_TotalIn, 0);
	counter_store_time(m_TotalOut, 0);
	m_iIn = 0;
}

uint CountPercent::In()
{
	if (m_iIn++)
		return 0;

	const uint64_t now = counter_get_time_ns();
	const uint64_t then = counter_load_time(m_Finger);
	uint64_t result = 0;

	if (then != 0)
	{
		const uint64_t total_out = counter_load_time(m_TotalOut) + (now - then);
		counter_store_time(m_TotalOut, total_out);
		result = now - then;
	}

	counter_store_time(m_Finger, now);
	return static_cast<uint>(result);
}

uint CountPercent::Out()
{
	if (--m_iIn)
		return 0;

	const uint64_t now = counter_get_time_ns();
	const uint64_t then = counter_load_time(m_Finger);
	const uint64_t result = now - then;
	const uint64_t total_in = counter_load_time(m_TotalIn) + result;

	counter_store_time(m_TotalIn, total_in);
	counter_store_time(m_Finger, now);
	return static_cast<uint>(result);
}

#endif // !_FINAL
