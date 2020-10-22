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
// @(#) $Revision: 4.116 $ $Source: /judy/src/JudyCommon/JudyIns.c $
//
// Judy1Set() and JudyLIns() functions for Judy1 and JudyL.
// Compile with one of -DJUDY1 or -DJUDYL.
//
// These are defined to generic values in JudyCommon/JudyPrivateTypes.h:
//
// TBD:  These should be exported from a header file, but perhaps not, as they
// are only used here, and exported from Judy*Decascade, which is a separate
// file for profiling reasons (to prevent inlining), but which potentially
// could be merged with this file, either in SoftCM or at compile-time.

#if (defined(JUDY1) == defined(JUDYL))
#error:  EXACTLY one of -DJUDY1 and -DJUDYL must be specified.
#endif


#ifdef JUDY1
#include "Judy1.h"
#else
#include "JudyL.h"
#endif

#include "JudyPrivate1L.h"

#ifdef TRACEJPI
#include "JudyPrintJP.c"
#endif  // TRACEJPI

// WARNING: Length of PDST must be 2 words longer that POP1
#define JU_INSERTCOPYtoWORD(PDST,PSRC,POP1,OFFSET,INDEX,cIS)    \
    assert((long)(POP1) > 0);                                   \
    assert((Word_t)(OFFSET) <= (Word_t)(POP1));                 \
    {                                                           \
        uint8_t *_Psrc = (uint8_t *)(PSRC);                     \
        PWord_t  _Pdst =  PDST;                                 \
        Word_t   __key;                                         \
        int _idx;                                               \
        for (_idx = 0; _idx < (OFFSET); _idx++)                 \
        {                                                       \
            JU_COPY3_PINDEX_TO_LONG(__key, _Psrc);              \
            *(_Pdst)++ = __key;                                 \
            _Psrc  += cIS;                                      \
        }                                                       \
        *(_Pdst)++  = (INDEX) & JU_LEASTBYTESMASK(cIS);         \
        for ( ; _idx < (POP1); _idx++)                          \
        {                                                       \
            JU_COPY3_PINDEX_TO_LONG(__key, _Psrc);              \
            *(_Pdst)++ = __key;                                 \
            _Psrc  += cIS;                                      \
        }                                                       \
    }

#ifdef JUDY1
extern Word_t j__udy1PartstoBranchB(Pjp_t, uint8_t *, Word_t, Pjpm_t);
extern int j__udy1ConvertBranchBtoU(Pjp_t, Pjpm_t);
extern int j__udy1ConvertBranchLtoU(Pjp_t, Pjpm_t);
extern int j__udy1Cascade2(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1Cascade3(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1Cascade4(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1Cascade5(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1Cascade6(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1Cascade7(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1Cascade8(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1InsertBranchL(Pjp_t Pjp, Word_t Index, Word_t Dcd, Word_t Btype, Pjpm_t);
#else /* JUDYL */
extern Word_t j__udyLPartstoBranchB(Pjp_t, uint8_t *, Word_t, Pjpm_t);
extern int j__udyLConvertBranchBtoU(Pjp_t, Pjpm_t);
extern int j__udyLConvertBranchLtoU(Pjp_t, Pjpm_t);
extern int j__udyLCascade2(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLCascade3(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLCascade4(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLCascade5(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLCascade6(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLCascade7(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLCascade8(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLInsertBranchL(Pjp_t Pjp, Word_t Index, Word_t Dcd, Word_t Btype, Pjpm_t);
#endif /* JUDYL */


#ifdef JUDY1
#define j__udyJPPop1 j__udy1JPPop1
#endif  // JUDY1

#ifdef  JUDYL
#define j__udyJPPop1 j__udyLJPPop1
#endif  // JUDYL

Word_t static j__udyJPPop1(Pjp_t Pjp);

// ****************************************************************************
// J U D Y   J P   P O P 1
//
// This function takes any type of JP other than a root-level JP (cJU_LEAF8* or
// cJU_JPBRANCH* with no number suffix) and extracts the Pop1 from it.  In some
// sense this is a wrapper around the JU_JP*_POP0 macros.  Why write it as a
// function instead of a complex macro containing a trinary?  (See version
// Judy1.h version 4.17.)  We think its cheaper to call a function containing
// a switch statement with "constant" cases than to do the variable
// calculations in a trinary.
//
// For invalid JP Types return cJU_ALLONES.  Note that this is an impossibly
// high Pop1 for any JP below a top level branch.

FUNCTION Word_t j__udyJPPop1(
     	Pjp_t Pjp)		// JP to count.
{
	switch (ju_Type(Pjp))
	{
	case cJU_JPNULL1:
	case cJU_JPNULL2:
	case cJU_JPNULL3:
	case cJU_JPNULL4:
	case cJU_JPNULL5:
	case cJU_JPNULL6:
	case cJU_JPNULL7:  
            return(0);

	case cJU_JPBRANCH_L2:
	case cJU_JPBRANCH_B2:
	case cJU_JPBRANCH_U2:
            return(ju_BranchPop0(Pjp, 2) + 1);

	case cJU_JPBRANCH_L3:
	case cJU_JPBRANCH_B3:
	case cJU_JPBRANCH_U3:
            return(ju_BranchPop0(Pjp, 3) + 1);

	case cJU_JPBRANCH_L4:
	case cJU_JPBRANCH_B4:
	case cJU_JPBRANCH_U4: 
            return(ju_BranchPop0(Pjp, 4) + 1);

	case cJU_JPBRANCH_L5:
	case cJU_JPBRANCH_B5:
	case cJU_JPBRANCH_U5: 
            return(ju_BranchPop0(Pjp, 5) + 1);

	case cJU_JPBRANCH_L6:
	case cJU_JPBRANCH_B6:
	case cJU_JPBRANCH_U6: 
            return(ju_BranchPop0(Pjp, 6) + 1);

	case cJU_JPBRANCH_L7:
	case cJU_JPBRANCH_B7:
	case cJU_JPBRANCH_U7: 
//      256 ^ 6 is as big of Pop possible (for now)
            return(ju_BranchPop0(Pjp, 7) + 1);

	case cJU_JPBRANCH_L8:
	case cJU_JPBRANCH_B8:
	case cJU_JPBRANCH_U8: 
//      256 ^ 6 is as big of Pop possible (for now)
            return(ju_BranchPop0(Pjp, 8) + 1);

#ifdef  JUDYL
	case cJU_JPLEAF1:
#endif  // JUDYL
	case cJU_JPLEAF2:
	case cJU_JPLEAF3:
	case cJU_JPLEAF4:
	case cJU_JPLEAF5:
	case cJU_JPLEAF6:
	case cJU_JPLEAF7:
             return(ju_LeafPop1(Pjp));
	case cJU_JPLEAF_B1U:
        {
            Word_t pop1 = ju_LeafPop1(Pjp);
#ifdef  JUDYL
            if (pop1 == 0) pop1 = 256;
#endif  // JUDYL
             return(pop1);
        }

#ifdef JUDY1
	case cJ1_JPFULLPOPU1:	
           return(cJU_JPFULLPOPU1_POP0 + 1);
#endif  // JUDY1
	case cJU_JPIMMED_1_01:
	case cJU_JPIMMED_2_01:
	case cJU_JPIMMED_3_01:
	case cJU_JPIMMED_4_01:
	case cJU_JPIMMED_5_01:
	case cJU_JPIMMED_6_01:
	case cJU_JPIMMED_7_01:	
            return(1);

	case cJU_JPIMMED_1_02:  return(2);
	case cJU_JPIMMED_1_03:  return(3);
	case cJU_JPIMMED_1_04:  return(4);
	case cJU_JPIMMED_1_05:  return(5);
	case cJU_JPIMMED_1_06:  return(6);
	case cJU_JPIMMED_1_07:  return(7);
	case cJU_JPIMMED_1_08:  return(8);
#ifdef  JUDY1
	case cJ1_JPIMMED_1_09:  return(9);
	case cJ1_JPIMMED_1_10:  return(10);
	case cJ1_JPIMMED_1_11:  return(11);
	case cJ1_JPIMMED_1_12:  return(12);
	case cJ1_JPIMMED_1_13:  return(13);
	case cJ1_JPIMMED_1_14:  return(14);
	case cJ1_JPIMMED_1_15:  return(15);
#endif  // JUDY1
	case cJU_JPIMMED_2_02:  return(2);
	case cJU_JPIMMED_2_03:  return(3);
	case cJU_JPIMMED_2_04:  return(4);
#ifdef  JUDY1
	case cJ1_JPIMMED_2_05:  return(5);
	case cJ1_JPIMMED_2_06:  return(6);
	case cJ1_JPIMMED_2_07:  return(7);
#endif  // JUDY1

	case cJU_JPIMMED_3_02:  return(2);
#ifdef  JUDY1
	case cJ1_JPIMMED_3_03:  return(3);
	case cJ1_JPIMMED_3_04:  return(4);
	case cJ1_JPIMMED_3_05:  return(5);
#endif  // JUDY1

	case cJU_JPIMMED_4_02:	return(2);

#ifdef  JUDY1
	case cJ1_JPIMMED_4_03:	return(3);

	case cJ1_JPIMMED_5_02:
	case cJ1_JPIMMED_6_02:
	case cJ1_JPIMMED_7_02:	return(2);
#endif  // JUDY1

	default:
//printf("in j__udyJPPop1 jp_Type = %d Line = %d\n", ju_Type(Pjp), (int)__LINE__);
                assert(FALSE);
                exit(-1);
	}
	/*NOTREACHED*/
} // j__udyJPPop1()

#ifdef  DEBUG_COUNT
static void j__udyCheckBranchPop(Pjp_t _Pjp, int _Level)
{
//        printf("===============================ju_Type(_Pjp) = %d\n", ju_Type(_Pjp));
        Pjbu_t    Pjbu = P_JBU(ju_PntrInJp(_Pjp));

        Word_t BranchUPop = ju_BranchPop0(_Pjp, _Level) + 1;
        Word_t branchsum  = 0;
        Word_t Bigsum     = 0;
        for (int numjpU = 0; numjpU < 256; numjpU++)
        {
            Word_t jpsum = j__udyJPPop1(Pjbu->jbu_jp + numjpU);
            branchsum += jpsum;
        }
        if (branchsum != BranchUPop)
        {
            printf("\n--OOps1-JudyIns JudyBranch_U%d Pop_jp = %ld, branchsum = %ld, diff = %ld\n", _Level, BranchUPop, branchsum, BranchUPop - branchsum);
            assert(branchsum == BranchUPop);
        }
}
#endif  // DEBUG_COUNT

// ****************************************************************************
// __ J U D Y   I N S   W A L K
//
// Walk the Judy tree to do a set/insert.  This is only called internally, and
// recursively.  Unlike Judy1Test() and JudyLGet(), the extra time required for
// recursion should be negligible compared with the total.
//
// Return -1 for error (details in JPM), 0 for Index already inserted, 1 for
// new Index inserted.
FUNCTION static int
j__udyInsWalk(Pjp_t  Pjp,               // current Pjp_t to descend.
	      Pjp_t  PPjp,
              Word_t Index,             // to insert.
              Pjpm_t Pjpm)              // for returning info to top Level.
{
    jp_t      newJP;                    // for creating a new Immed JP.
    Word_t    exppop1;                  // expanse (leaf) population.
    Word_t    RawJpPntr;                // Raw pointer in most jp_ts
    int       retcode;                  // return codes:  -1, 0, 1.
    uint8_t   digit;                    // from Index, current offset into a branch.
    uint8_t   jpLevel = 0;

  ContinueInsWalk:                      // for modifying state without recursing.

#ifdef TRACEJPI
        JudyPrintJP(Pjp, "i", __LINE__, Index, Pjpm);
#endif  // TRACEJPI

    RawJpPntr = ju_PntrInJp(Pjp);
    switch (ju_Type(Pjp))             // entry:  Pjp, Index.
    {
// ****************************************************************************
// JPNULL*:
//
// Convert JP in place from current null type to cJU_JPIMMED_*_01 by
// calculating new JP type.
    case cJU_JPNULL1:
    case cJU_JPNULL2:
    case cJU_JPNULL3:
    case cJU_JPNULL4:
    case cJU_JPNULL5:
    case cJU_JPNULL6:
    case cJU_JPNULL7:
    {
        assert(RawJpPntr == 0);
//      convert NULL to IMMED_1 a the same Level
        jpLevel = ju_Type(Pjp) - cJU_JPNULL1 + 1;
        ju_SetIMM01(Pjp, 0, Index, cJU_JPIMMED_1_01 - 1 + jpLevel);

#ifdef TRACEJPI
        JudyPrintJP(Pjp, "i", __LINE__, Index, Pjpm);
#endif  // TRACEJPI

#ifdef JUDYL
//      Value word is 2nd word of new Immed_*_01 jp_t:
        Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);          // ^ to Immed Value
#endif /* JUDYL */
        goto successreturn;
    }
// ****************************************************************************
// JPBRANCH_L*:
//
// If the new Index is not an outlier to the branchs expanse, and the branch
// should not be converted to uncompressed, extract the digit and record the
// Immediate type to create for a new Immed JP, before going to common code.
//
// Note:  JU_CHECK_IF_OUTLIER() is a no-op for BranchB3[7] on 32[64]-bit.
    case cJU_JPBRANCH_L2:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 2))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 2, Pjpm));
        digit = JU_DIGITATSTATE(Index, 2);
        exppop1 = ju_BranchPop0(Pjp, 2);
        jpLevel = 2;
        goto JudyBranchL;
    }

    case cJU_JPBRANCH_L3:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 3))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 3, Pjpm));
        digit = JU_DIGITATSTATE(Index, 3);
        exppop1 = ju_BranchPop0(Pjp, 3);
        jpLevel = 3;
        goto JudyBranchL;
    }

    case cJU_JPBRANCH_L4:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 4))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 4, Pjpm));
        digit = JU_DIGITATSTATE(Index, 4);
        exppop1 = ju_BranchPop0(Pjp, 4);
        jpLevel = 4;
        goto JudyBranchL;
    } 

    case cJU_JPBRANCH_L5:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 5))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 5, Pjpm));
        digit = JU_DIGITATSTATE(Index, 5);
        exppop1 = ju_BranchPop0(Pjp, 5);
        jpLevel = 5;
        goto JudyBranchL;
    }

    case cJU_JPBRANCH_L6:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 6))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 6, Pjpm));
        digit = JU_DIGITATSTATE(Index, 6);
        exppop1 = ju_BranchPop0(Pjp, 6);
        jpLevel = 6;
        goto JudyBranchL;
    }

    case cJU_JPBRANCH_L7:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 7))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 7, Pjpm));
        digit = JU_DIGITATSTATE(Index, 7);
        exppop1 = ju_BranchPop0(Pjp, 7);
        jpLevel = 7;
        goto JudyBranchL;
    }

