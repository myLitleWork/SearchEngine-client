# Промпт: авторизация по USB-токену (WMI + hardware serial + JSON)

**Для агента:** выполни задачу в репозитории SearchClient. Не коммить без явной просьбы пользователя.  
**Тестовая флешка уже на `E:\`.** Ожидаемый токен: `E:\searchclient-auth-token.json`.

Связанный промпт по классу serial (уже сделан): `docs/PROMPT_USB_HARDWARE_SERIAL.md`  
API уже в проекте: `UsbHardwareSerial.h` / `UsbHardwareSerial.cpp`.

---

## Цель

Заменить polling signed-token (`#if 0` / `token-HS.bin` / volume serial) на:

1. **WMI-события** подключения/извлечения USB-дисков (как FileWatcherService).
2. **Hardware/PNP serial** через уже существующий `UsbHardwareSerial`.
3. JSON-токен **`searchclient-auth-token.json`** в корне тома.
4. Локальная авторизация → только обновление UI (`lblUser`, `auth_`, `user_`).
5. **Серверную** регистрацию — только по кнопке **Подключение** (существующие команды).
6. При извлечении авторизующей флешки — **сразу** logout/блокировка UI, как сейчас в `tmLoginOutTimer`.

`admin984.bin` **оставить**. Приоритет описан ниже.

---

## Как сейчас устроено в клиенте (обязательно прочитать)

| Что | Где |
|-----|-----|
| Успешный локальный вход | `formMain.cpp` → `onSuccessfulAuthorization` (~3758): `lblUser`, `user_`, `auth_=true`, `componentsEnable(true)`, `token_dir_`, включает `tmLoginOut` |
| Таймер входа | `tmLoginInTimer` (~3832): сейчас `admin984.bin` + auto `tryConnect`/`registryOnServer` для admin |
| Старый flash-flow (выкл.) | тот же файл `#if 0` (~3870–3928): polling `enumerateRemovableTokenPaths`, `gTokens`/`gTick`, volume serial |
| Скан букв | `enumerateRemovableTokenPaths` (~3805) |
| Выход при исчезновении токена | `tmLoginOutTimer` (~4548): если нет файла `token_dir_` и не admin → `clear()`, «Нет авторизации!», `disconnectUiFull()`, `componentsEnable(false)` |
| Кнопка Подключение | `btnConnectClick` (~1577): `tryConnect()` + при `auth_` → `registryOnServer(user_)` |
| Регистрация имени | `registryOnServer` (~1199) → `USER_REGISTRY` |
| Новый wire-путь | `Net/MultiServerManager.h` → `authenticateV1(clientName, clientId, flashSerial, …)` → `COMMAND::AUTHENTICATE_V1` |
| Лейбл | `lblUser` в `formMain` |
| Состояние | `formMain.h`: `auth_`, `user_`, `token_dir_`, `gTokens`, `gTick`, таймеры `tmLoginIn` / `tmLoginOut` |

Документация текущего UX: `tutorials/Краткие_инструкции/01_Подключение_и_авторизация.html`, `docs/TECHNICAL_NOTES.md` (раздел временной авторизации).

Референс WMI watcher (события, не polling):

- `C:\Users\Sg\CLionProjects\FileWatcherService\src\UsbWatcher.h`
- `C:\Users\Sg\CLionProjects\FileWatcherService\src\UsbWatcher.cpp`  
  (подписка `__InstanceCreationEvent` / `__InstanceDeletionEvent` на `Win32_DiskDrive` + `InterfaceType='USB'`, serial из PNP, буква через ASSOCIATORS)

Hardware serial в этом проекте:

- `UsbHardwareSerial::GetSerialForDriveLetter`, `EnumerateConnectedUsbDisks`, `ExtractSerialFromPnp`

JSON в проекте уже через **nlohmann** (`my_json/ConverterJSON.h`).

---

## Формат токена

Файл на флешке: **`<буква>:\searchclient-auth-token.json`**

Пример:

```json
{
  "format": "searchclient-auth-token",
  "format_version": 1,
  "client_id": "C-001",
  "client_name": "Иванов И.И.",
  "flash_serial": "1234567890",
  "issued_at": "2026-08-16T12:00:00Z",
  "expires_at": null,
  "issuer": "auth-server",
  "signature": {
    "alg": "none",
    "encoding": "base64",
    "value": ""
  },
  "notes": ""
}
```

### Проверка v1 (обязательная сейчас)

1. Файл существует, парсится как JSON.
2. `format == "searchclient-auth-token"`, `format_version == 1`.
3. Непустые `client_name`, `flash_serial` (и желательно `client_id` — хранить в состоянии).
4. `flash_serial` (из токена, нормализовать UPPERCASE/trim) **==** фактический hardware serial тома (`UsbHardwareSerial::GetSerialForDriveLetter`).
5. `signature.alg == "none"` → подпись **не проверять**.
6. `expires_at: null` → срок не проверять (или явно игнор).

Невалидный токен → флешку не считать авторизующей.

---

## Приоритет источников авторизации

Стек / порядок (сверху — активный):

1. **Флешки с валидным токеном** — приоритетнее admin.  
   Несколько флешек: **последний insert выигрывает** (как старый `gTick`).  
   Если отключилась текущая (верхняя) — перейти на **предыдущую** ещё вставленную валидную.  
   Если отключилась неактивная — активную не трогать.
2. **`admin984.bin` рядом с EXE** — только если **нет ни одной** валидной авторизационной флешки.  
   Поведение admin как сейчас локально: имя `"admin"`, `onSuccessfulAuthorization`.  
   При появлении валидной флешки — **переключить** локальную авторизацию на `client_name` с флешки.  
   При извлечении последней валидной флешки — если admin-файл есть → снова admin; если нет → полный logout.
