# sftfs

`sftfs` is a small FUSE3 filesystem that mounts a remote directory over SFTP using `libssh`.

## Features

- Mount a remote path as a local filesystem
- SSH/SFTP backend built on `libssh`
- Optional caching layer for attributes, directory entries, and symlinks
- Unit tests for internal data structures and cache code

## Dependencies

- C compiler with C99 support
- CMake 3.13+
- `fuse3`
- `libssh`

## Build

```sh
mkdir -p build
cd build
cmake ../
make -j$(nproc)
```

Useful CMake options:

- `-DHAVE_TESTS=ON` to build unit tests
- `-DSFTFS_CACHED=OFF` to disable the cache layer
- `-DSFTFS_FUNCTION_TRACING=ON` to enable function tracing

## Usage

```sh
mkdir -p mnt
./sftfs [user@]host:/remote/path mnt [-p PORT]
```

Example:

```sh
./sftfs alice@example.com:/srv/data mnt
```

The program verifies the host key, prompts for the SSH password, and runs FUSE in single-threaded mode.

To unmount:

```sh
umount mnt
```

## Test

```sh
mkdir -p build
cd build
cmake ../ -DHAVE_TESTS=ON
make -j$(nproc) ut
./test/ut/run
```
