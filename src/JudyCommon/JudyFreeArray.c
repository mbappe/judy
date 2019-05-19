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

// @(#) $Revision: 4.51 $ $Source: /judy/src/JudyCommon/JudyFreeArray.c $
//
// Judy1FreeArray() and JudyLFreeArray() functions for Judy1 and JudyL.
// Compile with one of -DJUDY1 or -DJUDYL.
// Return the number of bytes freed from the array.

#if (! (defined(JUDY1) || defined(JUDYL)))
#error:  One of -DJUDY1 or -DJUDYL must be specified.
#endif

#ifdef JUDY1
#include "Judy1.h"
#else
#include "JudyL.h"
#endif

#include "JudyPrivate1L.h"


// ****************************************************************************
// J U D Y   1   F R E E   A R R A Y
// J U D Y   L   F R E E   A R R A Y
//
// See the Judy*(3C) manual entry for details.
//
// This code is written recursively, at least at first, because thats much
// simpler.  Hope its fast enough.

#ifdef JUDY1
FUNCTION Word_t Judy1FreeArray
#else
FUNCTION Word_t JudyLFreeArray
#endif
        (
	PPvoid_t  PPArray,	// array to free.
	PJError_t PJError	// optional, for returning error info.
        )
{
	jpm_t	  jpm;		// local to accumulate free statistics.

// CHECK FOR NULL POINTER (error by caller):

	if (PPArray == (PPvoid_t) NULL)
	{
	    JU_SET_ERRNO(PJError, JU_ERRNO_NULLPPARRAY);
	    return(JERR);
	}


// Zero jpm.jpm_Pop0 (meaning the array will be empty in a moment) for accurate
// logging in TRACEMI2.

	jpm.jpm_Pop0	      = 0;		// see above.
	jpm.jpm_TotalMemWords = 0;		// initialize memory freed.

// 	Empty array:

	if (P_JLL8(*PPArray) == (Pjll8_t) NULL) return(0);

// PROCESS TOP LEVEL "JRP" BRANCHES AND LEAF:

	if (JU_LEAF8_POP0(*PPArray) < cJU_LEAF8_MAXPOP1) // must be a LEAF8
	{
	    Pjll8_t Pjll8 = P_JLL8(*PPArray);	// first word of leaf.

	    j__udyFreeJLL8(Pjll8, Pjll8->jl8_Population0 + 1, &jpm);
	    *PPArray = (Pvoid_t) NULL;		// make an empty array.
	    return (-(jpm.jpm_TotalMemWords * cJU_BYTESPERWORD));  // see above.
	}
	else

// Rootstate leaves:  just free the leaf:

// Common code for returning the amount of memory freed.
//
// Note:  In a an ordinary LEAF8, pop0 = *PPArray[0].
//
// Accumulate (negative) words freed, while freeing objects.
// Return the positive bytes freed.

	{
	    Pjpm_t Pjpm	    = P_JPM(*PPArray);
	    Word_t TotalMem = Pjpm->jpm_TotalMemWords;

//	    j__udyFreeSM(&(Pjpm->jpm_JP), &jpm);  // recurse through tree.
	    j__udyFreeSM((Pjpm->jpm_JP + 0), &jpm);  // recurse through tree.
	    j__udyFreeJPM(Pjpm, &jpm);

// Verify the array was not corrupt.  This means that amount of memory freed
// (which is negative) is equal to the initial amount:

	    if (TotalMem + jpm.jpm_TotalMemWords)
	    {
	        *PPArray = (Pvoid_t) NULL;		// make an empty array.
		JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
		return(JERR);
	    }

	    *PPArray = (Pvoid_t) NULL;		// make an empty array.
	    return (TotalMem * cJU_BYTESPERWORD);
	}

} // Judy1FreeArray() / JudyLFreeArray()

#ifdef  DEBUG
        Word_t Line;
#endif  // DEBUG

// ****************************************************************************
// __ J U D Y   F R E E   S M
//
// Given a pointer to a JP, recursively visit and free (depth first) all nodes
// in a Judy array BELOW the JP, but not the JP itself.  Accumulate in *Pjpm
// the total words freed (as a negative value).  "SM" = State Machine.
//
// Note:  Corruption is not detected at this level because during a FreeArray,
// if the code hasnt already core dumped, its better to remain silent, even
// if some memory has not been freed, than to bother the caller about the
// corruption.  TBD:  Is this true?  If not, must list all legitimate JPNULL
// and JPIMMED above first, and revert to returning bool_t (see 4.34).

