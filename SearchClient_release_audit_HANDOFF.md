# SearchClient — предрелизный аудит кода / HANDOFF

> **Статический аудит.** Код не собирался и не запускался в RAD Studio; серверный/интеграционный стенд не запускался. Выводы ниже основаны на анализе текущего `main` репозитория `myLitleWork/SearchEngine-client` на commit `9abbeaf7bd54f3134c5e841f83745a88516b625c` (версия `4.3.24`). На этом commit GitHub status checks отсутствуют.
>
> Код в рамках этого этапа **не исправлялся**. Этот файл — handoff для отдельного этапа fixes.

## 1. Цель и охват

Цель: найти реальные предрелизные дефекты SearchClient, которые достижимы по текущему runtime-пути, и отделить их от гипотез, legacy-кода и изменений бизнес-логики.

Проверены:
- основной UI/runtime: `formMain.cpp/.h`;
- сетевой слой: `Net/myAsioClient.*`, `Net/MultiServerManager.h`;
- файловая публикация и replacement: `ClientFilePublish.h`, `UserFileReplace.h`;
- сохранение вложений: `Frames/FrameAttachments.cpp`, `Message/Message.cpp`;
- row-details/lifetime: `Frames/MyFrameDbg.*`, `FrameWay.cpp`, `FramePdtv.cpp`;
- локальная SQLite/миграция: `Database/SqliteDb.cpp`, `Database/ResDbMigration.h`;
- SQL-конструктор поиска: `SqlCondition.*`;
- DOCX worker/session: `DocxConversionWorker.cpp`, `WordBatchProcessor.*`, `WordServer/*`;
- активный состав проекта по `SearchClient_asio.cbproj`;
- вспомогательные сериализация/JSON/строковые модули, имеющие release-участие.

Не проверялся серверный репозиторий. Каталоги/файлы, запрещённые `AGENTS.md` для чтения/изменения, не использовались.

### Что специально перепроверено и НЕ перенесено из старых замечаний

На текущем `main` уже есть:
- create-new/no-replace публикация конечного файла в новом `requestFileToPath()`;
- лимит in-memory payload;
- `uiLive_`/`uiInflight_` и detach для frame lifetime;
- исправленный pure-virtual init frame;
- отдельный DOCX worker с timeout/cancel/restart;
- FireDAC/SQLite инфраструктура и текущая миграционная схема.

Поэтому старые проблемы не считались дефектами автоматически.

## 2. Краткий вердикт о готовности к релизу

**Релиз пока не рекомендую без отдельного fix-pass.**

Причина — два подтверждённых HIGH file-safety дефекта:
1. пользовательский существующий файл удаляется **до** начала надёжной передачи одиночного вложения;
2. потоковая загрузка безусловно удаляет соседний `<target>.part`, не доказывая, что этот файл принадлежит SearchClient.

Кроме них подтверждены:
- реальный UI-reentrancy путь через `Application->ProcessMessages()` во время записи результатов;
- выход автосоздаваемого каталога сохранения за выбранную пользователем базовую папку;
- неэкранированный `'` в обычном SQL-конструкторе поиска;
- неоднозначная бизнес-семантика retry после частично сохранённого пакета вложений;
- три LOW локальных дефекта.

При этом признаков системной lifetime-катастрофы в текущих фоновых frame/search путях не найдено: существующие `uiLive_`, `uiInflight_`, `TrackOp()` и `Detach()` реально используются.

---

## 3. HIGH — по вероятности

### SC-01 — существующий файл удаляется до успешного скачивания одиночного вложения

- **Status:** confirmed
- **Severity:** HIGH
- **Probability:** medium
- **Confidence:** high
- **Classification:** AUTO
- **Where:** `Frames/FrameAttachments.cpp`, `TfrAttachments::dbgAttachmentsColumns2CellButtons0Click`; downstream `MultiServerManager::requestFileToPath` → `MyAsioClient::requestFileToPath`