3. Иначе — нет авторизации (как сейчас «Нет авторизации!»).

---

## Поведение UI / сети (критично)

### При появлении валидной флешки (WMI insert)

- Обновить локальное состояние: `user_ = client_name`, сохранить `client_id`, `flash_serial`, путь токена / букву тома, `auth_ = true`.
- `lblUser->Caption = "Авторизация : " + client_name`, цвет зелёный.
- `componentsEnable(true)` — как в `onSuccessfulAuthorization` (чтобы кнопка Подключение и UI были доступны).
- **Не** вызывать `tryConnect` / `registryOnServer` / `authenticateV1` автоматически.

### При кнопке «Подключение» (`btnConnectClick`)

- Как сейчас: `tryConnect()`.
- Если активна **flash-авторизация** и есть `client_id` + `flash_serial` → предпочтительно **`authenticateV1(client_name, client_id, flash_serial, …)`** (уже в `MultiServerManager.h`).  
  Если по какой-то причине V1 нельзя — fallback `registryOnServer(user_)` / `USER_REGISTRY`, но в отчёте явно указать что выбрано.
- Если активен **admin** → оставить текущий путь `registryOnServer("admin")` / `USER_REGISTRY`.

### При извлечении флешки (WMI remove) — важнее insert

Если ушла флешка, которая была в стеке авторизации:

1. Пересчитать активный источник по правилам приоритета.
2. Если новая активная — **другая флешка**: переключить `user_`/`lblUser`/`token_dir_` на неё **без** авто-connect к серверу; если уже был сетевой сеанс — **сбросить сеть** (`disconnectUiFull` или эквивалент) и оставить только локальную авторизацию (пользователь жмёт Подключение снова). Либо мягче: только обновить имя и при уже подключённом транспорте перерегистрировать — **предпочтительнее безопасный вариант: disconnect + локальная смена имени**, без тихого reconnect.
3. Если новая активная — **admin**: локально `onSuccessfulAuthorization("admin", …)` **без** авто-`tryConnect` (отличие от текущего admin-таймера, который сам коннектится — здесь после снятия флешки не коннектить сами).
4. Если источников нет: **тот же cleanup, что `tmLoginOutTimer`**:
   - `clear()`
   - `lblUser = "Нет авторизации!"`, maroon
   - `auth_ = false`, `user_.clear()`
   - `disconnectUiFull()`
   - `componentsEnable(false)`
   - скрыть шаблоны и т.п. как сейчас

Срабатывание logout должно быть **по событию remove** (сразу), не ждать 0.5–1.5 с polling. Таймеры можно упростить/оставить как safety-net (проверка «токен ещё на месте»), но основной путь — WMI.

### Admin marker (`tmLoginIn`)

- Сохранить обнаружение `admin984.bin`.
- **Не** авторизовать admin, пока в стеке есть валидные флешки.
- Убрать или не делать авто-`tryConnect`/`registryOnServer` при появлении admin (единая политика: сервер только по кнопке). Если оставляешь авто-connect только для legacy admin — явно опиши в отчёте; **желательно единообразно: без авто-connect**.

---

## Архитектура реализации (рекомендация)

Не раздувать `formMain.cpp` ещё на сотни строк без нужды.

1. Небольшой модуль, например `UsbAuthWatcher` / `FlashAuthSession`:
   - WMI watch insert/remove (референс FileWatcherService; фон `TThread`, колбэки в UI через `TThread::Queue`).
   - На insert: буква → `GetSerialForDriveLetter` → поиск `X:\searchclient-auth-token.json` → parse/validate → push в стек.
   - На remove: найти по serial (или букве, если ещё известна) → pop из стека → `ApplyActiveAuth()` на UI-потоке.
2. Стек: список `{ serial, driveLetter, tokenPath, client_name, client_id, tick }`, активный = max tick / последний insert.
3. Переиспользовать `onSuccessfulAuthorization` / общий helper «применить локальную сессию» и общий «logout UI», чтобы не дублировать cleanup.
4. Старый `#if 0` binary-token код не удалять без просьбы; новый путь — основной.
5. Volume serial / `GetVolumeSerialNumber` / RSA `token-HS.bin` в новом пути **не использовать**.

---

## Критерии приёмки

- [ ] На `E:\` с валидным `searchclient-auth-token.json` и совпадающим `flash_serial` → `lblUser` показывает `client_name`, `auth_=true`, **без** автоподключения к серверу.
- [ ] Несовпадение serial / битый JSON → авторизации с этой флешки нет.
- [ ] Извлечение этой флешки → сразу logout UI + disconnect (если не осталось других источников).
- [ ] Если после извлечения остаётся другая валидная флешка → локально переключиться на неё (последний оставшийся по стеку).
- [ ] Если флешек нет, но есть `admin984.bin` → локальный `admin`.
- [ ] Пока есть валидная флешка, admin не перекрывает её.
- [ ] «Подключение» при flash-сессии шлёт имя (и ideally `authenticateV1` с id/serial); при admin — прежний registry.
- [ ] Debug|Win32 собирается; `AGENTS.md` соблюдён.

---

## Порядок работы агента

1. Прочитать участки `formMain` / `MultiServerManager` / `UsbHardwareSerial` / референс `UsbWatcher.cpp` выше.
2. Реализовать watcher + стек + интеграцию в login/logout/connect.
3. Собрать Debug|Win32.
4. Проверить на `E:\` (insert → лейбл; желательно remove → logout).
5. Краткий отчёт: файлы, приоритеты, какая команда на «Подключение», поведение admin.
