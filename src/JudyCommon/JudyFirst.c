// Copyright (C) 2000 - 2002 Hewlett-Packard Company
//
// This program is free software; you can redistribute it and/or modify it
// under the term of the GNU Lesser General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
// for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// _________________

// @(#) $Revision: 4.12 $ $Source: /judy/src/JudyCommon/JudyFirst.c $
//
// Judy*First[Empty]() and Judy*Last[Empty]() routines for Judy1 and JudyL.
// Compile with one of -DJUDY1 or -DJUDYL.
//
// These are inclusive versions of Judy*Next[Empty]() and Judy*Prev[Empty]().

#if (! (defined(JUDY1) || defined(JUDYL)))
#error:  One of -DJUDY1 or -DJUDYL must be specified.
#endif

#ifdef JUDY1
#include "Judy1.h"
#else
#include "JudyL.h"
#endif


// ****************************************************************************
// J U D Y   1   F I R S T
// J U D Y   L   F I R S T
//
// See the manual entry for details.

#ifdef JUDY1
FUNCTION int	  Judy1First
#else
FUNCTION PPvoid_t JudyLFirst
#endif
        (
	Pcvoid_t  PArray,	// Judy array to search.
	Word_t *  PIndex,	// starting point and result.
	PJError_t PJError	// optional, for returning error info.
        )
{
        if (PIndex == (PWord_t) NULL)		// caller error:
            return(0);

#ifdef JUDY1
	int ret = j__udy1Test(PArray, *PIndex, NULL);
        if (ret == 0)
            return(Judy1Next(PArray, PIndex, NULL));
	return(1);			// found *PIndex itself.
#else
	{
	    PPvoid_t PValue;

	    PValue = j__udyLGet(PArray, *PIndex, NULL);
	    if (PValue != (PPvoid_t) NULL) 
                return(PValue);  // found *PIndex.
	    return(JudyLNext(PArray, PIndex, NULL));
	}
#endif

} // Judy1First() / JudyLFirst()


// ****************************************************************************
// J U D Y   1   L A S T
// J U D Y   L   L A S T
//
// See the manual entry for details.

#ifdef JUDY1
FUNCTION int	  Judy1Last
#else
FUNCTION PPvoid_t JudyLLast
#endif
(
	Pcvoid_t  PArray,	// Judy array to search.
	Word_t *  PIndex,	// starting point and result.
	PJError_t PJError)	// optional, for returning error info.
{
        if (PIndex == (PWord_t) NULL)
            return(0);

#ifdef JUDY1
	int ret = j__udy1Test(PArray, *PIndex, NULL);
        if (ret == 0) 
            return(Judy1Prev(PArray, PIndex, NULL));
        return(1);
#else
	PPvoid_t PValue = j__udyLGet(PArray, *PIndex, NULL);
        if (PValue != (PPvoid_t) NULL) 
            return(PValue);  // found *PIndex.
	return(JudyLPrev(PArray, PIndex, NULL));
#endif

} // Judy1Last() / JudyLLast()


// ****************************************************************************
// J U D Y   1   F I R S T   E M P T Y
// J U D Y   L   F I R S T   E M P T Y
//
// See the manual entry for details.

#ifdef JUDY1
FUNCTION int Judy1FirstEmpty(
	Pcvoid_t  PArray,	// Judy array to search.
	Word_t *  PIndex,	// starting point and result.
	PJError_t PJError)	// optional, for returning error info.
#else
FUNCTION int JudyLFirstEmpty(
	Pcvoid_t  PArray,	// Judy array to search.
	Word_t *  PIndex,	// starting point and result.
	PJError_t PJError)	// optional, for returning error info.
#endif
{
        if (PIndex == (PWord_t) NULL)		// caller error:
            return(0);

#ifdef JUDY1
	int ret = j__udy1Test(PArray, *PIndex, NULL);
        if (ret == 0)
	    return(1);			// found *PIndex itself.
	return(Judy1NextEmpty(PArray, PIndex, NULL));
#else
	{
	    PPvoid_t PValue = j__udyLGet(PArray, *PIndex, NULL);
	    if (PValue == (PPvoid_t) NULL) 
                return(1);	// found *PIndex.
	    return(JudyLNextEmpty(PArray, PIndex, NULL));
	}
#endif

} // Judy1FirstEmpty() / JudyLFirstEmpty()


// ****************************************************************************
// J U D Y   1   L A S T   E M P T Y
// J U D Y   L   L A S T   E M P T Y
//
// See the manual entry for details.

#ifdef JUDY1
FUNCTION int Judy1LastEmpty(
	Pcvoid_t  PArray,	// Judy array to search.
	Word_t *  PIndex,	// starting point and result.
	PJError_t PJError)	// optional, for returning error info.
#else
FUNCTION int JudyLLastEmpty(
	Pcvoid_t  PArray,	// Judy array to search.
	Word_t *  PIndex,	// starting point and result.
	PJError_t PJError)	// optional, for returning error info.
#endif
{
        if (PIndex == (PWord_t) NULL)
            return(1);

#ifdef JUDY1
	int ret = j__udy1Test(PArray, *PIndex, NULL);
        if (ret == 0)
            return(1);
	return(Judy1PrevEmpty(PArray, PIndex, NULL));
#else
	PPvoid_t PValue = j__udyLGet(PArray, *PIndex, NULL);

        if (PValue == (PPvoid_t) NULL) 
            return(1);	// found *PIndex.
	return(JudyLPrevEmpty(PArray, PIndex, NULL));
#endif

} // Judy1LastEmpty() / JudyLLastEmpty()
