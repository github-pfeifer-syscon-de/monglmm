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

extern "C" {
    // contains GENCMDSERVICE_MSGFIFO_SIZE
    #include <interface/vmcs_host/vc_gencmd_defs.h>
}

#include <string>
#include <locale>       // std::locale
#include <sstream>

#include "RaspiSensor.hpp"
#include "RaspiSensors.hpp"
#include "Sensors.hpp"
#include "Monitor.hpp"


RaspiSensor::RaspiSensor(Sensors *raspiSensor, const char *_name, const char *_command, const char *_unit)
: Sensor(raspiSensor, _name)
, m_command(_command)
, m_unit(_unit)
{
}

RaspiSensor::~RaspiSensor()
{
}

double
RaspiSensor::read()
{
    //VCHI_CONNECTION_T *vchi_connection = ((RaspiSensors *)m_sensors)->getVchiConnection();
    char buffer[ GENCMDSERVICE_MSGFIFO_SIZE ];
    int ret;

    //fprintf( stdout, "vc_gencmd_read_response cmd %s\n", m_command );
    if (( ret = vc_gencmd_send( "%s", m_command )) != 0 )
    {
       fprintf(  stderr, "vc_gencmd_send returned %d\n", ret );
       return 0.0;
    }

    //get + print out the response!
    if (( ret = vc_gencmd_read_response( buffer, sizeof( buffer ) )) != 0 )
    {
       fprintf( stderr, "vc_gencmd_read_response returned %d\n", ret );
       return 0.0;
    }
    buffer[ sizeof(buffer) - 1 ] = '\0';
    //fprintf( stdout, "vc_gencmd_read_response returned %s\n", buffer );

    if (strncmp( buffer, "error=", 6) == 0 )
    {
        fprintf( stderr, "vc_gencmd_read_response returned %s\n", buffer);
        return 0.0;
    }
    char *pos = strchr(buffer, '=');
    if (pos != nullptr) {
        ++pos;
        double value;
        //ret = sscanf(pos, "%lfV", &m_volt);
        std::istringstream is(pos);
        is.imbue(std::locale("C"));
        is >> value;
        adjustMinMax(value);

        return value;
    }
    else {
        fprintf( stderr, "Not parsable vcgencmd result %s\n", buffer);
    }
    return 0.0;
}

std::string
RaspiSensor::getUnit()
{
    return std::string(m_unit);
}

std::string
RaspiSensor::getLabel()
{
    std::ostringstream oss;
    oss << m_name << " ";
    oss << Monitor::formatScale(read(), getUnit().c_str(), 1000.0);
    return oss.str();
}
