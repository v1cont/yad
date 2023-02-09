Yet Another Dialog -- GTK+-2 Maintenance Branch
====================

Use yad (yet another dialog) in shell scripts to displays GTK+ dialogs involving messages, lists, forms and several other commonly-used interface elements.

Summary
-------

[This](https://github.com/step-/yad) project is an independent fork of the [parent project](https://github.com/v1cont/yad).

This project is licensed under the GNU GPL3 license, see _License and copyright_ section in [this page](feature-comparison.md).

Project goals:

* Overwhelmingly, maintain compatibility with GTK+-2.
* Build against either GTK+-2 or GTK+-3. The two builds provide the same features.
* Backport features from the parent project.
* Introduce original features.

100% compatibility with the parent project is not a goal but keeping reasonably compatible still informs my decisions.
[Feature comparison](feature-comparison.md)

Several Linux [distributions](distributions.md) that still depend on GTK+-2 include a binary derived from this fork.

History
-------

The parent project removed GTK+-2 support in version 1.0.
This repository was forked from the 0.42.0 release (d0021d0 February 2019) with the goal to continue GTK+-2 support, mainly for the benefit of the [Fatdog64](http://distro.ibiblio.org/fatdog/web/) Linux distribution.
With time this fork has reached several other Linux [distributions](distributions.md) that need a GTK+-2 yad package.

Scope
-----

In the spirit of a maintainance project, fixing bugs takes precedence.
New features can be added as my time permits but only if they are tested with, and work equally well for, GTK+-2 and GTK+-3.
New features are introduced as either backports from the parent project or, more rarely, as original features. You can read all about this in the [feature comparison](feature-comparison.md) page.

Contributions and pull requests (PR) are always welcome!

Naming Hell
-----------

This repository is named `yad`, same as its parent project's.  Both projects build a binary file named `yad`.
[Fatdog64](http://distro.ibiblio.org/fatdog/web/) renames the binary `yad_gtk2` or `yad_gtk3`, according to the build, and only ships `yad_gtk2`, with a symbolic link in /usr/bin from name `yad` to target `yad_gtk2`. The Fatdog64 package repository provides packages `yad_gtk2`, `yad_gtk3`, `yad_doc` and `yad_ultimate`, which is Fatdog64's package name for the parent project (and the parent project's binary is named `yadu`).

For short I will use **yadL** for the products of this fork, and **yadU** for the parent project.

Building yadL from Git
----------------------

This fork's default branch is named `maintain-gtk2`. All development takes place in the default branch.

Get the latest source code with command:

```sh
git clone https://github.com/step-/yad.git maintain-gtk2
```

Generate build scripts, configure and build the project:

```sh
cd maintain-gtk2 &&
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
* GTK+-2 >= 2.24.0 (http://www.gtk.org)
* GTK+-3 >= 3.22.0 (http://www.gtk.org)

with appropriate *-dev* packages depending on your distro.

When you run `configure` you can pass some options to build yad with the following libraries:

* GtkSourceView - for syntax highlighting in the text-info dialog (https://wiki.gnome.org/Projects/GtkSourceView)
* GtkSpell3 - for spell checkinging text fields (http://gtkspell.sourceforge.net/)
* Webkit - for the HTML dialog widget (http://webkitgtk.org)

Distributions
-------------

Distributions known to package this fork: read [distributions](distributions).

Links
-----

* [yadL development](https://github.com/step-/yad)
* [yadL wiki](https://github.com/step-/yad/wiki)
* [yad thread on Puppy Linux old forum](https://forum.puppylinux.com/viewtopic.php?t=216)
* [yad thread on Puppy Linux forum](https://forum.puppylinux.com/viewtopic.php?t=3922)
* [yadU development](https://github.com/v1cont/yad)
* [yadU complex examples](https://github.com/v1cont/yad/wiki/YAD-Examples)
* [yadU mailing list](http://groups.google.com/group/yad-common)

