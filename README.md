spice-gtk
=========

A Gtk client and libraries for SPICE remote desktop servers.

Please report bugs at: spice-devel@lists.freedesktop.org

Project content
---------------

* **libspice-client-glib-2.0**

    Provides glib objects for spice protocol decoding and surface rendering.
    * SpiceSession (see spice-session.h).
    * SpiceChannel (see spice-channel.h).
    * SpiceAudio (see spice-audio.h).
    * Various Spice\<Type\>Channel (see channel-\<type\>.h).

* **libspice-client-gtk-3.0**

    Provides a GTK 3 widget to show spice display and accept user input.
    * SpiceDisplay (see spice-widget.h)

* **libspice-client-gtk-4.0** *(optional)*

    Provides a GTK 4 widget with the same API surface, built from
    native GTK 4 code in `src/gtk4/`.  Enable with `-Dgtk4=enabled`.
    Can be built alongside the GTK 3 library.

* **spicy**

   A GTK 3 test client. The recommended client for end user is
   [virt-viewer]

* **spicy-screenshot**

   Command line tool, connects to spice server and writes out a
   screen shot.

* **spicy-stats**

   Command line tool, connects to spice server and writes out a
   summary of connection details, amount of bytes transferred...

* **SpiceClientGlib** and **SpiceClientGtk** GObject-introspection modules.

[virt-viewer]: https://pagure.io/virt-viewer

Building
--------

The default build produces the GTK 3 client library and the `spicy`
test client:

```sh
meson setup build
ninja -C build
```

To also build the optional GTK 4 client library
(`libspice-client-gtk-4.0`) alongside the GTK 3 one:

```sh
meson setup build -Dgtk4=enabled
ninja -C build
```

To build only the GTK 4 library, without GTK 3:

```sh
meson setup build -Dgtk=disabled -Dgtk4=enabled
ninja -C build
```

Build dependencies
------------------

On Fedora, install the build dependencies of the packaged spice-gtk:

```sh
dnf builddep spice-gtk
```

or install them explicitly:

```sh
dnf install meson ninja-build gtk3-devel spice-protocol openssl-devel \
    pulseaudio-libs-devel pixman-devel json-glib-devel \
    gobject-introspection-devel libjpeg-turbo-devel zlib-devel \
    cyrus-sasl-devel gtk-doc gettext-devel vala vala-tools \
    python3 python3-pyparsing
```

For GTK 4 support, also install:

```sh
dnf install gtk4-devel
```

The GStreamer backend needs:

```sh
dnf install gstreamer1-devel gstreamer1-plugins-base-devel \
    gstreamer1-plugins-good gstreamer1-plugins-bad-free
```
