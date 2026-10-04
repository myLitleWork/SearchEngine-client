#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// Retry only discovery of a newly inserted disk, never network authentication.
// The probe and ready callback run on this worker, not on the VCL thread.
class UsbTokenRetryWorker
{
public:
	using ProbeFn = std::function<bool(const std::string&)>;
	using ReadyFn = std::function<void()>;
	using Clock = std::chrono::steady_clock;
	using Milliseconds = std::chrono::milliseconds;

	UsbTokenRetryWorker(ProbeFn probe, ReadyFn ready,
		Milliseconds interval = Milliseconds(500),
		Milliseconds window = Milliseconds(15000))
		: probe_(std::move(probe)), ready_(std::move(ready)),
		  interval_(interval), window_(window)
	{
	}

	~UsbTokenRetryWorker() { stop(); }
	UsbTokenRetryWorker(const UsbTokenRetryWorker&) = delete;
	UsbTokenRetryWorker& operator=(const UsbTokenRetryWorker&) = delete;

	void start()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!stopped_)
			return;
		stopped_ = false;
		worker_ = std::thread([this]() { run(); });
	}

	void stop()
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			stopped_ = true;
			pending_.clear();
		}
		wake_.notify_all();
		if (worker_.joinable())
			worker_.join();
	}

	void enqueue(const std::string& serial)
	{
		if (serial.empty())
			return;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			if (stopped_)
				return;
			// Duplicate notifications do not extend the discovery deadline.
			for (const auto& disk : pending_)
				if (disk.serial == serial)
					return;
			const auto now = Clock::now();
			pending_.push_back(Pending{serial, now + interval_, now + window_});
		}
		wake_.notify_all();
	}

	void cancel(const std::string& serial)
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			pending_.erase(std::remove_if(pending_.begin(), pending_.end(),
				[&](const Pending& disk) { return disk.serial == serial; }), pending_.end());
		}
		wake_.notify_all();
	}

private:
	struct Pending
	{
		std::string serial;
		Clock::time_point next;
		Clock::time_point deadline;
	};

	void run()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		while (!stopped_)
		{
			if (pending_.empty()) {
				wake_.wait(lock, [this]() { return stopped_ || !pending_.empty(); });
				continue;
			}
			auto disk = std::min_element(pending_.begin(), pending_.end(),
				[](const Pending& a, const Pending& b) { return a.next < b.next; });
			const auto now = Clock::now();
			if (now >= disk->deadline) {
				pending_.erase(disk);
				continue;
			}
			if (now < disk->next) {
				wake_.wait_until(lock, std::min(disk->next, disk->deadline));
				continue;
			}

			// Serialize the probe with cancel/stop so an old insertion cannot
			// publish a token after its removal has completed.
			bool accepted = false;
			try { accepted = probe_(disk->serial); }
			catch (...) { /* A malformed/unreadable token must not kill the worker. */ }
			if (accepted) {
				pending_.erase(disk);
				if (ready_)
					ready_();
			}
			else {
				disk->next = Clock::now() + interval_;
			}
		}
	}

	ProbeFn probe_;
	ReadyFn ready_;
	Milliseconds interval_;
	Milliseconds window_;
	std::mutex mutex_;
	std::condition_variable wake_;
	std::vector<Pending> pending_;
	bool stopped_ = true;
	std::thread worker_;
};
