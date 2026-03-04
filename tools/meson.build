spice_cmdline_sources = [
  'spice-cmdline.c',
  'spice-cmdline.h',
]

#
# spicy-stats and spicy-screenshot
#
foreach exe : ['spicy-stats', 'spicy-screenshot']
  executable(exe,
             sources : spice_cmdline_sources + ['@0@.c'.format(exe)],
             c_args : '-Wno-deprecated-declarations',
             install : true,
             dependencies : spice_client_glib_dep)
endforeach

#
# spicy
# Note: spicy is a developer test tool that uses many deprecated GTK3 APIs.
# It is not built for GTK4 targets.
#
if spice_gtk_has_gtk and spice_gtk_version == '3'
  spicy_sources = [
    'spicy.c',
    'spicy-connect.c',
    'spicy-connect.h',
  ]

  executable('spicy',
             sources : spicy_sources + spice_cmdline_sources,
             c_args : '-Wno-deprecated-declarations',
             install : true,
             dependencies : spice_client_gtk_dep)
endif

#
# spicy-test-gtk4 — minimal test client for the GTK4 port
#
if spice_gtk_has_gtk and spice_gtk_version == '4'
  executable('spicy-test-gtk4',
             sources : ['spicy-test-gtk4.c'] + spice_cmdline_sources,
             install : false,
             dependencies : spice_client_gtk_dep)
endif
