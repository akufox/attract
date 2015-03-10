/*
 *
 *  Attract-Mode frontend
 *  Copyright (C) 2013-15 Andrew Mickelson
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

#ifndef FE_XML_HPP
#define FE_XML_HPP

#include <string>
#include <set>
#include <map>
#include <vector>
#include "fe_romlist.hpp"

class FeXMLParser
{
public:
	typedef bool (*UiUpdate) (void *, int);

	void set_continue_parse( bool f ) { m_continue_parse=f; };
	bool get_continue_parse() { return m_continue_parse; };

protected:
	friend void exp_handle_data( void *, const char *, int );
	friend void exp_start_element( void *, const char *, const char ** );
	friend void exp_end_element( void *, const char * );

	void handle_data( const char *, int );
	virtual void start_element( const char *, const char ** )=0;
	virtual void end_element( const char * )=0;

	UiUpdate m_ui_update;
	void * m_ui_update_data;

	// "Parse state" variables:
	//
	bool m_element_open;
	bool m_keep_rom;
	bool m_continue_parse;
	std::string m_current_data;

protected:
	FeXMLParser( UiUpdate u=NULL, void *d=NULL );
	FeXMLParser( const FeXMLParser & );
	FeXMLParser &operator=( const FeXMLParser & );

	bool parse_internal( const std::string &, const std::string & );
};

class FeMapComp
{
public:
	bool operator()(const char *lhs, const char *rhs) const;
};

class FeMameXMLParser : private FeXMLParser
{
public:
	FeMameXMLParser( FeRomInfoListType &, UiUpdate u=NULL, void *d=NULL, bool full=false );
	bool parse( const std::string &base_command );

private:
	FeRomInfoListType &m_romlist;
	FeRomInfoListType::iterator m_itr;
	std::map<const char *, FeRomInfoListType::iterator, FeMapComp> m_map;
	std::vector<FeRomInfoListType::iterator> m_discarded;
	int m_count;
	int m_percent;
	int m_displays;
	bool m_collect_data;
	bool m_chd;
	bool m_mechanical;
	bool m_full;

	void start_element( const char *, const char ** );
	void end_element( const char * );
};

class FeMessXMLParser : private FeXMLParser
{
public:
	FeMessXMLParser( FeRomInfoListType &, UiUpdate u=NULL, void *d=NULL, bool full=false );
	bool parse( const std::string &command, const std::string &args );

private:
	FeRomInfoListType &m_romlist;
	FeRomInfoListType::iterator m_itr;
	std::string m_name;
	std::string m_description;
	std::string m_year;
	std::string m_man;
	std::string m_fuzzydesc;
	std::string m_cloneof;
	std::string m_altname;
	std::string m_alttitle;
	bool m_full;

	void set_info_values( FeRomInfo &r );

	void start_element( const char *, const char ** );
	void end_element( const char * );
	void clear_parse_state();
};

class FeHyperSpinXMLParser : private FeXMLParser
{
public:
	FeHyperSpinXMLParser( FeRomInfoListType & );
	bool parse( const std::string &filename );

private:
	void start_element( const char *, const char ** );
	void end_element( const char * );

	FeRomInfoListType &m_romlist;
	FeRomInfo m_current_rom;
	bool m_collect_data;
};

class FeGameDBPlatformParser : private FeXMLParser
{
public:
	bool parse( const std::string &filename );

	std::set<std::string> m_set;

private:
	void start_element( const char *, const char ** );
	void end_element( const char * );
};

class FeGameDBParser : private FeXMLParser
{
public:
	FeGameDBParser( FeRomInfo & );
	bool parse( const std::string &data, bool &exact_match );

private:
	void start_element( const char *, const char ** );
	void end_element( const char * );

	FeRomInfo &m_rom;
	bool m_collect_data;
	bool m_exact_match;
};
#endif
