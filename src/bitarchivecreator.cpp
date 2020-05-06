// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

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

#include "../include/bitarchivecreator.hpp"

#include "../include/bitexception.hpp"
#include "../include/cfileoutstream.hpp"
#include "../include/cmultivoloutstream.hpp"
#include "../include/cbufferoutstream.hpp"
#include "../include/updatecallback.hpp"

using namespace bit7z;

void compressOut( IOutArchive* out_arc, IOutStream* out_stream, UpdateCallback* update_callback ) {
    HRESULT result = out_arc->UpdateItems( out_stream, update_callback->itemsCount(), update_callback );

    if ( result == E_NOTIMPL ) {
        throw BitException( kUnsupportedOperation, ERROR_NOT_SUPPORTED );
    }

    if ( result != S_OK ) {
        update_callback->throwException( result );
    }
}

bool isValidCompressionMethod( const BitInOutFormat& format, BitCompressionMethod method ) {
    switch ( method ) {
        case BitCompressionMethod::Copy:
            return format == BitFormat::SevenZip || format == BitFormat::Zip || format == BitFormat::Tar ||
                   format == BitFormat::Wim;
        case BitCompressionMethod::Ppmd:
        case BitCompressionMethod::Lzma:
            return format == BitFormat::SevenZip || format == BitFormat::Zip;
        case BitCompressionMethod::Lzma2:
            return format == BitFormat::SevenZip || format == BitFormat::Xz;
        case BitCompressionMethod::BZip2:
            return format == BitFormat::SevenZip || format == BitFormat::BZip2 || format == BitFormat::Zip;
        case BitCompressionMethod::Deflate:
            return format == BitFormat::GZip || format == BitFormat::Zip;
        case BitCompressionMethod::Deflate64:
            return format == BitFormat::Zip;
        default:
            return false;
    }
}

CONSTEXPR auto MAX_LZMA_DICTIONARY_SIZE = 1536 * ( 1 << 20 ); // less than 1536 MiB
CONSTEXPR auto MAX_PPMD_DICTIONARY_SIZE = ( 1 << 30 );        // less than 1 GiB, i.e. 2^30 bytes
CONSTEXPR auto MAX_BZIP2_DICTIONARY_SIZE = 900 * ( 1 << 10 ); // less than 900 KiB
CONSTEXPR auto DEFLATE64_DICTIONARY_SIZE = ( 1 << 16 );       // equal to 64 KiB, i.e. 2^16 bytes
CONSTEXPR auto DEFLATE_DICTIONARY_SIZE = ( 1 << 15 );         // equal to 32 KiB, i.e. 2^15 bytes

bool isValidDictionarySize( BitCompressionMethod method, uint32_t dictionary_size ) {
    switch ( method ) {
        case BitCompressionMethod::Lzma:
        case BitCompressionMethod::Lzma2:
            return dictionary_size <= MAX_LZMA_DICTIONARY_SIZE;
        case BitCompressionMethod::Ppmd:
            return dictionary_size <= MAX_PPMD_DICTIONARY_SIZE;
        case BitCompressionMethod::BZip2:
            return dictionary_size <= MAX_BZIP2_DICTIONARY_SIZE;
        case BitCompressionMethod::Deflate64:
            return dictionary_size == DEFLATE64_DICTIONARY_SIZE;
        case BitCompressionMethod::Deflate:
            return dictionary_size == DEFLATE_DICTIONARY_SIZE;
        default:
            return true; //copy
    }
}

const wchar_t* methodName( BitCompressionMethod method ) {
    switch ( method ) {
        case BitCompressionMethod::Copy:
            return L"Copy";
        case BitCompressionMethod::Ppmd:
            return L"PPMd";
        case BitCompressionMethod::Lzma:
            return L"LZMA";
        case BitCompressionMethod::Lzma2:
            return L"LZMA2";
        case BitCompressionMethod::BZip2:
            return L"BZip2";
        case BitCompressionMethod::Deflate:
            return L"Deflate";
        case BitCompressionMethod::Deflate64:
            return L"Deflate64";
        default:
            return L"Unknown"; //this should not happen!
    }
}

BitArchiveCreator::BitArchiveCreator( const Bit7zLibrary& lib, const BitInOutFormat& format ) :
    BitArchiveHandler( lib ),
    mFormat( format ),
    mCompressionLevel( BitCompressionLevel::NORMAL ),
    mCompressionMethod( format.defaultMethod() ),
    mDictionarySize( 0 ),
    mCryptHeaders( false ),
    mSolidMode( false ),
    mUpdateMode( false ),
    mVolumeSize( 0 ) {}

const BitInFormat& BitArchiveCreator::format() const {
    return mFormat;
}

const BitInOutFormat& BitArchiveCreator::compressionFormat() const {
    return mFormat;
}

