//---------------------------------------------------------------------------

#ifndef FlashAuthSessionH
#define FlashAuthSessionH

#include "UsbAuthToken.h"

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

struct FlashAuthEntry
{
	std::string serial;
	std::string driveLetter; // "E:"
	std::string tokenPath;
	std::string clientName;
	std::string clientId;
	std::uint64_t tick = 0;
};

// Стек валидных USB JSON-токенов + WMI insert/remove.
// Колбэк changed вызывается с рабочего потока watcher / вызывающего потока scan.
class FlashAuthSession
{
public:
	using ChangedFn = std::function<void()>;

	FlashAuthSession();
	~FlashAuthSession();

	FlashAuthSession(const FlashAuthSession&) = delete;
	FlashAuthSession& operator=(const FlashAuthSession&) = delete;

	void setChangedCallback(ChangedFn cb);
	bool start();
	void stop();

	// Скан уже подключённых USB (стартовый / safety-net).
	void scanConnectedDisks();

	// Убрать запись, если токен пропал с диска.
	void dropInvalidActive(const std::string& serial, const std::string& tokenPath);

	bool hasValidFlash() const;
	std::optional<FlashAuthEntry> activeEntry() const;
	std::vector<FlashAuthEntry> entries() const;

private:
	void handleInsert(const std::string& serial, const std::string& driveLetter);
	void handleRemove(const std::string& serial);
	bool tryPushFromDrive(const std::string& serial, std::string driveLetter);
	void notifyChanged();

	mutable std::mutex mutex_;
	std::vector<FlashAuthEntry> stack_;
	std::uint64_t tick_ = 0;
	ChangedFn onChanged_;

	class WatcherHolder;
	WatcherHolder* watcher_ = nullptr;
};

#endif