// Similar to common code above, but no outlier check is needed, and the Immed
// type depends on the word size:
    case cJU_JPBRANCH_L8:
    {
        Word_t  PjblRaw;
        Word_t  PjbuRaw;
        digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
        exppop1 = Pjpm->jpm_Pop0;
        jpLevel = 8;

        // fall through:
// COMMON CODE FOR LINEAR BRANCHES:
//
// Come here with digit and exppop1 already set.
   JudyBranchL:
        PjblRaw         = RawJpPntr;
        Pjbl_t  Pjbl    = P_JBL(PjblRaw);
        Word_t  numJPs  = Pjbl->jbl_NumJPs;

        exppop1++;

#ifdef  DEBUG_COUNT
    {
        int ii = 0;
        Word_t branchsum;
        for (branchsum = 0; ii < numJPs; ii++)
            branchsum += j__udyJPPop1(Pjbl->jbl_jp + ii);
        if (branchsum != exppop1)
            printf("--------JudyIns JudyBranch_L%d exppop1 = %ld, branchsum = %ld, diff = %ld\n", jpLevel, exppop1, branchsum, exppop1 - branchsum);
        assert(branchsum == exppop1);
    }
#endif  // DEBUG_COUNT

//      If population under this branch greater than:
//      Convert to BranchU
        if ((exppop1 > JU_BRANCHL_MAX_POP) && (numJPs != 1))
        {
            int ret = j__udyConvertBranchLtoU(Pjp, Pjpm);
            if (ret == -1)
                return (-1);
            goto ContinueInsWalk;           // now insert Index into BRANCH_U
        }
//      else
//      Search for a match to the digit:
        int offset = j__udySearchBranchL(Pjbl->jbl_Expanse, numJPs, digit);

#ifdef xxxxx
        if (offset >= 0)
        {
            Word_t PjbbRaw = j__udyPartstoBranchB(Pjbl->jbl_jp, Pjbl->jbl_Expanse, numJPs, Pjpm);
            if (PjbbRaw == 0)
                return (-1);
            j__udyFreeJBL(PjblRaw, Pjpm);   // free old BranchL.
//          Convert jp_Type from linear branch to equivalent bitmap branch:
            ju_SetJpType(Pjp, ju_Type(Pjp) + cJU_JPBRANCH_B2 - cJU_JPBRANCH_L2);
            ju_SetPntrInJp(Pjp, PjbbRaw);
//          Having changed branch types, now do the insert in the new BranchB:
            goto ContinueInsWalk;
        }
#endif  // xxxxx

//      If Index is found, offset is into an array of 1..cJU_BRANCHLMAXJPS JPs:
        if (offset >= 0)
        {
            Pjp  = Pjbl->jbl_jp + offset;       // decode to next level
            break;                              // continue walk.
        }
//      else  Expanse is missing in BranchL for the passed Index, so insert an Immed
        if (numJPs < cJU_BRANCHLMAXJPS)         // -- if theres room:
        {
            offset  = ~offset;                  // insertion offset.
            jpLevel = ju_Type(Pjp) - cJU_JPBRANCH_L2 + 2;

//          Value == 0 in both Judy1 and JudyL
            ju_SetIMM01(&newJP, 0, Index, cJU_JPIMMED_1_01 - 1 + jpLevel - 1);
            JU_INSERTINPLACE(Pjbl->jbl_Expanse, numJPs, offset, digit);
            JU_INSERTINPLACE(Pjbl->jbl_jp, numJPs, offset, newJP);
            Pjbl->jbl_NumJPs++;

#ifdef JUDYL
//          Value area is in the new Immed 01 JP:
            Pjp              = Pjbl->jbl_jp + offset;
            Pjpm->jpm_PValue =  ju_PImmVal_01(Pjp);     // ^ to Immed_01 Value
#endif /* JUDYL */

            goto successreturn;
        }
//      else 
//      MAXED OUT LINEAR BRANCH, CONVERT TO A BITMAP BRANCH, THEN INSERT:

//      Copy the linear branch to a bitmap branch.
        assert((numJPs) <= cJU_BRANCHLMAXJPS);
        Word_t PjbbRaw = j__udyPartstoBranchB(Pjbl->jbl_jp, Pjbl->jbl_Expanse, numJPs, Pjpm);
        if (PjbbRaw == 0)
            return (-1);
        j__udyFreeJBL(PjblRaw, Pjpm);   // free old BranchL.

//      Convert jp_Type from linear branch to equivalent bitmap branch:
//        ju_SetJpType(Pjp, ju_Type(Pjp) + cJU_JPBRANCH_B2 - cJU_JPBRANCH_L2);
        ju_SetJpType(Pjp, cJU_JPBRANCH_B2 - 2 + jpLevel);

        ju_SetPntrInJp(Pjp, PjbbRaw);
//      Having changed branch types, now do the insert in the new BranchB:
        goto ContinueInsWalk;           // no go do the Insert
    }                                   // case cJU_JPBRANCH_L.

// ****************************************************************************
// JPBRANCH_B*:
//
// If the new Index is not an outlier to the branchs expanse, extract the
// digit and record the Immediate type to create for a new Immed JP, before
// going to common code.
//
    case cJU_JPBRANCH_B2:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 2))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 2, Pjpm));
        digit = JU_DIGITATSTATE(Index, 2);
        exppop1 = ju_BranchPop0(Pjp, 2);
        goto JudyBranchB;
    }

    case cJU_JPBRANCH_B3:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 3))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 3, Pjpm));
        digit = JU_DIGITATSTATE(Index, 3);
        exppop1 = ju_BranchPop0(Pjp, 3);
        goto JudyBranchB;
    }

    case cJU_JPBRANCH_B4:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 4))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 4, Pjpm));
        digit = JU_DIGITATSTATE(Index, 4);
        exppop1 = ju_BranchPop0(Pjp, 4);
        goto JudyBranchB;
    }

    case cJU_JPBRANCH_B5:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 5))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 5, Pjpm));
        digit = JU_DIGITATSTATE(Index, 5);
        exppop1 = ju_BranchPop0(Pjp, 5);
        goto JudyBranchB;
    }

    case cJU_JPBRANCH_B6:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 6))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 6, Pjpm));
        digit = JU_DIGITATSTATE(Index, 6);
        exppop1 = ju_BranchPop0(Pjp, 6);
        goto JudyBranchB;
    }

    case cJU_JPBRANCH_B7:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 7))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 7, Pjpm));
        digit = JU_DIGITATSTATE(Index, 7);
        exppop1 = ju_BranchPop0(Pjp, 7);
        goto JudyBranchB;
    }

    case cJU_JPBRANCH_B8:
    {
        Pjbb_t    Pjbb;                 // pointer to bitmap branch.
        Word_t    PjbbRaw;              // pointer to bitmap branch.
        Word_t    Pjp2Raw;              // 1 of N arrays of JPs.
        Pjp_t     Pjp2;                 // 1 of N arrays of JPs.
        Word_t    sub4exp;              // 1 of N subexpanses in bitmap.
        BITMAPB_t bitmap;               // for one subexpanse.
        BITMAPB_t bitmask;              // bit set for Indexs digit.
        Word_t    numJPs;               // number of JPs = populated expanses.
        int       offset;               // in bitmap branch.
// Similar to common code above, but no outlier check is needed, and the Immed
// type depends on the word size:
        digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
        exppop1 = Pjpm->jpm_Pop0; // TEMP, should be ju_BranchPop0(Pjp, 8);
        // fall through:

// COMMON CODE FOR BITMAP BRANCHES:
//
// Come here with digit and exppop1 already set.

JudyBranchB:
#ifdef FIXIT
#ifdef  DEBUG_COUNT
    {
        Word_t branchsum = 0;
        for (int ii = 0; ii < numJPs; ii++) 
            branchsum += j__udyJPPop1(Pjbb->jbb_jp + ii);

        int jpLevel = ju_Type(Pjp) - cJU_JPBRANCH_B2 + 2;
        printf("#--------JudyIns JudyBranch_B%d exppop1 = %ld, branchsum = %ld\n", jpLevel, exppop1, branchsum);
        assert(branchsum == exppop1);
    }
#endif  // DEBUG_COUNT
#endif  // FIXIT

#ifdef noB
//      Convert branchB to branchU
        if (j__udyConvertBranchBtoU(Pjp, Pjpm) == -1) 
                 return(-1);
                goto ContinueInsWalk;
#endif  // noB

#ifndef  noU

//#ifndef  noINC
//          If population increment is greater than..  (150):
//            if ((Pjpm->jpm_Pop0 - Pjpm->jpm_LastUPop0) > JU_BTOU_POP_INCREMENT)
//#endif  // noINC

        {

// If total array efficency is greater than 3 words per Key, then convert

#ifdef  JUDYL
                if (((Pjpm->jpm_TotalMemWords + 512) * 100 / Pjpm->jpm_Pop0) < 300)
#endif  // JUDYL

#ifdef  JUDY1
// If total array efficency is greater than 2 words per Key, then convert
                if (((Pjpm->jpm_TotalMemWords + 512) * 100 / Pjpm->jpm_Pop0) < 200)
#endif  // JUDY1
                {

// If population under the branch is greater than..  (135):

                    if (exppop1 > JU_BRANCHB_MIN_POP)
                    {
                        if (j__udyConvertBranchBtoU(Pjp, Pjpm) == -1) 
                            return(-1);

// Save global population of last BranchU conversion:

//#ifndef  noINC
//                        Pjpm->jpm_LastUPop0 = Pjpm->jpm_Pop0;
//#endif  // noINC

                        goto ContinueInsWalk;
                    }
                }
        }
#endif  // ! noU

// CONTINUE TO USE BRANCHB:
//
// Get pointer to bitmap branch (JBB):
//        PjbbRaw = ju_PntrInJp(Pjp);
        PjbbRaw = RawJpPntr;
        Pjbb    = P_JBB(PjbbRaw);
// Form the Int32 offset, and Bit offset values:
//
// 8 bit Decode | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//              |SubExpanse |    Bit offset     |
//
// Get the 1 of 8 expanses from digit, Bits 5..7 = 1 of 8, and get the 32-bit
// word that may have a bit set:
        sub4exp = digit / cJU_BITSPERSUBEXPB;
        bitmap  = JU_JBB_BITMAP(Pjbb, sub4exp);
        Pjp2Raw = JU_JBB_PJP(Pjbb, sub4exp);
        Pjp2    = P_JP(Pjp2Raw);
// Get the bit position that represents the desired expanse, and get the offset
// into the array of JPs for the JP that matches the bit.
        bitmask = JU_BITPOSMASKB(digit);
        offset  = j__udyCountBitsB(bitmap & (bitmask - 1));
// If JP is already in this expanse, get Pjp and continue the walk:
        if (bitmap & bitmask)
        {
            Pjp = Pjp2 + offset;
            break;                      // continue walk.
        }
// ADD NEW EXPANSE FOR NEW INDEX:
//
// The new expanse always an cJU_JPIMMED_*_01 containing just the new Index, so
// finish setting up an Immed JP.
//      Convert from BranchB to Immed at one less level
        jpLevel = ju_Type(Pjp) - cJU_JPBRANCH_B2 + 2;
        ju_SetIMM01(&newJP, 0, Index,  cJU_JPIMMED_1_01 - 1 + jpLevel - 1);

#ifdef TRACEJPI
        JudyPrintJP(&newJP, "i", __LINE__, Index, Pjpm);
#endif  // TRACEJPI

// Get 1 of the 8 JP arrays and calculate number of JPs in subexpanse array:
        Pjp2Raw = JU_JBB_PJP(Pjbb, sub4exp);
        Pjp2 = P_JP(Pjp2Raw);
        numJPs = j__udyCountBitsB(bitmap);
// Expand branch JP subarray in-place:
        if (JU_BRANCHBJPGROWINPLACE(numJPs))
        {
            assert(numJPs > 0);
            JU_INSERTINPLACE(Pjp2, numJPs, offset, newJP);
#ifdef JUDYL
            Pjpm->jpm_PValue =  ju_PImmVal_01(Pjp2 + offset);        // ^ to Immed Values
#endif  // JUDYL
        }
//      No room, allocate a bigger bitmap branch JP subarray:
        else
        {
            Word_t    PjpnewRaw;
            Pjp_t     Pjpnew;
            if ((PjpnewRaw = j__udyAllocJBBJP(numJPs + 1, Pjpm)) == 0)
                return (-1);
            Pjpnew = P_JP(PjpnewRaw);
// If there was an old JP array, then copy it, insert the new Immed JP, and
// free the old array:
            if (numJPs)
            {
                JU_INSERTCOPY(Pjpnew, Pjp2, numJPs, offset, newJP);
                j__udyFreeJBBJP(Pjp2Raw, numJPs, Pjpm);

#ifdef JUDYL
                // value area is first word of new Immed 01 JP:
//                Pjpm->jpm_PValue = (Pjv_t) (Pjpnew + offset);
                Pjpm->jpm_PValue =  ju_PImmVal_01(Pjpnew + offset);        // ^ to Immed Values
#endif /* JUDYL */
            }
// New JP subarray; point to cJU_JPIMMED_*_01 and place it:
            else
            {
                assert(JU_JBB_PJP(Pjbb, sub4exp) == 0);
                Pjp = Pjpnew;
                *Pjp = newJP;           // copy to new memory.
#ifdef JUDYL
//                Pjpm->jpm_PValue = (Pjv_t) &(Pjp->jp_ValueI);
                Pjpm->jpm_PValue =  ju_PImmVal_01(Pjp);    // ^ to Immed Value
#endif /* JUDYL */
            }
// Place new JP subarray in BranchB:
            JU_JBB_PJP(Pjbb, sub4exp) = PjpnewRaw;
        }                               // else
// Set the new Indexs bit:
        JU_JBB_BITMAP(Pjbb, sub4exp) |= bitmask;
        goto successreturn;
    }                                   // case

