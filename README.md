# Installation

```bash
    mkdir build
    cmake -B build .
    cmake --build build
    sudo cmake --install build
```

# Dev

```bash
    G_MESSAGES_DEBUG=Plugin_YT rofi -show yt -plugin-path ./build/lib -theme ./theme/yt.rasi
```
