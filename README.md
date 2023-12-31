# FM Player

### FM synthesis music player for DOS real mode.

A very, very old project. In 2019, I converted the player engine to the [Ad Plug 2.3.2](https://github.com/adplug/adplug) engine, just out of interest.

**Features**
- Pseudo-graphic interface. The player operates in text video mode, which makes it accessible for use on devices with limited graphics and computing capabilities.
- Playback speed control. This allows you to either speed up or slow down music playback, depending on your preference or needs.
- Playlist support. The player provides functionality for creating, editing and playing playlists.
- Supports the most popular FM synthesis music formats:
  - AMD (AMUSIC Adlib Tracker by Elyssis)
  - DMO (Twin TrackPlayer by TwinTeam)
  - HSC (HSC Adlib Composer by Hannes Seifert, HSC-Tracker by Electronic Rats)
  - SAT (Surprise! Adlib Tracker by Surprise! Productions)
  - SA2 (Surprise! Adlib Tracker 2 by Surprise! Productions)
  - S3M (Scream Tracker 3 by Future Crew)
  - LDS (LOUDNESS Music Format by Andras Molnar)
  - RAD (Reality ADlib Tracker by Reality)

**The AD Plug engine has been modified:**
- Adapted to 16-bit 8086 mode.
- Removed arithmetic with real numbers.
- Optimized functions for loading music from files.
- Functions for working with hardware have been rewritten into assembly language.
- Fixed a bug with volume control.
- New equalizer.
- New function for adjusting the speed of music playback.
- New function for calculating the duration of music playback.
- Other changes to optimize for speed and size.

**System requirements:**
- Cpu: 8086 XT, recommended 8086 XT Turbo.
- Video: EGA or later, recommended VGA.
- Memory: 510 Kb, recommended 640 Kb.

**Compiling and building:**

Borland C++ 3.1

make.bat - building for any x86 processor.

makext.bat - building a XT-optimized version.