// ****************************************************************************
// JPBRANCH_U*:
//
// Just drop through the JP for the correct digit.  If the JP turns out to be a
// JPNULL*, thats OK, the memory is already allocated, and the next walk
// simply places an Immed in it.
//
    case cJU_JPBRANCH_U2:
    {
        PPjp = Pjp;
        if (ju_DcdNotMatchKey(Index, Pjp, 2))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 2, Pjpm));

        Pjbu_t    Pjbu = P_JBU(RawJpPntr);

#ifdef  DEBUG_COUNT
        j__udyCheckBranchPop(Pjp, 2);
#endif  // DEBUG_COUNT

        digit   = JU_DIGITATSTATE(Index, 2);
        Pjp     = Pjbu->jbu_jp + digit;

#ifndef noBigU
        Pjbu->jbu_sums8[digit / 8]++;                   // grouping of 8 sums
#endif  // noBigU
        break;
    }

    case cJU_JPBRANCH_U3:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 3))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 3, Pjpm));

        Pjbu_t    Pjbu = P_JBU(RawJpPntr);

#ifdef  DEBUG_COUNT
        j__udyCheckBranchPop(Pjp, 3);
#endif  // DEBUG_COUNT

        digit   = JU_DIGITATSTATE(Index, 3);
        Pjp     = Pjbu->jbu_jp + digit;
#ifndef noBigU
        Pjbu->jbu_sums8[digit / 8]++;                   // grouping of 8 sums
#endif  // noBigU
        break;
    }

    case cJU_JPBRANCH_U4:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 4))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 4, Pjpm));

        Pjbu_t    Pjbu = P_JBU(RawJpPntr);

#ifdef  DEBUG_COUNT
        j__udyCheckBranchPop(Pjp, 4);
#endif  // DEBUG_COUNT

        digit   = JU_DIGITATSTATE(Index, 4);
        Pjp     = Pjbu->jbu_jp + digit;
#ifndef noBigU
        Pjbu->jbu_sums8[digit / 8]++;                   // grouping of 8 sums
#endif  // noBigU
        break;
    }

    case cJU_JPBRANCH_U5:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 5))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 5, Pjpm));

        Pjbu_t    Pjbu = P_JBU(RawJpPntr);

#ifdef  DEBUG_COUNT
        j__udyCheckBranchPop(Pjp, 5);
#endif  // DEBUG_COUNT

        digit   = JU_DIGITATSTATE(Index, 5);
        Pjp     = Pjbu->jbu_jp + digit;
#ifndef noBigU
        Pjbu->jbu_sums8[digit / 8]++;                   // grouping of 8 sums
#endif  // noBigU
        break;
    }

    case cJU_JPBRANCH_U6:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 6))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 6, Pjpm));

        Pjbu_t    Pjbu = P_JBU(RawJpPntr);

#ifdef  DEBUG_COUNT
        j__udyCheckBranchPop(Pjp, 6);
#endif  // DEBUG_COUNT

        digit   = JU_DIGITATSTATE(Index, 6);
        Pjp     = Pjbu->jbu_jp + digit;
#ifndef noBigU
        Pjbu->jbu_sums8[digit / 8]++;                   // grouping of 8 sums
#endif  // noBigU
        break;
    }

    case cJU_JPBRANCH_U7:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 7))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 7, Pjpm));

        Pjbu_t    Pjbu = P_JBU(RawJpPntr);

#ifdef  DEBUG_COUNT
        j__udyCheckBranchPop(Pjp, 7);
#endif  // DEBUG_COUNT


        digit   = JU_DIGITATSTATE(Index, 7);
        Pjp     = Pjbu->jbu_jp + digit;
#ifndef noBigU
        Pjbu->jbu_sums8[digit / 8]++;                   // grouping of 8 sums
#endif  // noBigU
        break;
    }

    case cJU_JPBRANCH_U8:
    {
        Pjbu_t    Pjbu = P_JBU(RawJpPntr);

#ifdef  DEBUG_COUNT
        j__udyCheckBranchPop(Pjp, 8);
#endif  // DEBUG_COUNT

        digit   = JU_DIGITATSTATE(Index, 8);
        Pjp     = Pjbu->jbu_jp + digit;
#ifndef noBigU
        Pjbu->jbu_sums8[digit / 8]++;                   // grouping of 8 sums
#endif  // noBigU
        break;
    }

// ****************************************************************************
// JPLEAF*:
//
// COMMON CODE FRAGMENTS TO MINIMIZE REDUNDANCY BELOW:
//
// These are necessary to support performance by function and loop unrolling
// while avoiding huge amounts of nearly identical code.
//
// Prepare to handle a linear leaf:  Check for an outlier; set pop1 and pointer
// to leaf:
// Add to, or grow, a linear leaf:  Find Index position; if the Index is
// absent, if theres room in the leaf, insert the Index [and value of 0] in
// place, otherwise grow the leaf:
//
// Note:  These insertions always take place with whole words, using
// JU_INSERTINPLACE() or JU_INSERTCOPY().
// Handle linear leaf overflow (cascade):  Splay or compress into smaller
// leaves:
// Wrapper around all of the above:
// END OF MACROS; LEAFL CASES START HERE:
//
#ifdef  JUDYL
    case cJU_JPLEAF1:
    {
        Word_t  Pjll1Raw = RawJpPntr;
        Pjll1_t Pjll1    = P_JLL1(Pjll1Raw);

        uint8_t index8   = Index;
        exppop1          = ju_LeafPop1(Pjp);
        assert(exppop1 != 0);
        assert(exppop1 <= cJU_LEAF1_MAXPOP1);

        Pjv_t Pjv = JL_LEAF1VALUEAREA(Pjll1, exppop1);

//      slow way for now
        int offset = j__udySearchLeaf1(Pjll1, exppop1, index8, 1 * 8);
        if (offset >= 0)
        {
            Pjpm->jpm_PValue = Pjv + offset;
            assert(0);
            return (0);
        }
        offset = ~offset;

        if (JU_LEAF1GROWINPLACE(exppop1))       /* insert into current leaf? */
        {
            JU_INSERTINPLACE(Pjll1->jl1_Leaf, exppop1, offset, index8);
            JU_PADLEAF1(Pjll1->jl1_Leaf, exppop1 + 1);
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);

//          Put the LastKey in the Leaf1
            Pjll1->jl1_LastKey = (Index & cJU_DCDMASK(1)) | Pjll1->jl1_Leaf[exppop1];

            Pjpm->jpm_PValue = Pjv + offset;
            ju_SetLeafPop1(Pjp, exppop1 + 1);               // increase Population0 by one
            Pjp->jp_subLeafPops += ((Word_t)1) << ((index8 >> (8 - 3)) * 8);    // and sum-exp
            goto successreturn;
        }
        if (exppop1 < cJU_LEAF1_MAXPOP1)      /* grow to new leaf */
        {
            Word_t Pjll1newRaw = j__udyAllocJLL1(exppop1 + 1, Pjpm);
            if (Pjll1newRaw == 0)
                return (-1);
            Pjll1_t  Pjll1new = P_JLL1(Pjll1newRaw);
            Pjv_t Pjvnew = JL_LEAF1VALUEAREA(Pjll1new, exppop1 + 1);
            JU_INSERTCOPY(Pjll1new->jl1_Leaf, Pjll1->jl1_Leaf, exppop1, offset, index8);
            JU_PADLEAF1(  Pjll1new->jl1_Leaf, exppop1 + 1);     // only needed for Parallel search

//          Put the LastKey in the Leaf1
            Pjll1new->jl1_LastKey = (Index & cJU_DCDMASK(1)) | Pjll1new->jl1_Leaf[exppop1];

            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);

            j__udyFreeJLL1(Pjll1Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll1newRaw);

            Pjpm->jpm_PValue = Pjvnew + offset;
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            Pjp->jp_subLeafPops += ((Word_t)1) << ((index8 >> (8 - 3)) * 8);    // and sum-exp
            goto successreturn;
        }
//      Max population, convert to LeafB1
        assert(exppop1 == cJU_LEAF1_MAXPOP1);

#ifdef  PCAS
        printf("\n====JudyIns======= Leaf1 to LeafB1, Pop1 = %ld\n", exppop1 + 1);
#endif  // PCAS

	Word_t PjlbRaw = j__udyAllocJLB1U(Pjpm);
	if (PjlbRaw == 0)
            return(-1);

	Pjlb_t Pjlb  = P_JLB1(PjlbRaw);

//      This area for DCD with uncompressed Value area -- LeafB1
//      NOTE:  this is distructive of jp_subLeafPops!!!
        ju_SetDcdPop0(Pjp, Pjll1->jl1_LastKey); // + cJU_LEAF1_MAXPOP1 - 1;

//	Build all 4 bitmaps from 1 byte index Leaf1 -- slow?
//	for (int off = 0; off < cJU_LEAF1_MAXPOP1; off++)
	for (int off = 0; off < exppop1; off++)
            JU_BITMAPSETL(Pjlb, Pjll1->jl1_Leaf[off]);

        Pjv_t Pjvnew = JL_JLB_PVALUE(Pjlb);

//      Copy Values to B1 in uncompressed style
//	for (int loff = 0; loff < cJU_LEAF1_MAXPOP1; loff++)
	for (int loff = 0; loff < exppop1; loff++)
            Pjvnew[Pjll1->jl1_Leaf[loff]] = Pjv[loff];

        j__udyFreeJLL1(Pjll1Raw, exppop1, Pjpm);
        Word_t DcdP0 = (Index & cJU_DCDMASK(1)) | cJU_LEAF1_MAXPOP1 - 1;
        Pjlb->jlb_LastKey = DcdP0;
//        ju_SetDcdPop0(Pjp, DcdP0);  Done above
        ju_SetJpType  (Pjp, cJL_JPLEAF_B1U);
        ju_SetPntrInJp(Pjp, PjlbRaw);
        goto ContinueInsWalk;   // now go do the Insert
    }
#endif  // JUDYL

    case cJU_JPLEAF2:
    {
        uint16_t index16 = Index;
        exppop1         = ju_LeafPop1(Pjp);
        assert(exppop1 <= (cJU_LEAF2_MAXPOP1));
        Word_t Pjll2Raw = RawJpPntr;
        Pjll2_t Pjll2   = P_JLL2(Pjll2Raw);

#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF2VALUEAREA(Pjll2, exppop1);
#endif /* JUDYL */

//      slow way
        int offset = j__udySearchLeaf2(Pjll2, exppop1, Index, 2 * 8);
        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            assert(0);
            return (0);
        }
        offset = ~offset;

//      bump sub-expanse pop from high 3 bits in Key -- imminent growth
        Pjp->jp_subLeafPops += ((Word_t)1) << ((index16 >> (16 - 3)) * 8);

        if (JU_LEAF2GROWINPLACE(exppop1))       // add to current leaf
        {
            JU_INSERTINPLACE(Pjll2->jl2_Leaf, exppop1, offset, index16);
            JU_PADLEAF2(Pjll2->jl2_Leaf, exppop1 + 1);     // only needed for Parallel search

//          Put the LastKey in the Leaf2
            Pjll2->jl2_LastKey = Pjll2->jl2_Leaf[exppop1] | (Index & cJU_DCDMASK(2));     
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            goto successreturn;
        }
        if (exppop1 < cJU_LEAF2_MAXPOP1)        // grow to new leaf
        {
            Word_t Pjll2newRaw = j__udyAllocJLL2(exppop1 + 1, Pjpm);
            if (Pjll2newRaw == 0)
                return (-1);
            Pjll2_t Pjll2new = P_JLL2(Pjll2newRaw);

            JU_INSERTCOPY(Pjll2new->jl2_Leaf, Pjll2->jl2_Leaf, exppop1, offset, Index);
            JU_PADLEAF2(Pjll2new->jl2_Leaf, exppop1 + 1);       // only needed for Parallel search

//          Put the LastKey in the Leaf2
            Pjll2new->jl2_LastKey = Pjll2new->jl2_Leaf[exppop1] | (Index & cJU_DCDMASK(2));     
#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF2VALUEAREA(Pjll2new, exppop1 + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL2(Pjll2Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll2newRaw);
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            goto successreturn;
        }
// At this point the Leaf2 may go directly to one (1) LeafB1 in the case of -S1
// So check and convert to B1 -- on 2nd thought, there may be some outliers, so
// this must be handled in Cascade2

//      else at max size, convert to Leaf1 probably for now
        assert(exppop1 == cJU_LEAF2_MAXPOP1);
       
        if (j__udyCascade2(Pjp, Index, Pjpm) == -1)
            return (-1);

