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

// @(#) $Revision: 4.43 $ $Source: /judy/src/JudyCommon/JudyGet.c $
//
// Judy1Test() and JudyLGet() functions for Judy1 and JudyL.
// Compile with one of -DJUDY1 or -DJUDYL.

#if (! (defined(JUDY1) || defined(JUDYL)))
#error:  One of -DJUDY1 or -DJUDYL must be specified.
#endif

#ifdef JUDY1
#include "Judy1.h"
#else
#include "JudyL.h"
#endif

#include "JudyPrivate1L.h"

#ifdef  TRACEJPG
#include "JudyPrintJP.c"
#endif  // TRACEJPG

#ifndef noPARALLEL1
#ifdef MMX1
#include <immintrin.h>
static  uint64_t
Hk64c(uint64_t *px, Word_t wKey)
{
     __m64 m64Keys = _mm_shuffle_pi8(_mm_cvtsi64_m64(wKey), _mm_setzero_si64());
     return (uint64_t)_mm_cmpeq_pi8(*(__m64*)px, m64Keys);
}
#endif // MMX1
#endif  // noPARALLEL1

//void(ju_BranchPop0());            // Not used in Get

#ifdef  noDCD
#undef ju_DcdNotMatchKey
#define ju_DcdNotMatchKey(INDEX,PJP,POP0BYTES) (0)
#endif  // DCD


// ****************************************************************************
// J U D Y   1   T E S T
// J U D Y   L   G E T
//
// See the manual entry for details.  Note support for "shortcut" entries to
// trees known to start with a JPM.

// See the manual entry for details.  Note support for "shortcut" entries to
// trees known to start with a JPM.

#ifdef  JUDYGETINLINE
#ifdef  JUDY1
FUNCTION int j__udy1Test (Pcvoid_t PArray,      // from which to retrieve.
                        Word_t Index,           // to retrieve.
                        PJError_t PJError       // not used deprecated
                         )
#endif  // JUDY1

#ifdef  JUDYL
FUNCTION PPvoid_t j__udyLGet (Pcvoid_t PArray,  // from which to retrieve.
                        Word_t Index,           // to retrieve.
                        PJError_t PJError       // not used deprecated
                             )
#endif  // JUDYL

{
    Pjp_t     Pjp;                      // current JP while walking the tree.
    Pjpm_t    Pjpm;                     // for global accounting.
    Word_t    Pop1 = 0;                 // leaf population (number of indexes).

#ifdef  JUDYL
    Pjv_t     Pjv;
#endif  // JUDYL

    Word_t    RawPntr;
    int       posidx = 0;
    uint8_t   Digit  = 0;                // byte just decoded from Index.

    (void) PJError;                     // no longer used

    if (PArray == (Pcvoid_t)NULL)  // empty array.
        goto NotFoundExit;

#ifdef  TRACEJPG
    if (startpop && (*(Word_t *)PArray >= startpop))
    {
#ifdef JUDY1
        printf("\n0x%lx j__udy1Test, Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAF8_POP0(PArray) + 1));
#endif  // JUDY1

#ifdef  JUDYL
        printf("\n0x%lx j__udyLGet,  Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAF8_POP0(PArray) + 1));
#endif  // JUDYL
    }
#endif  // TRACEJPG

// ****************************************************************************
// PROCESS TOP LEVEL BRANCHES AND LEAF:

        if (JU_LEAF8_POP0(PArray) < cJU_LEAF8_MAXPOP1) // must be a LEAF8
        {
            Pjll8_t Pjll8 = P_JLL8(PArray);        // first word of leaf.
            Pop1   = Pjll8->jl8_Population0 + 1;

            posidx = j__udySearchLeaf8(Pjll8, Pop1, Index);
            if (posidx < 0)
                goto NotFoundExit;              // no jump to middle of switch


#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
            return((PPvoid_t) (JL_LEAF8VALUEAREA(Pjll8, Pop1) + posidx));
#endif  // JUDYL
        }
//      else must be the tree under a jpm_t
        Pjpm = P_JPM(PArray);
        Pjp = Pjpm->jpm_JP + 0;  // top branch is below JPM.

// ****************************************************************************
// WALK THE JUDY TREE USING A STATE MACHINE:

ContinueWalk:           // for going down one level in tree; come here with Pjp set.

#ifdef TRACEJPG
        JudyPrintJP(Pjp, "G", __LINE__, Index, Pjpm);
#endif  // TRACEJPG

//      Used by many -- 1st de-reference to jp_t
        RawPntr = ju_PntrInJp(Pjp);
//        PREFETCH(RawPntr);              // no help with Judy1

//      switch() On object type
        switch (ju_Type(Pjp))
        {
// Ensure the switch table starts at 0 for speed; otherwise more code is
// executed:
        case 0: 
            goto ReturnCorrupt;     // save a little code (hopefully).

// ****************************************************************************
// JPNULL*:
//
// Note:  These are legitimate in a BranchU and do not constitute an Error
        case cJU_JPNULL1:
        case cJU_JPNULL2:
        case cJU_JPNULL3:
        case cJU_JPNULL4:
        case cJU_JPNULL5:
        case cJU_JPNULL6:
        case cJU_JPNULL7:
            goto NotFoundExit;

// Note:  Here the calls of ju_DcdNotMatchKey() are necessary and check
// whether Index is out of the expanse of a narrow pointer.
//
// ****************************************************************************
// JPBRANCH_L*:
//
        case cJU_JPBRANCH_L2:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 2))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 2);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L3:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 3))
                goto NotFoundExit;

            Digit = JU_DIGITATSTATE(Index, 3);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L4:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 4))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 4);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L5:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 5))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 5);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L6:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 6))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 6);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L7:
        {
//            if (ju_DcdNotMatchKey(Index, Pjp, 7))
//                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 7);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L8:
        {
            Pjbl_t Pjbl;
            Digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);

// Common code for all BranchLs; come here with Digit set:

JudyBranchL:
            Pjbl = P_JBL(RawPntr);
            posidx = j__udySearchBranchL(Pjbl->jbl_Expanse, Pjbl->jbl_NumJPs, Digit);

            if (posidx < 0)
                goto NotFoundExit;

            Pjp = Pjbl->jbl_jp + posidx;
            goto ContinueWalk;
        }

