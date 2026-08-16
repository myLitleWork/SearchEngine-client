//---------------------------------------------------------------------------

#pragma hdrstop

#include "FlashAuthSession.h"
#include "UsbAuthWatcher.h"
#include "UsbHardwareSerial.h"

#include <Windows.h>

#include <algorithm>

//---------------------------------------------------------------------------
#pragma package(smart_init)

namespace {

std::string NormalizeLetter(std::string drive)
{
	if (drive.empty())
		return std::string();
	while (!drive.empty() && (drive.back() == '\\' || drive.back() == '/'))
		drive.pop_back();
	if (drive.size() == 1)
		drive += ':';
	if (drive.size() >= 2 && drive[1] == ':')
	{
		drive[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(drive[0])));
		return drive.substr(0, 2);
	}
	return std::string();
}

std::string TokenPathForLetter(const std::string& letter)
{
	const std::string root = NormalizeLetter(letter);
	if (root.empty())
		return std::string();
	return root + "\\" + UsbAuthTokenFileName();
}

bool FileExistsUtf8(const std::string& pathUtf8)
{
	if (pathUtf8.empty())
		return false;
	const int n = MultiByteToWideChar(CP_UTF8, 0, pathUtf8.c_str(), -1, nullptr, 0);
	if (n <= 1)
		return false;
	std::wstring w(static_cast<size_t>(n - 1), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, pathUtf8.c_str(), -1, &w[0], n);
	const DWORD attr = GetFileAttributesW(w.c_str());
	return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

} // namespace

class FlashAuthSession::WatcherHolder
{
public:
	UsbAuthWatcher watcher;
};

FlashAuthSession::FlashAuthSession()
	: watcher_(new WatcherHolder())
{
}

FlashAuthSession::~FlashAuthSession()
{
	stop();
	delete watcher_;
	watcher_ = nullptr;
}

void FlashAuthSession::setChangedCallback(ChangedFn cb)
{
	std::lock_guard<std::mutex> lock(mutex_);
	onChanged_ = std::move(cb);
}

bool FlashAuthSession::start()
{
	if (!watcher_)
		return false;
	if (watcher_->watcher.running())
		return true;

	const bool ok = watcher_->watcher.start(
		[this](const std::string& serial, const std::string& letter) {
			handleInsert(serial, letter);
		},
		[this](const std::string& serial) {
			handleRemove(serial);
		});
	if (ok)
		scanConnectedDisks();
	return ok;
}

void FlashAuthSession::stop()
{
	if (watcher_)
		watcher_->watcher.stop();
}

void FlashAuthSession::notifyChanged()
{
	ChangedFn cb;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		cb = onChanged_;
	}
	if (cb)
		cb();
}

bool FlashAuthSession::tryPushFromDrive(const std::string& serialIn, std::string driveLetter)
{
	std::string serial = NormalizeFlashSerial(serialIn);
	if (serial.empty() || serial == "(UNKNOWN)")
		return false;

	driveLetter = NormalizeLetter(driveLetter);
	if (driveLetter.empty())
	{
		const auto disks = UsbHardwareSerial::EnumerateConnectedUsbDisks();
		for (size_t i = 0; i < disks.size(); ++i)
		{
			if (NormalizeFlashSerial(disks[i].Serial) == serial)
			{
				driveLetter = NormalizeLetter(disks[i].DriveLetter);
				break;
			}
		}
	}
	if (driveLetter.empty())
		return false;

	std::string hw = NormalizeFlashSerial(
		UsbHardwareSerial::GetSerialForDriveLetter(driveLetter));
	if (hw.empty())
		hw = serial;
	if (hw != serial)
		return false;

	const std::string path = TokenPathForLetter(driveLetter);
	if (!FileExistsUtf8(path))
		return false;

	UsbAuthToken token;
	if (!TryLoadUsbAuthToken(path, hw, token))
		return false;

	{
		std::lock_guard<std::mutex> lock(mutex_);
		for (size_t i = 0; i < stack_.size(); ++i)
		{
			if (stack_[i].serial == token.flashSerial)
			{
				stack_[i].driveLetter = driveLetter;
				stack_[i].tokenPath = token.tokenPath;
				stack_[i].clientName = token.clientName;
				stack_[i].clientId = token.clientId;
				stack_[i].signatureValue = token.signatureValue;
				stack_[i].tick = ++tick_;
				return true;
			}
		}

		FlashAuthEntry e;
		e.serial = token.flashSerial;
		e.driveLetter = driveLetter;
		e.tokenPath = token.tokenPath;
		e.clientName = token.clientName;
		e.clientId = token.clientId;
		e.signatureValue = token.signatureValue;
		e.tick = ++tick_;
		stack_.push_back(e);
	}
	return true;
}

