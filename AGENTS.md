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

- Для выпуска заменить `Debug` на `Release`.
- Release PostBuild вызывает `scripts\PostBuild-SearchClientPortable.bat` →
  `out\package\SearchClient-Release` и при заданном User-env
  `WORKSPACE_RELEASE_CLOUD_ROOT` публикует ZIP через
  `TOOLS\scripts\release\Publish-ReleasePackageIfConfigured.ps1`.
- Ручная упаковка без полной пересборки:
  `.\scripts\New-SearchClientPackage.ps1 -AllowDirty`.
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
