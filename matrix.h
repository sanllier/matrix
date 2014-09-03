#pragma once
#ifndef MATRIX_H
#define MATRIX_H

#include <cstring>
#include <memory>
#include <exception>
#include <cassert>
#include <complex>

namespace Matrix {
//--------------------------------------------------------------

template< class T >
class matrix
{
private:
	enum EDataType : int
	{
		UNDEFINED_DATA_TYPE,
		INT,
		LONG,
		FLOAT,
		DOUBLE,
        COMPLEX_INT,
        COMPLEX_LONG,
		COMPLEX_FLOAT,
		COMPLEX_DOUBLE
	};

	enum EMatrixType : int
	{
		UNDEFINED_MATRIX_TYPE,
		DENSE_NORMAL
	};

	#pragma pack(1)
	struct SHeader
	{
		long height;
		long width;
		EDataType   dataType;
		EMatrixType matrixType;
	};

	struct SPivot
	{
		long row;
		long col;
		SPivot():row(0), col(0) {}
	};

public:
	matrix()
		: m_data( nullptr )
		, m_height(0)
		, m_width(0)
        , m_dataWidth(0)
		, m_buf(0)
		, m_bufSize(0)
		, m_dataType( getType() )
	    , m_matrixType( DENSE_NORMAL ) {}
	matrix( long height, long width )
		: m_height( height )
		, m_width( width )
        , m_dataWidth( width )
		, m_buf(0)
		, m_bufSize(0)
		, m_dataType( getType() )
		, m_matrixType( DENSE_NORMAL )
	{
		if ( height && width )
		{
			m_data.reset( new T[ m_height * m_width ] );			
		}
		else
		{
			m_height = 0;
			m_width  = 0;
		}
	}
	matrix( const matrix& mat )
		: m_height( mat.m_height )
		, m_width( mat.m_width )
        , m_dataWidth( mat.m_dataWidth )
		, m_buf(0)
		, m_bufSize(0)
		, m_dataType( getType() )
		, m_matrixType( DENSE_NORMAL )
	{
		m_data = mat.m_data;
		m_pivot = mat.m_pivot;		
	}
	~matrix() 
	{
		m_data.reset();
		if ( m_buf )
		{
			delete[] m_buf;
			m_buf = 0;
		}		
	}

	//----------------------- COPY ------------------------
	void weakCopy( const matrix& mat )
	{
		if ( m_data )
			m_data.reset();

		m_height = mat.m_height;
		m_width  = mat.m_width;
		m_data = mat.m_data;
		m_pivot = mat.m_pivot;
	}
	void strongCopy( const matrix& mat )
	{
		if ( m_data )
			m_data.reset();

		m_height = mat.m_height;
		m_width  = mat.m_width;
		m_pivot.row = 0;
        m_pivot.col = 0;
		m_data.reset( new T[ m_height * m_width ] );

        size_t pos = 0;
        size_t shift = m_pivot.row * m_dataWidth + m_pivot.col;
        for ( long int i = 0; i < m_height; ++i )
        {
            std::memcpy( (char*)m_data.get() + pos, mat.m_data.get() + shift, m_width * sizeof(T) );
            shift += m_width;
            pos += m_width;
        }
		std::memcpy( m_data.get(), mat.m_data.get(), m_height * m_width * sizeof(T) );
	}
	matrix& operator=( const matrix& mat )
	{
		weakCopy( mat );
		return *this;
	}

	//---------------------- ACCESS -----------------------
	T& at( long row, long col )
	{
		if ( row >= m_height )
			throw std::out_of_range( "matrix row out of range" );
		else if ( col >= m_width )
			throw std::out_of_range( "matrix column out of range" );

		return m_data.get()[ ( row + m_pivot.row ) * m_dataWidth + ( col + m_pivot.col ) ];
	}
    const T& at( long row, long col ) const
	{
		if ( row >= m_height )
			throw std::out_of_range( "matrix row out of range" );
		else if ( col >= m_width )
			throw std::out_of_range( "matrix column out of range" );

		return m_data.get()[ ( row + m_pivot.row ) * m_dataWidth + ( col + m_pivot.col ) ];
	}
    inline long height() const { return m_height; }
    inline long width()  const { return m_width; }
    inline EDataType dataType() const { return m_dataType; }
    inline EMatrixType matrixType() const { return m_matrixType; }
    const void* raw() const { return m_data; } 

    //-----------------------------------------------------
    const matrix<T> submatrix( long row, long col, long height, long width ) const
    {
        static matrix<T> dummy;
        if ( row + height > m_height || col + width > m_width )
            return dummy;

        matrix<T> matr;
        matr.m_height = height;
        matr.m_width  = width;
        matr.m_dataWidth  = m_width;
        matr.m_matrixType = m_matrixType;
        matr.m_dataType   = m_dataType;
        matr.m_data = m_data;
        matr.m_pivot.row = row;
        matr.m_pivot.col = col;
        return matr;
    }

