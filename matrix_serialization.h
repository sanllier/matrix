#ifndef MATRIX_SERIALIZATION_H
#define MATRIX_SERIALIZATION_H

#include "matrix.h"

#include <fstream>

//-------------------------------------------------------------

#ifdef _MSC_VER 
    #ifdef _WIN64
        static const int ELEMENT_SIZE_BY_TYPE[] = { 0, 4, 8, 4, 8, 8, 16, 8, 16 };
    #else
        static const int ELEMENT_SIZE_BY_TYPE[] = { 0, 4, 4, 4, 8, 8, 8, 8, 16 };
    #endif
#endif

#ifdef __GNUC__
    #ifdef __x86_64__
        static const int ELEMENT_SIZE_BY_TYPE[] = { 0, 4, 8, 4, 8, 8, 16, 8, 16 };
    #else
        static const int ELEMENT_SIZE_BY_TYPE[] = { 0, 4, 4, 4, 8, 8, 8, 8, 16 };
    #endif
#endif

//-------------------------------------------------------------

namespace Matrix {
//--------------------------------------------------------------

#pragma pack(1)
struct SHeader
{
    long height;
	long width;
	EDataType   dataType;
	EMatrixType matrixType;
};

//--------------------------------------------------------------

class matrix_serialization
{
public:
    matrix_serialization():m_buf(0), m_bufSize(0), m_dataSize(0), m_pos(0) {}
    ~matrix_serialization()
    {
        if ( m_buf )
        {
            delete[] m_buf;
            m_buf = 0;
        }
    }

    //------------------- SERIALIZATION -------------------
    template< class T >
	size_t seriaizeStart( matrix<T>& mat, size_t bufSize )
	{
		if ( bufSize > 0 )
		{
			if ( m_buf )
				delete[] m_buf;

			m_bufSize = bufSize;
			m_buf = new char[ m_bufSize ];
			m_dataSize = sizeof( SHeader ) + mat.height() * mat.width() * sizeof(T);
			m_pos = 0;
			return m_dataSize;
		}
		return 0;
	}
    template< class T >
	const void* serializeStep( size_t& size, matrix<T>& mat )
	{
		if ( !m_buf || !m_bufSize )
		{
			size = 0;
			return 0;
		}

		size_t curSize = 0;

		if ( m_pos < sizeof( SHeader ) )
		{
			SHeader header = { mat.height(), mat.width(), mat.dataType(), mat.matrixType() };
			size_t temp = m_bufSize < sizeof( SHeader ) - m_pos ? m_bufSize : sizeof( SHeader ) - m_pos;
			std::memcpy( m_buf, (char*)&header + m_pos, temp );
			curSize += temp;
			m_pos += temp;
		}

		if ( curSize < m_bufSize && m_pos < m_dataSize )
		{
			size_t temp = m_bufSize - curSize;
			if ( m_pos + temp > m_dataSize )
				temp -= ( m_pos + temp ) - m_dataSize;

			std::memcpy( m_buf + curSize, (char*)mat.raw() + m_pos - sizeof( SHeader ), temp );
			curSize += temp;
			m_pos += temp;
		}

		size = curSize;
		return m_buf;				
	}
	void serializeStop()
	{
		if ( m_buf )
		{
			delete[] m_buf;
			m_buf = 0;
			m_bufSize  = 0;
            m_dataSize = 0;
			m_pos = 0;
		}
	}

    //------------------ DESERIALIZATION ------------------
	SHeader deserializeStart( size_t bufSize, const void* data )
	{
        if ( bufSize < sizeof( SHeader ) )
            return SHeader();

        SHeader header;
        std::memcpy( &header, data, sizeof( SHeader ) );
        m_dataSize = header.height * header.width * ELEMENT_SIZE_BY_TYPE[ header.dataType ];

		if ( m_buf )
		    delete[] m_buf;

        m_bufSize = bufSize;
		m_buf = new char[ bufSize ];
		m_pos = 0;
        return header;
	}
	template< class T >
    void deserializeStep( const void* data, size_t& dataSize, matrix<T>& mat )
	{
		if ( data )
		{
            size_t temp = m_bufSize > m_dataSize - m_pos ? m_dataSize - m_pos : m_bufSize;
			std::memcpy( (char*)mat.raw() + m_pos, data, temp );

			m_pos += temp;
			dataSize = temp;
		}
	}
	void deserializeStop()
	{
		if ( m_buf )
		{
			delete[] m_buf;
			m_buf = 0;
			m_bufSize  = 0;
            m_dataSize = 0;
			m_pos = 0;
		}
	}

