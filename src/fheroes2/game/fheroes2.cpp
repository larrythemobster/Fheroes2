/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2019 - 2025                                             *
 *                                                                         *
 *   Free Heroes2 Engine: http://sourceforge.net/projects/fheroes2         *
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <algorithm>

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wswitch-default"
#endif

#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_main.h> // IWYU pragma: keep
#include <SDL_mouse.h>

// Managing compiler warnings for SDL headers
#if defined( __GNUC__ )
#pragma GCC diagnostic pop
#endif

#if defined( _WIN32 )
#include <cassert>
#endif

#include <filesystem>

#include "../campaign/campaign_data.h"
#include "../campaign/campaign_scenariodata.h"
#include "../maps/map_format_helper.h"
#include "../maps/maps_fileinfo.h"
#include "../world/world.h"
#include "agg.h"
#include "agg_image.h"
#include "audio_manager.h"
#include "core.h"
#include "cursor.h"
#include "dir.h"
#include "embedded_image.h"
#include "exception.h"
#include "game.h"
#include "game_logo.h"
#include "game_video.h"
#include "game_video_type.h"
#include "h2d.h"
#include "icn.h"
#include "image.h"
#include "image_palette.h"
#include "localevent.h"
#include "logging.h"
#include "math_base.h"
#include "render_processor.h"
#include "screen.h"
#include "settings.h"
#include "system.h"
#include "timing.h"
#include "ui_language.h"
#include "ui_tool.h"
#include "zzlib.h"

namespace
{
    std::string toLower( std::string value )
    {
        std::transform( value.begin(), value.end(), value.begin(), []( unsigned char character ) { return static_cast<char>( std::tolower( character ) ); } );
        return value;
    }

    std::optional<Campaign::ScenarioInfoId> findCampaignScenarioInfo( const std::string & inputFile )
    {
        const std::string fileName = toLower( std::filesystem::path( inputFile ).filename().string() );

        const std::array<int, 6> campaignIds = {
            Campaign::ROLAND_CAMPAIGN,
            Campaign::ARCHIBALD_CAMPAIGN,
            Campaign::PRICE_OF_LOYALTY_CAMPAIGN,
            Campaign::DESCENDANTS_CAMPAIGN,
            Campaign::WIZARDS_ISLE_CAMPAIGN,
            Campaign::VOYAGE_HOME_CAMPAIGN,
        };

        for ( const int campaignId : campaignIds ) {
            const auto & scenarios = Campaign::CampaignData::getCampaignData( campaignId ).getAllScenarios();
            for ( const Campaign::ScenarioData & scenario : scenarios ) {
                const Maps::FileInfo scenarioMapInfo = scenario.loadMap();
                if ( scenarioMapInfo.filename.empty() ) {
                    continue;
                }

                if ( toLower( std::filesystem::path( scenarioMapInfo.filename ).filename().string() ) == fileName ) {
                    return scenario.getScenarioInfoId();
                }
            }
        }

        return std::nullopt;
    }
}

namespace
{
    std::string GetCaption()
    {
        return std::string( "fheroes2 engine, version: " + Settings::GetVersion() );
    }

    void ReadConfigs()
    {
        const std::string configurationFileName( Settings::configFileName );
        const std::string confFile = Settings::GetLastFile( "", configurationFileName );

        Settings & conf = Settings::Get();
        if ( System::IsFile( confFile ) && conf.Read( confFile ) ) {
            LocalEvent::Get().SetControllerPointerSpeed( conf.controllerPointerSpeed() );
        }
        else {
            conf.Save( configurationFileName );

            // Fullscreen mode can be enabled by default for some devices, we need to forcibly
            // synchronize reality with the default config if config file was not read
            conf.setFullScreen( conf.FullScreen() );
        }
    }

    void InitConfigDir()
    {
        const std::string configDir = System::GetConfigDirectory( "fheroes2" );

        System::MakeDirectory( configDir );
    }

