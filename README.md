# libxenbe

## Dependencies:
### Required:
* Xen tools
### Optional:
* Doxygen

## How to configure
```
mkdir ${BUILD_DIR}
cd ${BUILD_DIR}
cmake ${PATH_TO_SOURCES} -D${VAR1}=${VAL1} -D{VAR2}=${VAL2} ...
```
There are variables and options. Options can be set to ON and OFF.

Supported options:

| Option | Description |
| --- | --- |
| `WITH_DOC` | Creates target to build documentation. It required Doxygen to be installed. If configured, documentation can be create with `make doc` |
| `WITH_TEST` | Creates target to build unit tests. If configured, unit test can be built and checked with `make test`|

Supported variabels:

| Variable | Description |
| --- | --- |
| `CMAKE_BUILD_TYPE` | `Realease`, `Debug`, `RelWithDebInfo`, `MinSizeRel`, `Coverage`.<br/>`Coverage` is special type which creates target to check unit tests coverage.  Use `make coverage` |
| `CMAKE_INSTALL_PREFIX` | Default install path |
| `XEN_INCLUDE_PATH` | Path to Xen tools includes if they are located in non standard place |
| `XEN_LIB_PATH` | Path to Xen tools libraries if they are located in non standard place |

Example:
```
// Debug build with documentation
cmake ${PATH_TO_SOURCES} -DWITH_DOC=ON -DCMAKE_BUILD_TYPE=Debug
```

## How to build:
```
cd ${BUILD_DIR}
make          // build sources
make doc      // build documentation
make test     // build and check unit tests
make coverage // create unit tests coverage report (available if CMAKE_BUILD_TYPE=Coverage)
```
## How to install
```
make install // to default location
make DESTDIR=${PATH_TO_INSTALL} install //to other location
```