**Проблема**

После подтверждения overwrite в `TSaveDialog` код сразу делает `DeleteFile(dlg->FileName)`, и только затем начинает сетевой `requestFileToPath()`.

Новый `requestFileToPath()` специально работает в create-new/no-replace режиме. Из-за этого caller удаляет старый пользовательский файл заранее, чтобы обойти защиту.

**Реальный путь исполнения**

`Приложения` → кнопка сохранения конкретного вложения → пользователь выбирает уже существующий путь → подтверждает overwrite → старый файл удаляется → начинается сеть/чтение/запись/publish.

**Сценарий воспроизведения**

1. Создать `report.bin` с контрольным содержимым.
2. Выбрать его как destination для вложения и подтвердить замену.
3. После удаления target оборвать сервер/сеть либо вызвать ошибку записи.
4. `requestFileToPath()` вернёт failure, но исходного `report.bin` уже нет.

**Риск**

Безвозвратная потеря существующего пользовательского файла при обычной ошибке передачи.

**Рекомендуемый вариант**

Сначала скачать во **владельческий уникальный staging-файл** рядом с target; только после полного успешного скачивания и закрытия staging выполнить атомарную замену target через существующий replacement-helper (`user_file_replace` либо эквивалентный replace-after-success путь). До успешного staging target не трогать.

**Альтернативы**

1. Расширить `requestFileToPath()` отдельным режимом `replaceAfterSuccess`, реализованным внутри transport/file-publish слоя.
2. Запретить overwrite и всегда предлагать уникальное имя. Безопасно, но меняет UX.

**Тест после исправления**

- существующий target + принудительный network failure → target byte-for-byte неизменен;
- успешный overwrite → новый файл опубликован, staging отсутствует;
- failure при publish → старый target остаётся.

---

### SC-02 — `requestFileToPath()` безусловно удаляет чужой `<target>.part`

- **Status:** confirmed
- **Severity:** HIGH
- **Probability:** low
- **Confidence:** high
- **Classification:** AUTO
- **Where:** `Net/myAsioClient.cpp`, `MyAsioClient::requestFileToPath`

**Проблема**

Временный путь вычисляется детерминированно как `outPath + ".part"`, после чего перед любой сетевой операцией выполняется `fs::remove(partPath, ...)`.

Нет owner-marker, уникального имени или доказательства, что существующий `.part` создан именно этим экземпляром SearchClient.

Более того, удаление происходит до проверки живого сокета: даже вызов без подключения способен уничтожить уже существующий sibling `.part`.

**Сценарий воспроизведения**

1. Создать `C:\Export\a.bin.part` с контрольными данными.
2. Запросить сохранение `C:\Export\a.bin`.
3. Вызвать путь при отсутствующем подключении или принудительно оборвать передачу.
4. `a.bin.part` удалён SearchClient.

**Риск**

Потеря постороннего/пользовательского файла с совпавшим техническим суффиксом.

**Рекомендуемый вариант**

Использовать уникальный temporary sibling с `CREATE_NEW`/эквивалентом, по той же модели, что уже реализована в `client_file_publish::CreateUniqueTemporary()`. Удалять только тот temp, ownership которого получен текущей операцией.

**Альтернативы**

1. Оставить `.part`, но добавить process/sequence/random component и создавать файл exclusively.
2. Хранить temp в отдельном session temp-dir, затем атомарно публиковать в destination. Чуть сложнее при разных volume.

**Тест после исправления**

Предсозданный `a.bin.part` не меняется ни при success, ни при failure скачивания `a.bin`; SearchClient удаляет только собственный temp.

---

## 4. MEDIUM — по вероятности

### SC-03 — reentrancy: Connect/смена года может вызвать `clear()` внутри `convTelegs()`