void FlashAuthSession::handleInsert(const std::string& serial, const std::string& driveLetter)
{
	if (tryPushFromDrive(serial, driveLetter))
		notifyChanged();
}

void FlashAuthSession::handleRemove(const std::string& serial)
{
	const std::string key = NormalizeFlashSerial(serial);
	bool removed = false;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		const auto it = std::remove_if(stack_.begin(), stack_.end(),
			[&](const FlashAuthEntry& e) { return e.serial == key; });
		if (it != stack_.end())
		{
			stack_.erase(it, stack_.end());
			removed = true;
		}
	}
	if (removed)
		notifyChanged();
}

void FlashAuthSession::scanConnectedDisks()
{
	bool any = false;
	const auto disks = UsbHardwareSerial::EnumerateConnectedUsbDisks();
	for (size_t i = 0; i < disks.size(); ++i)
	{
		if (tryPushFromDrive(disks[i].Serial, disks[i].DriveLetter))
			any = true;
	}

	// Убрать записи, чей токен уже невалиден / диск пропал.
	{
		std::lock_guard<std::mutex> lock(mutex_);
		std::vector<FlashAuthEntry> kept;
		kept.reserve(stack_.size());
		for (size_t i = 0; i < stack_.size(); ++i)
		{
			UsbAuthToken token;
			const std::string hw = NormalizeFlashSerial(
				UsbHardwareSerial::GetSerialForDriveLetter(stack_[i].driveLetter));
			if (hw.empty() || hw != stack_[i].serial)
				continue;
			if (!TryLoadUsbAuthToken(stack_[i].tokenPath, hw, token))
				continue;
			FlashAuthEntry e = stack_[i];
			e.clientName = token.clientName;
			e.clientId = token.clientId;
			e.tokenPath = token.tokenPath;
			e.signatureValue = token.signatureValue;
			kept.push_back(e);
		}
		if (kept.size() != stack_.size())
			any = true;
		stack_.swap(kept);
	}

	if (any)
		notifyChanged();
}

void FlashAuthSession::dropInvalidActive(const std::string& serial, const std::string& tokenPath)
{
	const std::string key = NormalizeFlashSerial(serial);
	bool changed = false;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		const auto it = std::remove_if(stack_.begin(), stack_.end(),
			[&](const FlashAuthEntry& e) {
				return e.serial == key || (!tokenPath.empty() && e.tokenPath == tokenPath);
			});
		if (it != stack_.end())
		{
			stack_.erase(it, stack_.end());
			changed = true;
		}
	}
	if (changed)
		notifyChanged();
}

bool FlashAuthSession::hasValidFlash() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return !stack_.empty();
}

std::optional<FlashAuthEntry> FlashAuthSession::activeEntry() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (stack_.empty())
		return std::nullopt;
	auto best = stack_.begin();
	for (auto it = stack_.begin(); it != stack_.end(); ++it)
	{
		if (it->tick > best->tick)
			best = it;
	}
	return *best;
}

std::vector<FlashAuthEntry> FlashAuthSession::entries() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return stack_;
}
