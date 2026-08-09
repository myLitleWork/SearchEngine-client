# Правила проекта SearchClient

Область действия — весь клиентский репозиторий `SearchEngine_Cient_asio`.
Общие правила находятся в `D:\MyWorkSpaceMain\AGENTS.md`.

## Назначение и стек

- VCL-клиент выполняет поиск через Asio-транспорт и использует локальные
  SQLite-базы.
- Стек: C++Builder VCL, Boost.Asio, FireDAC/SQLite, JSON, EhLib и FastReport.
- Основной проект: `SearchClient_asio.cbproj`; целевая платформа — Win32.
- Сервер `SearchEngine_asio_new` находится в соседнем независимом репозитории.
  Изменение wire-протокола, таймаутов или формата ответа проверять на обеих
  сторонах.

## Сборка и проверка

```bat
call "C:\Program Files (x86)\Embarcadero\Studio\22.0\bin\rsvars.bat"
msbuild SearchClient_asio.cbproj /t:Build /p:Config=Debug /p:Platform=Win32 /v:n /nologo
```

- Debug: `tools\build.bat Debug`. Канонический агентский Release/publish:
  `tools\release.bat`; `tools\build.bat Release` является его алиасом.
- Агент запускает `tools\release.bat` сразу с доступом к установленному
  toolchain и системным модулям, без предварительной sandbox-пробы.
- Все быстрые проверки окружения выполняются до bump. Project-файл сверяется
  побайтно до и после MSBuild без `Get-FileHash`.
- Обычный Release не выполняет дополнительные capability-probes, массовые
  hashes или ручные fallback-команды.
- Подробности версии и упаковки: `BUILD.md`.
- IDE Release|Win32 использует ранние события с `MSBuildProjectDirectory`:
  PreBuild синхронизирует версию до компиляции, а PostBuild упаковывает в
  `out\package\<version>\SearchClient-<version>` и при заданном process/User-env
  `WORKSPACE_RELEASE_CLOUD_ROOT` публикует ZIP. PostBuild не зависит от
  `OUTPUTDIR`.
  Агентский entrypoint выполняет тот же полный workflow через
  `scripts\Build-SearchClientPackage.ps1 -AllowDirty`.
- Fingerprint использует ограниченный набор явно перечисленных входов; он не
  обходит build/runtime-каталоги. Обычный Release не требует массовых
  SHA/timestamp-проверок.
- Публикация выполняется через
  `TOOLS\scripts\release\Publish-ReleasePackageIfConfigured.ps1`.
- Запрос «собери Release и опубликуй» разрешает весь этот workflow без
  повторного вопроса о CloudRoot. Канонический packager уже передаёт
  `AllowSensitive` только для документированного санитизированного template;
  дополнительное подтверждение не требуется.
- Ручная упаковка уже собранного EXE без bump:
  `.\scripts\New-SearchClientPackage.ps1 -AllowDirty`.
- Успешный MSBuild сам по себе не означает успешный Release: exit 0 допустим
  только после проверки локального package и, при настроенном CloudRoot, ZIP и
  `.sha256`. Отсутствие обязательного артефакта — ошибка; прямой ручной
  packager не является нормальным продолжением workflow.
- `-SkipVersionBump` — поддерживаемый параметр и соответствует process-env
  `SEARCHCLIENT_SKIP_VERSION_BUMP=1`.
- Smoke-check сетевых изменений выполнять с контролируемым сервером и
  disposable-копиями локальных баз.

## Данные и границы изменений

- Миграции и код в `Database` должны сохранять существующие данные и быть
  повторяемыми; production-базы на месте не изменять.
- Старые абсолютные include-пути FastReport в `SearchClient_asio.cbproj` не
  чистить автоматически: текущая сборка успешна через установленный toolchain.
- `Net` содержит активный клиентский транспорт. Не переносить туда код из
  посторонних заготовок без проверки project-файла и протокола.
- Текущие untracked `AsioServer\`, `_db_test\`, `Database\res.sql`,
  `Database\settings.sql` и `main.sql` исключены из этапов Codex: не читать,
  не добавлять и не удалять без отдельного запроса.
- `AsioServer\` не подключён к сборке и не должен считаться частью клиента
  только из-за расположения внутри репозитория.
- После сборки проверить `SearchClient_asio.cbproj`, DFM-файлы и исходный
  untracked-набор.