    //--------------------- HELPERS -----------------------
	template< class T >
	bool writeBinary( const char* fileName, matrix<T>& matr )
	{
		static const size_t BUF_SIZE = sizeof( SHeader ); // CRAP?

		if ( !fileName || !fileName[0] )
			return false;

		std::ofstream oFstr( fileName );
		if ( !oFstr.good() )
			return false;

		seriaizeStart( matr, BUF_SIZE );
		size_t size = 0;
		const void* buf = 0;
		do
		{			
			buf = serializeStep( size, matr );
			oFstr.write( (const char*)buf, size );
		} while( size );
		serializeStop();

		oFstr.close();
		return true;
	}

	void* readBinary( const char* fileName, SHeader& header )
	{
		static const size_t BUF_SIZE = sizeof( SHeader ); // CRAP?

		if ( !fileName || !fileName[0] )
			return 0;

		std::ifstream iFstr( fileName );
		if ( !iFstr.good() )
			return 0;

		iFstr.seekg (0, iFstr.end);
		size_t length = (size_t)iFstr.tellg();
		iFstr.seekg (0, iFstr.beg);
		if ( length < sizeof( SHeader ) )
			return 0;

		char* buf = new char[ BUF_SIZE ];
		if ( !buf )
			return 0;

		auto onExit = [ buf, this ]( std::ifstream& iFstr ){ delete[] buf; iFstr.close(); deserializeStop(); };
		void* undefMatr;

		iFstr.read( buf, BUF_SIZE );
		length -= BUF_SIZE;
		header = deserializeStart( BUF_SIZE, buf );
		switch ( header.dataType )
		{
		case INT:
			undefMatr = new matrix<int>( header.height, header.width );
			break;
		case LONG:
			undefMatr = new matrix<long>( header.height, header.width );
			break;
		case FLOAT:
			undefMatr = new matrix<float>( header.height, header.width );
			break;
		case DOUBLE:
			undefMatr = new matrix<double>( header.height, header.width );
			break;
		case COMPLEX_INT:
			undefMatr = new matrix< std::complex<int> >( header.height, header.width );
			break;
		case COMPLEX_LONG:
			undefMatr = new matrix< std::complex<long> >( header.height, header.width );
			break;
		case COMPLEX_FLOAT:
			undefMatr = new matrix< std::complex<float> >( header.height, header.width );
			break;
		case COMPLEX_DOUBLE:
			undefMatr = new matrix< std::complex<double> >( header.height, header.width );
			break;
		case UNDEFINED_DATA_TYPE:
			onExit( iFstr );
			return 0;
		default:
			onExit( iFstr );
			return 0;
		}

		size_t size = 0;
		do
		{
			size = BUF_SIZE > length ? length : BUF_SIZE;
			iFstr.read( buf, size );
			length -= BUF_SIZE;

			switch ( header.dataType )
			{
			case INT:
				deserializeStep( buf, size, *( (matrix<int>*)undefMatr ) ); 
				break;
			case LONG:
				deserializeStep( buf, size, *( (matrix<long>*)undefMatr ) ); 
				break;
			case FLOAT:
				deserializeStep( buf, size, *( (matrix<float>*)undefMatr ) ); 
				break;
			case DOUBLE:
				deserializeStep( buf, size, *( (matrix<double>*)undefMatr ) ); 
				break;
			case COMPLEX_INT:
				deserializeStep( buf, size, *( (matrix< std::complex<int> >*)undefMatr ) ); 
				break;
			case COMPLEX_LONG:
				deserializeStep( buf, size, *( (matrix< std::complex<long> >*)undefMatr ) ); 
				break;
			case COMPLEX_FLOAT:
				deserializeStep( buf, size, *( (matrix< std::complex<float> >*)undefMatr ) ); 
				break;
			case COMPLEX_DOUBLE:
				deserializeStep( buf, size, *( (matrix< std::complex<double> >*)undefMatr ) ); 
				break;
			case UNDEFINED_DATA_TYPE:
				delete undefMatr;
				undefMatr = 0;
				onExit( iFstr );
				return 0;				
			default:
				delete undefMatr;
				undefMatr = 0;
				onExit( iFstr );
				return 0;	
			}
		} while( size );
		deserializeStop();

		onExit( iFstr );
		return undefMatr;
	}

private:
    char* m_buf;
    size_t m_bufSize;
    size_t m_dataSize;
    size_t m_pos;
};

//--------------------------------------------------------------
}

#endif
