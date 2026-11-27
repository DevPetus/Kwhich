# Kwhich

Lightweight KDE Plasma 6 (Wayland) utility: press a shortcut, click any window,
and find out where its program actually came from.

Kwhich tells you whether the clicked window's process is a Flatpak, a Snap, an
AppImage, a native distro package, or a manual/by-hand install, and shows:

- the executable path and full command line (with Wine/Proton `C:\...` paths
  and Flatpak sandbox `/app/...` paths translated back to real host paths)
- the process ancestry (useful for spotting a launcher/wrapper chain, e.g. a
  Steam game run through its runtime script)
- the binary's install date
- a note if both a native package and a Flatpak of the same app are installed
- one-click actions: open the file manager to the executable (or the
  resolved command-line target), copy its path, open System Monitor, or kill
  the process (escalating to `SIGKILL` if it ignores `SIGTERM`)

## Requirements

KDE Plasma 6 on Wayland. Detection relies on KWin's D-Bus `queryWindowInfo`
call and Linux's `/proc`, so this is Linux/KDE-only by design.

## Installing

### Fedora (COPR)

```
sudo dnf copr enable devpetus/kwhich
sudo dnf install kwhich
```

### Arch / CachyOS (AUR)

```
yay -S kwhich-git
```

(or manually: clone `packaging/aur/PKGBUILD` from this repo and run `makepkg -si`)

### Build from source

Dependencies: `cmake`, a C++17 compiler, Qt6 Widgets development headers, and
D-Bus development headers (`qt6-qtbase-devel` and `dbus-devel`/`pkgconfig(dbus-1)`
on Fedora; `qt6-base` and `dbus` on Arch).

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

## Setting up the shortcut

Kwhich launches fresh each time you trigger it — it doesn't run as a
background daemon. Bind it to a global shortcut the same way you would any
other installed application:

1. Open **System Settings → Shortcuts**.
2. Use the **Add Application** (or similarly labeled "+") option and search
   for "Kwhich" — the `.desktop` file installed above is what makes it
   discoverable here, the same way other installed applications (e.g.
   `kitty`) show up.
3. Assign a key combination, e.g. `Meta+Shift+W`.

Press that shortcut, click a window, and the inspector panel appears.

## Optional runtime dependencies

These enable specific buttons/checks but aren't required for Kwhich itself to
work — each degrades gracefully if missing:

- a file manager implementing the `org.freedesktop.FileManager1` D-Bus
  interface (Dolphin, Nautilus, Nemo, ...) — for the Open Folder button
- `plasma-systemmonitor` — for the System Monitor button
- `flatpak` — for the dual-install (native + Flatpak) check

## License

MIT, see [LICENSE](LICENSE).
