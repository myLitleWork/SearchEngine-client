# Технические заметки (приложение)

Краткая карта для проверки достоверности пользовательской документации. Не является руководством для конечного пользователя.

## Точка входа

- `SearchClient_asio.cpp` → `_tWinMain`
- Создаётся главная форма `TfMain` (`fMain`)
- Специальные режимы командной строки:
  - `--docx-worker` (всегда, для конвертации)
  - в Debug: `--multi-server-self-test`, `--auth-path-self-test`, docx-timeout self-test

## Версия и имя продукта

- `Version.h`, `app-version.json`
- Заголовок окна: `SearchClient_asio v4.3.0`
- Проект: `SearchClient_asio.cbproj` (Win32)

## Основные формы и фреймы

| Элемент | Файл | Роль |
|---|---|---|
| `TfMain` | `formMain.*` | Главное окно |
| `TfrWay` | `Frames/FrameWay.*` | Путь документа |
| `TfrAttachments` | `Frames/FrameAttachments.*` | Приложения |
| `TfrPdtv` | `Frames/FramePdtv.*` | Подтверждения |
| `TFrameDbg` | `Frames/MyFrameDbg.*` | Базовый каркас деталей |

Отдельного DataModule в проекте нет: подключения FireDAC размещены на главной форме (`ConnectionSettings`, `ConnectionRes`).

## Ключевые подсистемы

- **UI / сценарии:** `formMain.cpp`
- **Сеть:** `Net/myAsioClient.*`, `Net/MultiServerManager.*`, `Net/SocketFileSaver.*`
- **Локальные БД:** `Database/SqliteDb.*`, `Database/ResDbMigration.h`
- **JSON/модели:** `my_json/*`, `myRequest.*`, `SqlCondition.*`
- **DOCX:** `DocxConversionWorker.cpp`, `WordBatchProcessor.*`
- **История combo:** `RecentComboHistory.*`
- **Отчёты:** FastReport (`frxReport`, `report_client_asio.fr3`)
- **Сетки:** EhLib (`TDBGridEh`, `TMemTableEh`)

## Базы данных

| Файл | Назначение |
|---|---|
| `Settings.db3` | `servers`, `server_ports`, `ports`, `settings`, `map*` |
| `SearchRes.db3` | `res` — текущие результаты |
| Legacy `Res.db3` | миграция в пару Settings/SearchRes |

Доступ: FireDAC + SQLite.

## Сеть

- Boost.Asio TCP
- Кадр: заголовок с размером и командой + тело
- Многосерверный fan-out поиска, merge без dedup
- Админ-операции — только primary server

## Внешние зависимости (пользовательская среда)

- Windows
- Microsoft Word (только для DOCX)
- Доступ к поисковому серверу(ам)
- Релизные runtime-файлы по allowlist; `sht.docm` обязателен для DOCX
- Настройки организации задаются отдельно; файловая заглушка авторизации входит в релиз

## Основные локальные конфигурационные файлы

- `Settings.db3`
- `SearchRes.db3`
- `SERVER_IP.INI` — необязательный legacy fallback; без него используется `localhost`
- `Templates.json`
- `admin984.bin` — текущая временная заглушка рядом с EXE
- `sht.docm`
- `app-version.json` (версия продукта)

Реестр Windows и `TIniFile` для пользовательских настроек не используются.

Общий релиз содержит канонический `admin984.bin`, но не содержит рабочие
локальные БД, адреса серверов или RSA-ключи. Точный состав и основания описаны
в `docs/RELEASE_PACKAGE.md`.

## Временный режим авторизации

- `tmLoginInTimer` проверяет обычный файл `admin984.bin` рядом с EXE и включает
  пользователя `admin`; содержимое файла сейчас не анализируется.
- Путь строится в `AuthPaths.h` через `GetModuleFileNameW` и проверяется через
  `GetFileAttributesW`, без промежуточного ANSI-преобразования.
- Signed-token код для `token-HS.bin` сохранён в `formMain.cpp` под `#if 0` с
  пометкой о возможном возврате, но в текущую сборку не входит.
- `RSA::init()` не генерирует ключи. Прежний вызов `GenerateRSAKey` оставлен
  только внутри комментария для истории реализации.
- Локальная заглушка не является контролем прав. Сервер обязан независимо
  авторизовывать каждую операцию.

08.08.2026 Debug self-test подтвердил корректное обнаружение заглушки в пути с
кириллицей длиной 144 символа и корректное отсутствие авторизации без файла.

## Известные технические особенности, влияющие на UX

1. Клиентская групповая ACL удалена. Клиент не является границей
   авторизации: права обязан проверять сервер.
2. Форма и модель резолюций удалены как неактуальные. Wire-значения старых
   команд 4–9 зарезервированы, чтобы не сдвигать номера активных команд.
3. Сообщения `FramePdtv.cpp` приведены к читаемым формулировкам по контексту;
   символов повреждённой кодировки в файле не осталось.
4. `frxXLSExport1` принадлежит форме и не удаляется обработчиком экспорта.
   Изменённые на время экспорта свойства после вызова восстанавливаются.
5. Таймауты сети/DOCX зашиты в код.
