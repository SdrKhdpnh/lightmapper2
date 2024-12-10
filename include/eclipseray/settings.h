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
/// \file settings.h
/// \brief Defines a simple object for managing program options and settings
/// \author Dan Torres
//
///////////////////////////////////////////////////////////////////////////////
//
// Created On: 9:12:2008   11:39
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _OPTIONS_H
#define _OPTIONS_H


////////////////////////////////////////////////////////////////////////////
//
/// \class Settings
/// \author Dan Torres
/// \created 2008/12/09
/// \brief A simple object that keeps different runtime settings and parsers arguments
// 
/// This class parses command line parameters, and keep runtime settings. 
/// Command line parsing follows this syntax:
/// 
///     program.exe [PARAMS] filename.py 
/// 
/// PARAMS are prefixed with '-', and enable configuration switches. The filename
/// specifies a python script file with the code to execute. If no filename is
/// identified and the parameters don't contain enough information to tell the
/// program what to do, a help string is displayed. 
/// 
/// About global settings
/// 
/// You can use two functions to set and retrieve a temporary Settings
/// object, to be accessible from anywhere in the engine. Do this with
/// caution, though, and make sure you reset it to NULL if the settings
/// object is destroyed or goes out of context. Otherwise, callers that
/// depend on it will encounter an invalid object.
/// 
/// About setting types
/// 
/// We could had created an abstract interface for setting values that hid
/// their underlying type (i.e., something like ISetting), but in the
/// spirit of simplicity, we are just declaring three types and three
/// polymorphic functions.
//
//
////////////////////////////////////////////////////////////////////////////
class Settings
{
public:
    
    /// Default constructor
	Settings();

    /*!
     * Extended constructor
     *  @param a_nArgCount Number of arguments provided
     *  @param a_pArgValues Array with argument value strings
     */
    Settings( int a_nArgCount, char* a_pArgValues [] );

    /// Default destructor
    ~Settings();

    // -------------------------------------------------------------------------
    // Enumerators
    // -------------------------------------------------------------------------

    /*!
     *  @enum eBooleanSetting
     *  Defines all boolean settings
     */
    enum eBooleanSetting
    {
        Setting_Output_Verbose,         ///< PARAM(v|verbose) If true, dump strings with everything we do
        Setting_Python_OutputToFile,    ///< PARAM(l|logs) Redirect Python output to files instead of stdout    
        // ...

        Setting_Invalid                 ///< Invalid setting. Count of boolean options
    };

    /*!
     *  @enum eStringSetting
     *  Defines all string settings
     */
    enum eStringSetting
    {
        Setting_ProgramName,            ///< Name of this application
        Setting_MainScript,             ///< Name of the program or script to run
        Setting_PluginDir               ///< Path to the YR plugins
        // ...
    };

    /*!
     *	@enum eNumericSetting
     *  Defines all numeric settings
     */
    enum eNumericSetting
    {
        Setting_ErrorHeapSize,          ///< Number of errors to keep in our heap
        Setting_CPUCores                ///< Number of CPU cores available
        // ...
    };

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /*!
     *  Parses settings from the provided string array
     *  Parameters are translated on the GetSettingFromString function. Values
     *  are translated on the GetValueFromString function. For a description of
     *  what is expected here, look at both of them. In general, a setting=value
     *  set of strings is expected for non-booleans, and the presence of a
     *  setting name implies a
     *  @param a_nArgCount Number of arguments provided
     *  @param a_pArgValues Array with argument value strings
     */
    void Parse( int a_nArgCount, char* a_pArgValues [] );


    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------

    /*!
     *  Sets the value of a boolean setting
     *  @param a_nSetting A member of the eBooleanSetting enumeration
     *  @param a_bValue The value to set
     */
    void Set( eBooleanSetting a_nSetting, bool a_bValue );

    /*!
     *  Returns the requested boolean setting
     *  @param a_nSetting A member of the eBooleanSetting enumeration
     *  @return The value of the requested setting
     */
    bool Get( eBooleanSetting a_nSetting );

    /*!
     *  Returns the requested string setting
     *  @param a_nSetting A member of the eStringSetting enumeration
     */
    char* Get( eStringSetting a_nSetting );

    /*!
     *	Returns the requested numeric setting
     *  @param a_nSetting A member of the eNumericSetting enumeration
     */
    int Get( eNumericSetting a_nSetting );

    /*!
     *	Tells if we have enough data to run our program
     *  @return True if the program has enough information to continue
     */
    inline bool CanContinue(){ return m_bCanContinue; }

    /*!
     *	Prints help strings to stdout
     */
    void PrintSettingsHelp();

    /*!
     *  Sets the global settings pointer
     *  Note that this class is never responsible for the ownership of globally
     *  accessible settings. The caller owns the actual settings object.
     *  @param a_pSettings A pointer to a valid settings object
     */
    static void SetGlobalSettings( Settings* a_pSettings ){ m_pGlobalSettings = a_pSettings; }

    /*!
     *  Retrieves the global settings, or null if no settings have been set
     *  This function will simply return whatever settings have been assigned as
     *  the global settings object. It does not test to see if they are valid, so
     *  use it with care. See class declaration notes for more info.
     *  @return A pointer to the current global settings, or NULL if unset
     */
    static Settings* GetGlobalSettings(){ return m_pGlobalSettings; }


private:

    // Loads default settings
    void LoadDefaults();

    // Parses the provided string and returns an enumerated setting
    eBooleanSetting GetSettingFromString( char* a_sSetting );


    bool                m_booleanSettings[ Setting_Invalid ];   ///< Contains all of our boolean settings
    int                 m_nCores;                               ///< Number of CPU cores to use for multi-threading
    char*               m_sScriptName;                          ///< Name of the main program file to run
    char*               m_sPluginDir;                           ///< Plugin directory
    char*               m_sAppName;                             ///< First argument to this program
    bool                m_bCanContinue;                         ///< If true, the program has enough data to execute
    static Settings*    m_pGlobalSettings;                      ///< Convenience holder for globally accessible settings

};




#endif