bool BitArchiveCreator::cryptHeaders() const {
    return mCryptHeaders;
}

BitCompressionLevel BitArchiveCreator::compressionLevel() const {
    return mCompressionLevel;
}

BitCompressionMethod BitArchiveCreator::compressionMethod() const {
    return mCompressionMethod;
}

uint32_t BitArchiveCreator::dictionarySize() const {
    return mDictionarySize;
}

bool BitArchiveCreator::solidMode() const {
    return mSolidMode;
}

bool BitArchiveCreator::updateMode() const {
    return mUpdateMode;
}

uint64_t BitArchiveCreator::volumeSize() const {
    return mVolumeSize;
}

uint32_t BitArchiveCreator::threadsCount() const {
    return mThreadsCount;
}

void BitArchiveCreator::setPassword( const tstring& password ) {
    setPassword( password, mCryptHeaders );
}

void BitArchiveCreator::setPassword( const tstring& password, bool crypt_headers ) {
    mPassword = password;
    mCryptHeaders = ( password.length() > 0 ) && crypt_headers;
}

void BitArchiveCreator::setCompressionLevel( BitCompressionLevel compression_level ) {
    mCompressionLevel = compression_level;
    mDictionarySize = 0; //reset dictionary size to default for the compression level
}

void BitArchiveCreator::setCompressionMethod( BitCompressionMethod compression_method ) {
    if ( !isValidCompressionMethod( mFormat, compression_method ) ) {
        throw BitException( "Invalid compression method for the chosen archive format", E_INVALIDARG );
    }
    if ( mFormat.hasFeature( MULTIPLE_METHODS ) ) {
        /* even though the compression method is valid, we set it only if the format supports
         * different methods than the default one */
        mCompressionMethod = compression_method;
        mDictionarySize = 0; //reset dictionary size to default for the method
    }
}

void BitArchiveCreator::setDictionarySize( uint32_t dictionary_size ) {
    if ( !isValidDictionarySize( mCompressionMethod, dictionary_size ) ) {
        throw BitException( "Invalid dictionary size for the chosen compression method", E_INVALIDARG );
    }
    if ( mCompressionMethod != BitCompressionMethod::Copy &&
         mCompressionMethod != BitCompressionMethod::Deflate &&
         mCompressionMethod != BitCompressionMethod::Deflate64 ) {
        //ignoring setting dictionary size for copy method and for methods having fixed dictionary size (deflate family)
        mDictionarySize = dictionary_size;
    }
}

void BitArchiveCreator::setSolidMode( bool solid_mode ) {
    mSolidMode = solid_mode;
}

void BitArchiveCreator::setUpdateMode( bool update_mode ) {
    mUpdateMode = update_mode;
}

void BitArchiveCreator::setVolumeSize( uint64_t size ) {
    mVolumeSize = size;
}

void BitArchiveCreator::setThreadsCount( uint32_t threads_count ) {
    mThreadsCount = threads_count;
}

CMyComPtr< IOutArchive > BitArchiveCreator::initOutArchive() const {
    CMyComPtr< IOutArchive > new_arc;
    const GUID format_GUID = mFormat.guid();
    mLibrary.createArchiveObject( &format_GUID, &::IID_IOutArchive, reinterpret_cast< void** >( &new_arc ) );
    setArchiveProperties( new_arc );
    return new_arc;
}

CMyComPtr< IOutStream > BitArchiveCreator::initOutFileStream( const tstring& out_archive,
                                                              CMyComPtr< IOutArchive >& new_arc,
                                                              unique_ptr< BitInputArchive >& old_arc ) const {
    if ( mVolumeSize > 0 ) {
        return new CMultiVolOutStream( mVolumeSize, out_archive );
    }

    CMyComPtr< IOutStream > out_stream;
    std::error_code ec;
    if ( mUpdateMode && fs::exists( out_archive, ec ) ) {
        if ( !mFormat.hasFeature( FormatFeatures::MULTIPLE_FILES ) ) {
            //Update mode is set but format does not support adding more files
            throw BitException( "Format does not support updating existing archive files", E_INVALIDARG );
        }

        auto* file_out_stream = new CFileOutStream( out_archive + TSTRING( ".tmp" ), true );
        out_stream = file_out_stream;
        if ( file_out_stream->fail() ) {
            //could not create temporary file
            throw BitException( "Could not create temp archive file for updating", out_archive, GetLastError() );
        }

        old_arc = std::make_unique< BitInputArchive >( *this, out_archive );
        old_arc->initUpdatableArchive( &new_arc );
        setArchiveProperties( new_arc );
    } else {
        auto* file_out_stream = new CFileOutStream( out_archive );
        out_stream = file_out_stream;
        if ( file_out_stream->fail() ) {
            //Unknown error!
            throw BitException( "Cannot create output archive file", out_archive, GetLastError() );
        }
    }
    return out_stream;
}