FUNCTION void j__udyFreeSM(
	Pjp_t	Pjp,		// top of Judy (top-state).
	Pjpm_t	Pjpm)		// to return words freed.
{
	Word_t	Pop1;


	switch (ju_Type(Pjp))
	{
        case    cJU_JPNULL1:            // Index Size 1 bytes 
        case    cJU_JPNULL2:            // Index Size 2 bytes 
        case    cJU_JPNULL3:            // Index Size 3 bytes 
        case    cJU_JPNULL4:            // Index Size 4 bytes 
        case    cJU_JPNULL5:            // Index Size 5 bytes 
        case    cJU_JPNULL6:            // Index Size 6 bytes 
        case    cJU_JPNULL7:            // Index Size 7 bytes 
        case    cJU_JPIMMED_1_01:       // Index Size = 1, Pop1 = 1.
        case    cJU_JPIMMED_2_01:       // Index Size = 2, Pop1 = 1.
        case    cJU_JPIMMED_3_01:       // Index Size = 3, Pop1 = 1.
        case    cJU_JPIMMED_4_01:       // Index Size = 4, Pop1 = 1.
        case    cJU_JPIMMED_5_01:       // Index Size = 5, Pop1 = 1.
        case    cJU_JPIMMED_6_01:       // Index Size = 6, Pop1 = 1.
        case    cJU_JPIMMED_7_01:       // Index Size = 7, Pop1 = 1.

#ifdef  JUDY1
        case    cJ1_JPIMMED_1_02:       // Index Size = 1, Pop1 = 2.
        case    cJ1_JPIMMED_1_03:       // Index Size = 1, Pop1 = 3.
        case    cJ1_JPIMMED_1_04:       // Index Size = 1, Pop1 = 4.
        case    cJ1_JPIMMED_1_05:       // Index Size = 1, Pop1 = 5.
        case    cJ1_JPIMMED_1_06:       // Index Size = 1, Pop1 = 6.
        case    cJ1_JPIMMED_1_07:       // Index Size = 1, Pop1 = 7.
        case    cJ1_JPIMMED_1_08:       // Index Size = 1, Pop1 = 8.
        case    cJ1_JPIMMED_1_09:       // Index Size = 1, Pop1 = 9.
        case    cJ1_JPIMMED_1_10:       // Index Size = 1, Pop1 = 10.
        case    cJ1_JPIMMED_1_11:       // Index Size = 1, Pop1 = 11.
        case    cJ1_JPIMMED_1_12:       // Index Size = 1, Pop1 = 12.
        case    cJ1_JPIMMED_1_13:       // Index Size = 1, Pop1 = 13.
        case    cJ1_JPIMMED_1_14:       // Index Size = 1, Pop1 = 14.
        case    cJ1_JPIMMED_1_15:       // Index Size = 1, Pop1 = 15.
        case    cJ1_JPIMMED_2_02:       // Index Size = 2, Pop1 = 2.
        case    cJ1_JPIMMED_2_03:       // Index Size = 2, Pop1 = 3.
        case    cJ1_JPIMMED_2_04:       // Index Size = 2, Pop1 = 4.
        case    cJ1_JPIMMED_2_05:       // Index Size = 2, Pop1 = 5.
        case    cJ1_JPIMMED_2_06:       // Index Size = 2, Pop1 = 6.
        case    cJ1_JPIMMED_2_07:       // Index Size = 2, Pop1 = 7.
        case    cJ1_JPIMMED_3_02:       // Index Size = 3, Pop1 = 2.
        case    cJ1_JPIMMED_3_03:       // Index Size = 3, Pop1 = 3.
        case    cJ1_JPIMMED_3_04:       // Index Size = 3, Pop1 = 4.
        case    cJ1_JPIMMED_3_05:       // Index Size = 3, Pop1 = 5.
        case    cJ1_JPIMMED_4_02:       // Index Size = 4, Pop1 = 2.
        case    cJ1_JPIMMED_4_03:       // Index Size = 4, Pop1 = 3.
        case    cJ1_JPIMMED_5_02:       // Index Size = 5, Pop1 = 2.
        case    cJ1_JPIMMED_5_03:       // Index Size = 5, Pop1 = 3.
        case    cJ1_JPIMMED_6_02:       // Index Size = 6, Pop1 = 2.
        case    cJ1_JPIMMED_7_02:       // Index Size = 7, Pop1 = 2.
	case    cJ1_JPFULLPOPU1:        // FULL EXPANSE    Pop1 = 256
#endif  // JUDY1
#ifdef  DEBUG
            Line = __LINE__;
#endif  // DEBUG
	    break;              // no memory to Free

// JUDY BRANCH -- free the sub-tree depth first:

// LINEAR BRANCH -- visit each JP in the JBLs list, then free the JBL:
//
// Note:  There are no null JPs in a JBL.

        case cJU_JPBRANCH_L8:
	case cJU_JPBRANCH_L2:
	case cJU_JPBRANCH_L3:
	case cJU_JPBRANCH_L4:
	case cJU_JPBRANCH_L5:
	case cJU_JPBRANCH_L6:
	case cJU_JPBRANCH_L7:
	{
#ifdef  DEBUG
            Line = __LINE__;
#endif  // DEBUG
	    Pjbl_t Pjbl = P_JBL(ju_PntrInJp(Pjp));
	    Word_t offset;

	    for (offset = 0; offset < Pjbl->jbl_NumJPs; offset++)
	        j__udyFreeSM(Pjbl->jbl_jp + offset, Pjpm);

	    j__udyFreeJBL(ju_PntrInJp(Pjp), Pjpm);
	    break;
	}


// BITMAP BRANCH -- visit each JP in the JBBs list based on the bitmap, also
//
// Note:  There are no null JPs in a JBB.

	case cJU_JPBRANCH_B8:
	case cJU_JPBRANCH_B2:
	case cJU_JPBRANCH_B3:
	case cJU_JPBRANCH_B4:
	case cJU_JPBRANCH_B5:
	case cJU_JPBRANCH_B6:
	case cJU_JPBRANCH_B7:
	{
#ifdef  DEBUG
            Line = __LINE__;
#endif  // DEBUG
	    Word_t subexp;
	    Word_t offset;
	    Word_t jpcount;

	    Pjbb_t Pjbb = P_JBB(ju_PntrInJp(Pjp));

	    for (subexp = 0; subexp < cJU_NUMSUBEXPB; ++subexp)
	    {
	        jpcount = j__udyCountBitsB(JU_JBB_BITMAP(Pjbb, subexp));

	        if (jpcount)
	        {
		    for (offset = 0; offset < jpcount; ++offset)
		    {
		       j__udyFreeSM(P_JP(JU_JBB_PJP(Pjbb, subexp)) + offset, Pjpm);
		    }
		    j__udyFreeJBBJP(JU_JBB_PJP(Pjbb, subexp), jpcount, Pjpm);
	        }
	    }
	    j__udyFreeJBB(ju_PntrInJp(Pjp), Pjpm);

	    break;
	}


// UNCOMPRESSED BRANCH -- visit each JP in the JBU array, then free the JBU
// itself:
//
// Note:  Null JPs are handled during recursion at a lower state.

	case cJU_JPBRANCH_U8:
	case cJU_JPBRANCH_U2:
	case cJU_JPBRANCH_U3:
	case cJU_JPBRANCH_U4:
	case cJU_JPBRANCH_U5:
	case cJU_JPBRANCH_U6:
	case cJU_JPBRANCH_U7:
	{
#ifdef  DEBUG
            Line = __LINE__;
#endif  // DEBUG
	    Word_t offset;
	    Pjbu_t Pjbu = P_JBU(ju_PntrInJp(Pjp));

	    for (offset = 0; offset < cJU_BRANCHUNUMJPS; ++offset)
	        j__udyFreeSM(Pjbu->jbu_jp + offset, Pjpm);

	    j__udyFreeJBU(ju_PntrInJp(Pjp), Pjpm);
	    break;
	}


// -- Cases below here terminate and do not recurse. --


// LINEAR LEAF -- just free the leaf; size is computed from jp_Type:
//
// Note:  cJU_JPLEAF1 is a special case, see discussion in ../Judy1/Judy1.h

#ifdef  JUDYL
	case cJU_JPLEAF1:       // only JudyL
#ifdef  DEBUG
            Line = __LINE__;
#endif  // DEBUG
	    Pop1 = ju_LeafPop1(Pjp);
            if (Pop1 == 0) Pop1 = 256;
	    j__udyFreeJLL1(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    break;
#endif  // JUDYL

	case cJU_JPLEAF2:
#ifdef  DEBUG
            Line = __LINE__;
#endif  // DEBUG
	    Pop1 = ju_LeafPop1(Pjp);
	    j__udyFreeJLL2(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    break;

	case cJU_JPLEAF3:
#ifdef  DEBUG
            Line = __LINE__;
#endif  // DEBUG
	    Pop1 = ju_LeafPop1(Pjp);
	    j__udyFreeJLL3(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    break;

	case cJU_JPLEAF4:
#ifdef  DEBUG
            Line = __LINE__;
#endif  // DEBUG
	    Pop1 = ju_LeafPop1(Pjp);
	    j__udyFreeJLL4(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    break;

	case cJU_JPLEAF5:
#ifdef  DEBUG
            Line = __LINE__;
#endif  // DEBUG
	    Pop1 = ju_LeafPop1(Pjp);
	    j__udyFreeJLL5(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    break;

	case cJU_JPLEAF6:
#ifdef  DEBUG
            Line = __LINE__;
#endif  // DEBUG
	    Pop1 = ju_LeafPop1(Pjp);
	    j__udyFreeJLL6(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    break;

	case cJU_JPLEAF7:
#ifdef  DEBUG
            Line = __LINE__;
#endif  // DEBUG
	    Pop1 = ju_LeafPop1(Pjp);
	    j__udyFreeJLL7(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    break;


// BITMAP LEAF -- free sub-expanse arrays of JPs, then free the JBB.

#ifdef  OBSOLETE
#ifdef  JUDYL
        case cJL_JPLEAF_B1_UCOMP:
        {
#ifdef  DEBUG
            Line = __LINE__;
#endif  // DEBUG

	    j__udyFreeJLB1(ju_PntrInJp(Pjp), Pjpm);
            break;
	} // case cJL_JPLEAF_B1_UCOMP
#endif  // JUDYL
#endif  // OBSOLETE

	case cJU_JPLEAF_B1:
	{
#ifdef  DEBUG
            Line = __LINE__;
#endif  // DEBUG

	    Pop1 = ju_LeafPop1(Pjp);
	    j__udyFreeJLB1(ju_PntrInJp(Pjp), Pjpm);
	    break;

	} // case cJU_JPLEAF_B1

#ifdef JUDYL

// IMMED*:
//
// For JUDYL, all non JPIMMED_*_01s have a LeafV which must be freed:

	case cJL_JPIMMED_1_02:
	case cJL_JPIMMED_1_03:
	case cJL_JPIMMED_1_04:
	case cJL_JPIMMED_1_05:
	case cJL_JPIMMED_1_06:
	case cJL_JPIMMED_1_07:
	case cJL_JPIMMED_1_08:
	    Pop1 = ju_Type(Pjp) - cJU_JPIMMED_1_02 + 2;
	    j__udyLFreeJV(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    break;

	case cJL_JPIMMED_2_02:
	case cJL_JPIMMED_2_03:
	case cJL_JPIMMED_2_04:

	    Pop1 = ju_Type(Pjp) - cJU_JPIMMED_2_02 + 2;
	    j__udyLFreeJV(ju_PntrInJp(Pjp), Pop1, Pjpm);
	    break;

	case cJL_JPIMMED_3_02:
	    j__udyLFreeJV(ju_PntrInJp(Pjp), 2, Pjpm);
	    break;

	case cJL_JPIMMED_4_02:
	    j__udyLFreeJV(ju_PntrInJp(Pjp), 2, Pjpm);
	    break;
#endif // JUDYL


// OTHER JPNULL, JPIMMED, OR UNEXPECTED TYPE -- nothing to free for this type:
//
// Note:  Lump together no-op and invalid JP types; see function header
// comments.

	default: 
            break;

	} // switch (ju_Type(Pjp))

} // j__udyFreeSM()

// Dump the jp path to the subtree specified by wKeyPrefix and
// nBitsLeft and the entire subtree below.
FUNCTION void
#ifdef JUDY1
Judy1Dump
#else
JudyLDump
#endif
    (
    Word_t wRoot,     // root word of array
    int nBitsLeft,    // level of subtree to dump
    Word_t wKeyPrefix // prefix of subtree to dump
    )
{
    // later
}
