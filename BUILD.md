# Сборка SearchClient

Целевая конфигурация выпуска — `Release|Win32`, Embarcadero Studio 22.0.

```bat
tools\release.bat
```

Собирается `Release|Win32` (с bump patch в `app-version.json`), затем
пересоздаётся версионный пакет:

```text
out\package\<version>\SearchClient-<version>
```

Пример: `out\package\4.3.1\SearchClient-4.3.1`.

Каждая **Release**-сборка поднимает patch на 1:

- IDE Release|Win32 — через ранний PreBuild
  (`scripts\PreBuild-SearchClientVersion.bat`), вызываемый через
  `$(MSBuildProjectDirectory)`;
- `Build-SearchClientPackage.ps1` — bump до MSBuild (Pre/Post build
  events на этом пути отключаются, чтобы не поднять версию дважды).

`tools\release.bat` — канонический агентский entrypoint полного Release/publish;
`tools\build.bat Release` является его алиасом. `tools\build.bat Debug` остаётся
обычной Debug MSBuild-сборкой.

Агент запускает `tools\release.bat` сразу с доступом к установленному toolchain
и системным модулям, без предварительной sandbox-пробы. До bump выполняются все
быстрые проверки окружения; затем `SearchClient_asio.cbproj` сравнивается
побайтно до и после MSBuild без `Get-FileHash`. Обычный Release не выполняет
дополнительные capability-probes, массовые hashes или ручные fallback-команды.

Если ошибка произошла после уже выполненного bump, повторите workflow только с
env `SEARCHCLIENT_SKIP_VERSION_BUMP=1`; повторная сборка и второй bump не
выполняются. Поддерживаемый параметр `-SkipVersionBump` эквивалентен
`SEARCHCLIENT_SKIP_VERSION_BUMP=1`.

Если EXE уже собран, обновить только пакет **без** bump:

```powershell
.\scripts\New-SearchClientPackage.ps1 -AllowDirty
```

Debug-сборки (`tools\build.bat`, `Config=Debug`) не бампают версию и не
публикуют релиз: PreBuild/PostBuild привязаны только к Release.

Release PostBuild вызывает `scripts\PostBuild-SearchClientPortable.bat` через
`$(MSBuildProjectDirectory)`; скрипт всегда берёт канонический каталог
`Win32\Release` и не зависит от `OUTPUTDIR`. Если IDE была открыта во время
правки `.cbproj`, закройте проект и откройте снова.

Канон версии: project-local `app-version.json`. Перед Release локальный
`scripts\Sync-SearchClientVersion.ps1` считает SHA-256 документированных
release-входов и синхронизирует `SearchClientVersion.rc` (PE `FileVersion`
`A.B.C.0`, `ProductVersion` `A.B.C`). XML project-файла не входит в hash,
поскольку IDE может переписать эквивалентный XML; выпуск только с изменением
project settings требует явного `-ForceBumpPatch`.

Без параметров helper лишь создаёт/синхронизирует projections. `-BumpPatch`
повышает patch только когда fingerprint изменился, а `-ForceBumpPatch`
запрашивает новый patch независимо от него. Fingerprint формата 2 использует
ограниченный набор: корневые исходники и resources разрешённых типов,
документированные source-каталоги, scripts/tools, tutorials и явный allowlist
package-template. Он не обходит `.git`, Win32, out, AsioServer, _db_test и
другие build/runtime-каталоги; generated version-файлы, cbproj, базы и секреты
в него не входят. Обычный Release не требует массовых SHA/timestamp-проверок.

При заданном `WORKSPACE_RELEASE_CLOUD_ROOT` после упаковки ZIP+SHA256 уходят в
Google Drive (`TOOLS\instructions\RELEASE_CLOUD_PUBLISH.md`):

```text
<CloudRoot>\SearchClient\<version>\SearchClient-<version>.zip
```

Отключение на один запуск: `-SkipCloudPublish`.

Запрос «собери Release и опубликуй» является разрешением выполнить сборку,
package и запись ZIP+SHA256 в CloudRoot, уже выбранный через `-CloudRoot`,
process-env или User-env. Повторно подтверждать этот путь не требуется.
SearchClient packager сам передаёт `AllowSensitive` для документированного
санитизированного template из `deployment\SearchClientPortable`; это не
разрешает добавлять произвольные runtime Settings/DB или секреты.

Для validation можно отключить только IDE PostBuild packaging, сохранив
prebuild-versioning: `SEARCHCLIENT_SKIP_PORTABLE_PACKAGE=1`. Это не отключает
cloud publish для последующего нормального package workflow.

Обычный IDE Release|Win32 выполняет PreBuild sync/bump, compile/link и
PostBuild package. Агентский `tools\release.bat` (и его алиас
`tools\build.bat Release`) вызывает `Build-SearchClientPackage.ps1 -AllowDirty`
ровно один раз, отключает build events на своём MSBuild-прогоне и затем
запускает package-only `New-SearchClientPackage.ps1`. Последний не компилирует и не
повышает версию: до создания package output он сверяет Win32 PE и обе версии
из ресурса с JSON. Для повтора уже повышенной версии используйте
`SEARCHCLIENT_SKIP_VERSION_BUMP=1` или `-SkipVersionBump`.

Успешный MSBuild ещё не означает успешный Release. Exit 0 разрешён только после
проверки локального package и, при настроенном CloudRoot, опубликованных ZIP и
`.sha256`; отсутствие любого обязательного артефакта — ошибка. Прямой ручной
вызов packager не является нормальным продолжением Release workflow.

Состав portable-пакета и allowlist: `docs\RELEASE_PACKAGE.md`.