    void InitDataDir()
    {
        const std::string dataDir = System::GetDataDirectory( "fheroes2" );

        if ( dataDir.empty() ) {
            return;
        }

        const std::string dataFiles = System::concatPath( dataDir, "files" );
        const std::string dataFilesSave = System::concatPath( dataFiles, "save" );

        // This call will also create dataDir and dataFiles
        System::MakeDirectory( dataFilesSave );
    }

    void displayMissingResourceWindow()
    {
        fheroes2::Display & display = fheroes2::Display::instance();
        const fheroes2::Image & image = Compression::CreateImageFromZlib( 290, 190, errorMessage, sizeof( errorMessage ), false );

        display.fill( 0 );
        fheroes2::Resize( image, display );

        display.render();

        LocalEvent & le = LocalEvent::Get();

        // Display the message for 5 seconds so that the user sees it enough and not immediately closes without reading properly.
        const fheroes2::Time timer;

        bool closeWindow = false;

        while ( le.HandleEvents( true, true ) ) {
            if ( closeWindow && timer.getS() >= 5 ) {
                break;
            }

            if ( le.isAnyKeyPressed() || le.MouseClickLeft() ) {
                closeWindow = true;
            }
        }
    }

    class DisplayInitializer final
    {
    public:
        DisplayInitializer()
        {
            const Settings & conf = Settings::Get();

            fheroes2::Display & display = fheroes2::Display::instance();
            fheroes2::ResolutionInfo bestResolution{ conf.currentResolutionInfo() };

            if ( conf.isFirstGameRun() && System::isHandheldDevice() ) {
                // We do not show resolution dialog for first run on handheld devices. In this case it is wise to set 'widest' resolution by default.
                const std::vector<fheroes2::ResolutionInfo> resolutions = fheroes2::engine().getAvailableResolutions();

                for ( const fheroes2::ResolutionInfo & info : resolutions ) {
                    if ( info.gameWidth > bestResolution.gameWidth && info.gameHeight == bestResolution.gameHeight ) {
                        bestResolution = info;
                    }
                }
            }

            display.setWindowPos( conf.getSavedWindowPos() );
            display.setResolution( bestResolution );

            fheroes2::engine().setTitle( GetCaption() );

            // Hide system cursor.
            const int returnValue = SDL_ShowCursor( SDL_DISABLE );
            if ( returnValue < 0 ) {
                ERROR_LOG( "Failed to hide system cursor. Error description: " << SDL_GetError() )
            }

            fheroes2::RenderProcessor & renderProcessor = fheroes2::RenderProcessor::instance();

            display.subscribe( [&renderProcessor]( std::vector<uint8_t> & palette ) { return renderProcessor.preRenderAction( palette ); },
                               [&renderProcessor]() { renderProcessor.postRenderAction(); } );

            // Initialize system info renderer.
            _systemInfoRenderer = std::make_unique<fheroes2::SystemInfoRenderer>();

            renderProcessor.registerRenderers( [sysInfoRenderer = _systemInfoRenderer.get()]() { sysInfoRenderer->preRender(); },
                                               [sysInfoRenderer = _systemInfoRenderer.get()]() { sysInfoRenderer->postRender(); } );
            renderProcessor.startColorCycling();

            // Update mouse cursor when switching between software emulation and OS mouse modes.
            fheroes2::cursor().registerUpdater( Cursor::Refresh );

#if !defined( MACOS_APP_BUNDLE )
            const fheroes2::Image & appIcon = Compression::CreateImageFromZlib( 32, 32, iconImage, sizeof( iconImage ), true );
            fheroes2::engine().setIcon( appIcon );
#endif
        }

        DisplayInitializer( const DisplayInitializer & ) = delete;
        DisplayInitializer & operator=( const DisplayInitializer & ) = delete;

        ~DisplayInitializer()
        {
            fheroes2::RenderProcessor::instance().unregisterRenderers();

            fheroes2::Display & display = fheroes2::Display::instance();
            display.subscribe( {}, {} );
            display.release();
        }

