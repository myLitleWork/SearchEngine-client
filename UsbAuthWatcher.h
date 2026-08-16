//---------------------------------------------------------------------------

#ifndef UsbAuthWatcherH
#define UsbAuthWatcherH

#include <functional>
#include <string>

// Фоновый WMI-watcher USB DiskDrive insert/remove.
// Колбэки вызываются с рабочего потока (не UI).
class UsbAuthWatcher
{
public:
	using InsertFn = std::function<void(const std::string& serial, const std::string& driveLetter)>;
	using RemoveFn = std::function<void(const std::string& serial)>;

	UsbAuthWatcher();
	~UsbAuthWatcher();

	UsbAuthWatcher(const UsbAuthWatcher&) = delete;
	UsbAuthWatcher& operator=(const UsbAuthWatcher&) = delete;

	bool start(InsertFn onInsert, RemoveFn onRemove);
	void stop();
	bool running() const;

private:
	class Impl;
	Impl* pImpl_;
};

#endif
