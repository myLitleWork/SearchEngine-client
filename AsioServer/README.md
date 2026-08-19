# Stale server snapshot — NOT part of SearchClient build

This directory is an **archived copy** of an old server-side `AsioServer`
implementation. It is **not** referenced by `SearchClient_asio.cbproj` and must
not be used as the wire-protocol source of truth.

Production client protocol definitions live in:

```text
Net/myAsioClient.h
Net/myAsioClient.cpp
```

Server-side canonical definitions live in the SearchEngine repository:

```text
SearchEngine_asio_new/src/AsioServer/
```

Do not add new command handlers or wire behavior here.