- **Status:** confirmed
- **Severity:** MEDIUM
- **Probability:** medium
- **Confidence:** high
- **Classification:** TEST
- **Where:** `formMain.cpp`: `ComponentsGuard`, `TfMain::componentsEnable`, `TfMain::btnSearchClick`, `TfMain::finishSearch`, `TfMain::convTelegs`, `TfMain::cmbGodChange`, `TfMain::btnConnectClick`, `TfMain::clear`

**Проблема**

`ComponentsGuard(gSearchProcessing, this)` вызывает `componentsEnable(false)`, но `componentsEnable()` не отключает `btnConnect` и `cmbGod`.

`finishSearch()` вызывает `convTelegs()`. Внутри цикла вставки в `qryTelegi` на каждой строке вызывается `Application->ProcessMessages()`.

В этот момент обработчик `cmbGodChange` может синхронно вызвать `btnConnectClick`, а тот — `clear()`. `clear()` закрывает тот же `qryTelegi`, заменяет SQL на `DELETE FROM res`, затем `SELECT * FROM res`.

После возврата в `convTelegs()` код продолжает работать с уже переопределённым query/Params и той же транзакцией.

**Сценарий воспроизведения**

1. Получить достаточно большой набор результатов, чтобы `convTelegs()` выполнялся заметное время.
2. Во время локального заполнения результатов сменить год или нажать Connect.
3. Событие обрабатывается через `ProcessMessages()`.
4. Возможны очистка уже вставленных строк, `ParamByName`/query errors и частичный/пустой локальный результат.

**Риск**

Некорректные/неполные результаты поиска и трудно воспроизводимые UI/DB ошибки без межпоточной гонки — это same-thread reentrancy.

**Рекомендуемый вариант**

Ввести единый operation gate для search-result commit: пока `gSearchProcessing` активен, запретить/отложить Connect, Disconnect и смену года. Дополнительно убрать `Application->ProcessMessages()` из цикла DB-вставки, если UI responsiveness можно сохранить иначе.

**Альтернативы**

1. Только добавить `btnConnect/cmbGod` в disable-set и hard-guard соответствующих handlers.
2. Разделить network search и локальную DB commit-фазу отдельным состоянием (`SearchRunning`/`CommittingResults`) и разрешать reconfigure только после обеих фаз.

**Тест после исправления**

На искусственно медленном поиске/большом ответе многократно менять год/нажимать Connect. Reconfigure не должен выполняться до завершения commit; итоговое число строк стабильно, ошибок query/params нет.

---

### SC-05 — обычный SQL-поиск не экранирует одинарную кавычку

- **Status:** confirmed
- **Severity:** MEDIUM
- **Probability:** medium
- **Confidence:** high
- **Classification:** TEST
- **Where:** `SqlCondition.h`, `getOrArgEq/getOrArgLike`; caller `formMain.cpp`, `TfMain::btnSearchClick`

**Проблема**

Для обычного field-based SQL режима аргумент вставляется в SQL literal как `' + value + '`, но `'` внутри значения не превращается в `''`.

Это не относится к явному raw-режиму `cmbField == "SQL"`: raw SQL сейчас намеренно передаётся как выражение и не должен быть «исправлен» этим пунктом.

**Сценарий воспроизведения**

Поиск значения вроде `O'NEIL` в обычном поле строит синтаксически повреждённое либо семантически изменённое условие.

**Риск**

Ошибки поиска/неожиданные условия для легитимных строк с апострофом.

**Рекомендуемый вариант**

В `getOrArgEq/getOrArgLike` экранировать `'` как `''` **только внутри формируемого SQL literal**. Raw SQL mode оставить неизменным.

**Альтернативы**

1. Долгосрочно перейти на структурированный/параметризованный protocol request — требует согласованного изменения server side, поэтому не для локального release fix.
2. Отдельно определить семантику `%`/`_` в LIKE. Не менять её автоматически в рамках этого дефекта.

**Тест после исправления**

