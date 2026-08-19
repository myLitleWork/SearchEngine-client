# Технические заметки (приложение)

Краткая карта для проверки достоверности пользовательской документации. Не является руководством для конечного пользователя.

## Точка входа

- `SearchClient_asio.cpp` → `_tWinMain`
- Создаётся главная форма `TfMain` (`fMain`)
- Специальные режимы командной строки:
  - `--docx-worker` (всегда, для конвертации)
  - в Debug: `--multi-server-self-test`, `--device-auth-self-test`, `--auth-path-self-test`,
    `--docx-timeout-self-test`, `--docx-cancel-self-test`,
    `--docx-restart-self-test`

## Версия и имя продукта

- `Version.h`, `app-version.json`
- Заголовок окна: `SearchClient_asio v` + PE `ProductVersion`
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

## SVC-013 attachment container and legacy commands

- Live wire-container для `GET_ATTACHMENTS` на клиенте: `Attachments/AttachmentPackage.*`.
- `GET_MESSAGE=16` и `SAVE_MESSAGE_TO=2781032419` сохранены только как historical
  reserved enum values в `Net/myAsioClient.h` и не используются для production calls.
- Historical wire error codes `MessageNotFound`/`MessageSaveFailed`/`MessageReadFailed`
  (27/28/29) сохранены без renumber для cross-repo совместимости.

## Базы данных

| Файл | Назначение |
|---|---|
| `Settings.db3` | `servers`, `server_ports`, `ports`, `settings`, `map*` |
| `SearchRes.db3` | `res` — текущие результаты |
| Legacy `Res.db3` | миграция в пару Settings/SearchRes: сначала временные файлы `_SCM-set-`/`_SCM-res-`, публикация только после проверки таблиц и числа строк; `Res.db3` удаляется последним |

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

## Изоляция и диагностика DOCX

- Word Automation выполняется не в GUI-процессе, а в отдельном экземпляре
  `SearchClient_asio.exe --docx-worker`.
- Worker заранее помещается в Job Object. Родитель разделяет два дедлайна на
  документ: до 60 с на cold start `Word.Application` (пока worker не перешёл в
  `BUSY`) и 15 с на макрос/`SaveAs` после `BUSY`. Зависший worker завершается.
- PID точного экземпляра Word определяется через `Word.Application.Hwnd` и
  `GetWindowThreadProcessId`. Перед принудительным завершением полный путь
  проверяется через `QueryFullProcessImageNameW` с правом
  `PROCESS_QUERY_LIMITED_INFORMATION`; это работает для Win32-клиента и
  64-битного Word.
- Готовый файл публикуется только после успешной обработки временного
  `<имя>.part-<pid>-<tick>.docx`. Таймаут и отмена удаляют временный результат.
- Ошибка Word сбрасывает сессию: следующий документ получает новый worker и
  новый экземпляр Word.
- Постоянный технический журнал:
  `%LOCALAPPDATA%\SearchClient_asio\Logs\docx-worker.log`.
- Пользовательская кнопка **Отмена** выставляет атомарный флаг. Во время DOCX
  ожидание прерывается не позднее следующего 40-миллисекундного цикла; во время
  сетевой передачи отмена применяется после возврата сетевой операции.

Стадии `WORD_START`, `WORD_READY` и `RUN_MACRO` не выполняют рабочий макрос:
тестовая задержка ставится до него, а процесс уничтожается до продолжения.
Стадия `SAVE_DOCX` достигается уже после выполнения макроса и предназначена
только для отдельной ручной проверки на disposable-файлах.

Для `--docx-timeout-self-test` / `--docx-restart-self-test` аргумент timeout
(5-й) задаёт бюджет проверяемой фазы: при стадии `WORD_START` это таймаут
запуска Word, иначе — таймаут макроса/`SaveAs`. Стадия `WORD_START` должна
задавать delay больше word-start timeout; остальные стадии — delay больше
conversion timeout.

```powershell
Win32\Debug\SearchClient_asio.exe --docx-timeout-self-test <input.txt> <sht.docm> <out.docx> 4000 10000 WORD_START
Win32\Debug\SearchClient_asio.exe --docx-timeout-self-test <input.txt> <sht.docm> <out.docx> 4000 10000 WORD_READY
Win32\Debug\SearchClient_asio.exe --docx-timeout-self-test <input.txt> <sht.docm> <out.docx> 5000 12000 RUN_MACRO
Win32\Debug\SearchClient_asio.exe --docx-cancel-self-test <input.txt> <sht.docm> <out.docx> 15000 12000 4000 WORD_READY
Win32\Debug\SearchClient_asio.exe --docx-restart-self-test <input.txt> <sht.docm> <out.docx> 4000 10000 WORD_READY
```

Допустимые стадии: `WORD_START`, `WORD_READY`, `RUN_MACRO`, `SAVE_DOCX`.
Код возврата `0` означает правильный статус, отсутствие итоговых/`.part`
файлов и новой TEMP-сессии. При чистом исходном состоянии также проверяется
отсутствие оставшегося Word. Все процессы Word, существовавшие до теста,
должны остаться запущенными. Настоящий зависший VBA-макрос проверяется
отдельно вручную на disposable-файлах.

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
