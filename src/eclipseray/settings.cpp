///////////////////////////////////////////////////////////////////////////////
//
// settings
//
// Copyright (C) 2009 BioWare
//
// This file is part of EclipseRay.
// 
// EclipseRay is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// EclipseRay is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with EclipseRay.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////
//
/// \file settings.cpp
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 9:12:2008   11:49
//
///////////////////////////////////////////////////////////////////////////////

#include <eclipseray/settings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

// -----------------------------------------------------------------------------
// Hardcoded settings
// -----------------------------------------------------------------------------
#define SETTINGS_ERRORHEAP_SIZE     10      // Size of our error heap

// -----------------------------------------------------------------------------
// Some macros
// -----------------------------------------------------------------------------

// Default the provided option to the indicated value
#define DEFAULT_BVALUE( _enum, _value )m_booleanSettings[ _enum ] = _value

// fast and easy string destructor
#define SAFE_FREE_STRING( _string ) \
    if(_string){                    \
        free(_string);              \
        _string = NULL;             \
    }

// -----------------------------------------------------------------------------
// Class implementation
// -----------------------------------------------------------------------------

Settings* Settings::m_pGlobalSettings = NULL;

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
Settings::Settings()
{
    LoadDefaults();
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
Settings::~Settings()
{
    SAFE_FREE_STRING( m_sScriptName );
    SAFE_FREE_STRING( m_sAppName    );

}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
Settings::Settings(int a_nArgCount, char* a_pArgValues [] )
{
    LoadDefaults();
    Parse( a_nArgCount, a_pArgValues );
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
void Settings::Set(eBooleanSetting a_nSetting, bool a_bValue )
{
    if( a_nSetting < Setting_Invalid )
    {
        m_booleanSettings[ a_nSetting ] = a_bValue;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
bool Settings::Get(eBooleanSetting a_nSetting )
{
    if( a_nSetting < Setting_Invalid )
        return m_booleanSettings[ a_nSetting ];
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
char* Settings::Get(eStringSetting a_nSetting )
{
    switch(a_nSetting)
    {
    case Setting_ProgramName:
        return m_sAppName;
        break;

    case Setting_MainScript:
        return m_sScriptName;
        break;

    case Setting_PluginDir:
        return m_sPluginDir;

        // ...


    default:
        return NULL;
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 12/18/2008
////////////////////////////////////////////////////////////////////////////////
int Settings::Get( eNumericSetting a_nSetting )
{
    switch(a_nSetting)
    {
    case Setting_ErrorHeapSize:
        return SETTINGS_ERRORHEAP_SIZE;
        break;

    case Setting_CPUCores:
        return m_nCores;
        break;

        //...

    default:
        return 0;
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
void Settings::LoadDefaults()
{
    // Default booleans
    DEFAULT_BVALUE( Setting_Output_Verbose,         false );
    DEFAULT_BVALUE( Setting_Python_OutputToFile,    false );

    // ...

    // Default strings
    m_sScriptName = NULL;
    m_sAppName    = NULL;
    m_sPluginDir  = "plugins";

    // ...

    // Default values for other variables
    m_bCanContinue = false;
    m_nCores = 1;
    // JamesG 18/6/2009: Killing multi-thread for now. There seems to be a problem with
    // deadlocks in the Yafray kdtree code under some circumstances. Procexp thinks it's
    // trying to upcase a unicode string (??). It seems to occur when there is geometry
    // far outside the main body of the scene.
    // Update 13/7/2009: Can't reproduce the deadlock anymore. Adding # cpu option in,
    // with default to 1.

    // ...

}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
//
//  Parse our values into multiple string tokens, which are later identified.
//  Currently we only support one actual string configuration value (our program
//  filename), but with a little bit of work you could parse string values for
//  configuration settings that currently only translate to boolean switches.
//
//
////////////////////////////////////////////////////////////////////////////////
void Settings::Parse(int a_nArgCount, char* a_pArgValues [] )
{
    m_bCanContinue = false;
    char* pThisArg = NULL;

    // The first argument is always the program's name
    m_sAppName = strdup( a_pArgValues[0] );

    // Parse any other arguments
    if( a_nArgCount > 1)
    {
        // We'll need at least one program name
        for( int i = 1; i < a_nArgCount; i++ )
        {
            pThisArg = a_pArgValues[ i ];

            // A switch
            if( pThisArg[0] == '-' )
            {
                GetSettingFromString( &pThisArg[1] );
            }
            // The name of our main script
            else
            {
                if( !m_sScriptName )
                {
                    m_sScriptName  = strdup( pThisArg );
                    m_bCanContinue = true;
                }
            }
        }

    }
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
Settings::eBooleanSetting Settings::GetSettingFromString(char* a_sSetting )
{
    // React to different settings
    if( (!strcmp( a_sSetting, "v" )) || (!strcmp( a_sSetting, "verbose" )) )
        Set(Setting_Output_Verbose, true);

    if( (!strcmp( a_sSetting, "l" )) || (!strcmp( a_sSetting, "logs" )) )
        Set(Setting_Python_OutputToFile, true);

    if( !strncmp( a_sSetting, "-cpus=", 6 ) )
    {
        errno = 0;
        m_nCores = atoi((const char*)&a_sSetting[6]);
        if (errno != 0)
        {
            std::cout << "Error parsing # CPUs option. Defaulting to single thread.\n";
            m_nCores = 1;
        }
    }

    // This is just a dummy result
    return Setting_Invalid;
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
////////////////////////////////////////////////////////////////////////////////
void Settings::PrintSettingsHelp()
{
    char* sHelp = "\n"                                                              \
        "Syntax is: [SETTINGS] filename, where settings can be:\n"                  \
        "   -v, -verbose:   Print verbose information of what's going on\n"         \
        "   -l, -logs:      Print python output into logfiles instead of stdout\n"  \
        "\n"                                                                        \
        "   filename is the name of the python script to run. Must be a valid file.\n";

    printf( sHelp );
}