#ifndef noB
// ****************************************************************************
// JPBRANCH_B*:
//  Note: Only one (1) JPBRANCH_B* can be any path in the walk down the tree,
//  therefore the next level must be a Leaf or Immed.

        case cJU_JPBRANCH_B2:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 2))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 2);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B3:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 3))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 3);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B4:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 4))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 4);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B5:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 5))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 5);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B6:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 6))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 6);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B7:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 7))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 7);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B8:
        {
            Pjbb_t    Pjbb;
            Word_t    sub4exp;   // in bitmap, 0..7[3]
            BITMAPB_t BitMap;   // for one sub-expanse.
            BITMAPB_t BitMsk;   // bit in BitMap for Indexs Digit.

            Digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);

// Common code for all BranchBs; come here with Digit set:
//
JudyBranchB:
            Pjbb   = P_JBB(RawPntr);
            sub4exp = Digit / cJU_BITSPERSUBEXPB;

            BitMap = JU_JBB_BITMAP(Pjbb, sub4exp);
            BitMsk = JU_BITPOSMASKB(Digit);

//          No JP in sub-expanse for Index, implies not found:
            if ((BitMap & BitMsk) == 0) 
                goto NotFoundExit;

//          start address of Pjp_t array
            Pjp    = P_JP(JU_JBB_PJP(Pjbb, sub4exp));

            Pjp += j__udyCountBitsB(BitMap & (BitMsk - 1));
            goto ContinueWalk;

        } // case cJU_JPBRANCH_B*
#endif  // noB

// ****************************************************************************
// JPBRANCH_U*:


        case cJU_JPBRANCH_U8:
        {
// nop         if (ju_DcdNotMatchKey(Index, Pjp, 8)) break;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
                goto ContinueWalk;
        }

        case cJU_JPBRANCH_U7:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 7))
                goto NotFoundExit;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 7);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U6:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 6))
                goto NotFoundExit;

            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 6);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U5:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 5))
                goto NotFoundExit;

            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 5);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U4:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 4))
                goto NotFoundExit;

            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 4);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U3:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 3))
                goto NotFoundExit;

            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 3);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U2:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 2))
                goto NotFoundExit;

            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 2);
                goto ContinueWalk;
        }
// ****************************************************************************
// JPLEAF*:
//

