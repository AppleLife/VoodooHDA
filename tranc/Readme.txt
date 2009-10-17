voodoohda 0.2.2 release notes

note: this driver has been tested on only a few systems, so bugs and glitches should be expected.
      there is currently no official binary release, so while you are free to compile this source or
      use any derivative releases thereof, this driver should be considered experimental.

development
-----------

  * sources are hosted at the voodoohda project site at google code: http://voodoohda.googlecode.com/
  * code guidelines: four-space tab (not spaces), ~110 line width, k&r style
  * the provided "helper.sh" utility can be used to clean or build the project, create a compiled
    release package, or load and unload the driver for testing
  * the latest cvs revision of the freebsd hdac driver can be found here:
    http://www.freebsd.org/cgi/cvsweb.cgi/src/sys/dev/sound/pci/hda/
    (the revision corresponding to voodoohda sources is identified by HDAC_REVISION)
  * one of the main focuses in porting from hdac was to keep the structure and functionality of the
    code (especially the widget parser) relatively close to that of the original code, in order to
    ease integrating changes from upstream; that said, the code has been heavily reworked in some
    places due to porting necessity as well as clarification

hints
-----

  * use the provided 'getdump' utility to get a codec dump for debugging or to see how each logical
    pcm device is configured, see pcmAttach notices (same notion as pcmN with freebsd hdac)
  * one pcm device at a time can be active (unless aggregate devices are used), each is designated
    by "Analog/Digital PCM #N" in sound preference pane - separate selectors for "Master", "PCM",
    etc. are not different devices but correspond to standard oss controls
  * to change sample rate or bit depth or setup aggregate devices use Audio Midi Setup.app

known issues
------------

  * bad things (tm) will happen if some other hda driver is loaded or present in the catalogue
    when voodoohda is loaded
  * extremely cluttered and not particularly user-friendly audio controls in sound preference pane,
    only way around this is to dumb it down or implement a hal plugin (as applehda does)
  * distinction between oss controls and actual "ports" corresponding to pcm device play/rec channels
    is unclear - need to find out how to get audio port name (in "type" column) displayed
  * need to kextunload two or three times to actually unload the driver, seems to be an audio family
    bug as this happens with sample audio drivers too
  * manual specification of quirks and hints is currently unsupported

license
-------

see license.txt for details and copyright notices

changelog
---------

0.2.2 (4/14/09):

  * mute controls implemented
  * seperate left/right audio level controls implemented
  * showing just one device for each input/output in Sound preference pane

0.2.1 (4/10/09):

  * minor source clean-up in preparation for release
  * synchronized sources with hdac cvs revision 20090401_0132 (numerous codec-specific fixes, more
    controller/codec ids, improved widget parsing, cosmetic fixes)

0.2 (12/28/08):

  * new message logging system, can be adjusted with VoodooHDAVerboseLevel setting in Info.plist;
    overview follows, each level is inclusive of previous one
      0: quiet (except codec/controller id info at init, all errors)
      1: verbose init messages, audio engine/control debugging
      2: codec dump
      3: interrupt stats
      4: lock debugging
  * support for multiple sample rates and bit depths (16, 24, and 32-bit)
  * new audio controls in sound preference pane, separate selector for each logical oss control;
    each logical pcm device correponds to pcmN devices as with freebsd hdac (check codec dump for
    information on configurations) - this is temporary until a hal plugin is implemented which
    will allow better organization of controls
  * synchronized sources with latest hdac cvs revision 20081226_0122 (new controller and codec ids,
    also some special handling in parser for ad1986a)
  * implemented user client and included tool (getdump) for obtaining codec dump
  * many internal fixes and improvements as well as massive source cleanup
