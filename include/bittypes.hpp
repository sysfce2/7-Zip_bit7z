/*
 * bit7z - A C++ static library to interface with the 7-zip DLLs.
 * Copyright (c) 2014-2019  Riccardo Ostani - All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * Bit7z is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with bit7z; if not, see https://www.gnu.org/licenses/.
 */

#ifndef BITTYPES_HPP
#define BITTYPES_HPP

#include <string>

#ifdef BIT7Z_AUTO_FORMAT
#include <regex>
#endif

#if defined( _MSCV_VER ) && ( _MSC_VER <= 1800 )
#define CONSTEXPR const
#else
#define CONSTEXPR constexpr
#endif

namespace bit7z {
    /**
     * @brief A type representing a byte (equivalent to an unsigned char).
     */
    using byte_t = unsigned char;

#ifdef _WIN32 // Windows
    using tchar = wchar_t;
    using tstring = std::wstring;
#ifdef BIT7Z_AUTO_FORMAT
    using tregex = std::wregex;
#endif
#define TSTRING( str ) L##str
#define to_tstring std::to_wstring
#else // Unix
    using tchar = char;
    using tstring = std::string;
#ifdef BIT7Z_AUTO_FORMAT
    using tregex = std::regex;
#endif
#define TSTRING( str ) str
#define to_tstring std::to_string

    CONSTEXPR auto ERROR_OPEN_FAILED = EIO;
    CONSTEXPR auto ERROR_FILE_NOT_FOUND = ENOENT;
    CONSTEXPR auto ERROR_ACCESS_DENIED = EACCES;
    CONSTEXPR auto ERROR_NOT_SUPPORTED = ENOTSUP;
    CONSTEXPR auto ERROR_SEEK = EIO;
    CONSTEXPR auto ERROR_READ_FAULT = EIO;
    CONSTEXPR auto ERROR_WRITE_FAULT = EIO;
#endif
}
#endif // BITTYPES_HPP