//      Now Pjp->jp_Type is a Branch
        j__udyFreeJLL2(Pjll2Raw, cJU_LEAF2_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    }

    case cJU_JPLEAF3:
    {
//printf("\nRawJpPntr = 0x%016lx\n", RawJpPntr);
        exppop1 = ju_LeafPop1(Pjp);
        assert(exppop1 <= (cJU_LEAF3_MAXPOP1));
        Word_t Pjll3Raw = RawJpPntr;
        Pjll3_t Pjll3   = P_JLL3(Pjll3Raw);

#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF3VALUEAREA(Pjll3, exppop1);
#endif /* JUDYL */

        int offset = j__udySearchLeaf3(Pjll3, exppop1, Index, 3 * 8);
        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            assert(0);
            return (0);
        }
        offset = ~offset;
        if (JU_LEAF3GROWINPLACE(exppop1))       // add to current leaf
        {
            JU_INSERTINPLACE3(Pjll3->jl3_Leaf, exppop1, offset, Index);

//          Put the LastKey in the Leaf2
            Pjll3->jl3_LastKey = Pjll3->jl3_Leaf[exppop1] | (Index & cJU_DCDMASK(3));     
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            ju_SetLeafPop1(Pjp, exppop1 + 1);       // increase Population0 by one
            goto successreturn;
        }
        if (exppop1 < cJU_LEAF3_MAXPOP1)        // grow to new leaf
        {
            Word_t Pjll3newRaw = j__udyAllocJLL3(exppop1 + 1, Pjpm);
            if (Pjll3newRaw == 0)
                return (-1);
            Pjll3_t Pjll3new = P_JLL3(Pjll3newRaw);

            JU_INSERTCOPY(Pjll3new->jl3_Leaf, Pjll3->jl3_Leaf, exppop1, offset, Index);
            JU_PADLEAF3(Pjll3new->jl3_Leaf, exppop1 + 1);       // only needed for Parallel search

//          Put the LastKey in the Leaf3
            Pjll3new->jl3_LastKey = Pjll3new->jl3_Leaf[exppop1] | (Index & cJU_DCDMASK(3));     
#ifdef JUDYL
            Pjv_t Pjvnew = JL_LEAF3VALUEAREA(Pjll3new, exppop1 + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL3(Pjll3Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll3newRaw);
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            goto successreturn;
        }
        assert(exppop1 == (cJU_LEAF3_MAXPOP1));

#ifdef  later
        Word_t StageKeys[258];

        JU_INSERTCOPYtoWORD(StageKeys, Pjll3->jl3_Leaf, exppop1, offset, Index, 3); 

        for (int ii = 0; ii < cJU_LEAF3_MAXPOP1; ii++)
        {
            Word_t Key;
            JU_COPY3_PINDEX_TO_LONG(Key, Pjll3->jl3_Leaf + (ii * 3));
            printf("%3d = 0x%016lx = 0x%016lx", ii, StageKeys[ii], Key);
            if (ii == offset) printf("*\n");
            else printf("\n");
        }
#endif  //  later

//printf("\n3 Pjll3Raw = 0x%016lx\n", Pjll3Raw);
        if (j__udyCascade3(Pjp, Index, Pjpm) == -1)
            return (-1);
//printf("\n4 Pjll3Raw = 0x%016lx\n", Pjll3Raw);
//      Now Pjp->jp_Type is a BranchLBorU
        j__udyFreeJLL3(Pjll3Raw, cJU_LEAF3_MAXPOP1, Pjpm);
//printf("\n5 Pjll3Raw = 0x%016lx\n", Pjll3Raw);
        goto ContinueInsWalk;
    }

    case cJU_JPLEAF4:
    {
        exppop1         = ju_LeafPop1(Pjp);
        assert(exppop1 <= (cJU_LEAF4_MAXPOP1));
        Word_t Pjll4Raw = RawJpPntr;
        Pjll4_t Pjll4   = P_JLL4(Pjll4Raw);
        Word_t  PjblRaw = 0;
#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF4VALUEAREA(Pjll4, exppop1);
#endif /* JUDYL */
        int offset = j__udySearchLeaf4(Pjll4, exppop1, Index, 4 * 8);
        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            assert(0);
            return (0);
        }
        offset = ~offset;
        if (JU_LEAF4GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE(Pjll4->jl4_Leaf, exppop1, offset, Index);

//          Put the LastKey in the Leaf4
            Pjll4->jl4_LastKey = Pjll4->jl4_Leaf[exppop1] | (Index & cJU_DCDMASK(4));     

#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            goto successreturn;
        }
        if (exppop1 < cJU_LEAF4_MAXPOP1)      /* grow to new leaf */
        {
            Word_t Pjll4newRaw = j__udyAllocJLL4(exppop1 + 1, Pjpm);
            if (Pjll4newRaw == 0)
                return (-1);
            Pjll4_t Pjll4new = P_JLL4(Pjll4newRaw);
            JU_INSERTCOPY(Pjll4new->jl4_Leaf, Pjll4->jl4_Leaf, exppop1, offset, Index);

//          Put the LastKey in the Leaf4
            Pjll4new->jl4_LastKey = Pjll4new->jl4_Leaf[exppop1] | (Index & cJU_DCDMASK(4));     

#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF4VALUEAREA(Pjll4new, exppop1 + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL4(Pjll4Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll4newRaw);
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            goto successreturn;
        }
        assert(exppop1 == cJU_LEAF4_MAXPOP1);

//      We are going to change this Leaf4 to a Branch. If the parent is a
//      Branch_L4 with a single jp, then need to free it later.

        if (j__udyCascade4(Pjp, Index, Pjpm) == -1)
            return (-1);
//      Now Pjp->jp_Type is a BranchLBU
        j__udyFreeJLL4(Pjll4Raw, cJU_LEAF4_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    }

    case cJU_JPLEAF5:
    {
        exppop1         = ju_LeafPop1(Pjp);
        assert(exppop1 <= (cJU_LEAF5_MAXPOP1));
        Word_t Pjll5Raw = RawJpPntr;
        Pjll5_t Pjll5   = P_JLL5(Pjll5Raw);
#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF5VALUEAREA(Pjll5, exppop1);
#endif /* JUDYL */
        int offset = j__udySearchLeaf5(Pjll5, exppop1, Index, 5 * 8);
        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            assert(0);
            return (0);
        }
        offset = ~offset;
        if (JU_LEAF5GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE5(Pjll5->jl5_Leaf, exppop1, offset, Index);

//          Put the LastKey in the Leaf5
            Pjll5->jl5_LastKey = Pjll5->jl5_Leaf[exppop1];

#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            goto successreturn;
        }
        if (exppop1 < cJU_LEAF5_MAXPOP1)      /* grow to new leaf */
        {
            Word_t Pjll5newRaw = j__udyAllocJLL5(exppop1 + 1, Pjpm);
            if (Pjll5newRaw == 0)
                return (-1);
            Pjll5_t Pjll5new = P_JLL5(Pjll5newRaw);
            JU_INSERTCOPY5(Pjll5new->jl5_Leaf, Pjll5->jl5_Leaf, exppop1, offset, Index);

//          Put the LastKey in the Leaf5 (64 bit so no extra nonsense)
            Pjll5new->jl5_LastKey = Pjll5new->jl5_Leaf[exppop1];

#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF5VALUEAREA(Pjll5new, (exppop1) + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL5(Pjll5Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll5newRaw);
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            goto successreturn;
        }
        assert(exppop1 == (cJU_LEAF5_MAXPOP1));

        if (j__udyCascade5(Pjp, Index, Pjpm) == -1)
            return (-1);
//      Now Pjp->jp_Type is a BranchLBorU
        j__udyFreeJLL5(Pjll5Raw, cJU_LEAF5_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    }

    case cJU_JPLEAF6:
    {
        exppop1         = ju_LeafPop1(Pjp);
        assert(exppop1 <= (cJU_LEAF6_MAXPOP1));
        Word_t Pjll6Raw = RawJpPntr;
        Pjll6_t Pjll6   = P_JLL6(Pjll6Raw);
#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF6VALUEAREA(Pjll6, exppop1);
#endif /* JUDYL */
        int offset = j__udySearchLeaf6(Pjll6, exppop1, Index, 6 * 8);
        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            assert(0);
            return (0);
        }
        offset = ~offset;
        if (JU_LEAF6GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE6(Pjll6->jl6_Leaf, exppop1, offset, Index);

//          Put the LastKey in the Leaf6
            Pjll6->jl6_LastKey = Pjll6->jl6_Leaf[exppop1];

#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            goto successreturn;
        }
        if (exppop1 < cJU_LEAF6_MAXPOP1)      /* grow to new leaf */
        {
            Word_t Pjll6newRaw = j__udyAllocJLL6(exppop1 + 1, Pjpm);
            if (Pjll6newRaw == 0)
                return (-1);
            Pjll6_t Pjll6new = P_JLL6(Pjll6newRaw);
            JU_INSERTCOPY6(Pjll6new->jl6_Leaf, Pjll6->jl6_Leaf, exppop1, offset, Index);

//          Put the LastKey in the Leaf5 (64 bit so no extra nonsense)
            Pjll6new->jl6_LastKey = Pjll6new->jl6_Leaf[exppop1];

#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF6VALUEAREA(Pjll6new, (exppop1) + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL6(Pjll6Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll6newRaw);
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            goto successreturn;
        }
        assert(exppop1 == cJU_LEAF6_MAXPOP1);

        if (j__udyCascade6(Pjp, Index, Pjpm) == -1)
            return (-1);
//      Now Pjp->jp_Type is a BranchLBorU
        j__udyFreeJLL6(Pjll6Raw, cJU_LEAF6_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    }
    case cJU_JPLEAF7:
    {
        exppop1         = ju_LeafPop1(Pjp);
        assert(exppop1 <= cJU_LEAF7_MAXPOP1);
        Word_t Pjll7Raw = RawJpPntr;
        Pjll7_t Pjll7   = P_JLL7(Pjll7Raw);

#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF7VALUEAREA(Pjll7, exppop1);
#endif /* JUDYL */

        int offset = j__udySearchLeaf7(Pjll7, exppop1, Index, 7 * 8);
        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            assert(0);
            return (0);
        }
        offset = ~offset;
        if (JU_LEAF7GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE7(Pjll7->jl7_Leaf, exppop1, offset, Index);

//          Put the LastKey in the Leaf7
            Pjll7->jl7_LastKey = Pjll7->jl7_Leaf[exppop1];

#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif  // JUDYL
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            goto successreturn;
        }
        if (exppop1 < cJU_LEAF7_MAXPOP1)      /* grow to new leaf */
        {
            Word_t    Pjll7newRaw = j__udyAllocJLL7(exppop1 + 1, Pjpm);
            if (Pjll7newRaw == 0)
                return (-1);
            Pjll7_t Pjll7new = P_JLL7(Pjll7newRaw);
            JU_INSERTCOPY(Pjll7new->jl7_Leaf, Pjll7->jl7_Leaf, exppop1, offset, Index);

//          Put the LastKey in the Leaf7 (64 bit so no extra nonsense)
            Pjll7new->jl7_LastKey = Pjll7new->jl7_Leaf[exppop1];

#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF7VALUEAREA(Pjll7new, exppop1 + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif  // JUDYL
            j__udyFreeJLL7(Pjll7Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll7newRaw);
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            goto successreturn;
        }
        assert(exppop1 == cJU_LEAF7_MAXPOP1);

        if (j__udyCascade7(Pjp, Index, Pjpm) == -1)
            return (-1);
//      Now Pjp->jp_Type is a BranchLorB

        j__udyFreeJLL7(Pjll7Raw, cJU_LEAF7_MAXPOP1, Pjpm);
        goto ContinueInsWalk;   // go insert into Leaf6
    }

// ****************************************************************************
// JPLEAF_B1U:
//
// 8 bit Decode | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//              |SubExpanse |    Bit offset     |
//
// Note:  For JudyL, values are stored in 4 subexpanses, each a linear word
// array of up to 32[64] values each.

#ifdef  JUDYL
    case cJL_JPLEAF_B1U:
    {
//      Get last byte to decode from Index, and pointer to bitmap leaf:
        int       key     = JU_DIGITATSTATE(Index, 1);
        Word_t    sube    = key / cJU_BITSPERSUBEXPL;   // 0..3 which subexpanse.
        BITMAPL_t bitmask = JU_BITPOSMASKL(key);        // mask for Index.
        Pjlb_t    Pjlb    = P_JLB1(RawJpPntr);          // ^ to bitmap leaf
        BITMAPL_t bitmap  = JU_JLB_BITMAP(Pjlb, sube);  // 64 bit map.
        Pjv_t     Pjv     = JL_JLB_PVALUE(Pjlb);        // start of Value area
//       Pjp_t      Pjv     = Pjlb->jLlb_PV;

//      If Index already exists, value pointer and exit:
        Pjpm->jpm_PValue  = Pjv + key;                  // Value location
        if (bitmap & bitmask)
        {
            assert(0);
            return(0);
        }
        if (Index > Pjlb->jlb_LastKey)
            Pjlb->jlb_LastKey = Index;

        Pjv[key] = 0;                                   // Value
        JU_JLB_BITMAP(Pjlb, sube) |= bitmask;           // set Key bit.

        exppop1 = ju_LeafPop1(Pjp) + 1;
        ju_SetLeafPop1(Pjp, exppop1);                   // increase Pop by one
        goto successreturn;
    }
#endif /* JUDYL */

#ifdef JUDY1
    case cJ1_JPLEAF_B1U:
    {
        Pjlb_t    Pjlb    = P_JLB1(RawJpPntr);          // ^ to bitmap leaf

        if (JU_BITMAPTESTL(Pjlb, Index)) 
            return(0);                                  // duplicate Key

        exppop1 = ju_LeafPop1(Pjp) + 1;                 // bump population
        ju_SetLeafPop1(Pjp, exppop1);                   // set

//      If bitmap is not full, set the new Indexs bit; otherwise convert to a Full:
        if (exppop1 == 256)                             // full
        {
            j__udyFreeJLB1U(RawJpPntr, Pjpm);            // free LeafB1.
            ju_SetJpType(Pjp, cJ1_JPFULLPOPU1);         // mark at full population
            ju_SetPntrInJp(Pjp, 0);
            ju_SetDcdPop0(Pjp, Index & cJU_DCDMASK(1));
        }
        else
        {
            JU_BITMAPSETL(Pjlb, Index);
        }
        goto successreturn;
    } // case

// ****************************************************************************
// JPFULLPOPU1:
//
// If Index is not an outlier, then by definition its already set.

    case cJ1_JPFULLPOPU1:
    {
        return(0);      // duplicate Key
    }
#endif  // JUDY1

