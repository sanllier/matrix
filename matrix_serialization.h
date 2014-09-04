#ifndef MATRIX_SERIALIZATION_H
#define MATRIX_SERIALIZATION_H

#include "matrix.h"

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

class matrix_serialization
{
public:
    #pragma pack(1)
    struct SHeader
    {
        long height;
	    long width;
	    EDataType   dataType;
	    EMatrixType matrixType;
    };

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
			std::memcpy( (char*)mat.raw() + m_pos, (char*)data + m_pos, temp );
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
    
private:
    char* m_buf;
    size_t m_bufSize;
    size_t m_dataSize;
    size_t m_pos;
};

//--------------------------------------------------------------
}

#endif