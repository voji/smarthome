**Openhab 2 project main parts:**
- heater control
- boiler control
- daylight based lightning
- wake up light
- socket control
- garden watering system
- user habit prediction
- user location determination (using [OwnTracks](http://owntracks.org/))
- statistic of values (can process with PowerBI or other tools)

**Used addons:**
- Astro (http://docs.openhab.org/addons/bindings/astro/readme.html)
- Mqtt (http://docs.openhab.org/addons/bindings/mqtt1/readme.html)
- WoL (http://docs.openhab.org/addons/bindings/wol1/readme.html)
- JDBC (http://docs.openhab.org/addons/persistence/jdbc/readme.html)
- rrd4j (http://docs.openhab.org/addons/persistence/rrd4j/readme.html)
- MiLight (http://docs.openhab.org/addons/bindings/milight1/readme.html)
- Exec legacy addon (http://docs.openhab.org/addons/bindings/exec1/readme.html)
- Mail (http://docs.openhab.org/addons/actions/mail/readme.html)

**Log configuration for stat.rules:**
To add new loggers to oepnhab logging configuration, you need to modify the following file:
*/var/lib/openhab2/etc/org.ops4j.pax.logging.cfg*

    # File appender - stat.log
    log4j.appender.stat=org.apache.log4j.RollingFileAppender
    log4j.appender.stat.layout=org.apache.log4j.PatternLayout
    log4j.appender.stat.layout.ConversionPattern=%m%n
    log4j.appender.stat.file=${openhab.logdir}/stat.log
    log4j.appender.stat.append=true
    log4j.appender.stat.maxFileSize=32MB
    #log4j.appender.stat.maxBackupIndex=10

