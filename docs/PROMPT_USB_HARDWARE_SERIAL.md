# Промпт: класс аппаратного серийника USB (C++Builder)

**Для агента:** выполни задачу ниже. Не коммить без явной просьбы пользователя.

## Цель (узкий scope)

**Сначала только класс** с нужными функциями (methods).  
Не интегрировать в `formMain`, авторизацию, Net, токены.  
Не делать watcher/службу/подписку на insert-remove — это потом.  
Не менять `flashNum` / `FlashPodpis` / `GetVolumeSerialNumber`.

**Тестовая флешка уже на `E:\`.** После реализации — one-shot вызов методов класса должен найти этот диск и вернуть hardware serial + букву.

---

## Контекст

- Репо: `D:\MyWorkSpaceMain\PROJECTS\SS_ClientServerBackup\SearchEngine_Cient_asio`
- C++Builder VCL, `SearchClient_asio.cbproj`, Win32
- Правила: `AGENTS.md`
- Сейчас в `flashNum.h` есть только **volume serial** (`GetVolumeInformationA`) — это другой идентификатор. Нужен **hardware/PNP serial**.

Сборка после подключения файлов к `.cbproj`:

```bat
call "C:\Program Files (x86)\Embarcadero\Studio\22.0\bin\rsvars.bat"
msbuild SearchClient_asio.cbproj /t:Build /p:Config=Debug /p:Platform=Win32 /v:n /nologo
```

---

## Референс (логика парсинга и WMI-запросов)

`C:\Users\Sg\CLionProjects\FileWatcherService\src\UsbWatcher.cpp`

Взять **алгоритм**, не архитектуру службы:

| Фрагмент | Зачем |
|----------|--------|
| `extractSerialFromPnp` (~20–39) | парсинг serial из `PNPDeviceID` |
| чтение `PNPDeviceID` у `Win32_DiskDrive` | откуда брать строку |
| `findDriveLetter` (~191–250) | DiskDrive → Partition → LogicalDisk (`E:`) |

Подписку на `__InstanceCreationEvent` / `UsbWatcher::start` **не переносить** в эту задачу.

---

## Алгоритм методов

1. WMI `ROOT\CIMV2`, запрос уже подключённых `Win32_DiskDrive` с `InterfaceType = 'USB'`.
2. Serial из **`PNPDeviceID`**:
   - хвост после последнего `\`
   - обрезать по первому `&`
   - убрать пробелы/CR/LF/NUL, UPPERCASE
   - пусто → `"(UNKNOWN)"`
3. Буква: ASSOCIATORS как в `findDriveLetter` → `"E:"`.
4. **Не** использовать `GetVolumeInformation` / `GetVolumeSerialNumber` как serial.

---

## Deliverable

Файлы: `UsbHardwareSerial.h` + `UsbHardwareSerial.cpp` (имена можно слегка скорректировать под стиль проекта).

Класс, например `UsbHardwareSerial`, со статическими или instance-методами:

```text
ExtractSerialFromPnp(pnpDeviceId) -> string
ResolveDriveLetterFromDiskDeviceId(deviceId) -> string   // "E:"
GetSerialForDriveLetter(driveLetter) -> string           // вход "E:" / "E:\\" → hardware serial
EnumerateConnectedUsbDisks() -> список { Serial, DriveLetter, DeviceID, PnpDeviceId }
```

Опционально удобный хелпер: `GetSerialForE()` не нужен — достаточно `GetSerialForDriveLetter("E:")`.

Стиль unit: как `flashNum.h` / соседние `.cpp` (`#pragma hdrstop`, `#pragma package(smart_init)`, include guard).  
WMI/COM — что стабильно собирается в C++Builder Studio 22.  
Подключить оба файла к `SearchClient_asio.cbproj`.

**Вне scope:** StartWatch/StopWatch, колбэки, UI, правки `formMain.cpp`, смена volume→hardware в токенах.

---

## Проверка

1. Debug|Win32 собирается.
2. Вызов `EnumerateConnectedUsbDisks()` или `GetSerialForDriveLetter("E:")` на уже вставленной флешке `E:\` возвращает непустой hardware serial (не равный `GetVolumeSerialNumber("E:\\")`, кроме случайного совпадения).
3. В отчёте: сигнатуры API + пример вызова + полученный serial для `E:`.

---

## Порядок

1. Прочитать `UsbWatcher.cpp` (extractSerialFromPnp + findDriveLetter + как читается PNPDeviceID).
2. Реализовать только класс + подключить к проекту.
3. Собрать Debug|Win32.
4. Проверить на `E:\`, кратко отчитаться.
