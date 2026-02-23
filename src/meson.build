
sources = files(
    'CpuMonitor.cpp'
   ,'DiskMonitor.cpp'
   ,'Filesyses.cpp'
   ,'GpuMonitor.cpp'
   ,'GraphShaderContext.cpp'
   ,'HistMonitor.cpp'
   ,'MonglApp.cpp'
   ,'MonglAppWindow.cpp'
   ,'MonglView.cpp'
   ,'Monitor.cpp'
   ,'NetMonitor.cpp'
   ,'Page.cpp'
   ,'Processes.cpp'
   ,'ProcessesBase.cpp'
   ,'InfoPage.cpp'
   ,'Process.cpp'
   ,'ClkMonitor.cpp'
   ,'Infos.cpp'
   ,'DiskInfo.cpp'
   ,'DiskInfos.cpp'
   ,'MemMonitor.cpp'
   ,'DiagramMonitor.cpp'
   ,'TempMonitor.cpp'
   ,'Sensors.cpp'
   ,'Sensor.cpp'
   ,'NameValue.cpp'
   ,'FileByLine.cpp'
   ,'GpuCounter.cpp'
   ,'GpuExtAmdCounter.cpp'
   ,'NetConnection.cpp'
   ,'NetNode.cpp'
   ,'NetworkProperties.cpp'
   ,'LogProperties.cpp'
   ,'ProcessProperties.cpp'
   ,'BaseNetInfo.cpp'
   ,'NetInfo.cpp'
   )

if get_option('libg15')
    sources += files(
	 'G15Worker.cpp'
	,'libg15.c'
    )
endif

if get_option('lmsensors')
    sources += files(
	 'LmSensors.cpp'
	,'LmSensor.cpp'
    )
endif

# same use as in genericGlm,see main meson.build for query
if not use_gles
    sources += files(
	'GpuGLquery.cpp'
    )
    message('GpuGLquery included')
else
    message('GpuGLquery not included')
endif

if get_option('raspi')
    sources += files(
        'RaspiSensors.cpp'
        ,'RaspiSensor.cpp'
    )
endif