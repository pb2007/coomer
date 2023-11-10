# Coomer
Advanced zoomer application for Linux. Written in C. Inspired by [boomer](https://github.com/tsoding/boomer).

## Quick Start
```console
$ ./build.sh
$ CFLAGS="-O3" ./build.sh   # if you want optimizations
$ ./coomer
```

## Controls
| Action                   | Control                                             |
|--------------------------|-----------------------------------------------------|
| Zoom in/out              | Scroll wheel or <kbd>{+,=,i}</kbd>/<kbd>{-,o}</kbd> |
| Change flashlight radius | <kbd>Ctrl</kbd> + <kbd>[zoom key]</kbd>             |
| Reset state              | <kbd>r</kbd> or <kbd>0</kbd>                        |
| Toggle flashlight        | <kbd>f</kbd>                                        |
| Move image               | Click and drag                                      |
| Enter command mode       | <kbd>;</kbd> or <kbd>:</kbd>                        |
| Quit application         | <kbd>q</kbd>                                        |

### Command mode
| Action                   | Control                                             |
|--------------------------|-----------------------------------------------------|
| Enter normal mode        | <kbd>Esc</kbd>                                      |

Command mode allows for text entry with the alphanumeric keys and navigation
using the standard arrow keys.

## Commands
| Action                  | Command             |
|-------------------------|---------------------|
| Write whole screenshot  | `:w`, `:write`      |
| Write current view      | `:wv`, `:writeview` |
| Print working directory | `:pwd`              |
| Quit application        | `:q`, `:quit`       |

## Configuration
coomer configuration is done suckless-style by editing `config.h`.
See [the default configuration](./config.def.h) and its comments to learn how
to configure coomer.

