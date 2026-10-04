#pragma once

#ifdef _DEBUG
#include "UsbTokenRetryWorker.h"
#include <atomic>
#include <stdexcept>

inline int RunUsbTokenRetrySelfTest()
{
	using Ms = std::chrono::milliseconds;
	auto waitFor = [](const std::atomic<int>& value, int target) {
		const auto deadline = UsbTokenRetryWorker::Clock::now() + Ms(2000);
		while (value.load() < target && UsbTokenRetryWorker::Clock::now() < deadline)
			std::this_thread::sleep_for(Ms(5));
		return value.load() >= target;
	};

	// The volume and then its file become readable after several failed probes.
	{
		std::atomic<int> probes{0}, notifications{0};
		UsbTokenRetryWorker worker(
			[&](const std::string& serial) { return serial == "USB-A" && ++probes >= 3; },
			[&]() { ++notifications; }, Ms(10), Ms(2000));
		worker.start();
		worker.enqueue("USB-A");
		worker.enqueue("USB-A");
		if (!waitFor(notifications, 1)) return 101;
		std::this_thread::sleep_for(Ms(60));
		worker.stop();
		if (probes != 3 || notifications != 1) return 102;
	}

	// Removal cancels an unreadable disk; becoming readable later does nothing.
	{
		std::atomic<int> probes{0}, notifications{0};
		std::atomic<bool> readable{false};
		UsbTokenRetryWorker worker(
			[&](const std::string&) { ++probes; return readable.load(); },
			[&]() { ++notifications; }, Ms(10), Ms(2000));
		worker.start();
		worker.enqueue("USB-A");
		if (!waitFor(probes, 1)) return 103;
		worker.cancel("USB-A");
		const int before = probes.load();
		readable = true;
		std::this_thread::sleep_for(Ms(60));
		if (probes != before || notifications != 0) return 104;
		// A genuinely new insertion can be discovered after the cancellation.
		worker.enqueue("USB-A");
		if (!waitFor(notifications, 1)) return 105;
	}

	// Closing the client cancels the window immediately and rejects late events.
	{
		std::atomic<int> probes{0};
		UsbTokenRetryWorker worker(
			[&](const std::string&) { ++probes; return false; }, []() {}, Ms(10), Ms(15000));
		worker.start();
		worker.enqueue("USB-A");
		if (!waitFor(probes, 1)) return 106;
		const auto before = UsbTokenRetryWorker::Clock::now();
		worker.stop();
		if (UsbTokenRetryWorker::Clock::now() - before > Ms(1000)) return 107;
		const int count = probes.load();
		worker.enqueue("USB-B");
		std::this_thread::sleep_for(Ms(40));
		if (probes != count) return 108;
		worker.start();
		std::this_thread::sleep_for(Ms(40));
		if (probes != count) return 109;
	}

	// Duplicate WMI events cannot keep an invalid token polling indefinitely.
	{
		std::atomic<int> probes{0}, notifications{0};
		UsbTokenRetryWorker worker(
			[&](const std::string&) { ++probes; return false; },
			[&]() { ++notifications; }, Ms(10), Ms(100));
		worker.start();
		worker.enqueue("USB-A");
		if (!waitFor(probes, 1)) return 110;
		std::this_thread::sleep_for(Ms(30));
		worker.enqueue("USB-A");
		std::this_thread::sleep_for(Ms(180));
		const int before = probes.load();
		std::this_thread::sleep_for(Ms(50));
		worker.stop();
		if (before == 0 || probes != before || notifications != 0) return 111;
	}

	// One unreadable disk must not prevent a different disk from authorizing.
	{
		std::atomic<int> notifications{0}, attemptsA{0}, attemptsB{0};
		UsbTokenRetryWorker worker([&](const std::string& serial) {
			if (serial == "USB-A") { ++attemptsA; return false; }
			++attemptsB;
			return true;
		}, [&]() { ++notifications; }, Ms(10), Ms(2000));
		worker.start();
		worker.enqueue("USB-A");
		worker.enqueue("USB-B");
		if (!waitFor(notifications, 1)) return 112;
		worker.stop();
		if (attemptsA == 0 || attemptsB != 1 || notifications != 1) return 113;
	}

	// Parser exceptions are isolated to the current probe, not the worker thread.
	{
		std::atomic<int> probes{0}, notifications{0};
		UsbTokenRetryWorker worker([&](const std::string&) {
			if (++probes == 1) throw std::runtime_error("invalid token field");
			return true;
		}, [&]() { ++notifications; }, Ms(10), Ms(2000));
		worker.start();
		worker.enqueue("USB-A");
		if (!waitFor(notifications, 1)) return 114;
		worker.stop();
		if (probes != 2 || notifications != 1) return 115;
	}
	return 0;
}
#endif