Проверить `O'NEIL`, кириллицу с `'`, `%`, `_`; raw SQL mode должен формировать прежний запрос.

---

### SC-04 — каталог из поискового запроса может выйти за выбранную base-dir

- **Status:** confirmed
- **Severity:** MEDIUM
- **Probability:** low
- **Confidence:** high
- **Classification:** AUTO
- **Where:** `formMain.cpp`, `TfMain::btnSaveClick`

**Проблема**

При `SelectedRows > 1 && chkMakeDir` код берёт `cmbReq->Text`, заменяет только `/` на `-` и затем делает:

`out_dir_async = out_dir + "\\" + dir_zapr + "\\"`.

`\`, `..`, drive/root components и `:` не проходят через `ResolveUnderBase()` как **имя подкаталога**. После этого уже escaped path используется как новая base-dir, поэтому нижележащая защита file name не возвращает его в исходную выбранную папку.

**Сценарий воспроизведения**

Выбрать `C:\Export` и выполнить поиск `..\Outside`, затем сохранить несколько выбранных результатов с `Создать каталог`.

Фактическая normalized base становится вне `C:\Export`.

**Риск**

Файлы сохраняются не там, куда выбрал пользователь.

**Рекомендуемый вариант**

Рассматривать поисковую строку только как **leaf display name**, а не path: удалить/заменить path separators и запрещённые компоненты, затем обязательно пропустить получившееся относительное имя через `ResolveUnderBase(originalBase, ...)`.

**Альтернативы**

1. Использовать нейтральное имя (`Search <timestamp>`), а исходный запрос писать в metadata/log.
2. При небезопасной строке не создавать подкаталог и сохранять прямо в выбранную base-dir.

**Тест после исправления**

`..\Outside`, `a\b`, `.`, `..`, `C:foo`, UNC-подобные строки — итоговые файлы всегда остаются под исходной выбранной папкой.

---

### SC-06 — retry пакета вложений повторно сохраняет уже записанные файлы

- **Status:** confirmed
- **Severity:** MEDIUM
- **Probability:** low
- **Confidence:** high
- **Classification:** OWNER
- **Where:** `formMain.cpp`, `TfMain::btnAttachSaveClick`; `Message/Message.cpp`, `Message::saveToDirectory`

**Проблема**

`Message::saveToDirectory()` публикует вложения последовательно прямо в конечный каталог. Если файл N падает после успешной записи 1..N-1, предыдущие файлы остаются.

`btnAttachSaveClick` предлагает `Повторить?` и через `goto retry_save` запускает весь `saveToDirectory()` заново. Уже существующие файлы проходят через `EnsureUniqueFilePath()` и сохраняются повторно под `(1) ...`, `(2) ...`.

При `Нет` частичный набор остаётся без детального manifest результата.

**Сценарий воспроизведения**

Пакет минимум из двух вложений; искусственно вызвать failure на втором/последующем файле. Нажать Retry. Первый файл получает дубликат с новым именем.

**Риск**

Пользователь получает смесь частичного результата и дубликатов; по UI трудно понять, какой набор полный.

**Рекомендуемый вариант (OWNER)**

Сделать batch **resumable по элементам**: хранить результат каждой записи и при Retry повторять только failed/unwritten attachments. После операции показать `saved/failed` с именами.

**Альтернативы**

1. All-or-nothing staging: сначала записать весь пакет в уникальный temp-dir и публиковать только после полной готовности. Надёжнее, но нужно решить семантику merge с уже существующим дневным каталогом.
2. Сохранить нынешний partial-success, но убрать автоматический полный retry: показать список уже сохранённых и дать отдельную команду «повторить только ошибки».

**Нужно решение владельца**

Выбрать ожидаемую семантику: resumable partial, all-or-nothing или explicitly partial. Самостоятельно бизнес-политику не менять.

**Тест после решения**

Failure на 2-м файле из 3 + Retry: ни один уже успешный файл не дублируется; UI точно сообщает итог.

---

## 5. LOW — по вероятности

### SC-08 — progress загрузки файлов на сервер увеличивается на один лишний шаг

- **Status:** confirmed
- **Severity:** LOW
- **Probability:** high
- **Confidence:** high
- **Classification:** AUTO
- **Where:** `formMain.cpp`, `TfMain::btnLoadOnServerFilesClick`, `updateFileLoadUI`, `deleteFilesFromUI`

**Проблема**

`MaxValue = fileList.size()`. Каждый обработанный файл вызывает `updateFileLoadUI()` → `AddProgress(1)`. После завершения `deleteFilesFromUI()` ещё раз вызывает `AddProgress(1)`.

Итого batch из N файлов даёт N+1 increments при MaxValue=N.

**Рекомендуемый вариант**

Удалить финальный `AddProgress(1)` из `deleteFilesFromUI()`. Эта функция должна только синхронизировать список/controls.

**Альтернатива**

Если финальный шаг должен отображаться отдельно, тогда явно задавать `MaxValue=N+1`; это менее естественная семантика.

**Тест**

N=1, N=3, failure на первом/середине: progress не выходит за MaxValue и отражает число обработанных файлов.

---

### SC-07 — общий `%TEMP%\ZipExtractTemp` не изолирован между экземплярами клиента

- **Status:** confirmed
- **Severity:** LOW
- **Probability:** very low
- **Confidence:** high
- **Classification:** AUTO
- **Where:** `formMain.cpp`, `TfMain::SaveFileIsh`; `mySomeFunc.h`, `ExtZip`

**Проблема**

`SaveFileIsh` использует process-local счётчик для zip-dir, а `ExtZip()` извлекает mapped files в фиксированный `%TEMP%\ZipExtractTemp\`.

Single-instance guard в entry point не установлен. Два процесса SearchClient, сохраняющие исходящие вложения одновременно, используют один extraction directory. Каждый в конце рекурсивно удаляет его.

**Подтверждённое проявление**

Один экземпляр способен удалить extracted files другого до его `FileExists/MoveNewFile`, вызвав ошибку сохранения. Для утверждения о «подмене содержимого» данных недостаточно — поэтому она здесь не заявляется.

**Рекомендуемый вариант**

Создавать уникальный extraction temp-dir на операцию (`PID + monotonic/unique token`) через create-new directory loop и удалять только его.

**Тест**

Два процесса одновременно извлекают архивы с одинаковыми internal names: обе операции завершаются независимо; cross-delete отсутствует.

---

### SC-09 — несколько активных `TStringList*` не защищены RAII на exception path

- **Status:** confirmed
- **Severity:** LOW
- **Probability:** very low
- **Confidence:** high
- **Classification:** AUTO
- **Where:** как минимум:
  - `formMain.cpp`, `TfMain::loadTextToEditFromServer`;
  - `formMain.cpp`, `TfMain::tmLoadTextTimer`;
  - `formMain.cpp`, DOCX ветка `TfMain::SaveFileVh`;
  - `Frames/FramePdtv.cpp`, `dbgPdtvRowDetailPanelShow`.

**Проблема**

Паттерн `new TStringList()` → несколько потенциально throwing операций → `delete` оставляет leak на исключении. В `tmLoadTextTimer` исключение до queued completion дополнительно обходится без штатного re-enable timer.

**Рекомендуемый вариант**

Использовать stack object или `std::unique_ptr<TStringList>`; для `tmLoadTextTimer` отдельно обеспечить completion/finalizer, который восстанавливает UI/timer state на всех исходах.

**Тест**

Debug fault injection/исключение после allocation: `ReportMemoryLeaksOnShutdown` не показывает leak, timer/UI восстанавливаются.

---

## 6. AUTO — локальные безопасные исправления

Исправлять без изменения бизнес-логики:

| ID | Что менять |
|---|---|
| SC-01 | replace-after-success для одиночного attachment overwrite; не удалять final заранее |
| SC-02 | уникальный owned temp вместо фиксированного `.part` |
| SC-04 | sanitize + `ResolveUnderBase` для auto-created directory |
| SC-07 | уникальный per-operation ZIP extraction temp-dir |
| SC-08 | убрать лишний progress increment |
| SC-09 | RAII/finalizer для `TStringList`/timer cleanup |

AUTO-пункты **не исправлены в этом commit/handoff** — только подготовлены для следующего этапа.

## 7. TEST — изменения, требующие регрессионной проверки

| ID | Почему не AUTO | Обязательная проверка |
|---|---|---|
| SC-03 | меняет доступность UI/reentrancy и момент reconfigure | slow/large search + попытка смены года/Connect |
| SC-05 | меняет построение runtime SQL condition | apostrophe + `%`/`_` + raw SQL regression |

Рекомендуемый вариант всегда указан первым в карточке дефекта.

## 8. OWNER / реальные примеры

### SC-06 — политика partial/retry при сохранении пакета вложений

Нужно зафиксировать одно из правил:

1. **Рекомендуется: resumable partial** — успешно сохранённые элементы фиксируются, Retry работает только по failed.
2. All-or-nothing staging — пакет появляется в destination только после полной готовности.
3. Explicit partial — текущая последовательная запись остаётся, но retry не повторяет success и UI показывает manifest.

До решения владельца не менять семантику автоматически.

## 9. DEFERRED / legacy / нет release-caller

### D-01 — `Net/SocketFileSaver.cpp`

Файл компилируется проектом и содержит старый опасный алгоритм: deterministic `.part`, удаление target перед rename. Однако в текущем runtime caller не установлен; новый `MyAsioClient::requestFileToPath()` его не использует.

**Не считать подтверждённым release defect.** Если класс снова подключат к runtime — сначала заменить алгоритм на текущую safe-publish схему.

### D-02 — signed-token / RSA path

Signed removable-token логика в `tmLoginInTimer` закрыта `#if 0`; текущая release-авторизация использует marker file beside EXE. `gTokens/gTick` и RSA ownership не имеют текущего release caller.

