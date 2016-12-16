Microcomputer Emulator
======================

16 December 2016

The [Acorn Atom](https://en.wikipedia.org/wiki/Acorn_Atom) was an
microcomputer produced by Acorn Computers in the early 1980s.  It was
my first computer and, as was common in the day, I got to know it in a
lot of detail.  Since then, I've owned many computers, but have become
more and more distanced from the details of the machine.

Around 2000, I thought that writing an emulation of my Atom could be
an interesting little project.  I started an
[Object-based](https://en.wikipedia.org/wiki/Object-based_language) C
implementation using [ncurses](https://en.wikipedia.org/wiki/Ncurses)
for the display.  Since then the program has evolved and been
rewritten *many* times.

It now makes use of the following:
* [git](https://git-scm.com/) for version control ☺;
* [UMLet](http://www.umlet.com/) for the UML design diagrams;
* [SCons](http://scons.org/) for the build system;
* C++ and the Standard Template Libraries;
* [Log4cxx](https://logging.apache.org/log4cxx/latest_stable/) for the
  extensive logging facilities;
* [Gnome Glibmm libraries](https://developer.gnome.org/glibmm/stable/)
  mostly for the UTF-8 string type;
* [SDL v2](https://www.libsdl.org/) for the event loop, graphics
  output and keypress inputs;
* [libxml++](http://libxmlplusplus.sourceforge.net/) to parse the XML
  configuration files;
* [RelaxNG](http://relaxng.org/) to define (compact) schemas for the
  XML configuration files;
* [Trang](http://www.thaiopensource.com/relaxng/trang.html) to convert
  the compact schema to the XML schema files;
  * I tried using
    [Jing](http://www.thaiopensource.com/relaxng/jing.html) to
    validate the schemas, but it sometimes passed and sometimes failed.
* [Robot Framework](http://robotframework.org/) to provide an
  acceptance test framework;
* [Python](https://www.python.org/) for ad hoc utilities;
* Various ROM files that I've scoured the Web for.

Indeed, in many ways the project has evolved in to tool that I've used
to explore various libraries since the emulator itself was largely
complete many years ago.

It is still a _Work-In-Progress_ but I thought I'd publish and be
damned.

Steve Palmer
