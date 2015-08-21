FireStep: change log
====================

v0.2.3
------
* CHG: MTO_FPD "mov:{a,d}" changed to "mov:{angle,d}" move in XY plane to polar coordinate `d` millimeters from X0Y0 at `angle` degrees CCW from x-axis.
* CHG: prb pn attribute deprecated in favor of "pb" to match "syspb"
* CHG: "sysas" configuration autoSync is no longer saved and must be enabled explicitly
* FIX: MTO_FPD single axis home (e.g., "hom1") will not reposition to origin
* FIX: "movrx" is now identified as a bad command (the correct command is "movxr")
* NEW: latch backoff is now an axis attribute (e.g. "xlb"), but you can still use "syslb" to set all axes at once.
* NEW: "calsv" provides adaptive calibration via exponential smoothing
* NEW: "calgr1" calibrate gear ratio for arm 1 at given angle in degrees
* NEW: "calgr2" calibrate gear ratio for arm 2 at given angle in degrees
* NEW: "calgr3" calibrate gear ratio for arm 3 at given angle in degrees
* FIX: static strings now stored in PROGMEM to free up SRAM
* FIX: "sys" no longer overwrites custom pin configuration on query
* NEW: "syspb" sets custom probe pin
* NEW: "dimhp" user provided home pulses 
* NEW: {"pgmx":"dim-fpd"} Set default FirePick Delta dimensions
* NEW: {"pgmx":"cal-home-coarse"} Use hex probe to calibrate home angle and Z-bed plane (adaptive coarse)
* NEW: {"pgmx":"cal-home-medium"} Use hex probe to calibrate home angle and Z-bed plane (adaptive medium)
* NEW: {"pgmx":"cal-home-fine"}   Use hex probe to calibrate home angle and Z-bed plane (adaptive fine)
* NEW: {"pgmx":"cal-bed-coarse"}  Use hex probe to calibrate Z-bed plane (adaptive coarse)
* NEW: {"pgmx":"cal-bed-medium"}  Use hex probe to calibrate Z-bed plane (adaptive medium)
* NEW: {"pgmx":"cal-bed-fine"}    Use hex probe to calibrate Z-bed plane (adaptive fine)

v0.2.2
------
* FIX: Jason's fix for mismatched delta configuration attributes
* FIX: querying axis will no longer disable it
* FIX: Saving MTO_FPD dimensions now saves proper values
* FIX: eTheta implementation replaced with single, common home angle
* FIX: reordered saved configuration for proper precedence on restore
* FIX: home angle digitized properly so that home angle and home pulses are in sync
* FIX: DeltaCalculator home now initialized properly from axis home
* CHG: MTO_FPD "movrx" renamed to "movxr" for consistent naming (i.e. X-Relative). Ditty with y and z.
* NEW: STATUS_WAIT_IDLE NeoPixel display shows; MTO_FPD/green, and MTO_RAW/white
* NEW: "prbd" returns last 9 probe data values, with most recent first
* NEW: "mrk" gets/sets value of one of four mark registers
* NEW: MTO_RAW "mrka1", "mrka2", "mrka3, "mrka4"" mark axis stepper position	
* NEW: MTO_FPD "mrkax", "mrakay", "mrkaz" mark Cartesian axis position 
* NEW: MTO_FPD "mrkwp" mark Cartesian 3-axis waypoint 
* NEW: MTO_FPD "movxm", "movym", "movzm" move to marked Cartesian axis position 
* NEW: MTO_FPD "movwp" move to marked Cartesian 3-axis waypoint 