// JPIMMED*:
//
// This is some of the most complex code in Judy. The following comments attempt
// to make this clearer.
//
// Of the 2 words in a JP, for Immediate keys Judy1 can use 2 words, -1 byte
// == 15 bytes. JudyL can use 1 word == 8 bytes for keys.
// The other word is needed for a pointer to Value area.
//
// For both Judy1 and JudyL, cJU_JPIMMED_*_01 indexes are in word 2; otherwise
// for Judy1 only, a list of 2 or more indexes starts in word 1.  JudyL keeps
// the list in word 2 because word 1 is a pointer (to a LeafV, that is, a leaf
// containing only values).  Furthermore, cJU_JPIMMED_*_01 indexes are stored
// all-but-first-byte in jp_DcdPopO, not just the Index Sizes bytes.
//
// Maximum Immed JP types for Judy1/JudyL, depending on Index Size (cIS):
//
//          32-bit  64-bit
//
//    bytes:  7/ 3   15/ 7   (Judy1/JudyL)
//
//    cIS
//    1_     07/03   15/07   (as in: cJ1_JPIMMED_1_07)
//    2_     03/01   07/03
//    3_     02/01   05/02
//    4_             03/01
//    5_             03/01
//    6_             02/01
//    7_             02/01
//
// State transitions while inserting an Index, matching the above table:
// (Yes, this is very terse...  Study it and it will make sense.)
// (Note, parts of this diagram are repeated below for quick reference.)
//
//      +-- reformat JP here for Judy1 only, from word-2 to word-1
//      |
//      |                  JUDY1                    JUDY1           
//      V
// 1_01 => 1_02 => 1_03 => [ 1_04 => ... => 1_07 => [ 1_08..15 => ]] Leaf1 (*)
// 2_01 =>                 [ 2_02 => 2_03 =>        [ 2_04..07 => ]] Leaf2
// 3_01 =>                 [ 3_02 =>                [ 3_03..05 => ]] Leaf3
// 4_01 =>                                         [[ 4_02..03 => ]] Leaf4
// 5_01 =>                                         [[ 5_02..03 => ]] Leaf5
// 6_01 =>                                         [[ 6_02     => ]] Leaf6
// 7_01 =>                                         [[ 7_02     => ]] Leaf7
//
// (*) For Judy1 & 64-bit, go directly from cJU_JPIMMED_1_15 to a LeafB1; skip
//     Leaf1, as described in Judy1.h regarding cJ1_JPLEAF1.
// COMMON CODE FRAGMENTS TO MINIMIZE REDUNDANCY BELOW:
//
// These are necessary to support performance by function and loop unrolling
// while avoiding huge amounts of nearly identical code.
//
// The differences between Judy1 and JudyL with respect to value area handling
// are just too large for completely common code between them...  Oh well, some
// big ifdefs follow.  However, even in the following ifdefd code, use cJU_*,
// JU_*, and Judy*() instead of cJ1_* / cJL_*, J1_* / JL_*, and
// Judy1*()/JudyL*(), for minimum diffs.
//
// Handle growth of cJU_JPIMMED_*_01 to cJU_JPIMMED_*_02, for an even or odd
// Index Size (cIS), given oldIndex, Index, and Pjll in the context:
//
// Put oldIndex and Index in their proper order.  For odd indexes, must copy
// bytes.
// Variations to also handle value areas; see comments above:
// The "real" *_01 Copy macro:
//
#ifdef JUDYL
// For JudyL, Pjv (start of value area) and oldValue are also in the context;
// leave Pjv set to the value area for Index.
// The old value area is in the first word (*Pjp), and Pjv and Pjpm are also in
// the context.  Also, unlike Judy1, indexes remain in word 2 (jp_LIndex),
// meaning insert-in-place rather than copy.
//
// Return jpm_PValue pointing to Indexs value area.  If Index is new, allocate
// a 2-value-leaf and attach it to the JP.
// The following is a unique mix of JU_IMMSET_01() and JU_IMMSETCASCADE() for
// going from cJU_JPIMMED_*_01 directly to a cJU_JPLEAF* for JudyL:
//
// If Index is not already set, allocate a leaf, copy the old and new indexes
// into it, clear and return the new value area, and modify the current JP.
// Note that jp_DcdPop is set to a pop0 of 0 for now, and incremented later.
#else /* JUDY1 */
// Trim the high byte off Index, look for a match with the old Index, and if
// none, insert the new Index in the leaf in the correct place, given Pjp and
// Index in the context.
//
// Note:  A single immediate index lives in the jp_DcdPopO field, but two or
// more reside starting at Pjp->jp_1Index.
#endif /* JUDY1 */
// Handle growth of cJU_JPIMMED_*_[02..15]:
#ifdef JUDYL
// Variations to also handle value areas; see comments above:
#else /* JUDY1 */
// Insert an Index into an immediate JP that has room for more, if the Index is
// not already present; given Pjp, Index, exppop1, Pjv, and Pjpm in the
// context:
//
// Note:  Use this only when the JP format doesnt change, that is, going from
// cJU_JPIMMED_X_0Y to cJU_JPIMMED_X_0Z, where X >= 2 and Y+1 = Z.
#endif /* JUDY1 */
//
#ifdef JUDYL
// For JudyL, Pjv (start of value area) is also in the context.
#else /* JUDY1 */
// Note:  Incrementing jp_Type is how to increase the Index population.
// Insert an Index into an immediate JP that has no room for more:
#endif /* JUDY1 */
//
#ifdef JUDYL
// TBD:  This code makes a true but weak assumption that a JudyL 32-bit 2-index
// value area must be copied to a new 3-index value area.  AND it doesnt know
// anything about JudyL 64-bit cases (cJU_JPIMMED_1_0[3-7] only) where the
// value area can grow in place!  However, this should not break it, just slow
// it down.
#else /* JUDY1 */
// If the Index is not already present, do a cascade (to a leaf); given Pjp,
// Index, Pjv, and Pjpm in the context.
#endif /* JUDY1 */
// Common convenience/shorthand wrappers around JU_IMMSET_01_COPY() for
// even/odd index sizes:
// END OF MACROS; IMMED CASES START HERE:
// cJU_JPIMMED_*_01 cases:
//
// 1_01 always leads to 1_02:
//
// (1_01 => 1_02 => 1_03 => [ 1_04 => ... => 1_07 => [ 1_08..15 => ]] LeafL)

    case cJU_JPIMMED_1_01:
    {
        Word_t  oldIndex = ju_IMM01Key(Pjp);
        Word_t  newIndex = JU_TrimToIMM01(Index);

        if (oldIndex == newIndex)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);   // ^ to Immed_x_01 Value
#endif  // JUDYL
            assert(0);
            return (0);
        }
#ifdef JUDYL
        Word_t  PjvRaw   = j__udyLAllocJV(2, Pjpm);
        if (PjvRaw == 0)    
            return (-1);
        Pjv_t   Pjv      = P_JV(PjvRaw);
        Word_t  oldValue = ju_ImmVal_01(Pjp);
        ju_SetPntrInJp(Pjp, PjvRaw);     // put ^ to Value area in Pjp
#endif  // JUDYL
        uint8_t *PImmed1 = ju_PImmed1(Pjp);
        if (oldIndex < newIndex)           // sort
        {
            PImmed1[0] = oldIndex;
            PImmed1[1] = newIndex;
#ifdef JUDYL
            Pjv[0] = oldValue;
            Pjv[1] = 0;
            Pjpm->jpm_PValue = Pjv + 1; // return ^ to new Value
#endif /* JUDYL */
        }
        else
        {
            PImmed1[0] = newIndex;
            PImmed1[1] = oldIndex;
#ifdef JUDYL
            Pjv[0] = 0;
            Pjv[1] = oldValue;
            Pjpm->jpm_PValue = Pjv;         // return ^ to new Value
#endif /* JUDYL */
        }
        ju_SetJpType(Pjp, cJU_JPIMMED_1_02);
        goto successreturn;
    }

// 2_01 leads to 2_02, and 3_01 leads to 3_02, except for JudyL 32-bit, where
// they lead to a leaf:
//
// (2_01 => [ 2_02 => 2_03 => [ 2_04..07 => ]] LeafL)
// (3_01 => [ 3_02 =>         [ 3_03..05 => ]] LeafL)
    case cJU_JPIMMED_2_01:
    {
        Word_t  oldIndex = ju_IMM01Key(Pjp);
        Word_t  newIndex = JU_TrimToIMM01(Index);

        if (oldIndex == newIndex)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);   // ^ to Immed_x_01 Value
#endif  // JUDYL
            assert(0);
            return (0);
        }
#ifdef JUDYL
        Word_t   PjvRaw = j__udyLAllocJV(2, Pjpm);
        if (PjvRaw == 0)    
            return (-1);
        Pjv_t   Pjv     = P_JV(PjvRaw);
        Word_t oldValue = ju_ImmVal_01(Pjp);
        ju_SetPntrInJp(Pjp, PjvRaw);     // put ^ to Value area in Pjp
#endif  // JUDYL
        uint16_t *PImmed2 = ju_PImmed2(Pjp);
        if (oldIndex < newIndex)           // sort
        {
            PImmed2[0] = oldIndex;
            PImmed2[1] = newIndex;
#ifdef JUDYL
            Pjv[0] = oldValue;
            Pjv[1] = 0;
            Pjpm->jpm_PValue = Pjv + 1; // return ^ to new Value
#endif /* JUDYL */
        }
        else
        {
            PImmed2[0] = newIndex;
            PImmed2[1] = oldIndex;
#ifdef JUDYL
            Pjv[0] = 0;
            Pjpm->jpm_PValue = Pjv + 0;
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
        ju_SetJpType(Pjp, cJU_JPIMMED_2_02);
        goto successreturn;
    }

    case cJU_JPIMMED_3_01:
    {
        Word_t  oldIndex = ju_IMM01Key(Pjp);
        Word_t  newIndex = JU_TrimToIMM01(Index);

        if (oldIndex == newIndex)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);   // ^ to Immed_x_01 Value
#endif  // JUDYL
            assert(0);
            return (0);
        }
#ifdef JUDYL
        Word_t  PjvRaw = j__udyLAllocJV(2, Pjpm);
        if (PjvRaw == 0)    
            return (-1);
        Pjv_t   Pjv      = P_JV(PjvRaw);
        Word_t  oldValue = ju_ImmVal_01(Pjp);
        ju_SetPntrInJp(Pjp, PjvRaw);     // put ^ to Value area in Pjp
#endif  // JUDYL
        uint32_t  *PImmed3 = ju_PImmed3(Pjp);
        if (oldIndex < newIndex)           // sort
        {
            PImmed3[0] = oldIndex;
            PImmed3[1] = newIndex;
#ifdef JUDYL
            Pjv[0] = oldValue;
            Pjv[1] = 0;
            Pjpm->jpm_PValue = Pjv + 1; // return ^ to new Value
#endif /* JUDYL */
        }
        else
        {
            PImmed3[0] = newIndex;
            PImmed3[1] = oldIndex;
#ifdef JUDYL
            Pjv[0] = 0;
            Pjv[1] = oldValue;
            Pjpm->jpm_PValue = Pjv + 0;
#endif /* JUDYL */
        }
        ju_SetJpType(Pjp, cJU_JPIMMED_3_02);
        goto successreturn;
    }

// [4-7]_01 lead to [4-7]_02 for Judy1, and to leaves for JudyL:
//
// (4_01 => [[ 4_02..03 => ]] LeafL)
// (5_01 => [[ 5_02..03 => ]] LeafL)
// (6_01 => [[ 6_02 =>     ]] LeafL)
// (7_01 => [[ 7_02 =>     ]] LeafL)
    case cJU_JPIMMED_4_01:
    {
        Word_t  oldIndex = ju_IMM01Key(Pjp);
        Word_t  newIndex = JU_TrimToIMM01(Index);

        if (oldIndex == newIndex)                       // Duplicate
        {
            assert(0);
#ifdef JUDYL
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);      // ^ to Immed_x_01 Value
#endif  // JUDYL
            assert(0);
            return (0);
        }
//      Note: Special knowledge: we know this is really 3 words
#ifdef JUDYL
        Word_t  PjvRaw = j__udyLAllocJV(2, Pjpm);
        if (PjvRaw == 0)    
            return (-1);
        Pjv_t   Pjv      = P_JV(PjvRaw);
        Word_t  oldValue = ju_ImmVal_01(Pjp);
        ju_SetPntrInJp(Pjp, PjvRaw);     // put ^ to Value area in Pjp
#endif  // JUDYL
        uint32_t  *PImmed4 = ju_PImmed4(Pjp);
        if (oldIndex < newIndex)           // sort
        {
            PImmed4[0] = oldIndex;
            PImmed4[1] = newIndex;
#ifdef JUDYL
            Pjv[0] = oldValue;
            Pjv[1] = 0;
            Pjpm->jpm_PValue = Pjv + 1; // return ^ to new Value
#endif /* JUDYL */
        }
        else
        {
            PImmed4[0] = newIndex;
            PImmed4[1] = oldIndex;
#ifdef JUDYL
            Pjv[0] = 0;
            Pjv[1] = oldValue;
            Pjpm->jpm_PValue = Pjv + 0;
#endif /* JUDYL */
        }
        ju_SetJpType(Pjp, cJU_JPIMMED_4_02);
        goto successreturn;
    }

#ifdef JUDYL
    case cJL_JPIMMED_5_01:
    {
        Word_t newIndex = JU_TrimToIMM01(Index);
        Word_t oldIndex = ju_IMM01Key(Pjp);
        if (oldIndex == newIndex)
        {
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);
            assert(0);
            return (0);
        }
        int offset;
        if (newIndex > oldIndex)
            offset = 1;                         // sort
        else
            offset = 0;

        Word_t  Pjll5Raw = j__udyAllocJLL5((Word_t)2, Pjpm);
        if (Pjll5Raw  == 0)
            return (-1);
        Pjll5_t Pjll5    = P_JLL5(Pjll5Raw);
        Pjv_t    Pjv     = JL_LEAF5VALUEAREA(Pjll5, 2);

        Word_t oldValue = ju_ImmVal_01(Pjp);
        Pjv[0]          = oldValue;
        Pjv[1]          = oldValue;
        Pjv[offset]     = 0;
        Pjpm->jpm_PValue = Pjv + offset;

//      Make all Keys at full 64 bit accuracy
        oldIndex |= Index & cJU_DCDMASK(7);
        JU_INSERTCOPY(Pjll5->jl5_Leaf, &oldIndex, 1, offset, Index);

