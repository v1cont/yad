
Yet Another Dialog -- GTK+ 2 Maintenance Branch
====================

Yad allows you to display GTK+ dialog boxes from the command line or
shell scripts. YAD depends on GTK+ only. The minimum GTK+ version is 2.16.0.

This software is licensed under the GPL v.3

This branch (maintain-gtk2) is maintained independently from the upstream
[project](https://github.com/v1cont/yad), and can be considered a fork at release
0.42 (d0021d0) aimed at continuing GTK+ 2 support mainly for the
[Fatdog64](http://distro.ibiblio.org/fatdog/web/) Linux distribution. It can
benefit other light distributions that need a GTK+ 2 build of yad.

In the spirit of maintainance project, backporting upstream bug fixes takes
precedence.  New upstream features can be added as my time permits, and if they
are compatible with GTK+ 2.  Read [feature comparison](feature-comparison.md).

Note about version numbers.  This fork is versioned with the major.minor number
frozen at the split from upstream, `0.42`, followed by a counter of the
"release-quality" commits in this fork.

Building the git version
----------------------

While this branch can be built against GTK+ 2 and GTK+ 3, it is only tested on GTK+ 2.

Get the latest source code with command:

```sh
git clone https://github.com/step-/yad.git yad_gtk2
```

Generate build scripts, configure and build the project:

```sh
cd yad_gtk2 &&
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
* EasyOS, from version 3.0 onwards: https://easyos.org
* Woof, the Puppy Linux builder: https://github.com/puppylinux-woof-CE/woof-CE

