Name:       elm-misc
Summary:    Elementary config files
Version:    0.1.179
Release:    1
Group:      TO_BE/FILLED_IN
License:    APLv2
BuildArch:  noarch
Source0:    %{name}-%{version}.tar.gz
BuildRequires: sec-product-features
BuildRequires: eet-bin

%description
Elementary configuration files

%prep
%setup -q

%build
%if "%{sec_product_feature_uifw_efl_theme}" == "hive"
   export ELM_PROFILE=mobile
   export TARGET=2.3-hive
   export SIZE=WVGA
%else
   %if 0%{?sec_product_feature_uifw_efl_b3_theme}
      export ELM_PROFILE=wearable
      export ELM_SCALE=1
      export TARGET=2.3-wearable
      export SIZE=b3
   %else
      %if 0%{?sec_product_feature_uifw_efl_c1_theme}
         export ELM_PROFILE=wearable
         export TARGET=2.3-wearable-circle
         export SIZE=c1
      %else
         export ELM_PROFILE=mobile
         export TARGET=2.3-mobile
         %if 0%{?sec_product_feature_profile_lite}
            %if "%{?sec_product_feature_display_resolution}" == "480x800"
               export SIZE=WVGA
            %else
               %if "%{?sec_product_feature_display_resolution}" == "720x1280"
                  export SIZE=HD
               %else
                  %if "%{?sec_product_feature_display_resolution}" == "FHD"
                     export SIZE=FHD
                  %else
                     %if "%{?sec_product_feature_display_resolution}" == "800x1280"
                        export SIZE=WVGA
                     %else
                        export SIZE=HVGA
                     %endif
                  %endif
               %endif
            %endif
         %else
           export SIZE=WVGA
         %endif
      %endif
   %endif
%endif

%if 0%{?tizen_build_binary_release_type_daily}
    %if 0%{?sec_product_feature_uifw_efl_abort_enable}
        export EFL_ABORT_ENABLE=off
    %else
        export EFL_ABORT_ENABLE=off
    %endif
%else
    export EFL_ABORT_ENABLE=off
%endif

make

%install
%if "%{sec_product_feature_uifw_efl_theme}" == "hive"
   export ELM_PROFILE=mobile
   export TARGET=2.3-hive
   export SIZE=WVGA
%else
   %if 0%{?sec_product_feature_uifw_efl_b3_theme}
      export ELM_PROFILE=wearable
      export TARGET=2.3-wearable
      export SIZE=b3
   %else
      %if 0%{?sec_product_feature_uifw_efl_c1_theme}
         export ELM_PROFILE=wearable
         export TARGET=2.3-wearable-circle
         export SIZE=c1
      %else
         export ELM_PROFILE=mobile
         export TARGET=2.3-mobile
         %if 0%{?sec_product_feature_profile_lite}
            %if "%{?sec_product_feature_display_resolution}" == "480x800"
               export SIZE=WVGA
            %else
               %if "%{?sec_product_feature_display_resolution}" == "720x1280"
                  export SIZE=HD
               %else
                  %if "%{?sec_product_feature_display_resolution}" == "FHD"
                     export SIZE=FHD
                  %else
                     %if "%{?sec_product_feature_display_resolution}" == "800x1280"
                        export SIZE=WVGA
                     %else
                        export SIZE=HVGA
                     %endif
                  %endif
               %endif
            %endif
         %else
            export SIZE=WVGA
         %endif
      %endif
   %endif
%endif

make install prefix=%{_prefix} DESTDIR=%{buildroot}

mkdir -p %{buildroot}%{_datadir}/license
cp %{_builddir}/%{buildsubdir}/COPYING %{buildroot}/%{_datadir}/license/%{name}

%post
chown root:root /etc/profile.d/ecore.sh
chown root:root /etc/profile.d/edje.sh
chown root:root /etc/profile.d/eina.sh
chown root:root /etc/profile.d/elm.sh
chown root:root /etc/profile.d/evas.sh

%files
%defattr(-,root,root,-)
%{_sysconfdir}/profile.d/*
%{_datadir}/themes/*
%{_datadir}/datetimeformats/*
%{_datadir}/elementary/config/*
%{_datadir}/license/%{name}
%manifest %{name}.manifest
