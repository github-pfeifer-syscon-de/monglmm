# monglmm

A fancy system-monitor for Linux using OpenGL.


![Monglmm](monglmm.png "monglmm")

- graphs for cpu,memory,network,disk,gpu,clock and sensors
- display of top three processes using cpu and memory
- display of pocess tree indicating high cpu usage, details are visible on selection, on double click classic gui views
- overview of disk alloation
- display of network connections with protocol and domain-names
- viewing of log entries (as configured with genericImg)
- support for g15-keyboard lcd and keys

Build with autotools, requires genericImg, genericGlm (see there for some basic build infos).

<pre>
configure:<br>
     allow --with-gles using GL ES 3 e.g. useful on Raspi's (requires same use on GenericGlm)
     allow --with-libg15 using G15 lcd display see libg15
     allow --with-lmsensors using lmsensors for display
    (obsolet  --with-raspi build with raspi core voltage (which is not moving) info (function is (c) broadcom) beware: breaks --with-gles)
</pre>

## Log view

- this was build with a desktop-system in mind, if your system has GBytes of logs it will probably not display all infos.
    If you still feel the limit of displayed lines dosn't fit your needs
    change it in mongl.conf section LogProperties key logViewRowLimit
    if you don't mind the used resources
- with systemd-logging there is a overhead to assign bootId'sto times,
   this may result in some waiting period for the first invocation
   (depending on the used storage), but over time that should improve.
