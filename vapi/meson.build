if spice_gtk_has_vala
  spice_glib_vapi = gnome.generate_vapi('spice-client-glib-2.0',
                      install : true,
                      packages : ['gio-2.0', 'gstreamer-1.0'],
                      sources : spice_client_glib_gir[0])

  if spice_gtk_has_gtk
    if spice_gtk_version == '4'
      spice_gtk_vapi_name = 'spice-client-gtk-4.0'
      spice_gtk_vapi_pkg = 'gtk4'
    else
      spice_gtk_vapi_name = 'spice-client-gtk-3.0'
      spice_gtk_vapi_pkg = 'gtk+-3.0'
    endif
    gnome.generate_vapi(spice_gtk_vapi_name,
                        install : true,
                        packages : [spice_gtk_vapi_pkg, 'gstreamer-1.0', spice_glib_vapi],
                        gir_dirs : meson.project_build_root() / 'src',
                        vapi_dirs : meson.current_build_dir(),
                        sources : spice_client_gtk_gir[0])
  endif
endif