    private:
        // This member must not be initialized before Display.
        std::unique_ptr<fheroes2::SystemInfoRenderer> _systemInfoRenderer;
    };

    class DataInitializer final
    {
    public:
        DataInitializer()
        {
            const fheroes2::ScreenPaletteRestorer screenRestorer;

            try {
                _aggInitializer.reset( new AGG::AGGInitializer );

                _h2dInitializer.reset( new fheroes2::h2d::H2DInitializer );

                // Verify that the font is present and it is not corrupted.
                fheroes2::AGG::GetICN( ICN::FONT, 0 );
            }
            catch ( ... ) {
                displayMissingResourceWindow();

                throw;
            }
        }

        DataInitializer( const DataInitializer & ) = delete;
        DataInitializer & operator=( const DataInitializer & ) = delete;
        ~DataInitializer() = default;

        const std::string & getOriginalAGGFilePath() const
        {
            return _aggInitializer->getOriginalAGGFilePath();
        }

        const std::string & getExpansionAGGFilePath() const
        {
            return _aggInitializer->getExpansionAGGFilePath();
        }

    private:
        std::unique_ptr<AGG::AGGInitializer> _aggInitializer;
        std::unique_ptr<fheroes2::h2d::H2DInitializer> _h2dInitializer;
    };

    // This function checks for a possible situation when a user uses a demo version
    // of the game. There is no 100% certain way to detect this, so assumptions are made.
    bool isProbablyDemoVersion()
    {
        if ( Settings::Get().isPriceOfLoyaltySupported() ) {
            return false;
        }

        // The demo version of the game only has 1 map.
        const ListFiles maps = Settings::FindFiles( "maps", ".mp2", false );
        return maps.size() == 1;
    }
}