//      Put the LastKey in the Leaf5 (64 bit so no extra nonsense)
        Pjll5->jl5_LastKey = Pjll5->jl5_Leaf[1];

        ju_SetLeafPop1(Pjp, 2);
        ju_SetJpType(Pjp, cJU_JPLEAF5);
        ju_SetPntrInJp(Pjp, Pjll5Raw);
        goto successreturn;
    }
    case cJL_JPIMMED_6_01:
    {
        Word_t newIndex = JU_TrimToIMM01(Index);
        Word_t oldIndex = ju_IMM01Key(Pjp);
        if (oldIndex == newIndex)
        {
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);
            assert(0);
            return (0);
        }
        int offset;
        if (newIndex > oldIndex)
            offset = 1;                         // sort
        else
            offset = 0;

        Word_t  Pjll6Raw = j__udyAllocJLL6((Word_t)2, Pjpm);
        if (Pjll6Raw  == 0)
            return (-1);
        Pjll6_t Pjll6    = P_JLL6(Pjll6Raw);
        Pjv_t    Pjv     = JL_LEAF6VALUEAREA(Pjll6, 2);

        Word_t oldValue = ju_ImmVal_01(Pjp);
//      Do it without branches
        Pjv[0]          = oldValue;
        Pjv[1]          = oldValue;
        Pjv[offset]     = 0;
        Pjpm->jpm_PValue = Pjv + offset;

//      Make all Keys at full 64 bit accuracy
        oldIndex |= Index & cJU_DCDMASK(7);
        JU_INSERTCOPY(Pjll6->jl6_Leaf, &oldIndex, 1, offset, Index);

//      Put the LastKey in the Leaf6 (64 bit so no extra nonsense)
        Pjll6->jl6_LastKey = Pjll6->jl6_Leaf[1];

        ju_SetLeafPop1(Pjp, 2);
        ju_SetJpType(Pjp, cJU_JPLEAF6);
        ju_SetPntrInJp(Pjp, Pjll6Raw);
        goto successreturn;
    }
    case cJL_JPIMMED_7_01:
    {
        Word_t newIndex = JU_TrimToIMM01(Index);
        Word_t oldIndex = ju_IMM01Key(Pjp);

        if (oldIndex == newIndex)
        {
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);
            assert(0);
            return (0);
        }
        int offset;
        if (newIndex > oldIndex)
            offset = 1;                         // sort
        else
            offset = 0;

        Word_t  Pjll7Raw = j__udyAllocJLL7((Word_t)2, Pjpm);
        if (Pjll7Raw  == 0)
            return (-1);
        Pjll7_t Pjll7    = P_JLL7(Pjll7Raw);
        Pjv_t    Pjv     = JL_LEAF7VALUEAREA(Pjll7, 2);

        Word_t oldValue = ju_ImmVal_01(Pjp);
//      Do it without branches
        Pjv[0]          = oldValue;
        Pjv[1]          = oldValue;
        Pjv[offset]     = 0;
        Pjpm->jpm_PValue = Pjv + offset;

//      Make all Keys at full 64 bit accuracy
        oldIndex |= Index & cJU_DCDMASK(7);
        JU_INSERTCOPY(Pjll7->jl7_Leaf, &oldIndex, 1, offset, Index);

//      Put the LastKey in the Leaf7 (64 bit so no extra nonsense)
        Pjll7->jl7_LastKey = Pjll7->jl7_Leaf[1];

        ju_SetLeafPop1(Pjp, 2);
        ju_SetJpType(Pjp, cJU_JPLEAF7);
        ju_SetPntrInJp(Pjp, Pjll7Raw);
        goto successreturn;
    }
#endif  // JUDYL

#ifdef  JUDY1 
    case cJ1_JPIMMED_5_01:
        jpLevel = cJ1_JPIMMED_5_02;     // not really jpLevel, but handy below
        goto Immed01to02;

    case cJ1_JPIMMED_6_01:
        jpLevel = cJ1_JPIMMED_6_02;     // not really jpLevel, but handy below
        goto Immed01to02;

    case cJ1_JPIMMED_7_01:
    {
        Word_t Key0;
        Word_t Key1;
        jpLevel = cJ1_JPIMMED_7_02;     // not really jpLevel, but handy below

  Immed01to02:
        Key0 = ju_IMM01Key(Pjp);
        Key1 = JU_TrimToIMM01(Index);
//      Need high byte from Immed_x_01 7 byte Key?????
        if (Key0 == Key1)
        {
            assert(0);
            return (0);
        }
//      Sort so that Key1 is greater than Key0
        if (Key0 > Key1)                // Sort with just registers
        {
            Key0 = Key0 ^ Key1;         // Swap Key0 with Key1
            Key1 = Key0 ^ Key1;
            Key0 = Key0 ^ Key1;
        }
        Key0 |= Index & cJU_DCDMASK(7);
        Key1 |= Index & cJU_DCDMASK(7);
        ju_SetIMM02(Pjp, Key0, Key1, jpLevel);  // for here
        goto successreturn;
    }
#endif  // JUDY1

// cJU_JPIMMED_1_* cases that can grow in place:
//
// (1_01 => 1_02 => 1_03 => [ 1_04 => ... => 1_07 => [ 1_08..15 => ]] LeafL)
    case cJU_JPIMMED_1_02:
    case cJU_JPIMMED_1_03:
    case cJU_JPIMMED_1_04:
    case cJU_JPIMMED_1_05:
    case cJU_JPIMMED_1_06:
    case cJU_JPIMMED_1_07:
#ifdef JUDY1
    case cJ1_JPIMMED_1_08:
    case cJ1_JPIMMED_1_09:
    case cJ1_JPIMMED_1_10:
    case cJ1_JPIMMED_1_11:
    case cJ1_JPIMMED_1_12:
    case cJ1_JPIMMED_1_13:
    case cJ1_JPIMMED_1_14:
#endif /* JUDY1 */
    {
        exppop1           = ju_Type(Pjp) - (cJU_JPIMMED_1_02) + 2;
        uint8_t *PImmed1  = ju_PImmed1(Pjp);
        int      offset   = j__udySearchImmed1(PImmed1, exppop1, Index, 1 * 8);
#ifdef JUDYL
        Word_t   PjvRaw   = RawJpPntr;    // Raw ^ to Immed Values
        Pjv_t    Pjv      = P_JV(PjvRaw);
#endif  // JUDYL

        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            assert(0);
            return (0);
        }
        offset = ~offset;
        JU_INSERTINPLACE(PImmed1, exppop1, offset, Index);

#ifdef JUDYL
        if (JL_LEAFVGROWINPLACE(exppop1))
        {
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
        }
        else
        {
//          Increase size of value area:
            Word_t PjvnewRaw = j__udyLAllocJV(exppop1 + 1, Pjpm);
            if (PjvnewRaw == 0)
                return(-1);
            Pjv_t Pjvnew     = P_JV(PjvnewRaw);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            j__udyLFreeJV(PjvRaw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, PjvnewRaw);
            Pjv = Pjvnew;
        }
        Pjpm->jpm_PValue = Pjv + offset;                // new value area.
#endif // JUDYL
        ju_SetJpType(Pjp, ju_Type(Pjp) + 1);
        goto successreturn;
    }

// cJU_JPIMMED_1_* cases that must cascade:
// (1_01 => 1_02 => 1_03 => [ 1_04 => ... => 1_07 => [ 1_08..15 => ]] LeafL)

#ifdef  JUDY1
    case cJ1_JPIMMED_1_15:
#endif  // JUDY1

#ifdef  JUDYL
    case cJL_JPIMMED_1_08:
#endif  // JUDYL
    {
        uint8_t *PImmed1 = ju_PImmed1(Pjp);

//      Check if duplicate Key
        int offset = j__udySearchImmed1(PImmed1, cJU_IMMED1_MAXPOP1, Index, 1 * 8);

#ifdef  JUDYL
        Word_t PjvRaw    = RawJpPntr;              // Value area
        Pjv_t  Pjv       = P_JV(PjvRaw);
        Pjpm->jpm_PValue = Pjv + offset;
#endif  // JUDYL

        if (offset >= 0)
        {
            assert(0);
            return (0);         // Duplicate
        }
//        offset = ~offset;  only used for Value offset

#ifdef  JUDY1
//        printf("\n# --- Pop1 at 1st cJ1_JPIMMED_1_15 splay = %lu jptype = %d\n", j__udyJPPop1(PPjp), ju_Type(PPjp));
//      Produce a LeafB1
	Word_t PjlbRaw = j__udyAllocJLB1U(Pjpm);
	if (PjlbRaw == 0)
            return(-1);
	Pjlb_t Pjlb    = P_JLB1(PjlbRaw);

        Pjlb->jlb_LastKey = (Index & cJU_DCDMASK(1)) 
            | PImmed1[cJU_IMMED1_MAXPOP1 - 1];     

//	Copy 1 byte index Leaf to bitmap Leaf
	for (int off = 0; off < cJU_IMMED1_MAXPOP1; off++)
            JU_BITMAPSETL(Pjlb, PImmed1[off]);

        ju_SetDcdPop0(Pjp, Index & cJU_DCDMASK(1));     

        ju_SetPntrInJp(Pjp, PjlbRaw);
        ju_SetJpType(Pjp, cJU_JPLEAF_B1U);
#endif  // JUDY1

#ifdef  JUDYL
//      Produce Leaf1 from IMMED_1_08 JudyL=8
        Word_t Pjll1newRaw = j__udyAllocJLL1(cJU_IMMED1_MAXPOP1, Pjpm);
        if (Pjll1newRaw == 0)
            return (-1);
        Pjll1_t Pjll1new = P_JLL1(Pjll1newRaw);
        JU_COPYMEM(Pjll1new->jl1_Leaf, PImmed1, cJU_IMMED1_MAXPOP1);

//      Note: Pjll1new->jl1_LastKey is set when the Insert Leaf1 is done
         
//      fill sub-expanse counts to the new Leaf1, NOT old PImmed1 (overwrite!)
        Word_t Counts = 0;
        for (int loff = 0; loff < cJU_IMMED1_MAXPOP1; loff++) 
        {       // bump the sub-expanse determined by hi 3 bits of Key
            uint8_t  subexp = Pjll1new->jl1_Leaf[loff] >> (8 - 3);
            Counts += ((Word_t)1) << (subexp * 8);              // add 1 to sub-expanse
            Word_t   subLeafPops = Pjp->jp_subLeafPops;         // in-cache 8 sub-expanses pops
        }
        Pjp->jp_subLeafPops = Counts;      // place all 8 SubExps populations

        Pjv_t  Pjvnew = JL_LEAF1VALUEAREA(Pjll1new, cJU_IMMED1_MAXPOP1);
//      Copy Values from Immed to Leaf
        JU_COPYMEM(Pjvnew, Pjv, cJU_IMMED1_MAXPOP1);        

//      Free Immed1 Value area
        j__udyLFreeJV(PjvRaw, cJU_IMMED1_MAXPOP1, Pjpm);

        ju_SetPntrInJp(Pjp, Pjll1newRaw);
        ju_SetJpType(Pjp, cJU_JPLEAF1);
#endif  // JUDYL

        ju_SetLeafPop1(Pjp, cJU_IMMED1_MAXPOP1);
        goto ContinueInsWalk;       // now go insert the Key into new Leaf1
    }

// cJU_JPIMMED_[2..7]_[02..15] cases that grow in place or cascade:
//
// (2_01 => [ 2_02 => 2_03 => [ 2_04..07 => ]] LeafL)
    case cJU_JPIMMED_2_02:
    case cJU_JPIMMED_2_03:
#ifdef JUDY1
    case cJ1_JPIMMED_2_04:
    case cJ1_JPIMMED_2_05:
    case cJ1_JPIMMED_2_06:
#endif /* JUDY1 */
    {   // increase Pop of IMMED_2
        exppop1           = ju_Type(Pjp) - (cJU_JPIMMED_2_02) + 2;
        uint16_t *PImmed2 = ju_PImmed2(Pjp);
        int       offset  = j__udySearchImmed2(PImmed2, exppop1, Index, 2 * 8);

#ifdef JUDYL
        Word_t  PjvRaw = RawJpPntr;
        Pjv_t   Pjv    = P_JV(PjvRaw);
#endif // JUDYL

        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            assert(0);
            return (0);
        }
        offset = ~offset;
        JU_INSERTINPLACE(PImmed2, exppop1, offset, Index);

#ifdef JUDYL
        if (JL_LEAFVGROWINPLACE(exppop1))       // only 2 to 3
        {
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
        }
        else    // only 3 to 4
        {
//          Increase size of value area 3 to 4:
            Word_t PjvnewRaw = j__udyLAllocJV(exppop1 + 1, Pjpm);
            if (PjvnewRaw == 0) 
                return(-1);
            Pjv_t Pjvnew = P_JV(PjvnewRaw);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            j__udyLFreeJV(PjvRaw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, PjvnewRaw);
            Pjv = Pjvnew;
        }
        Pjpm->jpm_PValue = Pjv + offset;                // new value area.
#endif // JUDYL

        ju_SetJpType(Pjp, ju_Type(Pjp) + 1);
        goto successreturn;
    }

//      Terminal size, convert to something else
#ifdef JUDYL
    case cJL_JPIMMED_2_04:
#else /* JUDY1 */
    case cJ1_JPIMMED_2_07:
#endif /* JUDY1 */
    {   // convert this IMMED_2 to Leaf2
        exppop1           = ju_Type(Pjp) - cJU_JPIMMED_2_02 + 2;
        uint16_t *PImmed2 = ju_PImmed2(Pjp);
        int       offset  = j__udySearchImmed2(PImmed2, exppop1, Index, 2 * 8);
#ifdef JUDYL
        Word_t  PjvOldRaw = RawJpPntr;
        Pjv_t   PjvOld    = P_JV(PjvOldRaw);
#endif  // JUDYL

        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = PjvOld + offset;
#endif /* JUDYL */
            assert(0);
            return (0);
        }
        offset = ~offset;
        Word_t Pjll2Raw = j__udyAllocJLL2(exppop1 + 1, Pjpm);
        if (Pjll2Raw == 0)
            return (-1);
        Pjll2_t Pjll2 = P_JLL2(Pjll2Raw);

        JU_INSERTCOPY(Pjll2->jl2_Leaf, PImmed2, exppop1, offset, Index);
        JU_PADLEAF2(Pjll2->jl2_Leaf, exppop1 + 1);       // only needed for Parallel search

