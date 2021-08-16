/*
 * bit7z - A C++ static library to interface with the 7-zip DLLs.
 * Copyright (c) 2014-2021  Riccardo Ostani - All Rights Reserved.
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

#ifndef HRESULTCATEGORY_HPP
#define HRESULTCATEGORY_HPP

#include <system_error>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#else
#include <myWindows/StdAfx.h>
#endif

#include "bittypes.hpp"

namespace bit7z {
    struct hresult_category_t : public std::error_category {
        static_assert( sizeof( int ) >= sizeof( HRESULT ), "HRESULT type must be at least the size of int" );

        explicit hresult_category_t() = default;

        BIT7Z_NODISCARD const char* name() const noexcept override;

        BIT7Z_NODISCARD std::string message( int ev ) const override;

        BIT7Z_NODISCARD std::error_condition default_error_condition( int ev ) const noexcept override;
    };

    std::error_category& hresult_category() noexcept;
}

#endif //HRESULTCATEGORY_HPP