	//------------------- SERIALIZATION -------------------
	size_t seriaizeStart( size_t bufSize )
	{
		if ( bufSize > 0 )
		{
			if ( m_buf )
				delete[] m_buf;

			m_bufSize = bufSize;
			m_buf = new char[ m_bufSize ];
			m_size = sizeof( SHeader ) + m_height * m_width * sizeof(T);
			m_pos = 0;
			return m_size;
		}
		return 0;
	}
	const void* serializeStep( size_t& size )
	{
		if ( !m_buf || !m_bufSize )
		{
			size = 0;
			return 0;
		}

		size_t curSize = 0;

		if ( m_pos < sizeof( SHeader ) )
		{
			SHeader header = { m_height, m_width, m_dataType, m_matrixType };
			size_t temp = m_bufSize < sizeof( SHeader ) - m_pos ? m_bufSize : sizeof( SHeader ) - m_pos;
			std::memcpy( m_buf, (char*)&header + m_pos, temp );
			curSize += temp;
			m_pos += temp;
		}

		if ( curSize < m_bufSize && m_pos < m_size )
		{
			size_t temp = m_bufSize - curSize;
			if ( m_pos + temp > m_size )
				temp -= ( m_pos + temp ) - m_size;

			std::memcpy( m_buf + curSize, (char*)m_data.get() + m_pos - sizeof( SHeader ), temp );
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
			m_bufSize = 0;
			m_pos = 0;
		}
	}

    //------------------ DESERIALIZATION ------------------
	void deserializeStart( size_t bufSize )
	{
		m_size = m_height * m_width * sizeof(T);
		if ( bufSize > 0 && m_size - sizeof( SHeader ) >= 0 )
		{
			if ( m_buf )
				delete[] m_buf;

			m_buf = new char[ bufSize ];
			m_pos = 0;
		}
	}
	void deserializeStep( const void* data, size_t& dataSize )
	{
		const char* charData = (const char*)data;
		if ( charData && dataSize > 0 )
		{
			if ( m_pos < sizeof( SHeader ) )
			{
				static SHeader header;
				size_t temp = sizeof( SHeader ) - m_pos > dataSize ? dataSize : sizeof( SHeader ) - m_pos;
				std::memcpy( (char*)&header + m_pos, charData, temp );
				m_pos += temp;
				dataSize -= temp;
				charData += temp;
				if ( m_pos == sizeof( SHeader ) )
				{
					m_height = header.height;
					m_width  = header.width;
					m_dataType   = header.dataType;
					m_matrixType = header.matrixType;
					m_data.reset( new T[ m_height * m_width ] );
				}
			}

			if ( dataSize > 0 )
			{
				size_t temp = dataSize > m_size - m_pos ? m_size - m_pos : dataSize;
				std::memcpy( (char*)m_data.get() + m_pos - sizeof( SHeader ), charData, temp );
				m_pos += temp;
				dataSize -= temp;
			}
		}
	}
	void deserializeStop()
	{
		if ( m_buf )
		{
			delete[] m_buf;
			m_buf = 0;
			m_bufSize = 0;
			m_pos = 0;
		}
	}

private:
	EDataType getType() { return UNDEFINED_DATA_TYPE; }

private:
	long m_height;
	long m_width;
    long m_dataWidth;
	SPivot m_pivot;
	EDataType m_dataType;
	EMatrixType m_matrixType;

	std::shared_ptr<T> m_data;

	char* m_buf;
	size_t m_bufSize;
	size_t m_pos;
	size_t m_size;
};

template<> matrix<int>::EDataType matrix<int>::getType() { return INT; }
template<> matrix<long>::EDataType matrix<long>::getType() { return LONG; }
template<> matrix<float>::EDataType matrix<float>::getType() { return FLOAT; }
template<> matrix<double>::EDataType matrix<double>::getType() { return DOUBLE; }
template<> matrix< std::complex<int> >::EDataType matrix< std::complex<int> >::getType() { return COMPLEX_INT; }
template<> matrix< std::complex<long> >::EDataType matrix< std::complex<long> >::getType() { return COMPLEX_LONG; }
template<> matrix< std::complex<float> >::EDataType matrix< std::complex<float> >::getType() { return COMPLEX_FLOAT; }
template<> matrix< std::complex<double> >::EDataType matrix< std::complex<double> >::getType() { return COMPLEX_DOUBLE; }

//--------------------------------------------------------------
}

#endif
