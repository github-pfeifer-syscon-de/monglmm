/*
Copyright (c) 2012, Broadcom Europe Ltd
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <iostream>

#include "RaspiSensors.hpp"
#include "RaspiSensor.hpp"

#define TEMP_NAME "coreTemp"
#define VOLT_NAME "coreVolt"
#define CLK_CORE_NAME "coreClock"
#define CLK_VIDEO_NAME "videoClock"

RaspiSensors::RaspiSensors()
: Sensors()
, vchi_connection{nullptr}
{
    vcos_init();

    if ( vchi_initialise( &vchi_instance ) != 0)  {
       fprintf( stderr, "VCHI initialization failed\n" );
       return;
    }

    //create a vchi connection
    if ( vchi_connect( nullptr, 0, vchi_instance ) != 0) {
       fprintf( stderr, "VCHI connection failed\n" );
       return;
    }

    // the pointer is set to null ...
    vc_vchi_gencmd_init(vchi_instance, &vchi_connection, 1 );

    // https://www.raspberrypi.org/documentation/raspbian/applications/vcgencmd.md
    auto sensor0 = std::make_shared<RaspiSensor>(this, VOLT_NAME, "measure_volts core", "V");
    add(sensor0);
    auto sensor1 = std::make_shared<RaspiSensor>(this, TEMP_NAME, "measure_temp", "°C");
    add(sensor1);
    auto sensor2 = std::make_shared<RaspiSensor>(this, CLK_CORE_NAME, "measure_clock arm", "Hz");
    add(sensor2);
    auto sensor3 = std::make_shared<RaspiSensor>(this, CLK_VIDEO_NAME, "measure_clock core", "Hz");
    add(sensor3);
}

RaspiSensors::~RaspiSensors()
{
    vc_gencmd_stop();

    //close the vchi connection
    int ret = vchi_disconnect( vchi_instance );
    if (ret != 0) {
       std::cerr << "VCHI disconnect failed ret" << ret << std::endl;
    }
}

VCHI_CONNECTION_T *
RaspiSensors::getVchiConnection()
{
    return vchi_connection;
}

const std::shared_ptr<Sensor>
RaspiSensors::getPrimDefault()
{
    return getSensor(VOLT_NAME);
}

const std::shared_ptr<Sensor>
RaspiSensors::getSecDefault()
{
    return getSensor(TEMP_NAME);
}
