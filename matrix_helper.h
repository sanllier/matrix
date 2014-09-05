#ifndef MATRIX_HELPER_H
#define MATRIX_HELPER_H

#include <ostream>
#include <algorithm>

#include "matrix.h"

namespace Matrix {
//--------------------------------------------------------------

template< class T >
class matrix_helper
{
public:
    static void print( const matrix<T>& matr, std::ostream& oStr )
    {
        const long height = matr.height();
        const long width  = matr.width();

        oStr << "HEIGHT: " << height << "\r\n";
        oStr << "WIDTH:  " << width  << "\r\n-----------------------\r\n";

        for ( long i = 0; i < height; ++i )
        {
            for ( long q = 0; q < width; ++q )
            {
                oStr << matr.at( i, q ) << " ";
            }
            oStr << "\r\n";
        }
        oStr << "-----------------------\r\n";
    }

    static void fillRandom( matrix<T>& matr )
    {
        const long height = matr.height();
        const long width  = matr.width();

        for ( long i = 0; i < height; ++i )
            for ( long q = 0; q < width; ++q )
                matr.at( i, q ) = (T)rand();
    }

    static void makeIdentity( matrix<T>& matr )
    {
        const long size = matr.height() < matr.width() ? matr.height() : matr.width();
        for ( long i = 0; i < size; ++i )
            matr.at( i, i ) = T(1);
    }

    static void transpose( matrix<T>& matr )    // NOTE: lazy
    {
        const long height = matr.height();
        const long width  = matr.width();

        matrix<T> temp( matr.width(), matr.height() );
        for ( long i = 0; i < height; ++i )
            for ( long q = 0; q < width; ++q )
                temp.at( q, i ) = matr.at( i, q );

        matr.strongCopy( temp );
    }

	// CRAP
    /*static void blockMul( matrix<T>& matrA, matrix<T>& matrB, long blockSize )
    {
        const long fullBHeightA = matrA.height() / blockSize;
        const long fullBHeightB = matrB.height() / blockSize;
        const long fullBWidthA  = matrA.width()  / blockSize;
        const long fullBWidthB  = matrA.width()  / blockSize;

        matrix<T> blockA( fullBHeightA, fullBWidthA );
        matrix<T> blockB( fullBHeightB, fullBWidthB );

        for ( long i = 0; i < matrA.height() / fullBHeightA; ++i )
        {
            for ( long q = 0; q < matrB.width() / fullBWidthB; ++q )
            {
                for ( long k = 0; k < matrA.width() / fullBWidthA; ++k )
                {
                    blockA.strongSubmatrix( matrA, i * fullBHeightA, k * fullBWidthA, fullBHeightA, fullBWidthA );
                    blockB.weakSubmatrix( matrA, k * fullBHeightA, q * fullBWidthA, fullBHeightA, fullBWidthA );
                }
            }
        }

    }*/
};

//--------------------------------------------------------------
}

#endif