int main( int argc, char ** argv )
{
// SDL2main.lib converts argv to UTF-8, but this application expects ANSI, use the original argv
#if defined( _WIN32 )
    assert( argc == __argc );

    argv = __argv;
#else
    (void)argc;
#endif

    try {
        const fheroes2::HardwareInitializer hardwareInitializer;
        Logging::InitLog();

        COUT( GetCaption() )

        Settings & conf = Settings::Get();
        conf.SetProgramPath( argv[0] );

        InitConfigDir();
        InitDataDir();
        ReadConfigs();

        std::set<fheroes2::SystemInitializationComponent> coreComponents{ fheroes2::SystemInitializationComponent::Audio,
                                                                          fheroes2::SystemInitializationComponent::Video };

#if defined( TARGET_PS_VITA ) || defined( TARGET_NINTENDO_SWITCH )
        coreComponents.emplace( fheroes2::SystemInitializationComponent::GameController );
#endif

        const fheroes2::CoreInitializer coreInitializer( coreComponents );

        DEBUG_LOG( DBG_GAME, DBG_INFO, conf.String() )

        const DisplayInitializer displayInitializer;
        const DataInitializer dataInitializer;

        ListFiles midiSoundFonts;
        {
            const std::string path = System::concatPath( "files", "soundfonts" );
            midiSoundFonts.Append( Settings::FindFiles( path, ".sf2", false ) );
            midiSoundFonts.Append( Settings::FindFiles( path, ".sf3", false ) );
        }

#ifdef WITH_DEBUG
        for ( const std::string & file : midiSoundFonts ) {
            DEBUG_LOG( DBG_GAME, DBG_INFO, "MIDI SoundFont to load: " << file )
        }
#endif

        const std::string timidityCfgPath = []() -> std::string {
            if ( std::string path; Settings::findFile( System::concatPath( "files", "timidity" ), "timidity.cfg", path ) ) {
                return path;
            }

            return {};
        }();

#ifdef WITH_DEBUG
        if ( !timidityCfgPath.empty() ) {
            DEBUG_LOG( DBG_GAME, DBG_INFO, "Path to the timidity.cfg file: " << timidityCfgPath )
        }
#endif

        const AudioManager::AudioInitializer audioInitializer( dataInitializer.getOriginalAGGFilePath(), dataInitializer.getExpansionAGGFilePath(), midiSoundFonts,
                                                               timidityCfgPath );

        // Load palette.
        fheroes2::setGamePalette( AGG::getDataFromAggFile( "KB.PAL", false ) );
        const fheroes2::Display & display = fheroes2::Display::instance();
        display.changePalette( nullptr, true );

        // Update the fonts according to the game language set in the configuration.
        // NOTICE: it must be done before initializing the engine to properly load all
        // language-specific font characters for the selected language because during
        // initialization the English language is forced to properly read the configuration files.
        conf.setGameLanguage( conf.getGameLanguage() );

        // Initialize game data.
        Game::Init();

        for ( int i = 1; i < argc; ++i ) {
            if ( std::string(argv[i]) == "--convert" && i + 1 < argc ) {
                std::string dirPath = argv[i + 1];
                std::cout << "Starting native fheroes2 map conversion in: " << dirPath << "\n";

                for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
                    if (!entry.is_regular_file()) continue;

                    std::string ext = entry.path().extension().string();
                    std::transform( ext.begin(), ext.end(), ext.begin(), []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );

                    if (ext == ".mp2" || ext == ".mpx" || ext == ".mx2" || ext == ".h2c" || ext == ".hxc") {
                        std::string inputFile = entry.path().string();
                        std::string outputFile = inputFile.substr(0, inputFile.find_last_of('.')) + ".fh2m";
                        const bool isCampaignMap = ext == ".h2c" || ext == ".hxc";

                        std::cout << "Converting " << inputFile << "...\n";

                        Maps::FileInfo mapInfo;
                        if ( !mapInfo.readMP2Map( inputFile, false ) ) {
                            std::cout << " -> Failed to read map metadata.\n";
                            continue;
                        }

                        bool hasFactionChoice = false;
                        if ( isCampaignMap ) {
                            if ( const std::optional<Campaign::ScenarioInfoId> scenarioInfo = findCampaignScenarioInfo( inputFile ); scenarioInfo.has_value() ) {
                                Campaign::CampaignData::updateScenarioGameplayConditions( *scenarioInfo, mapInfo );
                                
                                // Apply campaign-specific victory/loss conditions
                                const auto & scenarios = Campaign::CampaignData::getCampaignData( scenarioInfo->campaignId ).getAllScenarios();
                                const Campaign::ScenarioData * scenarioData = nullptr;
                                for ( const Campaign::ScenarioData & scenario : scenarios ) {
                                    if ( scenario.getScenarioInfoId() == *scenarioInfo ) {
                                        scenarioData = &scenario;
                                        break;
                                    }
                                }
                                
                                if ( scenarioData != nullptr ) {
                                    const Campaign::ScenarioVictoryCondition campaignVictory = scenarioData->getVictoryCondition();
                                    if ( campaignVictory != Campaign::ScenarioVictoryCondition::STANDARD ) {
                                        // Campaign has a specific victory condition - convert and apply it
                                        if ( campaignVictory == Campaign::ScenarioVictoryCondition::CAPTURE_DRAGON_CITY ) {
                                            mapInfo.victoryConditionType = Maps::FileInfo::VICTORY_CAPTURE_TOWN;
                                        }
                                        else if ( campaignVictory == Campaign::ScenarioVictoryCondition::OBTAIN_ULTIMATE_CROWN ) {
                                            mapInfo.victoryConditionType = Maps::FileInfo::VICTORY_OBTAIN_ARTIFACT;
                                        }
                                        else if ( campaignVictory == Campaign::ScenarioVictoryCondition::OBTAIN_SPHERE_NEGATION ) {
                                            mapInfo.victoryConditionType = Maps::FileInfo::VICTORY_OBTAIN_ARTIFACT;
                                        }
                                    }
                                    
                                    const Campaign::ScenarioLossCondition campaignLoss = scenarioData->getLossCondition();
                                    if ( campaignLoss != Campaign::ScenarioLossCondition::STANDARD ) {
                                        // Campaign has a specific loss condition
                                        if ( campaignLoss == Campaign::ScenarioLossCondition::LOSE_ALL_SORCERESS_VILLAGES ) {
                                            mapInfo.lossConditionType = Maps::FileInfo::LOSS_TOWN;
                                        }
                                    }

                                    // Detect if this scenario offers a faction choice (STARTING_RACE bonus).
                                    // When true, the converter will export the human player's castle as random
                                    // so the campaign's chosen race is applied correctly at game start.
                                    for ( const Campaign::ScenarioBonusData & bonus : scenarioData->getBonuses() ) {
                                        if ( bonus._type == Campaign::ScenarioBonusData::STARTING_RACE
                                             || bonus._type == Campaign::ScenarioBonusData::STARTING_RACE_AND_ARMY ) {
                                            hasFactionChoice = true;
                                            break;
                                        }
                                    }
                                }
                            }
                        }

                        conf.setCurrentMapInfo( mapInfo );
                        conf.GetPlayers().Init( mapInfo );
                        conf.GetPlayers().SetStartGame();

                        if ( !Maps::MapFormatHelper::convertMapFile( inputFile, outputFile, mapInfo, isCampaignMap, hasFactionChoice ) ) {
                            std::cout << " -> Engine failed to convert map.\n";
                            continue;
                        }
                        std::cout << " -> Success!\n";
                    }
                }
                std::cout << "Conversion complete! Exiting engine.\n";
                return EXIT_SUCCESS; // Exit before the main menu launches
            }

            if ( std::string(argv[i]) == "--load-fh2m" && i + 1 < argc ) {
                const std::string inputFile = argv[i + 1];
                std::cout << "Loading FH2M map: " << inputFile << "\n";

                Maps::FileInfo mapInfo;
                if ( !mapInfo.readResurrectionMap( inputFile, false, fheroes2::getLanguageFromAbbreviation( conf.getGameLanguage() ) ) ) {
                    std::cout << " -> Failed to read FH2M metadata.\n";
                    return EXIT_FAILURE;
                }

                conf.setCurrentMapInfo( mapInfo );
                conf.GetPlayers().Init( mapInfo );
                conf.GetPlayers().SetStartGame();

                if ( !World::Get().loadResurrectionMap( inputFile ) ) {
                    std::cout << " -> Failed to load FH2M map.\n";
                    return EXIT_FAILURE;
                }

                std::cout << " -> FH2M load completed.\n";
                return EXIT_SUCCESS;
            }
        }
        if ( conf.isShowIntro() ) {
            fheroes2::showTeamInfo();
            for ( const char * logo : { "NWCLOGO.SMK", "CYLOGO.SMK", "H2XINTRO.SMK" } ) {
                Video::ShowVideo( { { logo, Video::VideoControl::PLAY_CUTSCENE } } );
            }
        }

        try {
            const CursorRestorer cursorRestorer( true, Cursor::POINTER );
            const fheroes2::Point pos = conf.getSavedWindowPos();
            Game::mainGameLoop( conf.isFirstGameRun(), isProbablyDemoVersion() );
            const fheroes2::Point currentPos = display.getWindowPos();
            if ( pos != currentPos ) {
                conf.setStartWindowPos( currentPos );
                conf.Save( Settings::configFileName );
            }
        }
        catch ( const fheroes2::InvalidDataResources & ex ) {
            ERROR_LOG( ex.what() )
            displayMissingResourceWindow();
            return EXIT_FAILURE;
        }
    }
    catch ( const std::exception & ex ) {
        ERROR_LOG( "Exception '" << ex.what() << "' occurred during application runtime." )
        return EXIT_FAILURE;
    }
    catch ( ... ) {
        ERROR_LOG( "An unknown exception occurred during application runtime." )
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
