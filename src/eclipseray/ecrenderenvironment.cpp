///////////////////////////////////////////////////////////////////////////////
//
// renderenvironment
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
/// \file renderenvironment.cpp
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 11:12:2008   11:14
//
///////////////////////////////////////////////////////////////////////////////

#include <eclipseray/ecrenderenvironment.h>
#include <eclipseray/utils.h>
#include <eclipseray/settings.h>


// -----------------------------------------------------------------------------
// RenderEnvironment implementation
// -----------------------------------------------------------------------------

YRRenderEnvironment*    RenderEnvironment::m_pEnvironment   = NULL; 
bool                    RenderEnvironment::m_bInit          = false;

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 11:12:2008
////////////////////////////////////////////////////////////////////////////////
void RenderEnvironment::Start()
{
    if(!m_bInit)
    {
        // Attempt to load all available plugins
        Settings* pSettings = Settings::GetGlobalSettings();
        m_pEnvironment      = NULL;
        if( pSettings )
        {
            m_pEnvironment = new YRRenderEnvironment();
            m_pEnvironment->loadPlugins( pSettings->Get(Settings::Setting_PluginDir) );
            Utils::PrintMessage("Creating render environment");
            m_bInit = true;
        }
        else
        {
            Utils::ErrorManager::Report( Utils::ErrorManager::ErrorType_Params, NULL,
                "Can't load plug-ins: Mising settings!" );
        }
    }
}

void RenderEnvironment::ClearAll()
{
    m_pEnvironment->clearAll();
}

////////////////////////////////////////////////////////////////////////////////
/// \author Dan Torres
/// \date 11:12:2008
////////////////////////////////////////////////////////////////////////////////
void RenderEnvironment::Stop()
{
    if( m_bInit )
    {
        Utils::PrintMessage("Destroying render environment");
        delete m_pEnvironment;
        m_pEnvironment = NULL;
        m_bInit        = false;
    }
}