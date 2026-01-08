# Flashback

A platform for experts to transform complex technical subjects into muscle memory.

The project is divided by subdirectories, each holding the implementation of a specific component:

- **common**: basic types shared between other components
- **daemon**: the flashback server
- **database**: the database schema and interface for the server

## USAGE

You can use one of the following strategies to use this application sorted by accessibility and ease.

### 1. Released Packages

You can download any of the packages supported by your platform in [releases](releases/latest) to install `flashback` on your system.

### 2. Containerized Application

Alternatively, you can use the application in a containerized and isolated environment. You will need `docker` up and running:

```sh
docker container run --interactive --tty --name flashback --restart on-failure briansalehi/flashback:latest
```

Once you close the application, the container will also shutdown. Subsequently, use the same container to run the application:

```sh
docker container exec -it flashback
```

### 3. Native Installation

Alternatively, the project can be natively built on your system assuming you already have all the dependencies:

- `cmake` >= 4.0.0
- `make` >= 4.4.0
- `g++` >= 15.2.0
- `boost-devel` >= 1.83.0
- `qt6-devel` >= 6.10.0
- `grpc-devel` >= 1.48.0
- `protobuf-compiler` >= 3.19.0
- `gtest-devel` >= 1.15.0

Clone the project and build as follows:

```sh
cmake --workflow --preset client
```

## LICENSE

[GPL-3.0 License](LICENSE.md)