//      Do it every time because the next Insert to Leaf2 may be too late
        Pjll2->jl2_LastKey = Pjll2->jl2_Leaf[exppop1] | (Index & cJU_DCDMASK(2));

//      fill sub-expanse counts to the  Leaf1, NOT old PImmed2 (overwrite!)
        Word_t Counts = 0;
        for (int loff = 0; loff < (exppop1 + 1); loff++) 
        {       // bump the sub-expanse determined by hi 3 bits of Key
            uint16_t  subexp = Pjll2->jl2_Leaf[loff] >> (16 - 3);
            Counts += ((Word_t)1) << (subexp * 8);      // add 1 to sub-expanse
        }
        Pjp->jp_subLeafPops = Counts;      // place all 8 SubExps populations

#ifdef JUDYL
        Pjv_t   Pjv = JL_LEAF2VALUEAREA(Pjll2, exppop1 + 1);
        JU_INSERTCOPY(Pjv, PjvOld, exppop1, offset, 0);
        j__udyLFreeJV(PjvOldRaw, /* 4 */ exppop1, Pjpm);
        Pjpm->jpm_PValue = Pjv + offset;
#endif  // JUDYL

// not nec?        Word_t D_P0 = (Index & cJU_DCDMASK(2)) + exppop1 - 1;
// not nec?        ju_SetDcdPop0(Pjp, D_P0);
        ju_SetLeafPop1(Pjp, exppop1 + 1);
        ju_SetJpType(Pjp, cJU_JPLEAF2);
        ju_SetPntrInJp(Pjp, Pjll2Raw);      // Combine these 3 !!!!!!!!!!!!!!
        goto successreturn;
    }

// (3_01 => [ 3_02 => [ 3_03..05 => ]] LeafL)

#ifdef JUDYL
    case cJL_JPIMMED_3_02:
#endif  // JUDYL
    {   // Convert IMMED to Leaf3
        uint32_t *PImmed3 = ju_PImmed3(Pjp);

#ifdef JUDYL
        Word_t  PjvOldRaw = RawJpPntr;
        Pjv_t   PjvOld = P_JV(PjvOldRaw);
#endif  // JUDYL

        exppop1 = ju_Type(Pjp) - cJU_JPIMMED_3_02 + 2;
        int offset = j__udySearchImmed3(PImmed3, exppop1, Index, 3 * 8);
        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = PjvOld + offset;
#endif /* JUDYL */
            assert(0);
            return (0);
        }
        offset = ~offset;
        Word_t Pjll3Raw = j__udyAllocJLL3(exppop1 + 1, Pjpm);
        if (Pjll3Raw == 0)
            return (-1);
        Pjll3_t Pjll3 = P_JLL3(Pjll3Raw);
        JU_INSERTCOPY3(Pjll3->jl3_Leaf, PImmed3, exppop1, offset, Index);
        JU_PADLEAF3(Pjll3->jl3_Leaf, exppop1 + 1);       // only needed for Parallel search

//      Do it every time because the next Insert to Leaf3 may be too late
        Pjll3->jl3_LastKey = Pjll3->jl3_Leaf[exppop1] | (Index & cJU_DCDMASK(3));

#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF3VALUEAREA(Pjll3Raw, exppop1 + 1);
        JU_INSERTCOPY(Pjv, PjvOld, exppop1, offset, 0);
        j__udyLFreeJV(PjvOldRaw, exppop1, Pjpm);
        Pjpm->jpm_PValue = Pjv + offset;
#endif  // JUDYL
        ju_SetPntrInJp(Pjp, Pjll3Raw);       // Combine all 3?
        ju_SetLeafPop1(Pjp, exppop1 + 1);       // New Leaf
        ju_SetJpType(Pjp, cJU_JPLEAF3);
        goto successreturn;
    }

//      Terminal size for JUDYL, convert to something else
#ifdef  JUDYL
    case cJU_JPIMMED_4_02:
    {
        uint32_t *PImmed4 = ju_PImmed4(Pjp);
        int       offset  = j__udySearchImmed4(PImmed4, 2, Index, 4 * 8);
        Word_t PjvRaw     = RawJpPntr;
        Pjv_t  Pjv        = P_JV(PjvRaw);               // corresponding values.
        if (offset >= 0)
        {
            Pjpm->jpm_PValue = Pjv + offset;
            assert(0);
            return (0);
        }
        offset = ~offset;
        Word_t Pjll4Raw = j__udyAllocJLL4((Word_t)2 + 1, Pjpm);
        if (Pjll4Raw == 0)
            return (-1);
        Pjll4_t Pjll4 = P_JLL4(Pjll4Raw);
        JU_INSERTCOPY(Pjll4->jl4_Leaf, PImmed4, 2, offset, Index);
        JU_PADLEAF4(Pjll4->jl4_Leaf, 2 + 1);       // only needed for Parallel search

//      Do it every time because the next Insert to Leaf4 may be too late
        Pjll4->jl4_LastKey = Pjll4->jl4_Leaf[2] | (Index & cJU_DCDMASK(4));

        Pjv_t Pjvnew = JL_LEAF4VALUEAREA(Pjll4, 2 + 1);
        JU_INSERTCOPY(Pjvnew, Pjv, 2, offset, 0);
        Pjpm->jpm_PValue = Pjvnew + offset;                //  value area.
        j__udyLFreeJV(PjvRaw, 2, Pjpm);

        ju_SetPntrInJp(Pjp, Pjll4Raw);
        ju_SetLeafPop1(Pjp, 2 + 1);
        ju_SetJpType(Pjp, cJU_JPLEAF4);
        goto successreturn;
    }
#endif  // JUDYL

#ifdef  JUDY1
    case cJ1_JPIMMED_3_02:
    {   // increase Pop of IMMED_3
        uint32_t *PImmed3 = ju_PImmed3(Pjp);

        exppop1 = 2;
        int offset = j__udySearchImmed3(PImmed3, exppop1, Index, 3 * 8);
        if (offset >= 0)
        {
            assert(0);
            return (0);
        }
        offset = ~offset;
        JU_INSERTINPLACE3(PImmed3, exppop1, offset, Index);
        ju_SetJpType(Pjp, cJ1_JPIMMED_3_03);
        goto successreturn;
    }
//      Terminal size, convert to Leaf3
    case cJ1_JPIMMED_3_03:
    {   // Convert IMMED to Leaf3
        uint32_t *PImmed3 = ju_PImmed3(Pjp);

        exppop1 = ju_Type(Pjp) - cJU_JPIMMED_3_02 + 2;
        int offset = j__udySearchImmed3(PImmed3, exppop1, Index, 3 * 8);
        if (offset >= 0)
        {
            assert(0);
            return (0);
        }
        offset = ~offset;
        Word_t Pjll3Raw = j__udyAllocJLL3(exppop1 + 1, Pjpm);
        if (Pjll3Raw == 0)
            return (-1);
        Pjll3_t Pjll3 = P_JLL3(Pjll3Raw);
        JU_INSERTCOPY3(Pjll3->jl3_Leaf, PImmed3, exppop1, offset, Index);
        JU_PADLEAF3(Pjll3->jl3_Leaf, exppop1 + 1);       // only needed for Parallel search

//      Do it every time because the next Insert to Leaf3 may be too late
        Pjll3->jl3_LastKey = Pjll3->jl3_Leaf[exppop1] | (Index & cJU_DCDMASK(3));

        ju_SetPntrInJp(Pjp, Pjll3Raw);          // Combine all 3?
        ju_SetLeafPop1(Pjp, exppop1 + 1);       // New Leaf
        ju_SetJpType(Pjp, cJU_JPLEAF3);
        goto successreturn;
    }
 
    case cJ1_JPIMMED_4_02:
    {
        uint32_t *PImmed4 = ju_PImmed4(Pjp);
        int       offset  = j__udySearchImmed4(PImmed4, 2, Index, 4 * 8);
        if (offset >= 0)
        {
            assert(0);
            return (0);
        }
        offset = ~offset;
        JU_INSERTINPLACE(PImmed4, 2, offset, Index);
        ju_SetJpType(Pjp, cJ1_JPIMMED_4_03);
        goto successreturn;
    }

//      Terminal size, convert to something else
    case cJ1_JPIMMED_4_03:
    {
        uint32_t *PImmed4 = ju_PImmed4(Pjp);
        int offset = j__udySearchImmed4(PImmed4, 3, Index, 4 * 8);
        if (offset >= 0)
        {
            assert(0);
            return (0);
        }
        offset = ~offset;
        Word_t Pjll4Raw = j__udyAllocJLL4((Word_t)3 + 1, Pjpm);
        if (Pjll4Raw == 0)
            return (-1);
        Pjll4_t Pjll4 = P_JLL4(Pjll4Raw);
        JU_INSERTCOPY(Pjll4->jl4_Leaf, PImmed4, 3, offset, Index);

//      Do it every time because the next Insert to Leaf3 may be too late
        Pjll4->jl4_LastKey = Pjll4->jl4_Leaf[3] | (Index & cJU_DCDMASK(4));

        ju_SetPntrInJp(Pjp, Pjll4Raw);
        ju_SetLeafPop1(Pjp, 3 + 1);
        ju_SetJpType(Pjp, cJ1_JPLEAF4);
        goto successreturn;
    }

//      Terminal size, convert to Leaf
    case cJ1_JPIMMED_5_02:
    {
        Word_t StageIMM5[2];
        StageIMM5[0] = ju_IMM01Key(Pjp) | (Index & cJU_DCDMASK(7));
        StageIMM5[1] = ju_IMM02Key(Pjp) | (Index & cJU_DCDMASK(7));

        int offset = j__udy1SearchImm02(Pjp, Index);
        if (offset >= 0)
        {
            assert(0);
            return (0);
        }
        offset = ~offset;

        Word_t Pjll5Raw = j__udyAllocJLL5((Word_t)2 + 1, Pjpm);
        if (Pjll5Raw == 0)
            return (-1);
        Pjll5_t Pjll5 = P_JLL5(Pjll5Raw);
        JU_INSERTCOPY5(Pjll5->jl5_Leaf, StageIMM5, 2, offset, Index);

//      Do it every time because the next Insert to Leaf5 may be too late
        Pjll5->jl5_LastKey = Pjll5->jl5_Leaf[2];

        ju_SetPntrInJp(Pjp, Pjll5Raw);
        ju_SetLeafPop1(Pjp, 2 + 1);
        ju_SetJpType(Pjp, cJ1_JPLEAF5);
        goto successreturn;
    }

//      Terminal size, convert to Leaf
    case cJ1_JPIMMED_6_02:
    {
        Word_t StageIMM6[2];
        StageIMM6[0] = ju_IMM01Key(Pjp) | (Index & cJU_DCDMASK(7));
        StageIMM6[1] = ju_IMM02Key(Pjp) | (Index & cJU_DCDMASK(7)); 
        int offset = j__udy1SearchImm02(Pjp, Index);
        if (offset >= 0)
        {
            assert(0);
            return (0);
        }
        offset = ~offset;
        Word_t Pjll6Raw = j__udyAllocJLL6((Word_t)2 + 1, Pjpm);
        if (Pjll6Raw == 0)
            return (-1);
        Pjll6_t Pjll6 = P_JLL6(Pjll6Raw);

        JU_INSERTCOPY6(Pjll6->jl6_Leaf, StageIMM6, 2, offset, Index);

//      Do it every time because the next Insert to Leaf5 may be too late
        Pjll6->jl6_LastKey = Pjll6->jl6_Leaf[2];

        ju_SetPntrInJp(Pjp, Pjll6Raw);
        ju_SetLeafPop1(Pjp, 2 + 1);
        ju_SetJpType(Pjp, cJ1_JPLEAF6);
        goto successreturn;
    }

//      Terminal size, convert to Leaf
    case cJ1_JPIMMED_7_02:
    {
        Word_t StageIMM7[2];
        StageIMM7[0] = ju_IMM01Key(Pjp) | (Index & cJU_DCDMASK(7));
        StageIMM7[1] = ju_IMM02Key(Pjp) | (Index & cJU_DCDMASK(7)); 
        int offset = j__udy1SearchImm02(Pjp, Index);
        if (offset >= 0)
        {
            assert(0);
            return (0);
        }
        offset = ~offset;
        Word_t Pjll7Raw = j__udyAllocJLL7((Word_t)2 + 1, Pjpm);
        if (Pjll7Raw == 0)
            return (-1);
        Pjll7_t Pjll7 = P_JLL7(Pjll7Raw);

        JU_INSERTCOPY7(Pjll7->jl7_Leaf, StageIMM7, 2, offset, Index);

//      Do it every time because the next Insert to Leaf5 may be too late
        Pjll7->jl7_LastKey = Pjll7->jl7_Leaf[2];

        ju_SetPntrInJp(Pjp, Pjll7Raw);
        ju_SetLeafPop1(Pjp, 2 + 1);
        ju_SetJpType(Pjp, cJ1_JPLEAF7);
        goto successreturn;
    }
#endif /* JUDY1 */

// ****************************************************************************
// INVALID JP TYPE:
    default:

#ifdef  DEBUG
    if (PPjp) printf("\nOOps JudyIns: ju_Type(PPjp) = %d\n", ju_Type(PPjp));
    printf("\nOOps JudyIns: ju_Type(Pjp) = %d\n", ju_Type(Pjp));
#ifdef  TRACEJPI
    startpop      = -(Word_t)1;
    j__udyEnabled = TRUE;
    JudyPrintJP(PPjp, "i PPjp", __LINE__, Index, Pjpm);
    JudyPrintJP(Pjp, "i Pjp", __LINE__, Index, Pjpm);
#endif  // TRACEJPI
    assert(0);
    exit(-1);
#endif  //  DEBUG

        JU_SET_ERRNO_NONNULL(Pjpm, JU_ERRNO_CORRUPT);
        return (-1);
    } // end of switch on JP type

// PROCESS JP -- RECURSIVELY:
//
// For non-Immed JP types, if successful, post-increment the population count
// at this Level.
    retcode = j__udyInsWalk(Pjp, PPjp, Index, Pjpm);