**Не исправлять в release fix-pass.** Перед re-enable нужен отдельный crypto/ownership/security audit.

### Не повышено до дефекта после перепроверки

Паттерны `toW()/Narrow()` с буфером `size-1` и завершающим NUL не включены в список подтверждённых дефектов: для текущих caller/input недостаточно оснований утверждать реальную memory corruption. Если позже появится caller с произвольным invalid UTF-8, пересмотреть отдельно.

## 10. Smoke-чеклист после fix-pass

### Build / startup

- [ ] Win32 Release собирается в RAD Studio/C++Builder без новых warnings/errors.
- [ ] Запуск с существующими `Settings.db3`/`SearchRes.db3`.
- [ ] Запуск на копии legacy `Res.db3` и проверка миграции/повторного запуска.
- [ ] Версия приложения остаётся согласованной с `SearchClientVersion.rc`.
- [ ] В рабочем каталоге/рядом с EXE нет неожиданно удалённых пользовательских файлов.

### Существующие debug self-tests

- [ ] `--multi-server-self-test` → exit code 0.
- [ ] `--docx-timeout-self-test` → exit code 0.
- [ ] `--docx-cancel-self-test` → exit code 0.
- [ ] `--docx-restart-self-test` → exit code 0.
- [ ] `--auth-path-self-test` → ожидаемый exit code для подготовленного marker.

