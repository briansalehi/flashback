# ![flashback logo](doc/flashback.png) Flashback

A program for experts to study resources, review references, and practice subjects.

## USAGE

You can use one of the following strategies to use this application sorted by accessibility and ease.

### 1. Released Packages
You can download any of the packages supported by your platform in [releases](releases/latest) to install `flashback` on your system.

### 2. Containerized Application

Alternatively, you can use the application in a containerized and isolated environment. You will need `docker` up and running to build without dependencies:

```sh
docker container run -it -n flashback briansalehi/flashback:latest
```

### 3. Native Installation

Alternatively, the project can be natively built on your system assuming you already have all the dependencies:

* `CMake` > 3.27.0
* `GCC` > 4.2.0
* `Boost` > 1.80.0
* `Qt` > 6.9.0
* `gRPC` > 1.48.0
* `Protobuf` > 3.19.0

Clone the project and build as follows:

```sh
cmake --workflow --preset client
```

## LICENSE

[MIT License](LICENSE.md)