#ifdef  JUDYL
        case cJL_JPLEAF1:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll1_t Pjll1 = P_JLL1(RawPntr);
            Pjv  = JL_LEAF1VALUEAREA(Pjll1, Pop1);
            posidx = j__udySearchLeaf1(Pjll1, Pop1, Index, 1 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // JUDYL

        case cJU_JPLEAF2:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll2_t Pjll2 = P_JLL2(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF2VALUEAREA(Pjll2, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf2(Pjll2, Pop1, Index, 2 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF3:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll3_t Pjll3 = P_JLL3(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF3VALUEAREA(Pjll3, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf3(Pjll3, Pop1, Index, 3 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF4:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll4_t Pjll4 = P_JLL4(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF4VALUEAREA(Pjll4, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf4(Pjll4, Pop1, Index, 4 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF5:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll5_t Pjll5 = P_JLL5(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF5VALUEAREA(Pjll5, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf5(Pjll5, Pop1, Index, 5 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF6:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll6_t Pjll6 = P_JLL6(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF6VALUEAREA(Pjll6, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf6(Pjll6, Pop1, Index, 6 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF7:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll7_t Pjll7 = P_JLL7(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF7VALUEAREA(Pjll7, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf7(Pjll7, Pop1, Index, 7 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

// ****************************************************************************
// JPLEAF_B1U:
//
        case cJU_JPLEAF_B1U:
        {
            Pjlb_t    Pjlb   = P_JLB1(RawPntr);

            DIRECTHITS;       // not necessary, because always 100%
            SEARCHPOPULATION(ju_LEAF_POP1(Pjp)); 

            posidx           = (uint8_t)Index;
            Word_t  sube     = posidx / cJU_BITSPERSUBEXPL;        // 0..3

            BITMAPL_t BitMap = JU_JLB_BITMAP(Pjlb, sube);
            BITMAPL_t BitMsk = JU_BITPOSMASKL(posidx);

            if ((BitMap & BitMsk) != 0)
                goto LeafFoundExit;
        } // case cJU_JPLEAF_B1U

#ifdef JUDY1
// ****************************************************************************
// JPFULLPOPU1:
//
// If the Index is in the expanse, it is necessarily valid (found).

        case cJ1_JPFULLPOPU1:
        {
            return(1);
        }
#endif // JUDY1

// ****************************************************************************
// JPIMMED*:
//
// Note that the contents of jp_DcdPopO is different for cJU_JPIMMED_*_01:

        case cJU_JPIMMED_1_01:          // 1 byte decode
        case cJU_JPIMMED_2_01:          // 2 byte decode
        case cJU_JPIMMED_3_01:          // 3 byte decode
        case cJU_JPIMMED_4_01:          // 4 byte decode
        case cJU_JPIMMED_5_01:          // 5 byte decode
        case cJU_JPIMMED_6_01:          // 6 byte decode
        case cJU_JPIMMED_7_01:          // 7 byte decode
        {
//          The "<< 8" makes a possible bad assumption!!!!, but
//          this version does not have an conditional branch
            if ((ju_IMM01Key(Pjp) ^ Index) << 8) // mask off high byte
                goto NotFoundExit;
#ifdef  JUDY1
        return(1);
#endif  // JUDY1

#ifdef  JUDYL
        return((PPvoid_t) ju_PImmVal_01(Pjp));  // ^ to immediate Value
#endif  // JUDYL
        }

#ifdef  JUDY1
        case cJ1_JPIMMED_1_15:
        case cJ1_JPIMMED_1_14:
        case cJ1_JPIMMED_1_13:
        case cJ1_JPIMMED_1_12:
        case cJ1_JPIMMED_1_11:
        case cJ1_JPIMMED_1_10:
        case cJ1_JPIMMED_1_09:
#endif  // JUDY1
        case cJU_JPIMMED_1_08:
        case cJU_JPIMMED_1_07:
        case cJU_JPIMMED_1_06:
        case cJU_JPIMMED_1_05:
        case cJU_JPIMMED_1_04:
        case cJU_JPIMMED_1_03:
        case cJU_JPIMMED_1_02:
        {
            Pop1 = ju_Type(Pjp) - cJU_JPIMMED_1_02 + 2;
#ifdef  JUDYL
            Pjv = P_JV(RawPntr);       // Get ^ to Values
#endif  // JUDYL
            uint8_t *Pleaf1 = ju_PImmed1(Pjp);     // Get ^ to Keys
            posidx = j__udySearchImmed1(Pleaf1, Pop1, Index, 1 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

#ifdef  JUDY1
        case cJ1_JPIMMED_2_07:
        case cJ1_JPIMMED_2_06:
        case cJ1_JPIMMED_2_05:
#endif  // JUDY1

        case cJU_JPIMMED_2_04:
        case cJU_JPIMMED_2_03:
        case cJU_JPIMMED_2_02:
        {
            Pop1 = ju_Type(Pjp) - cJU_JPIMMED_2_02 + 2;
#ifdef  JUDYL
            Pjv = P_JV(RawPntr);  // ^ immediate values area
#endif  // JUDYL
            uint16_t *Pleaf2 = ju_PImmed2(Pjp);
            posidx = j__udySearchImmed2(Pleaf2, Pop1, Index, 2 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

#ifdef  JUDYL
        case cJL_JPIMMED_3_02:
        {
            Pop1 = 2;
            Pjv = P_JV(RawPntr);  // ^ immediate values area
            uint32_t *Pleaf3 = ju_PImmed3(Pjp);
            posidx = j__udySearchImmed3(Pleaf3, Pop1, Index, 3 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJL_JPIMMED_4_02:
        {
            Pop1 = 2;
            Pjv = P_JV(RawPntr);  // ^ immediate values area
            uint32_t *Pleaf4 = ju_PImmed4(Pjp);
            posidx = j__udySearchImmed4(Pleaf4, Pop1, Index, 4 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // JUDYL

#ifdef  JUDY1
        case cJ1_JPIMMED_3_03:
        case cJ1_JPIMMED_3_02:
        {
            Pop1 = ju_Type(Pjp) - cJU_JPIMMED_3_02 + 2;
            uint32_t *Pleaf3 = ju_PImmed3(Pjp);
            posidx = j__udySearchImmed3(Pleaf3, Pop1, Index, 3 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
        case cJ1_JPIMMED_4_02:
        case cJ1_JPIMMED_4_03:
        {
            Pop1 = ju_Type(Pjp) - cJ1_JPIMMED_4_02 + 2;
            uint32_t *Pleaf4 = ju_PImmed4(Pjp);
            posidx = j__udySearchImmed4(Pleaf4, Pop1, Index, 4 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJ1_JPIMMED_5_02:
        case cJ1_JPIMMED_6_02:
        case cJ1_JPIMMED_7_02:
        {
            posidx = j__udy1SearchImm02(Pjp, Index);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // JUDY1

// ****************************************************************************
// INVALID JP TYPE:

        default:
ReturnCorrupt:                  // just return not found
            goto LeafFoundExit;

//            JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
//            JUDY1CODE(return(JERRI );)
//            JUDYLCODE(return(PPJERR);)

        } // switch on JP type

NotFoundExit:

#ifdef  TRACEJPG
    if (startpop && (*(Word_t *)PArray >= startpop))
    {
#ifdef JUDY1
        printf("\n0x%lx j__udy1Test, Exit Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAF8_POP0(PArray) + 1));
#else   // JUDYL
        printf("\n0x%lx j__udyLGet,  Exit Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAF8_POP0(PArray) + 1));
        printf("---j__udyLGet Key = 0x%016lx NOT FOUND\n", Index);
#endif // JUDYL
    }
#endif  // TRACEJPG

#ifdef  JUDY1
            return(0);
#endif  // JUDY1

#ifdef  JUDYL
            return((PPvoid_t) NULL);
#endif  // JUDYL

CommonLeafExit:         // posidx & (only JudyL) Pjv
            if (posidx < 0) 
                goto NotFoundExit;
LeafFoundExit:
#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
            return((PPvoid_t) (Pjv + posidx));
#endif  // JUDYL

} // j__udy1Test() / j__udyLGet()

#endif  // JUDYGETINLINE


#ifndef JUDYGETINLINE
// ****************************************************************************
// J U D Y   1   T E S T
// J U D Y   L   G E T
//

#ifdef JUDY1
FUNCTION int Judy1Test (Pcvoid_t PArray,        // from which to retrieve.
                         Word_t Index,          // to retrieve Key.
                         PJError_t PJError      // not used, deprecated
                       )
#endif  //JUDY1

#ifdef  JUDYL
FUNCTION PPvoid_t JudyLGet (Pcvoid_t PArray,    // from which to retrieve.
                Word_t Index,                   // to retrieve.
                PJError_t PJError               // deprecated, not used
                           )
#endif  // JUDYL
{

    Pjp_t     Pjp;                      // current jp_t while walking the tree.
    Pjpm_t    Pjpm;                     // for global accounting.
    Word_t    Pop1 = 0;                 // leaf population (number of indexes).

    Word_t    RawPntr;
    int       posidx;
    uint8_t   Digit = 0;                // byte just decoded from Index.

#ifdef  JUDYL
    Pjv_t     Pjv;
#endif  // JUDYL

    (void) PJError;                     // no longer used

    if (PArray == (Pcvoid_t)NULL)  // empty array.
        goto NotFoundExit;

#ifdef  TRACEJPG
    if (startpop && (*(Word_t *)PArray >= startpop))
    {
#ifdef JUDY1
        printf("\n0x%lx ---Judy1Test   Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAF8_POP0(PArray) + 1));
#else   // JUDYL
        printf("\n0x%lx ---JudyLGet    Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAF8_POP0(PArray) + 1));
        printf("---j__udyLGet Key = 0x%016lx NOT FOUND\n", Index);
#endif // JUDYL
    }
#endif  // TRACEJPG

// ****************************************************************************
// PROCESS TOP LEVEL BRANCHES AND LEAF:

        if (JU_LEAF8_POP0(PArray) < cJU_LEAF8_MAXPOP1) // must be a LEAF8
        {
            Pjll8_t Pjll8 = P_JLL8(PArray);        // first word of leaf.
            Pop1          = Pjll8->jl8_Population0 + 1;
            assert(Pop1 == JU_LEAF8_POP0(PArray) + 1);

            if ((posidx = j__udySearchLeaf8(Pjll8, Pop1, Index)) < 0)
            {
                goto NotFoundExit;              // no jump to middle of switch
            }



//            printf("\n--Get Leaf8 Index = 0x%016lx\n", Index);
//            for (int ii = 0; ii < Pop1; ii++)
//            {
//                printf("%3d = 0x%016lx ", ii, Pjll8->jl8_Leaf[ii]);
//                if (Pjll8->jl8_Leaf[ii] == Index) printf("*");
//                printf("\n");
//            }


#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
            return((PPvoid_t) (JL_LEAF8VALUEAREA(Pjll8, Pop1) + posidx));
#endif  // JUDYL
        }
//      else must be the tree under a jpm_t
        Pjpm = P_JPM(PArray);
        Pjp = Pjpm->jpm_JP + 0;  // top branch is below JPM.


// ****************************************************************************
// WALK THE JUDY TREE USING A STATE MACHINE:

ContinueWalk:           // for going down one level in tree; come here with Pjp set.

#ifdef TRACEJPG
        JudyPrintJP(Pjp, "g", __LINE__, Index, Pjpm);
#endif  // TRACEJPG

//      Used by many
        RawPntr = ju_PntrInJp(Pjp);
        posidx = 0;
        switch (ju_Type(Pjp))
        {

// Ensure the switch table starts at 0 for speed; otherwise more code is
// executed:

        case 0: goto ReturnCorrupt;     // save a little code.

// ****************************************************************************
// JPNULL*:
//
// Note:  These are legitimate in a BranchU and do not constitute an Error
        case cJU_JPNULL1:
        case cJU_JPNULL2:
        case cJU_JPNULL3:
        case cJU_JPNULL4:
        case cJU_JPNULL5:
        case cJU_JPNULL6:
        case cJU_JPNULL7:
            goto NotFoundExit;              // no jump to middle of switch

// ****************************************************************************
// JPBRANCH_L*:
//
        case cJU_JPBRANCH_L2:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 2))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 2);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L3:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 3))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 3);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L4:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 4))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 4);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L5:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 5))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 5);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L6:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 6))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 6);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L7:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 7))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 7);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L8:
        {
            Pjbl_t Pjbl;
            Digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);

// Common code for all BranchL; come here with Digit set:

JudyBranchL:
            Pjbl = P_JBL(RawPntr);
//            int State =  Pjp->jp_Type - cJU_JPBRANCH_L2 + 2;
            posidx = j__udySearchBranchL(Pjbl->jbl_Expanse, Pjbl->jbl_NumJPs, Digit);

            if (posidx < 0)
                goto NotFoundExit;

            Pjp = Pjbl->jbl_jp + posidx;
            goto ContinueWalk;
        }

#ifndef noB
// ****************************************************************************
// JPBRANCH_B*:
//  Note: Only one (1) JPBRANCH_B* can be any path in the walk down the tree,
//  therefore the next level must be a Leaf or Immed.

        case cJU_JPBRANCH_B2:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 2))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 2);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B3:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 3))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 3);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B4:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 4))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 4);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B5:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 5))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 5);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B6:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 6))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 6);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B7:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 7))
                goto NotFoundExit;
            Digit = JU_DIGITATSTATE(Index, 7);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B8:
        {
            Pjbb_t    Pjbb;
            Word_t    sub4exp;   // in bitmap, 0..7[3]
            BITMAPB_t BitMap;   // for one sub-expanse.
            BITMAPB_t BitMsk;   // bit in BitMap for Indexs Digit.

            Digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);