### Search / DB / UI

- [ ] Обычный поиск по одному серверу.
- [ ] Multi-server search, оба сервера успешны.
- [ ] Один сервер недоступен: partial result корректно обозначен, `source_server_id` сохранён.
- [ ] Большой result set: во время локального commit попытаться Connect/сменить год — reconfigure не врывается в `convTelegs`.
- [ ] Поиск обычного поля со строкой `O'NEIL`.
- [ ] LIKE-поиск с `%` и `_` сохранил заранее согласованную семантику.
- [ ] Raw SQL mode не изменён.
- [ ] Row details: Путь / Приложения / Подтверждения, затем закрытие формы во время активной операции.

### File safety

- [ ] Single attachment overwrite: forced transfer failure оставляет старый target неизменным.
- [ ] Single attachment overwrite: success корректно заменяет target только после полной передачи.
- [ ] Предсозданный `<target>.part` с sentinel data не удаляется и не меняется.
- [ ] Race: другой процесс создаёт final перед publish → операция fail-closed, чужой final не удалён.
- [ ] Multi-save + `chkMakeDir`: `..\x`, `a\b`, `.`, `..`, `C:foo` не выводят файлы за исходную base-dir.
- [ ] Входящие attachments с небезопасным relative path отклоняются.
- [ ] Исходящие ZIP attachments сохраняются с корректными именами и без overwrite.
- [ ] Два экземпляра клиента одновременно сохраняют ZIP attachments — temp dirs независимы.