//#ifdef JUDY1
        if (retcode == 0) return (0);               // == JU_RET_*_JPM().
//#endif  // JUDY1

#ifdef  PCAS
//    if (retcode >= 2)
//    {
//        printf("#----Cascade%d, jp_t = %d\n", retcode, ju_Type(PPjp));
//    }
#endif  // PCAS

//  Successful insert, increment JP and subexpanse count:
    if (retcode > 0)
    {
//      Need to increment population of every Branch up the tree
        if (ju_Type(Pjp) <= cJU_JPMAXBRANCH)    
        {
            PPjp = Pjp;
            Word_t DcdP0 = ju_DcdPop0(Pjp) + 1;
            ju_SetDcdPop0(Pjp, DcdP0);  // update DcdP0 by + 1
        }
    }
 successreturn:
    return (1);
}                                       // j__udyInsWalk()

// ****************************************************************************
// J U D Y   1   S E T
// J U D Y   L   I N S
//
// Main entry point.  See the manual entry for details.
#ifdef JUDY1
FUNCTION int Judy1Set(PPvoid_t PPArray, // in which to insert.
                      Word_t Index,     // to insert.
                      PJError_t PJError // optional, for returning error info.
                     )
#endif /* JUDY1 */

#ifdef JUDYL
FUNCTION PPvoid_t JudyLIns(PPvoid_t PPArray,    // in which to insert.
                      Word_t Index,     // to insert.
                      PJError_t PJError // optional, for returning error info.
                          )
#endif /* JUDYL */
{
    int       offset;                   // position in which to store new Index.
    Pjpm_t    Pjpm;                     // ^ to root struct

// CHECK FOR NULL POINTER (error by caller):
    if (PPArray == (PPvoid_t) NULL)
    {
        JU_SET_ERRNO(PJError, JU_ERRNO_NULLPPARRAY);
#ifdef  JUDY1
        return (JERRI);
#endif  // JUDY1
#ifdef  JUDYL
        return (PPJERR);
#endif  // JUDYL
    }
    Word_t Pjll8Raw = (Word_t)*PPArray;

#ifndef noPREGETTEST
#ifdef JUDY1
    int retcode = Judy1Test((Pvoid_t)Pjll8Raw, Index, PJE0);
    if (retcode == 1)
        return((Word_t)0);                      // fail to do insert
#else   // JUDYL
     PPvoid_t PPvalue = JudyLGet((Pvoid_t)Pjll8Raw, Index, PJE0);
    if (PPvalue != (PPvoid_t) NULL)          
        return ((PPvoid_t)PPvalue);            // fail to do insert
#endif  // JUDYL
#endif  // noPREGETTEST

    Pjll8_t Pjll8   = P_JLL8(Pjll8Raw);   // first word of leaf (always TotalPop0)

// ****************************************************************************
// PROCESS TOP LEVEL "JRP" BRANCHES AND LEAVES:
// ****************************************************************************

// ****************************************************************************
// JRPNULL (EMPTY ARRAY):  BUILD A LEAF8 WITH ONE INDEX:
// ****************************************************************************

// if a valid empty array (null pointer), so create an array of population == 1:
    if (Pjll8Raw == 0)
    {
        Pjll8Raw = j__udyAllocJLL8((Word_t)1);        // currently 3 words
        if (Pjll8Raw == 0)
        { // out of memory
            JU_SET_ERRNO(PJError, JU_ERRNO_NOMEM);
#ifdef JUDY1
            return(JERRI);
#else /* JUDYL */
            return(PPJERR);
#endif /* JUDYL */
        }
        Pjll8 = P_JLL8(Pjll8Raw);       // first word of leaf.
//      build 1st Leaf8
        Pjll8->jl8_Population0 = 1 - 1; // TotalPop0 = 0 (== 1 at base 1)
        Pjll8->jl8_Leaf[0] = Index;
        *PPArray = (Pvoid_t)Pjll8Raw;   // put Raw Alloc in ROOT pointer
#ifdef  JUDY1
        return (1);
#endif  // JUDY1
#ifdef  JUDYL
        Pjv_t Pjv = JL_LEAF8VALUEAREA(Pjll8, 1);
        Pjv[0] = 0;         // value area 
        return ((PPvoid_t) (Pjv + 0));
#endif  // JUDYL
    }                                   // NULL JRP

// ****************************************************************************
//  ELSE more than 0 Keys
// ****************************************************************************

#ifdef  TRACEJPI
    if (startpop && (Pjll8->jl8_Population0 >= startpop))
    {
//        assert(Pjll8->jl8_Population0 == **(PWord_t)PPArray);   // first word is TotalPop0
#ifdef JUDY1
        printf("\n---Judy1Set, Key = 0x%016lx(%ld), Array Pop1 = %lu\n", Index,
            Index, Pjll8->jl8_Population0 + 1); // TotalPop0 = 0 (== 1 at base 1)
#else /* JUDYL */
        printf("\n---JudyLIns, Key = 0x%016lx(%ld), Array Pop1 = %lu\n", Index,
            Index, Pjll8->jl8_Population0 + 1); // TotalPop0 = 0 (== 1 at base 1)
#endif /* JUDYL */
    }
#endif  // TRACEJPI

// ****************************************************************************
// Add to a  LEAF8 (2..cJU_LEAF8_MAXPOP1)
// ****************************************************************************

    if (Pjll8->jl8_Population0 < cJU_LEAF8_MAXPOP1)    // must be a LEAF8
    {
        Word_t pop1 = Pjll8->jl8_Population0 + 1;
        offset      = j__udySearchLeaf8(Pjll8, pop1, Index);
#ifdef JUDYL
        Pjv_t PjvOld = JL_LEAF8VALUEAREA(Pjll8, pop1);
#endif /* JUDYL */
        if (offset >= 0)                // index is already valid:
        {
            assert(0);
#ifdef JUDY1
            return (0);
#else /* JUDYL */
            return ((PPvoid_t) (PjvOld + offset));
#endif /* JUDYL */
        }
        offset = ~offset;
// Insert index in cases where no new memory is needed:
        if (JU_LEAF8GROWINPLACE(pop1))
        {
            JU_INSERTINPLACE(Pjll8->jl8_Leaf, pop1, offset, Index);
            Pjll8->jl8_Population0 = pop1;      // inc pop0 in new leaf

//for (int ii = 0; ii < pop1; ii++)
//    printf("%3d = 0x%016lx\n", ii, Pjll8->jl8_Leaf[ii]);

#ifdef JUDY1
            return (1);                                 // success
#else /* JUDYL */
            JU_INSERTINPLACE(PjvOld, pop1, offset, 0);
            return ((PPvoid_t) (PjvOld + offset));      // success
#endif /* JUDYL */
        }
//      else must alloc new Leaf for growth
        if (pop1 < cJU_LEAF8_MAXPOP1)   // can grow to a larger leaf.
        {
            Word_t   Pjll8newRaw = j__udyAllocJLL8(pop1 + 1);
//          Insert index into a new, larger leaf:
            if (Pjll8newRaw == 0)
            { // out of memory
                JU_SET_ERRNO(PJError, JU_ERRNO_NOMEM);
#ifdef JUDY1
                return(JERRI);
#else /* JUDYL */
                return(PPJERR);
#endif /* JUDYL */
            }
            Pjll8_t Pjll8new = P_JLL8(Pjll8newRaw);
            JU_INSERTCOPY(Pjll8new->jl8_Leaf, Pjll8->jl8_Leaf, pop1, offset, Index);
#ifdef JUDYL
            Pjv_t Pjvnew = JL_LEAF8VALUEAREA(Pjll8new, pop1 + 1);
            JU_INSERTCOPY(Pjvnew, PjvOld, pop1, offset, 0);
#endif /* JUDYL */
            j__udyFreeJLL8(Pjll8Raw, pop1, NULL);  // no jpm_t
            *PPArray = (Pvoid_t)Pjll8new;
            Pjll8new->jl8_Population0 = pop1;   // inc pop0 in new leaf
#ifdef JUDY1
            return (1);                                 // success
#else /* JUDYL */
            return ((PPvoid_t) (Pjvnew + offset));      // success
#endif /* JUDYL */
        }
//      else 
// ****************************************************************************
// Add ROOT (jpm_t) struct and splay with Cascade8() (> than cJU_LEAF8_MAXPOP1)
// ****************************************************************************

        assert(pop1 == cJU_LEAF8_MAXPOP1);
//      Upon cascading from a LEAF8 leaf to the first branch, must allocate and
//      initialize a JPM.
        Pjpm = j__udyAllocJPM();
        if (Pjpm == NULL)
        {
            JU_SET_ERRNO(PJError, JU_ERRNO_NOMEM);
#ifdef JUDY1
            return(JERRI);
#else /* JUDYL */
            return(PPJERR);
#endif /* JUDYL */
        }

#ifdef  PCAS
        printf("sizeof(jp_t) = %d, sizeof(jpm_t) = %d, Pjll8 = 0x%lx\n", (int)sizeof(jp_t), (int)sizeof(jpm_t), (Word_t)Pjll8);
#endif  // PCAS

        Pjp_t Pjp         = Pjpm->jpm_JP + 0;
        Pjpm->jpm_Pop0    = cJU_LEAF8_MAXPOP1 - 1;

        ju_SetLeafPop1(Pjp, cJU_LEAF8_MAXPOP1);         // only in the Leaf ^
//        ju_SetDcdPop0 (Pjp, cJU_LEAF8_MAXPOP1 - 1);     // Level = 8, no pre-Key bytes
//
        ju_SetPntrInJp(Pjp, Pjll8Raw);
        ju_SetJpType(Pjp, cJU_JPLEAF8);                 // Cascade8 knows it is a cJU_JPLEAF8
//
//      This cJU_JPLEAF8 does not last long under a jpm
        if (j__udyCascade8(Pjp, Index, Pjpm) == -1)     // Now do the Splay
        {
            JU_COPY_ERRNO(PJError, Pjpm);
#ifdef JUDY1
            return (JERRI);
#else /* JUDYL */
            return (PPJERR);
#endif /* JUDYL */
        }

//printf("Ins: return from Cascade8, j__udyJPPop1 = %ld, LEAF8 = %ld, jpType = %d\n", j__udyJPPop1(Pjp), cJU_LEAF8_MAXPOP1, ju_Type(Pjp));

//      Now Pjp->jp_Type is a BranchL*]

// Note:  No need to pass Pjpm for memory decrement; LEAF8 memory is never
// counted in the jpm_t because there is no jpm_t.
        j__udyFreeJLL8(Pjll8Raw, cJU_LEAF8_MAXPOP1, NULL);

        assert(Pjpm->jpm_Pop0 == (cJU_LEAF8_MAXPOP1 - 1));

//   printf("Pjpm->jpm_Pop0 + 1 = %lu\n", Pjpm->jpm_Pop0 + 1);

        *PPArray = (Pvoid_t)Pjpm;       // put Cascade8 results in Root pointer
    }   // fall thru

// ****************************************************************************
// Now do the Insert in the Tree Walk code
    {
        Pjpm = P_JPM(*PPArray); // Caution: this is necessary

//      Now "Walk" the tree recursively
        int retcode = j__udyInsWalk(Pjpm->jpm_JP, Pjpm->jpm_JP, Index, Pjpm);

#ifdef JUDY1
        if (retcode == 0) return (0);               // == JU_RET_*_JPM().
#endif  // JUDY1

        if (retcode == -1)
        {
            JU_COPY_ERRNO(PJError, Pjpm);
#ifdef JUDY1
            return (JERRI);
#else /* JUDYL */
            return (PPJERR);
#endif /* JUDYL */
        }

        if (retcode == 2) 
        {
//            printf("\n--------=Just finished a Cascade at Top\n");
        }

        if (retcode > 0)
        {
//printf("\n  Ins exit: Pjpm->jpm_Pop0 = %lu, Pjpm->jpm_JP = %lu, DIF = %ld\n", Pjpm->jpm_Pop0, ju_DcdPop0(Pjpm->jpm_JP), Pjpm->jpm_Pop0 - ju_DcdPop0(Pjpm->jpm_JP));

//          254 chances out of 255 test
//            assert(((Pjpm->jpm_Pop0 ^ ju_DcdPop0(Pjpm->jpm_JP)) & 0xFF) == 0); 

            Pjpm->jpm_Pop0++;                                   // incr total array popu.

//          Note: The Pop0 field are in lower bits, so the Dcd field wont be changed
            Word_t DcdP0 = ju_DcdPop0(Pjpm->jpm_JP) + 1;
            ju_SetDcdPop0(Pjpm->jpm_JP, DcdP0);                 // update DcdP0 by + 1

//            Pjpm->jpm_Pop0 = JU_LEAF8_POP0(*PPArray) + 1);
//            if (ju_DcdPop0(Pjpm->jpm_JP) == Pjpm->jpm_Pop0)
//            {
//                    printf("Ins exit ju_DcdPop0(Pjpm->jpm_JP) = %ld\n", ju_DcdPop0(Pjpm->jpm_JP) + 1);
//                    printf("Ins exit Pjpm->jpm_Pop0           = %ld\n", Pjpm->jpm_Pop0 + 1);
//                    printf("Ins exit JU_LEAF8_POP0PArray      = %ld\n", JU_LEAF8_POP0(*PPArray) + 1);
//                    printf("Ins exit ju_LeafPop1(Pjpm->jpm_JP)= %ld\n", ju_LeafPop1(Pjpm->jpm_JP));
//            }
//            ju_SetLeafPop0(Pjpt, cJU_LEAF8_MAXPOP1);    // should be done in cascade
        }
        assert((retcode == 0) || (retcode == 1));

#ifdef JUDY1
        return (retcode);               // == JU_RET_*_JPM().
#else   // JUDYL

        assert(Pjpm->jpm_PValue != (Pjv_t) NULL);

//      return Pointer (^) to Value, even if a duplicate insert attempt
        return ((PPvoid_t) Pjpm->jpm_PValue);
#endif  // JUDYL
    }
 /*NOTREACHED*/
}                        // Judy1Set() / JudyLIns()
