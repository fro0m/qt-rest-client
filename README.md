# qt-rest-client

A small, dependency-light HTTP/REST client built on top of Qt's
`QNetworkAccessManager`. It takes care of the tedious parts of building a
request (query strings, form-urlencoded / JSON / multipart bodies, file
uploads, custom headers) and exposes the response through a simple signal.

Licensed under the Apache License 2.0.

## Features

- GET / POST / PUT / HEAD / DELETE, plus arbitrary custom verbs (PATCH, …)
- Request body layouts: query string, `application/x-www-form-urlencoded`,
  `application/json`, and `multipart/form-data`
- File uploads from disk or from an in-memory byte array
- Custom raw headers
- Asynchronous by default — emits `executionFinished()` when done
- Each worker owns its own access manager, so workers are independent and can
  run from different threads

## Requirements

- Qt 6 (Core + Network)
- C++17
- CMake ≥ 3.15

## Building

```bash
cmake -S . -B build
cmake --build build
```

To install system-wide:

```bash
cmake --install build
```

The qmake files (`restClient.pro`, `RESTclient.pri`) are still around for the
test project, but CMake is the primary build system for the library.

## Usage

```cpp
#include "HttpRequestWorker.h"

HttpRequestInput input(QStringLiteral("https://httpbin.org/get?foo=bar"),
                       HttpMethod::Get);

HttpRequestWorker* worker = new HttpRequestWorker(this);
worker->setReadResponseOnError(true);

QObject::connect(worker, &HttpRequestWorker::executionFinished,
                 [](HttpRequestWorker* w) {
                     qDebug() << "status:" << w->statusCode;
                     qDebug() << "body:" << w->response;
                     w->deleteLater();
                 });

worker->execute(&input);
```

Posting JSON is just a different layout:

```cpp
HttpRequestInput input(QStringLiteral("https://httpbin.org/post"), HttpMethod::Post);
input.varLayout = HttpRequestVarLayout::Json;
input.addVar("name", "alice");
input.addVar("meta", R"({"role":"admin"})"); // embedded as a nested object
```

See `main.cpp` for a runnable example and `test/` for the functional tests.

## Tests

The tests in `test/` hit the public `postman-echo.com` echo endpoint, so they
need network access. Build and run them with qmake:

```bash
cd test
qmake && make && ./rest_client_test
```
