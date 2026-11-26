%global commit0 851003db1a19c1a84139a99e392bc2de3a52b115
%global shortcommit0 %(c=%{commit0}; echo ${c:0:7})

Name:           kwhich
Version:        0
Release:        0.1.git%{shortcommit0}%{?dist}
Summary:        Click a window to see where its program came from

License:        MIT
URL:            https://github.com/DevPetus/Kwhich
Source0:        https://github.com/DevPetus/Kwhich/archive/%{commit0}/Kwhich-%{commit0}.tar.gz

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
%setup -n Kwhich-%{commit0}

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

%post
%desktop_database_post

%postun
%desktop_database_postun

%changelog
* Thu Nov 26 2026 DevPetus <devebourgeo@gmail.com> - 0-0.1.git851003d
- Initial packaging