v0.2.1
------
* NEW: User EEPROM JSON commands at EEPROM address 2000 will execute after system startup JSON
* NEW: "syseu" required to enable user EEPROM initially and after configuration change
* NEW: MTO_FPD "hom" now moves to Z0 after hitting the limit switch. Motion is over-constrained at limit switch, making it rather useless as a starting position. However, at Z0, motion constraints are much more relaxed, which makes X0Y0Z0 a great "at rest" position. If you really want to park the effector "up there", just "mov" to X0Y0Z0, then "movz" up to the limit switch position.
* NEW: MTO_FPD "prbpd" stores up to 9 Z-probe data archived by "prb"
* NEW: MTO_FPD "movrx" moves X-relative (vs. absolute)
* NEW: MTO_FPD "movry" moves Y-relative (vs. absolute)
* NEW: MTO_FPD "movrz" moves Z-relative (vs. absolute)
* NEW: MTO_FPD "mov:{a,d}" move in XY plane to polar coordinate `d` millimeters from X0Y0 at `a` degrees CCW from x-axis.
* NEW: "sysas" configuration auto-sync to EEPROM
* NEW: "sysch" configuration hash (changes when configuration changes)
* NEW: "sysah" auto-home on startup
* NEW: "idl" idle for given milliseconds
* NEW: "cmt" comment (ignored)
* NEW: "msg" write message line to serial
* NEW: "sysom:1" output mode option shows individual responses from JSON command arrays
* NEW: "sysom:2" writes out comments
* NEW: "sysom" bit values can be OR'ed together (e.g., sysom:3)
* NEW: status display now indicates EEPROM reading/writing (NeoPixel blue)

v0.2.0
------
* NEW: "mov" scaled by 2 for memory savings
* NEW: "eep!" executes value as JSON command before saving it to EEPROM (great way to save config)
* NEW: DeltaCalculator support
* NEW: "systo" changes machine topology (0:MTO_RAW, 1:MTO_FPD)
* NEW: MTO_FPD "mov" interprets x,y,z attributes as cartesian coordinates
* NEW: MTO_FPD "mov" interprets x,y,z attributes as cartesian coordinates
* NEW: MTO_FPD "mpo" interprets x,y,z attributes as cartesian coordinates
* FIX: NeoPixel displays uses millis() instead of ticks and no longer freezes with extended use

v0.1.15
-------
* NEW: "sysps" display status pin (e.g., NeoPixel) is soft configurable
* NEW: "sysmv" maximum stepper velocity (default: 12800 pulses/second) 
* NEW: "systv" time to  maximum stepper velocity (default: 0.7 microseconds) 
* NEW: "prbsd" probe search delay (default: syssd)
* NEW: "prbip" probe invert logic (default: false)
* FIX: [command array] now processes homing commands correctly
* FIX: "prb" returns STATUS_PROBE_FAILED on non-contact

v0.1.14
-------
* NEW: "prb" command probes to specified coordinate, testing given pin
* NEW: "sd" search delay consolidated from individual axes into common "syssd"	
* CHANGE: "syssd" defaults to 800us for each homing/probing calibration pulse
* CHANGE: "syshp" defaults to 3 for coarse homing speed

v0.1.13
------
* NEW: homing pulses synchronized for smoothness
* NEW: "syshp" configures homing pulses, which controls homing speed
* NEW: homing pulses reduced from 32 to 8
* NEW: latchBackoff is now universal instead of per axis (i.e., "syslb")
* NEW: "hom" now double checks at a slower speed for higher accuracy
* NEW: latchBackoff default is now 200 vs 32 pulses
* FIX: renamed Libraries to libraries as per Arduino standards

v0.1.12
------
* FIX: homing will now ignore axes with no minimum limit switch

v0.1.11
------
* MERGE: Neil's changes for OpenPnP support and full EMC02 setup

v0.1.10
------
* FIX: direction pin cache did not track configuration direction inversion

v0.1.9
------
* FIX: stepper enabled flag now matches actual enable pin value on startup.
* FIX: steppers disabled/enabled during pin configuration change

v0.1.8
------
* EEPROM read/write
* EEPROM startup command

v0.1.7
------
* io for digital/analog pins
* remove idle whine

v0.1.6
------
* fix for individual axis homing bug 

v0.1.5
------
* JSON command array
* dvs scale now supported

v0.1.4
------
* EMC02 pins corrected

v0.1.3
------
* Travel minimum default is now -32000 since FirePick Delta has negative homing coordinate

v0.1
------
* Initial release

