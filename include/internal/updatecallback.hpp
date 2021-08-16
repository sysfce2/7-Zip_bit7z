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

#ifndef UPDATECALLBACK_HPP
#define UPDATECALLBACK_HPP

#ifndef _WIN32
#include <include_windows/windows.h>  //Needed for WINAPI macro definition used in IArchive of p7zip
#endif

#include <7zip/Archive/IArchive.h>
#include <7zip/ICoder.h>
#include <7zip/IPassword.h>

#include "bitarchivecreator.hpp"
#include "bitexception.hpp"
#include "bitinputarchive.hpp"
#include "bititemsvector.hpp"
#include "bitoutputarchive.hpp"
#include "internal/callback.hpp"
#include "internal/util.hpp"

namespace bit7z {
    constexpr auto kUnsupportedOperation = "Unsupported operation";
    constexpr auto kUnsupportedInMemoryFormat = "Unsupported format for in-memory compression";
    constexpr auto kCannotOverwriteBuffer = "Cannot overwrite or update a non empty buffer";

    class UpdateCallback final : public Callback,
                                 public IArchiveUpdateCallback2,
                                 public ICompressProgressInfo,
                                 protected ICryptoGetTextPassword2 {
        public:
            explicit UpdateCallback( const BitOutputArchive& output );

            UpdateCallback( const UpdateCallback& ) = delete;

            UpdateCallback( UpdateCallback&& ) = delete;

            UpdateCallback& operator=( const UpdateCallback& ) = delete;

            UpdateCallback& operator=( UpdateCallback&& ) = delete;

            ~UpdateCallback() override;

            MY_UNKNOWN_IMP3( IArchiveUpdateCallback2, ICompressProgressInfo, ICryptoGetTextPassword2 )

            HRESULT Finalize() noexcept;

            // IProgress from IArchiveUpdateCallback2
            BIT7Z_STDMETHOD( SetTotal, UInt64 size );

            BIT7Z_STDMETHOD( SetCompleted, const UInt64* completeValue );

            // ICompressProgressInfo
            BIT7Z_STDMETHOD( SetRatioInfo, const UInt64* inSize, const UInt64* outSize );

            // IArchiveUpdateCallback2
            BIT7Z_STDMETHOD( GetProperty, UInt32 index, PROPID propID, PROPVARIANT* value );

            BIT7Z_STDMETHOD( GetStream, UInt32 index, ISequentialInStream** inStream );

            BIT7Z_STDMETHOD_NOEXCEPT( GetVolumeSize, UInt32 index, UInt64* size );

            BIT7Z_STDMETHOD( GetVolumeStream, UInt32 index, ISequentialOutStream** volumeStream );

            BIT7Z_STDMETHOD_NOEXCEPT( GetUpdateItemInfo, UInt32 index,
                                      Int32* newData,
                                      Int32* newProperties,
                                      UInt32* indexInArchive );

            BIT7Z_STDMETHOD_NOEXCEPT( SetOperationResult, Int32 operationResult );

            //ICryptoGetTextPassword2
            BIT7Z_STDMETHOD( CryptoGetTextPassword2, Int32* passwordIsDefined, BSTR* password );

        private:
            const BitOutputArchive& mOutputArchive;
            bool mNeedBeClosed;
    };
}

#endif // UPDATECALLBACK_HPP