// Common code for all BranchBs; come here with Digit set:

JudyBranchB:
            Pjbb   = P_JBB(RawPntr);
            sub4exp = Digit / cJU_BITSPERSUBEXPB;

            BitMap = JU_JBB_BITMAP(Pjbb, sub4exp);
            BitMsk = JU_BITPOSMASKB(Digit);

//          No JP in sub-expanse for Index, implies not found:
            if ((BitMap & BitMsk) == 0) 
                goto NotFoundExit;

//          start address of Pjp_t array
            Pjp    = P_JP(JU_JBB_PJP(Pjbb, sub4exp));

            Pjp += j__udyCountBitsB(BitMap & (BitMsk - 1));
            goto ContinueWalk;

        } // case cJU_JPBRANCH_B*
#endif  // noB

// ****************************************************************************
// JPBRANCH_U*:

#ifdef  BRANCHUCOMMONCASE
// Check preformance difference -- Judy1 -B32 2.2 to 4.8nS slower
        case cJU_JPBRANCH_U8:
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
            goto ContinueWalk;

        case cJU_JPBRANCH_U7:
        case cJU_JPBRANCH_U6:
        case cJU_JPBRANCH_U5:
        case cJU_JPBRANCH_U4:
        case cJU_JPBRANCH_U3:
        case cJU_JPBRANCH_U2:
        {
            int level = ju_Type(Pjp) - cJU_JPBRANCH_U2 + 2;
            if (ju_DcdNotMatchKey(Index, Pjp, level))
                goto NotFoundExit;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, level);
            goto ContinueWalk;
        }
#endif  // BRANCHUCOMMONCASE

#ifndef BRANCHUCOMMONCASE
        case cJU_JPBRANCH_U8:
        {
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U7:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 7))
                goto NotFoundExit;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 7);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U6:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 6))
                goto NotFoundExit;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 6);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U5:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 5))
                goto NotFoundExit;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 5);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U4:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 4))
                goto NotFoundExit;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 4);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U3:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 3))
                goto NotFoundExit;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 3);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U2:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 2))
                goto NotFoundExit;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 2);
            goto ContinueWalk;
        }
#endif  // ! BRANCHUCOMMONCASE

// ****************************************************************************
// JPLEAF*:
//
// Note:  Here the calls of ju_DcdNotMatchKey() are necessary and check
// whether Index is out of the expanse of a narrow pointer.
#ifdef doCHECKIT
#define CHECKIT(INDEX, LASTKEY, STATE, POP)                     \
{                                                               \
    if (((INDEX) ^ (LASTKEY)) & cJU_DCDMASK(STATE))             \
    {                                                           \
        printf("\n------------Oops Lastkey fail Line = %d, Level = %d, Pop1 = %d\n", (int)__LINE__, STATE, (int)POP); \
        printf("INDEX          = 0x%016lx\n", INDEX);                  \
        printf("LASTKEY        = 0x%016lx\n", LASTKEY);                \
        printf("XOR            = 0x%016lx\n", LASTKEY ^ INDEX);        \
        printf("cJU_DCDMASK(%d) = 0x%016lx\n", STATE, cJU_DCDMASK(STATE)); \
        exit(-1);                                               \
    }                                                           \
}
#else  // CHECKIT
#define CHECKIT(INDEX, LASTKEY, STATE, POP)
#endif  // noCHECKIT

// magic constant to sum char sized populations with mpy
#define CHARSUMS ((Word_t)0x0101010101010101)