void BitArchiveCreator::compressToFile( const tstring& out_file, UpdateCallback* update_callback ) const {
    unique_ptr< BitInputArchive > old_arc = nullptr;
    CMyComPtr< IOutArchive > new_arc = initOutArchive();
    CMyComPtr< IOutStream > out_stream = initOutFileStream( out_file, new_arc, old_arc );
    update_callback->setOldArc( old_arc.get() );
    compressOut( new_arc, out_stream, update_callback );
    if ( old_arc ) {
        old_arc->close();
        /* NOTE: In the following instruction, we use . (dot) not -> (arrow) operator: in fact, both CMyComPtr and
         *       IOutStream have a Release() method, so we need to call only the one of CMyComPtr! */
        out_stream.Release(); //Releasing the output stream so that we can remove the original file

        //remove old file and rename tmp file (move file with overwriting)
        std::error_code error;
        fs::rename( out_file + TSTRING( ".tmp" ), out_file, error );
        if ( error ) {
            throw BitException( "Cannot rename temp archive file", out_file, GetLastError() );
        }
    }
}

void BitArchiveCreator::compressToBuffer( vector< byte_t >& out_buffer, UpdateCallback* update_callback ) const {
    if ( !out_buffer.empty() ) {
        throw BitException( kCannotOverwriteBuffer, E_INVALIDARG );
    }

    CMyComPtr< IOutArchive > new_arc = initOutArchive();
    CMyComPtr< IOutStream > out_mem_stream = new CBufferOutStream( out_buffer );
    compressOut( new_arc, out_mem_stream, update_callback );
}

void BitArchiveCreator::compressToStream( ostream& out_stream, UpdateCallback* update_callback ) const {
    CMyComPtr< IOutArchive > new_arc = initOutArchive();
    CMyComPtr< IOutStream > out_std_stream = new CStdOutStream( out_stream );
    compressOut( new_arc, out_std_stream, update_callback );
}

void BitArchiveCreator::setArchiveProperties( IOutArchive* out_archive ) const {
    vector< const wchar_t* > names;
    vector< BitPropVariant > values;
    if ( mCryptHeaders && mFormat.hasFeature( HEADER_ENCRYPTION ) ) {
        names.push_back( L"he" );
        values.emplace_back( true );
    }
    if ( mFormat.hasFeature( COMPRESSION_LEVEL ) ) {
        names.push_back( L"x" );
        values.emplace_back( static_cast< uint32_t >( mCompressionLevel ) );

        if ( mFormat.hasFeature( MULTIPLE_METHODS ) && mCompressionMethod != mFormat.defaultMethod() ) {
            names.push_back( mFormat == BitFormat::SevenZip ? L"0" : L"m" );
            values.emplace_back( methodName( mCompressionMethod ) );
        }
    }
    if ( mFormat.hasFeature( SOLID_ARCHIVE ) ) {
        names.push_back( L"s" );
        values.emplace_back( mSolidMode );
#ifndef _WIN32
        if ( mSolidMode ) {
            /* NOTE: Apparently, p7zip requires the filters to be set off for the solid compression to work.
               The most strange thing is... according to my tests this happens only in WSL!
               I've tested the same code on a Linux VM and it works without disabling the filters! */
            // TODO: So, for now I disable them, but this will need further investigation!
            names.push_back( L"f" );
            values.emplace_back( false );
        }
#endif
    }
    if ( mThreadsCount != 0 ) {
        names.push_back( L"mt" );
        values.emplace_back( mThreadsCount );
    }
    if ( mDictionarySize != 0 ) {
        const wchar_t* prop_name;
        //cannot optimize the following if-else, if we use tstring we have invalid pointers in names!
        if ( mFormat == BitFormat::SevenZip ) {
            prop_name = ( mCompressionMethod == BitCompressionMethod::Ppmd ? L"0mem" : L"0d" );
        } else {
            prop_name = ( mCompressionMethod == BitCompressionMethod::Ppmd ? L"mem" : L"d" );
        }
        names.push_back( prop_name );
        values.emplace_back( std::to_wstring( mDictionarySize ) + L"b" );
    }

    if ( !names.empty() ) {
        CMyComPtr< ISetProperties > set_properties;
        if ( out_archive->QueryInterface( ::IID_ISetProperties,
                                          reinterpret_cast< void** >( &set_properties ) ) != S_OK ) {
            throw BitException( "ISetProperties unsupported", ERROR_NOT_SUPPORTED );
        }
        if ( set_properties->SetProperties( names.data(), values.data(),
                                            static_cast< uint32_t >( names.size() ) ) != S_OK ) {
            throw BitException( "Cannot set properties of the archive", E_INVALIDARG );
        }
    }
}