### Attachment batch policy

- [ ] Искусственный failure на 2-м файле из 3.
- [ ] Retry следует принятой OWNER-семантике.
- [ ] Уже successful attachment не создаёт дубль.
- [ ] Пользователь видит точный saved/failed итог.

### Upload / progress / RAII

- [ ] Upload 1 файла: progress 1/1.
- [ ] Upload N файлов: progress N/N, не N+1.
- [ ] Failure upload: progress соответствует фактически обработанным.
- [ ] Fault injection на `TStringList` paths: leak отсутствует, timer/UI восстанавливаются.

### DOCX

- [ ] Один DOCX.
- [ ] Пакет DOCX.
- [ ] Word cold start.
- [ ] Timeout макроса.
- [ ] Cancel.
- [ ] Worker restart после kill/timeout.
- [ ] Старый существующий `.docx` не перезаписывается случайно; unique-name/publish semantics сохранены.

## 11. Минимум перед релизом

**Release gate:**

1. Исправить SC-01 и SC-02 — оба HIGH file-safety.
2. Исправить SC-04 — containment выбранной пользователем папки.
3. Исправить SC-03 и пройти reentrancy regression.
4. Исправить SC-05 и пройти SQL regression, не меняя raw SQL mode.
5. Получить OWNER-решение по SC-06; если текущее partial/retry поведение сознательно принимается, зафиксировать это явно.
6. Выполнить file-safety + search/DB + существующие debug self-tests из раздела 10.

SC-07/SC-08/SC-09 не выглядят самостоятельными release-blocker’ами, но это дешёвые локальные AUTO-пункты; разумно закрыть их в том же fix-pass.

## 12. Известные реализованные риски / не поднятые как дефекты

- Multi-server search допускает partial success, если часть серверов недоступна; UI явно показывает число успешных серверов. Это выглядит как осознанная текущая политика.
- `convTelegs()` при невозможности `StartTransaction()` продолжает локальную запись без транзакции и помечает результат как partial. Поскольку `SearchRes` используется как локальный cache результата и UI сообщает partial, это не классифицировано как дефект без требования владельца на all-or-nothing.
- Ошибки parsing отдельных дат переводятся в `NULL` с отчётом пользователю; не менять без бизнес-решения.
- Текущий code audit не доказывает корректность внешних server-side форматов/семантики за пределами client repo.

---

## Итог для следующего этапа

Начинать fix-pass с `SC-01`, затем `SC-02`, `SC-04`, `SC-03`, `SC-05`.  
AUTO можно исправлять локально.  
TEST — только с указанной регрессией.  
SC-06 — только после OWNER-решения.  
DEFERRED не трогать в release patch.
