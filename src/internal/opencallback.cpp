// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "internal/opencallback.hpp"

#include "internal/cfileinstream.hpp"
#include "internal/util.hpp"

using namespace bit7z;
using namespace bit7z::filesystem;

/* Most of this code is taken from the COpenCallback class in Client7z.cpp of the 7z SDK
 * Main changes made:
 *  + Use of wstring instead of UString (see Callback base interface)
 *  + Error messages are not showed (see comments in ExtractCallback) */

OpenCallback::OpenCallback( const BitArchiveHandler& handler, const tstring& filename )
    : Callback( handler ), mSubArchiveMode( false ), mFileItem( filename ) {}

COM_DECLSPEC_NOTHROW
STDMETHODIMP OpenCallback::SetTotal( const UInt64* /* files */, const UInt64* /* bytes */ ) noexcept {
    return S_OK;
}

COM_DECLSPEC_NOTHROW
STDMETHODIMP OpenCallback::SetCompleted( const UInt64* /* files */, const UInt64* /* bytes */ ) noexcept {
    return S_OK;
}

COM_DECLSPEC_NOTHROW
STDMETHODIMP OpenCallback::GetProperty( PROPID propID, PROPVARIANT* value ) {
    BitPropVariant prop;
    if ( mSubArchiveMode ) {
        if ( propID == kpidName ) {
            prop = mSubArchiveName;
            // case kpidSize:  prop = _subArchiveSize; break; // we don't use it for now
        }
    } else {
        switch ( propID ) {
            case kpidName:
                prop = fs::path( mFileItem.name() ).wstring();
                break;
            case kpidIsDir:
                prop = mFileItem.isDir();
                break;
            case kpidSize:
                prop = mFileItem.size();
                break;
            case kpidAttrib:
                prop = mFileItem.attributes();
                break;
            case kpidCTime:
                prop = mFileItem.creationTime();
                break;
            case kpidATime:
                prop = mFileItem.lastAccessTime();
                break;
            case kpidMTime:
                prop = mFileItem.lastWriteTime();
                break;
            default: //prop is empty
                break;
        }
    }
    *value = prop;
    prop.bstrVal = nullptr;
    return S_OK;
}

COM_DECLSPEC_NOTHROW
STDMETHODIMP OpenCallback::GetStream( const wchar_t* name, IInStream** inStream ) {
    try {
        *inStream = nullptr;
        if ( mSubArchiveMode ) {
            return S_FALSE;
        }
        if ( mFileItem.isDir() ) {
            return S_FALSE;
        }
        auto stream_path = fs::path{ mFileItem.path() };
        if ( name != nullptr ) {
            stream_path = stream_path.parent_path();
            stream_path.append( name );
            const auto stream_status = fs::status( stream_path );
            if ( !fs::exists( stream_status ) || fs::is_directory( stream_status ) ) {  // avoid exceptions using status
                return S_FALSE;
            }
        }
        auto inStreamTemp = bit7z::make_com< CFileInStream >( stream_path );
        if ( inStreamTemp->fail() ) {
            return HRESULT_FROM_WIN32( ERROR_OPEN_FAILED );
        }
        *inStream = inStreamTemp.Detach();
        return S_OK;
    } catch ( ... ) {
        return E_OUTOFMEMORY;
    }
}

COM_DECLSPEC_NOTHROW
STDMETHODIMP OpenCallback::SetSubArchiveName( const wchar_t* name ) {
    mSubArchiveMode = true;
    try {
        mSubArchiveName = name;
    } catch ( ... ) {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

COM_DECLSPEC_NOTHROW
STDMETHODIMP OpenCallback::CryptoGetTextPassword( BSTR* password ) {
    std::wstring pass;
    if ( !mHandler.isPasswordDefined() ) {
        // You can ask real password here from user
        // Password = GetPassword(OutStream);
        // PasswordIsDefined = true;
        if ( mHandler.passwordCallback() ) {
            pass = WIDEN( mHandler.passwordCallback()() );
        }

        if ( pass.empty() ) {
            mErrorMessage = kPasswordNotDefined;
            return E_ABORT;
        }
    } else {
        pass = WIDEN( mHandler.password() );
    }

    return StringToBstr( pass.c_str(), password );
}