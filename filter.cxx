/*
 * Filter tabs -> spaces.
 *
 * Author: Jan Holesovsky <kendy@suse.cz>
 * License: MIT <http://www.opensource.org/licenses/mit-license.php>
 */

#include "error.hxx"
#include "filter.hxx"

#include <regex.h>

#include <cstring>
#include <cstdio>
#include <iostream>
#include <vector>

using namespace std;

struct Tabs {
    int spaces;
    FilterType type;
    regex_t regex;

    Tabs( int spaces_, FilterType type_ ) : spaces( spaces_ ), type( type_ ) {}
    ~Tabs() { regfree( &regex ); }

    bool matches( const string& fname_ ) { return regexec( &regex, fname_.c_str(), 0, NULL, 0 ) == 0; }
};

static std::vector< Tabs* > tabs_vector;

Filter::Filter( const string& fname_ )
    : spaces( 0 ),
      column( 0 ),
      spaces_to_write( 0 ),
      nonspace_appeared( false ),
      type( NO_FILTER )
{
    data.reserve( 16384 );

    for ( std::vector< Tabs* >::const_iterator it = tabs_vector.begin(); it != tabs_vector.end(); ++it )
    {
        if ( (*it)->matches( fname_.c_str() ) )
        {
            spaces = (*it)->spaces;
            type = (*it)->type;
            break; // 1st wins
        }
    }
}

inline void addDataLoopOld( char*& dest, char what, int& column, int& spaces_to_write, bool& nonspace_appeared, int no_spaces )
{
    if ( what == '\t' && !nonspace_appeared )
    {
        column += no_spaces;
        spaces_to_write += no_spaces;
    }
    else if ( what == ' ' )
    {
        ++column;
        ++spaces_to_write;
    }
    else if ( what == '\n' )
    {
        // write out any spaces that we need
        for ( int i = 0; i < spaces_to_write; ++i )
            *dest++ = ' ';

        *dest++ = what;
        column = 0;
        spaces_to_write = 0;
        nonspace_appeared = false;
    }
    else
    {
        nonspace_appeared = true;

        // write out any spaces that we need
        for ( int i = 0; i < spaces_to_write; ++i )
            *dest++ = ' ';

        *dest++ = what;
        ++column;
        spaces_to_write = 0;
    }
}

inline void addDataLoopCombined( char*& dest, char what, int& column, int& spaces_to_write, bool& nonspace_appeared, int no_spaces )
{
    if ( what == '\t' )
    {
        if ( nonspace_appeared )
        {
            // new behavior
            const int tab_size = no_spaces - ( column % no_spaces );
            column += tab_size;
            spaces_to_write += tab_size;
        }
        else
        {
            // old one
            column += no_spaces;
            spaces_to_write += no_spaces;
        }
    }
    else if ( what == ' ' )
    {
        ++column;
        ++spaces_to_write;
    }
    else if ( what == '\n' )
    {
        *dest++ = what;
        column = 0;
        spaces_to_write = 0;
        nonspace_appeared = false;
    }
    else
    {
        nonspace_appeared = true;

        // write out any spaces that we need
        for ( int i = 0; i < spaces_to_write; ++i )
            *dest++ = ' ';

        *dest++ = what;
        ++column;
        spaces_to_write = 0;
    }
}

inline void addDataLoopAll( char*& dest, char what, int& column, int& spaces_to_write, bool& nonspace_appeared, int no_spaces )
{
    if ( what == '\t' )
    {
        const int tab_size = no_spaces - ( column % no_spaces );
        column += tab_size;
        spaces_to_write += tab_size;
    }
    else if ( what == ' ' )
    {
        ++column;
        ++spaces_to_write;
    }
    else if ( what == '\n' )
    {
        *dest++ = what;
        column = 0;
        spaces_to_write = 0;
    }
    else
    {
        // write out any spaces that we need
        for ( int i = 0; i < spaces_to_write; ++i )
            *dest++ = ' ';

        *dest++ = what;
        ++column;
        spaces_to_write = 0;
    }
}

void Filter::addData( const char* data_, size_t len_ )
{
    if ( type == NO_FILTER || spaces <= 0 )
    {
        data.append( data_, len_ );
        return;
    }

    // type == FILTER_ALL
    char *tmp = new char[spaces*len_];
    char *dest = tmp;

    // convert the tabs to spaces (according to spaces)
    switch ( type )
    {
        case FILTER_OLD:
            for ( const char* it = data_; it < data_ + len_; ++it )
                addDataLoopOld( dest, *it, column, spaces_to_write, nonspace_appeared, spaces );
            break;
        case FILTER_COMBINED:
            for ( const char* it = data_; it < data_ + len_; ++it )
                addDataLoopCombined( dest, *it, column, spaces_to_write, nonspace_appeared, spaces );
            break;
        case FILTER_ALL:
            for ( const char* it = data_; it < data_ + len_; ++it )
                addDataLoopAll( dest, *it, column, spaces_to_write, nonspace_appeared, spaces );
            break;
        case NO_FILTER:
            // NO_FILTER already handled
            break;
    }

    data.append( tmp, dest - tmp );

    delete[] tmp;
}

void Filter::addData( const string& data_ )
{
    addData( data_.data(), data_.size() );
}

void Filter::write( std::ostream& out_ )
{
    out_ << "data " << data.size() << endl
         << data << endl;
}

void Filter::addTabsToSpaces( int how_many_spaces_, FilterType type_, const std::string& files_regex_ )
{
    Tabs* tabs = new Tabs( how_many_spaces_, type_ );

    int status = regcomp( &tabs->regex, files_regex_.c_str(), REG_EXTENDED | REG_NOSUB );
    if ( status == 0 )
        tabs_vector.push_back( tabs );
    else
    {
        Error::report( "Cannot create regex '" + files_regex_ + "' (for tabs_to_spaces_files)." );
        delete tabs;
    }
}
