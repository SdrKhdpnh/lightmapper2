///////////////////////////////////////////////////////////////////////////////
//
// aergia
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
/// \file aergia.cpp
/// \brief Main project file for Eclipse's raytracer
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 9:12:2008   16:43
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <eclipseray/settings.h>
#include <eclipseray/pythoninterface.h>
#include <eclipseray/utils.h>
#include <eclipseray/ecrenderenvironment.h>

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 9:12:2008
///
/// Main entry point for our program
///
////////////////////////////////////////////////////////////////////////////////
int main( int argc, char *argv[] )
{
    // Parse input strings and derive settings from them
    Settings mySettings(argc,argv);

    if( mySettings.CanContinue() )
    {
        // Use these settings for the rest of the lifecycle of this program
        Settings::SetGlobalSettings( &mySettings );

        // Start our interfaces
        Utils::ErrorManager ::Start( mySettings.Get( Settings::Setting_ErrorHeapSize ) );
        RenderEnvironment   ::Start();
        PythonInterface     ::Start(argc-2,&argv[2]); // These arguments are for the script

        // Make sure we have a render environment before executing anything
        if( RenderEnvironment::GetREObject() )
        {
            // Run the python file requested by the caller
            const char* sScriptName = mySettings.Get( Settings::Setting_MainScript );

            Utils::PrintMessage( "Executing %s", sScriptName );
            PythonInterface::RunSimpleFile( sScriptName );

            // ...
        }       
        else
        {
            Utils::ErrorManager::Report( Utils::ErrorManager::ErrorType_Internal, NULL, 
                "Failed to initialize environment! Check plugins directory and make sure settings are valid" );
        }

        // Finalize interfaces
        PythonInterface     ::Stop();    
        RenderEnvironment   ::Stop();
        Utils::ErrorManager ::Stop();
    }
    else
    {
        mySettings.PrintSettingsHelp();        
    }


    return 0;

}