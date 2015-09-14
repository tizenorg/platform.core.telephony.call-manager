%define major 0
%define minor 1
%define patchlevel 38
%define ext_feature 0

Name:           call-manager
Version:        %{major}.%{minor}.%{patchlevel}
Release:        1
License:        Apache-2.0
Summary:        Call Manager
Group:          System/Libraries
Source0:        call-manager-%{version}.tar.gz

BuildRequires: cmake
BuildRequires: python-xml
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(gio-unix-2.0)
BuildRequires: pkgconfig(glib-2.0)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(aul)
BuildRequires: pkgconfig(tapi)
BuildRequires: pkgconfig(capi-media-sound-manager)
BuildRequires: pkgconfig(capi-media-player)
BuildRequires: pkgconfig(capi-media-tone-player)
BuildRequires: pkgconfig(capi-appfw-app-manager)
BuildRequires: pkgconfig(appsvc)
BuildRequires: pkgconfig(contacts-service2)
BuildRequires: pkgconfig(capi-network-bluetooth)
BuildRequires: pkgconfig(capi-media-metadata-extractor)
BuildRequires: pkgconfig(capi-media-recorder)
BuildRequires: pkgconfig(capi-content-media-content)
BuildRequires: pkgconfig(motion)
BuildRequires: pkgconfig(capi-system-device)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(badge)
BuildRequires: pkgconfig(mm-sound)

BuildRequires: edje-tools
BuildRequires: gettext-tools
BuildRequires: pkgconfig(capi-ui-efl-util)
BuildRequires: pkgconfig(appcore-efl)
#BuildRequires: pkgconfig(ecore-x)
BuildRequires: pkgconfig(ecore-input)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(efl-extension)
BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(storage)
BuildRequires: pkgconfig(cynara-client)
BuildRequires: pkgconfig(cynara-creds-gdbus)
BuildRequires: pkgconfig(cynara-session)
BuildRequires: pkgconfig(msg-service)

Requires(post): /sbin/ldconfig
Requires(post): sys-assert
Requires(postun): /sbin/ldconfig

%if "%{profile}" != "mobile"
ExcludeArch: %{arm} %ix86 x86_64
%endif

%description
Call Manager Daemon

%package -n org.tizen.callmgr-popup
Summary:    Display popup about call
Group:      Development/Libraries

%description -n org.tizen.callmgr-popup
Display Voice mail notification popup

%prep
%setup -q

%build
export LDFLAGS+=" -Wl,-z,nodelete "

%if 0%{?enable_slient_log}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif

cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DVERSION=%{version} \
-DLIB_INSTALL_DIR=%{_libdir} \
-DUNIT_INSTALL_DIR=%{_unitdir} \
%if 0%{?ext_feature}
-D_ENABLE_EXT_FEATURE:BOOL=ON \
%else
-D_ENABLE_EXT_FEATURE:BOOL=OFF \
%endif
%if 0%{?enable_slient_log}
-DTIZEN_ENGINEER_MODE=1 \
%endif

#make %{?_smp_mflags} VERBOSE=1
make %{?_smp_mflags}

%post

%postun -p /sbin/ldconfig


%install
%make_install
mkdir -p %{buildroot}%{_unitdir}/multi-user.target.wants
ln -s ../callmgr.service %{buildroot}%{_unitdir}/multi-user.target.wants/callmgr.service

mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/call-manager

%if 0%{?enable_slient_log}
mkdir -p %{buildroot}/opt/usr/data/call/
ln -s %{_unitdir}/call-logger.service %{buildroot}%{_unitdir}/multi-user.target.wants/call-logger.service
%endif

%files
%manifest callmgr.manifest
%defattr(644,system,system,-)
%attr(755,system,system) %{_bindir}/callmgrd
#/etc/*
%{_unitdir}/*
%{_datadir}/license/call-manager
%if 0%{?enable_slient_log}
%attr(755,system,system) /opt/usr/devel/usr/bin/call_silent_logger
%attr(755,system,system) /opt/etc/dump.d/module.d/call_silent_log_dump.sh
/opt/usr/data/call
%endif
/usr/share/call-manager/*

%files -n org.tizen.callmgr-popup
%manifest callmgr-popup/org.tizen.callmgr-popup.manifest
/usr/apps/org.tizen.callmgr-popup/bin/callmgr-popup
/usr/share/packages/org.tizen.callmgr-popup.xml
/usr/apps/org.tizen.callmgr-popup/res/edje/popup_custom.edj
/usr/apps/org.tizen.callmgr-popup/res/locale/*/*/callmgr-popup.mo
