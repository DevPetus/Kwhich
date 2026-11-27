Name:           kwhich
Version:        0
Release:        %autorelease
Summary:        Click a window to see where its program came from

License:        MIT
URL:            https://github.com/DevPetus/Kwhich
Source0:        https://github.com/DevPetus/Kwhich/archive/refs/heads/main.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  qt6-qtbase-devel
BuildRequires:  desktop-file-utils

Recommends:     dolphin
Recommends:     plasma-systemmonitor
Recommends:     flatpak

%description
Kwhich is a small KDE Plasma 6 (Wayland) utility. Trigger it with a
keyboard shortcut, click any window, and it shows you where that
program came from: the executable path and full command line, and
whether it's a Flatpak, AppImage, Snap, a native distro package, or a
manual install -- plus the process ancestry, install date, and a note
if both a native and a Flatpak version are installed.

%prep
%autosetup -n Kwhich-main

%build
%cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo
%cmake_build

%install
%cmake_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/kwhich.desktop

%files
%license LICENSE
%doc README.md
%{_bindir}/kwhich-cli
%{_datadir}/applications/kwhich.desktop

%changelog
%autochangelog
