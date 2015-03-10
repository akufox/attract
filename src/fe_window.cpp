/*
 *
 *  Attract-Mode frontend
 *  Copyright (C) 2013-2014 Andrew Mickelson
 *
 *  This file is part of Attract-Mode.
 *
 *  Attract-Mode is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Attract-Mode is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Attract-Mode.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "fe_window.hpp"
#include "fe_settings.hpp"
#include "fe_icon.hpp"
#include "fe_util.hpp"

#ifdef SFML_SYSTEM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // SFML_SYSTEM_WINDOWS

#ifdef SFML_SYSTEM_MACOS
#include "fe_util_osx.hpp"
#endif // SFM_SYSTEM_MACOS

#include <iostream>
#include <fstream>

#include <SFML/System/Sleep.hpp>

class FeWindowPosition : public FeBaseConfigurable
{
public:
	sf::Vector2i m_pos;
	sf::Vector2u m_size;

	static const char *FILENAME;

	FeWindowPosition( const sf::Vector2i &pos, const sf::Vector2u &size )
		: m_pos( pos ),
		m_size( size )
	{
	}

	int process_setting( const std::string &setting,
		const std::string &value,
		const std::string &filename )
	{
		size_t pos=0;
		std::string token;
		if ( setting.compare( "position" ) == 0 )
		{
			token_helper( value, pos, token, "," );
			m_pos.x = as_int( token );

			token_helper( value, pos, token );
			m_pos.y = as_int( token );
		}
		else if ( setting.compare( "size" ) == 0 )
		{
			token_helper( value, pos, token, "," );
			m_size.x = as_int( token );

			token_helper( value, pos, token );
			m_size.y = as_int( token );
		}
		return 1;
	};

	void save( const std::string &filename )
	{
		std::ofstream outfile( filename.c_str() );
		if ( outfile.is_open() )
		{
			outfile << "position " << m_pos.x << "," << m_pos.y << std::endl;
			outfile << "size " << m_size.x << "," << m_size.y << std::endl;
		}
		outfile.close();
	}
};

const char *FeWindowPosition::FILENAME = "window.am";

FeWindow::FeWindow( FeSettings &fes )
	: m_fes( fes )
{
}

void FeWindow::onCreate()
{
#ifdef SFML_SYSTEM_WINDOWS
	// In Windows, the "WS_POPUP" style creates grief switching to MAME.
	// Use the "WS_BORDER" style to fix this...
	//
	sf::WindowHandle hw = getSystemHandle();
	if ( ( GetWindowLong( hw, GWL_STYLE ) & WS_POPUP ) != 0 )
	{
		sf::VideoMode mode = sf::VideoMode::getDesktopMode();

		SetWindowLong( hw, GWL_STYLE,
			WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS );

		// resize the window off screen 1 pixel in each direction so we don't see the window border
		SetWindowPos(hw, HWND_TOP, -1, -1,
			mode.width + 2, mode.height + 2, SWP_FRAMECHANGED);

		ShowWindow(hw, SW_SHOW);
	}
#endif

#ifndef SFML_SYSTEM_MACOS
	// We don't set the icon on OS X, it looks like crap (too low res).
	setIcon( fe_icon.width, fe_icon.height, fe_icon.pixel_data );
#endif

	setVerticalSyncEnabled(true);
	setKeyRepeatEnabled(false);
	setMouseCursorVisible(false);
	setJoystickThreshold( 1.0 );

	sf::RenderWindow::onCreate();
}

void FeWindow::initial_create()
{
	int style_map[3] =
	{
		sf::Style::None,			// FeSettings::Default
		sf::Style::Fullscreen,	// FeSettings::Fullscreen
		sf::Style::Default		// FeSettings::Window
	};

	int win_mode = m_fes.get_window_mode();

	// Create window
	create(
		sf::VideoMode::getDesktopMode(),
		"Attract-Mode",
		style_map[ win_mode ] );

	if ( win_mode == FeSettings::Window )
	{
		FeWindowPosition win_pos(
			sf::Vector2i( 0, 0 ),
			sf::Vector2u( 480, 320 ) );

		win_pos.load_from_file( m_fes.get_config_dir() + FeWindowPosition::FILENAME );

		setPosition( win_pos.m_pos );
		setSize( win_pos.m_size );
	}
#ifdef SFML_SYSTEM_MACOS
	else if ( win_mode == FeSettings::Default )
	{
		osx_hide_menu_bar();
		setPosition( sf::Vector2i( 0, 0 ) );
	}
#endif

	sf::Vector2u wsize = getSize();
	m_fes.init_mouse_capture( wsize.x, wsize.y );

	// Only mess with the mouse position if mouse moves mapped
	if ( m_fes.test_mouse_reset( 0, 0 ) )
		sf::Mouse::setPosition( sf::Vector2i( wsize.x / 2, wsize.y / 2 ), *this );
}

bool FeWindow::run()
{
	int min_run;

	sf::Vector2i reset_pos = sf::Mouse::getPosition();

	sf::Vector2i hide_pos = getPosition();
	hide_pos.x += getSize().x;
	hide_pos.y += getSize().y;

	sf::Mouse::setPosition( hide_pos );

#ifdef SFML_SYSTEM_LINUX
	//
	// On Linux, fullscreen mode is confirmed to block the emulator
	// from running...  So we close our main window each time we run
	// an emulator and then recreate it when the emulator is done.
	//
	if ( m_fes.get_window_mode() == FeSettings::Fullscreen )
	{
		close();
		m_fes.run( min_run );
		sf::VideoMode mode = sf::VideoMode::getDesktopMode();
		create( mode, "Attract-Mode", sf::Style::Fullscreen );

		return true;
	}
#endif

	sf::Clock timer;

	//
	// For Steam support (at least on Windows) we have a "minimum run"
	// value that can be set per emulator.  run() below sets this value.
	// and we wait at least this amount of time (in seconds) and then wait
	// for focus to return to Attract-Mode if this value is set greater than 0
	//
	m_fes.run( min_run );

	if ( min_run <= 0 )
		return true;

	sf::Time elapsed = timer.getElapsedTime();
	if ( elapsed < sf::seconds( min_run ) )
		sf::sleep( sf::seconds( min_run ) - elapsed );

	//
	// Wait for focus to return
	//
	while ( isOpen() )
	{
		sf::Event ev;
		while (pollEvent(ev))
		{
			if ( ev.type == sf::Event::GainedFocus )
				return true;
			else if ( ev.type == sf::Event::Closed )
				return false;
		}

		sf::sleep( sf::milliseconds( 250 ) );
	}

	sf::Mouse::setPosition( reset_pos );

	return false;
}

void FeWindow::on_exit()
{
	if ( m_fes.get_window_mode() == FeSettings::Window )
	{
		FeWindowPosition win_pos( getPosition(), getSize() );
		win_pos.save( m_fes.get_config_dir() + FeWindowPosition::FILENAME );
	}
}
