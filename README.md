
Yet Another Dialog -- GTK+ 2 Maintenance Branch
====================

Yad allows you to display GTK+ dialog boxes from the command line or
shell scripts. YAD depends on GTK+ only. The minimum GTK+ version is 2.16.0.

This software is licensed under the GPL v.3

This branch (maintain-gtk2) was forked from the upstream project at release
0.42 (d0021d0) to continue GTK+ 2 support for the Fatdog64 Linux distribution.
It should serve other minimal distributions that rely on GTK+ 2, and package yad.

In the spirit of a maintainance project, the focus is on fixing bugs.
New features are considered but rarely added.

Building the git version
----------------------

While this branch can be built against either of version 2 and version 3 of the
GTK+ library, it is only maintained and tested for GTK+ 2.

Get the latest source code with command:

```sh
git clone https://github.com/step-/yad.git yad_gtk2
```

Generate build scripts, configure and build the project:

```sh
cd yad &&
git checkout maintain-gtk2 &&
autoreconf -ivf &&
intltoolize &&
./configure &&
make &&
: install with: make install
```

To build successfully you may need to install the following packages:
* GNU Autotools (https://www.gnu.org/software/autoconf/ http://www.gnu.org/software/automake/)
* Intltool >= 0.40.0 (http://freedesktop.org/wiki/Software/intltool/)
* GTK+ >= 2.16.0 (http://www.gtk.org)
with appropriate *-dev* packages depending on your distro.

When you run `configure` you can pass some options to build yad with the the following libraries:
* Webkit - for supporting the HTML dialog option (http://webkitgtk.org)
* GtkSourceView - for syntax highlighting in text text-info dialog (https://wiki.gnome.org/Projects/GtkSourceView)
* GtkSpell3 - for spell checkinging text fields (http://gtkspell.sourceforge.net/)

This Project
--------------

* Project homepage: https://github.com/step-/yad

Upstream Project
--------------

Look here for GTK+ 3 support and new features.

* Project homepage: https://sourceforge.net/projects/yad-dialog/
* Example usage: https://sourceforge.net/p/yad-dialog/wiki/browse_pages/
* Mailing list: http://groups.google.com/group/yad-common

Distributions
-----------

Distributions known to package this branch:

* Fatdog64 Linux: https://distro.ibiblio.org/fatdog/web/