//  ***************************************************************
//  This is only for JudyL because Judy1 does not have a Leaf1
//  ***************************************************************
#ifdef  JUDYL

#ifdef  noLEAF1
        case cJL_JPLEAF1:
        {
            Pop1                = ju_LeafPop1(Pjp);
            Pjll1_t Pjll1       = P_JLL1(RawPntr);
            CHECKIT(Index, Pjll1->jl1_LastKey, 1, Pop1);
            uint8_t index8      = Index;                // trim to 16 decoded bits
            Pjv = JL_LEAF1VALUEAREA(Pjll1, Pop1);

//          linear interpolation of the location of Value
            posidx  = LERP(Pop1, index8, (1 * 8));     // 1st try of offset if Key

// no Effect  PREFETCH(Pjv + posidx);                     // start read of Value (hopefully)
// no Effect  PREFETCH(Pjll1->jl1_Leaf + posidx);         // start read of Key (hopefully)

            SEARCHPOPULATION(Pop1);                     // enabled -DSEARCHMETRICS at compile time

            posidx = j__udySearchRawLeaf1(Pjll1->jl1_Leaf, Pop1, index8, posidx);
            goto CommonLeafExit;                        // posidx & (only JudyL) Pjv
        }
#else   // noLEAF1

// Stacked Leaf1 with option of a Parallel search of sub-expanse.

        case cJL_JPLEAF1:
        {
            Pop1 = ju_LeafPop1(Pjp);                            // leaf pop1 from jp_t
//          Pointer to Leaf1
            Pjll1_t Pjll1       = P_JLL1(RawPntr);
            CHECKIT(Index, Pjll1->jl1_LastKey, 1, Pop1);

            uint8_t index8      = Index;                        // trim off decoded bits
            int     subkey5     = index8 % (1 << (8 - 3));      // 0..31 Least 5 bits of Key
            int     subexp      = index8 >> (8 - 3);            // hi 3 bits = sub-expanse (0..7) to search
            subexp             *= 8;                            // convert shift bytes to shift bits
            Word_t  subLeafPops = Pjp->jp_subLeafPops;          // in-cache sub-expanse pop1

//          NOTE: max pop must be less than 256 !!!
            assert(((subLeafPops * CHARSUMS) >> (64 - 8)) == Pop1);   // cute huh!!

            SEARCHPOPULATION(Pop1);     // enabled -DSEARCHMETRICS at compile time

//          get offset to current sub expanse by summing previous sub-Leaf
            int  leafOff  = j__udySubLeaf8Pops(subLeafPops & (((Word_t)1 << subexp) - 1));
            uint8_t *subLeaf8  = Pjll1->jl1_Leaf + leafOff;     // ^ to the subLeaf

            Pjv = JL_LEAF1VALUEAREA(Pjll1, Pop1) + leafOff;     // Sub-Value area ^ 

//          Get the population of the sub-expanse containing the subkey5
            uint8_t subpop8       = (subLeafPops >> subexp) /* & 0xFF */; // 0..32 - (mask 0..63)

#ifndef  SUB0CHECK
//          Note: this is slightly faster when not found, but slightly slower when found
            if (subpop8 == 0)            // if sub-Leaf empty, then not found
                goto NotFoundExit;
#endif  // SUB0CHECK
            
//          do a linear interpolation of the location (0..31) of key in the sub-expanse
//            int start         = (subpop8 * subkey5) / 32;        // 32 == sub-expanse size
            int start = LERP(subpop8, subkey5, 1 * (8 - 3));
#ifdef  JUDYL
//          cover 3 cache lines
//  no Effect PREFETCH(Pjv + start - (64/8));     // start read of Value (hopefully)
//  no Effect PREFETCH(Pjv + start + (64/8));     // start read of Value (hopefully)
            PREFETCH(Pjv + start);              // start read of Value (hopefully)
#endif  // JUDYL

#ifndef  noLEAF1STACKEDPARA
// NOTE: The Direct hits are > 90%, so leave out parallel miss code
//          Replicate the Lsb & Msb in every Key position (at compile time)
#undef cbPK
#define cbPK    (8)            // bits per Key
            Word_t repLsbKey = REPKEY(cbPK, 1);
            Word_t repMsbKey = repLsbKey << (cbPK - 1);
//          Make 8 copys of Key in Word_t
            Word_t repKey    = REPKEY(cbPK, index8);

//          Make a word aligned starting place for search
            Word_t RawBucketPtr = ((Word_t)subLeaf8 + start) & (-(Word_t)8);
            Word_t newBucket    = *((PWord_t)RawBucketPtr) ^ repKey;
            newBucket    = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
            if (newBucket)         // Key found in Bucket?
            {
                DIRECTHITS;               // Count direct hits
                posidx = RawBucketPtr - (Word_t)subLeaf8;
                posidx += __builtin_ctzl(newBucket) / cbPK;
                return((PPvoid_t) (Pjv + posidx));
            }
#endif  //  noLEAF1STACKEDPARA

//          search an expanse of maximum of 32 keys -- 4 Words max
            posidx = j__udySearchRawLeaf1(subLeaf8, subpop8, index8, start);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // noFANCYLEAF1

#endif  // JUDYL



#ifdef  noFANCYLEAF2
        case cJU_JPLEAF2:
        {
            uint16_t index16    = Index;                // trim to 16 decoded bits
            Pop1                = ju_LeafPop1(Pjp);
            posidx  = LERP(Pop1, index16, (2 * 8));     // 1st try of offset if Key
            Pjll2_t Pjll2       = P_JLL2(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF2VALUEAREA(Pjll2, Pop1);
            PREFETCH(Pjv + posidx);                     // start read of Value (hopefully)
// no Effect  PREFETCH(Pjv + posidx + 8);                 // start read of Value (hopefully)
// no Effect  PREFETCH(Pjv + posidx - 8);                 // start read of Value (hopefully)
#endif  // JUDYL

// no Effect  PREFETCH(Pjll2->jl2_Leaf + posidx);         // start read of Key (hopefully)
            SEARCHPOPULATION(Pop1);                     // enabled -DSEARCHMETRICS at compile time
            posidx = j__udySearchRawLeaf2(Pjll2->jl2_Leaf, Pop1, index16, posidx);
            goto CommonLeafExit;                        // posidx & (only JudyL) Pjv
        }
#else   // FANCYLEAF2 STACKEDLEAF2

// Stacked Leaf2 with option of a Parallel search of sub-expanse.
// Need check if any good in JUDYL

// make any Pointer a PWord_t aligned pointer by masking off least bits
#define PWORDMASK(ANYPNTR)      ((PWord_t)(((Word_t)(ANYPNTR)) & (-(Word_t)0x8)))

        case cJU_JPLEAF2:
        {
//          NOTE: max pop < 256 !!! (8 bits)                           
            Pop1 = ju_LeafPop1(Pjp);                            // leaf pop1 from jp_t
//          Pointer to Leaf2
            Pjll2_t Pjll2    = P_JLL2(RawPntr);
            CHECKIT(Index, Pjll2->jl2_LastKey, 2, Pop1);              // Check if in-leaf Dcd works
            uint16_t index16 = Index;                           // trim to 16 decoded bits
#ifdef  JUDYL
            Pjv = JL_LEAF2VALUEAREA(Pjll2, Pop1);               // start of Value area ^ 
#endif  // JUDYL
            SEARCHPOPULATION(Pop1);                             // enabled -DSEARCHMETRICS at compile time

            assert(((Pjp->jp_subLeafPops * CHARSUMS) >> (64 - 8)) == Pop1);     // cute huh!!

            int     subexpx8   = (index16 >> (16 - 3)) * 8;       // hi 3 bits * 8 = sub-expanse (0..7) * 8 to search
//          Get population of the sub-expanse containing the key
            uint8_t subpop16 = Pjp->jp_subLeafPops >> subexpx8;   // population of sub-Leaf

//          Note: this is about 1nS slower faster when found, but slightly faster when not found
//          And fixes the bug of possible testing for a Key outside of the malloc() of the Leaf
            if (subpop16 == 0)  // if no sub-Leaf empty, then not found
                goto NotFoundExit;

            Word_t PrePopMask = ((Word_t)1 << subexpx8) - 1;
            Word_t PrevPops = Pjp->jp_subLeafPops & PrePopMask; // mask to just previous Pops

            int  leafOff  = (PrevPops * CHARSUMS) >> 56;        // offset by sum of previous sub-Leafs
#ifdef  JUDYL
            Pjv += leafOff;                                     // start of Sub-Value area ^ 
#endif  // JUDYL
//          offset start of search ^ to sub expanse containing Key
            uint16_t *subLeaf16 = Pjll2->jl2_Leaf + leafOff;    // ^ to the sub-Leaf

#ifndef  SUBPOP1
//          Parallel Search does not work with just one(1) Key (if it is zero)
            if (subpop16 == 1) 
            {
                if (subLeaf16[0] == index16)
                {
                    DIRECTHITS;                                 // Count direct hits
                    posidx = 0;
                    goto LeafFoundExit;
                }
                goto NotFoundExit;
            }
#endif  // noSUBPOP1

//          linear interpolation of the location (0..31) of key in the sub-expanse
            posidx  = LERP(subpop16, Index, (16 - 3));          // 1st try of offset in sub-Leaf

#ifdef  JUDYL
            PREFETCH(Pjv + posidx);                             // start read of Value (hopefully)
#endif  // JUDYL

#ifndef  noLEAF2STACKEDPARA                                     // if stacked Leaf
//          Do a parallel search (4 Keys at a time)
            PWord_t BucketPtr = PWORDMASK(subLeaf16 + posidx);  // ^ Word containg Starting Key?

            assert(BucketPtr <  (PWord_t)(Pjll2->jl2_Leaf + Pop1));
            assert(BucketPtr >= (PWord_t)(Pjll2->jl2_Leaf));
//          Replicate the Lsb & Msb in every Key position (at compile time)
#undef cbPK
#define cbPK    (16)            // bits per Key
            Word_t repLsbKey = REPKEY(cbPK, 1);
            Word_t repMsbKey = repLsbKey << (cbPK - 1);
//          Make 4 copys of Key in Word_t
            Word_t repKey    = REPKEY(cbPK, index16);

//          Word_t aligned Begin pointer
//          Note:  This may cross a sub-expanse boundry, but that is OK

//          Also the BucketPntr could be at Beginning or End of sub-Leaf
            Word_t newBucket  = *BucketPtr ^ repKey;            // Key will now be 0
            newBucket         = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
            if (newBucket)                                      // Key found in Bucket?
            {
                DIRECTHITS;                             // Count direct hits
#ifdef  JUDYL
//              calculate position of Value from offset of Key in Leaf
                posidx = (uint16_t *)(BucketPtr) - subLeaf16;
//              add (0..3) position of Value from offset of Key in bucket
                posidx += __builtin_ctzl(newBucket) / cbPK;
#endif  // JUDYL
                goto LeafFoundExit;                // posidx & (only JudyL) Pjv

            }
//          Not found with the LERP prediction of Key
            if (subLeaf16[posidx] > index16)
            {
//              Pointer to Word_t containing 1st Key in sub-Leaf 
                PWord_t SubLeaf16Limit = PWORDMASK(subLeaf16 + 0);      

                for (--BucketPtr; BucketPtr >= SubLeaf16Limit; --BucketPtr)      // Decrement
                {
                    Word_t newBucket    = *BucketPtr ^ repKey;
                    newBucket           = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
                    if (newBucket)                          // Key found in Bucket?
                    {
                        MISCOMPARESM((PWord_t)subLeaf16 - BucketPtr);       // number of tries
#ifdef  JUDYL
//                      calculate position of Value from offset of Key in sub-Leaf
                        posidx = (uint16_t *)(BucketPtr) - subLeaf16;
                        posidx += __builtin_ctzl(newBucket) / cbPK;
#endif  // JUDYL
                        goto LeafFoundExit;                // posidx & (only JudyL) Pjv
                    }
                }
            }
            else
            {
//              Pointer to Word_t containing Last Key in sub-Leaf 
                PWord_t SubLeaf16Limit = PWORDMASK(subLeaf16 + subpop16 - 1);      

                for (++BucketPtr; BucketPtr <= SubLeaf16Limit; ++BucketPtr)      // Increment
                {
                    Word_t newBucket    = *BucketPtr ^ repKey;
                    newBucket           = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
                    if (newBucket)                          // Key found in Bucket?
                    {
                        MISCOMPARESM(BucketPtr - (PWord_t)subLeaf16);       // number of tries
#ifdef  JUDYL
//                      calculate position of Value from offset of Key in sub-Leaf
                        posidx = (uint16_t *)BucketPtr - subLeaf16;
                        posidx += __builtin_ctzl(newBucket) / cbPK;
#endif  // JUDYL
                        goto LeafFoundExit;                // posidx & (only JudyL) Pjv
                    }
                }
            }
//          reached end of sub-expanse
            goto NotFoundExit;
#else   //  ! noLEAF2STACKEDPARA

//          search a hopefully small expanse
            posidx = j__udySearchRawLeaf2(subLeaf16, subpop16, index16, posidx);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
#endif  // noLEAF2STACKEDPARA
        }
#endif  // noFANCYLEAF2

        case cJU_JPLEAF3:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll3_t Pjll3 = P_JLL3(RawPntr);
            CHECKIT(Index, Pjll3->jl3_LastKey, 3, Pop1);
#ifdef  JUDYL
            Pjv = JL_LEAF3VALUEAREA(Pjll3, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf3(Pjll3, Pop1, Index, 3 * 8);
            goto CommonLeafExit;        // posidx & JudyL Pjv

CommonLeafExit:         // with posidx & JudyL Pjv only
            if (posidx < 0) 
                goto NotFoundExit;
LeafFoundExit:
#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
            return((PPvoid_t) (Pjv + posidx));
#endif  // JUDYL

        }

        case cJU_JPLEAF4:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll4_t Pjll4 = P_JLL4(RawPntr);
            CHECKIT(Index, Pjll4->jl4_LastKey, 4, Pop1);
#ifdef  JUDYL
            Pjv = JL_LEAF4VALUEAREA(Pjll4, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf4(Pjll4, Pop1, Index, 4 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF5:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll5_t Pjll5 = P_JLL5(RawPntr);
            CHECKIT(Index, Pjll5->jl5_LastKey, 5, Pop1);
#ifdef  JUDYL
            Pjv = JL_LEAF5VALUEAREA(Pjll5, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf5(Pjll5, Pop1, Index, 5 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF6:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll6_t Pjll6 = P_JLL6(RawPntr);
            CHECKIT(Index, Pjll6->jl6_LastKey, 6, Pop1);
#ifdef  JUDYL
            Pjv = JL_LEAF6VALUEAREA(Pjll6, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf6(Pjll6, Pop1, Index, 6 * 8);





#ifdef later
            if (Pop1 == cJU_LEAF6_MAXPOP1)
            {
                printf("\n--Get Leaf6 Index = 0x%016lx\n", Index);
                for (int ii = 0; ii < Pop1; ii++)
                {
                    printf("%3d = 0x%016lx ", ii, Pjll6->jl6_Leaf[ii]);
                    if (Pjll6->jl6_Leaf[ii] == Index) printf("*");
                    printf("\n");
                }
            }
#endif  // later




            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF7:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll7_t Pjll7 = P_JLL7(RawPntr);
            CHECKIT(Index, Pjll7->jl7_LastKey, 7, Pop1);
#ifdef  JUDYL
            Pjv = JL_LEAF7VALUEAREA(Pjll7, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf7(Pjll7, Pop1, Index, 7 * 8);



#ifdef later
            if (Pop1 == cJU_LEAF7_MAXPOP1)
            {
                printf("\n--Get Leaf7 Index = 0x%016lx\n", Index);
                for (int ii = 0; ii < Pop1; ii++)
                {
                    printf("%3d = 0x%016lx ", ii, Pjll7->jl7_Leaf[ii]);
                    if (Pjll7->jl7_Leaf[ii] == Index) printf("*");
                    printf("\n");
                }
            }
#endif  // later

            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

// ****************************************************************************
// JPLEAF_B1U:

        case cJU_JPLEAF_B1U:
        {
            Pop1          = ju_LeafPop1(Pjp);   // for CHECKIT
            Pjlb_t Pjlb   = P_JLB1(RawPntr);
            CHECKIT(Index, Pjlb->jlb_LastKey, 1, Pop1);
#ifdef  JUDYL
            Pjv = JL_JLB_PVALUE(Pjlb);
#endif  // JUDYL
            posidx = (uint8_t)Index;

            DIRECTHITS;       // not necessary, because always 100%
            SEARCHPOPULATION(ju_LEAF_POP1(Pjp)); // special one to handle 256

            Word_t  sube     = posidx / cJU_BITSPERSUBEXPL;        // 0..3
            BITMAPL_t BitMap = JU_JLB_BITMAP(Pjlb, sube);
            BITMAPL_t BitMsk = JU_BITPOSMASKL(posidx);

// No value in sub-expanse for Index => Index not found:

            if (BitMap & BitMsk)
                goto LeafFoundExit;
            goto NotFoundExit;
        } // case cJU_JPLEAF_B1U

#ifdef JUDY1
// ****************************************************************************
// JPFULLPOPU1:
//
// If the Index is in the expanse, it is necessarily valid (found).

        case cJ1_JPFULLPOPU1:
        {
//            CHECKIT(Index, Pjll1->jl1_LastKey, 1); not nec, always in a Branch
//            DIRECTHITS;       // not necessary, because always 100%
//            SEARCHPOPULATION(256);
            return(1);
        }
#endif // JUDY1

// ****************************************************************************
// JPIMMED*:
//
// Note that the contents of jp_DcdPopO is different for cJU_JPIMMED_*_01:

        case cJU_JPIMMED_1_01:          // 1 byte decode
        case cJU_JPIMMED_2_01:          // 2 byte decode
        case cJU_JPIMMED_3_01:          // 3 byte decode
        case cJU_JPIMMED_4_01:          // 4 byte decode
        case cJU_JPIMMED_5_01:          // 5 byte decode
        case cJU_JPIMMED_6_01:          // 6 byte decode
        case cJU_JPIMMED_7_01:          // 7 byte decode
        {
            SEARCHPOPULATION(1);        // Too much overhead????
            DIRECTHITS;                 // Count direct hits

//          The "<< 8" makes a possible bad assumption!!!!, but
//          this version does not have an conditional branch
            if ((ju_IMM01Key(Pjp) ^ Index) << 8) 
                goto NotFoundExit;
#ifdef  JUDY1
        return(1);
#endif  // JUDY1

#ifdef  JUDYL
        return((PPvoid_t) ju_PImmVal_01(Pjp));  // ^ to immediate Value
#endif  // JUDYL
        }

#ifdef  JUDY1
        case cJ1_JPIMMED_1_15:
        case cJ1_JPIMMED_1_14:
        case cJ1_JPIMMED_1_13:
        case cJ1_JPIMMED_1_12:
        case cJ1_JPIMMED_1_11:
        case cJ1_JPIMMED_1_10:
        case cJ1_JPIMMED_1_09:
#endif  // JUDY1
        case cJU_JPIMMED_1_08:
        case cJU_JPIMMED_1_07:
        case cJU_JPIMMED_1_06:
        case cJU_JPIMMED_1_05:
        case cJU_JPIMMED_1_04:
        case cJU_JPIMMED_1_03:
        case cJU_JPIMMED_1_02:
        {
#ifdef  JUDYL
            Pjv = P_JV(RawPntr);                // Get ^ to Values
#endif  // JUDYL
            Pop1            = ju_Type(Pjp) - cJU_JPIMMED_1_02 + 2;
            uint8_t *Pleaf1 = ju_PImmed1(Pjp);  // Get ^ to Keys
#ifdef OLDWAYI
            posidx = j__udySearchImmed1(Pleaf1, Pop1, Index, 1 * 8);
#else   // ! OLDWAYI
            int start       = LERP(Pop1, Index, 1 * 8);
#ifdef  JUDYL
            PREFETCH(Pjv + start);              // start read of Value (hopefully)
#endif  // JUDYL
            SEARCHPOPULATION(Pop1);
            posidx = j__udySearchRawLeaf1(Pleaf1, Pop1, Index, start);  // should be Parallel Search!!!!
#endif  // ! OLDWAYI
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

#ifdef  JUDY1
        case cJ1_JPIMMED_2_07:
        case cJ1_JPIMMED_2_06:
        case cJ1_JPIMMED_2_05:
#endif  // JUDY1

        case cJU_JPIMMED_2_04:
        case cJU_JPIMMED_2_03:
        case cJU_JPIMMED_2_02:
        {
#ifdef  JUDYL
            Pjv = P_JV(RawPntr);                // Get ^ to Values
#endif  // JUDYL
            Pop1             = ju_Type(Pjp) - cJU_JPIMMED_2_02 + 2;
            uint16_t *Pleaf2 = ju_PImmed2(Pjp); // Get ^ to Keys
            int start = LERP(Pop1, Index, 1 * 16);
#ifdef  JUDYL
            PREFETCH(Pjv + start);              // start read of Value (hopefully)
#endif  // JUDYL
            SEARCHPOPULATION(Pop1);
            posidx           = j__udySearchRawLeaf2(Pleaf2, Pop1, Index, start);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

#ifdef  JUDYL
        case cJL_JPIMMED_3_02:
        {
            uint32_t *Pleaf3 = ju_PImmed3(Pjp);
            posidx = 0;
            Pjv = P_JV(RawPntr);                // ^ immediate values area
            if (Pleaf3[0] == (uint32_t)Index)   // only 2 Keys to check
                goto CommonLeafExit;            // posidx & (only JudyL) Pjv
            posidx = 1;
            if (Pleaf3[1] == (uint32_t)Index)
                goto CommonLeafExit;            // posidx & (only JudyL) Pjv
            goto NotFoundExit;
        }

        case cJL_JPIMMED_4_02:
        {
            Pjv              = P_JV(RawPntr);   // ^ immediate values area
            uint32_t *Pleaf4 = ju_PImmed4(Pjp);
            posidx = 0;
            if (Pleaf4[0] == (uint32_t)Index)   // only 2 Keys to check
                goto CommonLeafExit;            // posidx & (only JudyL) Pjv
            posidx = 1;
            if (Pleaf4[1] == (uint32_t)Index)
                goto CommonLeafExit;            // posidx & (only JudyL) Pjv
            goto NotFoundExit;
        }
#endif  // JUDYL

#ifdef  JUDY1
        case cJ1_JPIMMED_3_02:
        case cJ1_JPIMMED_3_03:
        {
            uint32_t *Pleaf3 = ju_PImmed3(Pjp);     // Get ^ to Keys
#ifdef  SLOWIMMED3
            Pop1             = ju_Type(Pjp) - cJU_JPIMMED_3_02 + 2;
            posidx = j__udySearchImmed3(Pleaf3, Pop1, Index, 3 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
#else   // SLOWIMMED3    Faster?????
            if (Pleaf3[0] == (uint32_t)Index)
                return(1);
            if (Pleaf3[1] == (uint32_t)Index)
                return(1);
            if (ju_Type(Pjp) != cJ1_JPIMMED_3_03)
                goto NotFoundExit;
            if (Pleaf3[2] == (uint32_t)Index)
                return(1);
            goto NotFoundExit;
#endif  // SLOWIMMED3
        }
        case cJ1_JPIMMED_4_02:
        case cJ1_JPIMMED_4_03:
        {
            uint32_t *Pleaf4 = ju_PImmed4(Pjp);     // Get ^ to Keys
#ifdef  SLOWIMMED4
            Pop1             = ju_Type(Pjp) - cJU_JPIMMED_4_02 + 2;
            posidx = j__udySearchImmed4(Pleaf4, Pop1, Index, 4 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
#else   // SLOWIMMED4    Faster?????
            if (Pleaf4[0] == (uint32_t)Index)
                return(1);
            if (Pleaf4[1] == (uint32_t)Index)
                return(1);
            if (ju_Type(Pjp) != cJ1_JPIMMED_4_03)
                goto NotFoundExit;
            if (Pleaf4[2] == (uint32_t)Index)
                return(1);
            goto NotFoundExit;
#endif  // SLOWIMMED4
        }

        case cJ1_JPIMMED_5_02:
        case cJ1_JPIMMED_6_02:
        case cJ1_JPIMMED_7_02:
        {
            posidx = j__udy1SearchImm02(Pjp, Index);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // JUDY1

// ****************************************************************************
// INVALID JP TYPE:

        default:
ReturnCorrupt:                  // return not found -- no error return now
            goto NotFoundExit;

//            JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
//            JUDY1CODE(return(JERRI );)
//            JUDYLCODE(return(PPJERR);)

        } // switch on JP type

NotFoundExit:

#ifdef  TRACEJPG
    if (PArray && startpop && (*(Word_t *)PArray >= startpop))
    {
#ifdef JUDY1
        printf("0x%lx ---Judy1Test NotFoundExit Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAF8_POP0(PArray) + 1));
#else   // JUDYL
        printf("0x%lx ---JudyLGet  NotFoundExit Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAF8_POP0(PArray) + 1));
#endif // JUDYL
    }
#endif  // TRACEJPG

#ifdef  JUDY1
            return(0);
#endif  // JUDY1

#ifdef  JUDYL
            return((PPvoid_t) NULL);
#endif  // JUDYL

} // Judy1Test() / JudyLGet()
#endif  // ! JUDYGETINLINE
