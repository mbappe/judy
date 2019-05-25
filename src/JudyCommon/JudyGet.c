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

#ifdef TRACEJPG                 // different macro name, for "retrieval" only.
#include "JudyPrintJP.c"
#endif

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


// Special version when Pop is going to be 256 - no branches
#define ju_LEAF_POP1(Pjp) ((uint8_t)(ju_LeafPop1(Pjp) - 1) + 1)

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
    uint8_t   Digit = 0;                // byte just decoded from Index.

    (void) PJError;                     // no longer used

    if (PArray == (Pcvoid_t)NULL)  // empty array.
        goto NotFoundExit;

#ifdef  TRACEJPG
    if (startpop && (*(Word_t *)PArray >= startpop))
    {
        printf("*PArray = 0x%016lx, startpop = 0x%016lx\n", *(Word_t *)PArray, startpop);
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
        {
            break;
        }

// Note:  Here the calls of ju_DcdNotMatchKey() are necessary and check
// whether Index is out of the expanse of a narrow pointer.
//
// ****************************************************************************
// JPBRANCH_L*:
//
        case cJU_JPBRANCH_L2:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 2)) break;
            Digit = JU_DIGITATSTATE(Index, 2);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L3:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 3)) break;

            Digit = JU_DIGITATSTATE(Index, 3);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L4:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 4)) break;
            Digit = JU_DIGITATSTATE(Index, 4);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L5:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 5)) break;
            Digit = JU_DIGITATSTATE(Index, 5);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L6:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 6)) break;
            Digit = JU_DIGITATSTATE(Index, 6);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L7:
        {
//            if (ju_DcdNotMatchKey(Index, Pjp, 7)) break;
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
                break;

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
            if (ju_DcdNotMatchKey(Index, Pjp, 2)) break;
            Digit = JU_DIGITATSTATE(Index, 2);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B3:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 3)) break;
            Digit = JU_DIGITATSTATE(Index, 3);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B4:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 4)) break;
            Digit = JU_DIGITATSTATE(Index, 4);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B5:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 5)) break;
            Digit = JU_DIGITATSTATE(Index, 5);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B6:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 6)) break;
            Digit = JU_DIGITATSTATE(Index, 6);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B7:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 7)) break;
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
                break;

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
            if (ju_DcdNotMatchKey(Index, Pjp, 7)) break;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 7);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U6:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 6)) break;

            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 6);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U5:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 5)) break;

            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 5);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U4:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 4)) break;

            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 4);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U3:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 3)) break;

            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 3);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U2:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 2)) break;

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
// JPLEAF_B1:
//
        case cJU_JPLEAF_B1:
        {
            Pjlb_t    Pjlb   = P_JLB1(RawPntr);

            DIRECTHITS;       // not necessary, because always 100%
            SEARCHPOPULATION(ju_LEAF_POP1(Pjp)); 

            uint8_t digit  = (uint8_t)Index;
            Word_t  sube   = digit / cJU_BITSPERSUBEXPL;        // 0..3

            BITMAPL_t BitMap = JU_JLB_BITMAP(Pjlb, sube);
            BITMAPL_t BitMsk = JU_BITPOSMASKL(digit);

            if (! (BitMap & BitMsk)) 
                break;                          // not found
#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
//          offset to pointer of Value area
            return((PPvoid_t) (JL_JLB_PVALUE(Pjlb) + digit));
#endif // JUDYL

        } // case cJU_JPLEAF_B1

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
            SEARCHPOPULATION(1);      // Too much overhead?
            DIRECTHITS;               // Count direct hits
//          This version does not have an conditional branch - I hope
            if ((ju_IMM01Key(Pjp) ^ Index) << 8) // mask off high byte
                break;
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

#ifdef  JUDY1
        case cJ1_JPIMMED_3_05: 
        case cJ1_JPIMMED_3_04:
        case cJ1_JPIMMED_3_03:
#endif  // JUDY1

        case cJU_JPIMMED_3_02:
        {
            Pop1 = ju_Type(Pjp) - cJU_JPIMMED_3_02 + 2;
#ifdef  JUDYL
            Pjv = P_JV(RawPntr);  // ^ immediate values area
#endif  // JUDYL
            uint8_t *Pleaf3 = ju_PImmed3(Pjp);
            posidx = j__udySearchImmed3(Pleaf3, Pop1, Index, 3 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

#ifdef  JUDYL
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
        case cJ1_JPIMMED_4_02:
        case cJ1_JPIMMED_4_03:
        {
            Pop1 = ju_Type(Pjp) - cJ1_JPIMMED_4_02 + 2;
            uint32_t *Pleaf4 = ju_PImmed4(Pjp);
            posidx = j__udySearchImmed4(Pleaf4, Pop1, Index, 4 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJ1_JPIMMED_5_02:
        case cJ1_JPIMMED_5_03:
        {
            Pop1 = ju_Type(Pjp) - cJ1_JPIMMED_5_02 + 2;
            uint8_t *Pleaf5 = ju_PImmed5(Pjp);  // Get ^ to Keys
            posidx = j__udySearchImmed5(Pleaf5, Pop1, Index, 5 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJ1_JPIMMED_6_02:
        {
            Pop1 = 2;
            uint8_t *Pleaf6 = ju_PImmed6(Pjp);  // Get ^ to Keys
            posidx = j__udySearchImmed6(Pleaf6, Pop1, Index, 6 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJ1_JPIMMED_7_02:
        {
            Pop1 = 2;
            uint8_t *Pleaf7 = ju_PImmed7(Pjp);  // Get ^ to Keys
            posidx = j__udySearchImmed7(Pleaf7, Pop1, Index, 7 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // JUDY1

// ****************************************************************************
// INVALID JP TYPE:

        default:
ReturnCorrupt:                  // just return not found
            break;

//            JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
//            JUDY1CODE(return(JERRI );)
//            JUDYLCODE(return(PPJERR);)

        } // switch on JP type

NotFoundExit:

#ifdef  TRACEJPG
    if (startpop && (*(Word_t *)PArray >= startpop))
    {
#ifdef JUDY1
        printf("---j__udy1Test Key = 0x%016lx NOT FOUND\n", Index);
#else /* JUDYL */
        printf("---j__udy1LGet Key = 0x%016lx NOT FOUND\n", Index);
#endif /* JUDYL */
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
        {
            break;
        }

// ****************************************************************************
// JPBRANCH_L*:
//
        case cJU_JPBRANCH_L2:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 2)) break;
            Digit = JU_DIGITATSTATE(Index, 2);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L3:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 3)) break;
            Digit = JU_DIGITATSTATE(Index, 3);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L4:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 4)) break;
            Digit = JU_DIGITATSTATE(Index, 4);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L5:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 5)) break;
            Digit = JU_DIGITATSTATE(Index, 5);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L6:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 6)) break;
            Digit = JU_DIGITATSTATE(Index, 6);
            goto JudyBranchL;
        }

        case cJU_JPBRANCH_L7:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 7)) break;
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
                break;

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
            if (ju_DcdNotMatchKey(Index, Pjp, 2)) break;
            Digit = JU_DIGITATSTATE(Index, 2);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B3:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 3)) break;
            Digit = JU_DIGITATSTATE(Index, 3);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B4:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 4)) break;
            Digit = JU_DIGITATSTATE(Index, 4);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B5:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 5)) break;
            Digit = JU_DIGITATSTATE(Index, 5);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B6:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 6)) break;
            Digit = JU_DIGITATSTATE(Index, 6);
            goto JudyBranchB;
        }

        case cJU_JPBRANCH_B7:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 7)) break;
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
                break;

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
            if (ju_DcdNotMatchKey(Index, Pjp, level)) break;
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
            if (ju_DcdNotMatchKey(Index, Pjp, 7)) break;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 7);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U6:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 6)) break;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 6);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U5:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 5)) break;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 5);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U4:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 4)) break;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 4);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U3:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 3)) break;
            Pjp =  P_JBU(RawPntr)->jbu_jp + JU_DIGITATSTATE(Index, 3);
            goto ContinueWalk;
        }

        case cJU_JPBRANCH_U2:
        {
            if (ju_DcdNotMatchKey(Index, Pjp, 2)) break;
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
#define CHECKIT(INDEX, LASTKEY, STATE)                          \
{                                                               \
    if (((INDEX) ^ (LASTKEY)) & cJU_DCDMASK(STATE))             \
    {                                                           \
        printf("\n------------Oops Lastkey fail Line = %d, Level = %d\n", (int)__LINE__, STATE); \
        printf("INDEX          = 0x%016lx\n", INDEX);                  \
        printf("LASTKEY        = 0x%016lx\n", LASTKEY);                \
        printf("cJU_DCDMASK(%d) = 0x%016lx\n", STATE, cJU_DCDMASK(STATE)); \
        exit(0);                                                \
    }                                                           \
}
#else  // CHECKIT
#define CHECKIT(INDEX, LASTKEY, STATE) (0)
#endif  // noCHECKIT

// magic constant to sum char sized populations with mpy
#define CHARSUMS ((Word_t)0x0101010101010101)

//  ***************************************************************
//  This is only for JudyL because Judy1 does not have a Leaf1
//  ***************************************************************
#ifdef  JUDYL

#ifdef  noLEAF1STACKED
// Linear Interpolation
// Linear interpolation is done in j__udySearchLeaf2()
#ifdef  WITHOUTPREFETCH
        case cJL_JPLEAF1:
        {
            Pop1          = ju_LeafPop1(Pjp);

//          Prepare for Stacked Leaf1 -- mpy does shifts and adds
            assert(((Pjp->jp_subLeafPops * CHARSUMS) >> (64 - 8)) == Pop1); // cute huh!!

            Pjll1_t Pjll1 = P_JLL1(RawPntr);
            Pjv  = JL_LEAF1VALUEAREA(Pjll1, Pop1);

            posidx = j__udySearchLeaf1(Pjll1, Pop1, Index, 1 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#else   // ! WITHOUTPREFETCH
        case cJL_JPLEAF1:
        {
            Pjll1_t Pjll1 = P_JLL1(RawPntr);
            uint8_t index8 = Index;
            Pop1           = ju_LeafPop1(Pjp);
            Pjv  = JL_LEAF1VALUEAREA(Pjll1, Pop1);

            SEARCHPOPULATION(Pop1);     // enabled -DSEARCHMETRICS at compile time

            posidx = LERP(Pop1, index8, 1 * 8);
//            posidx = (Pop1 * index8) / (Pjll1->jl1_Leaf[Pop1 - 1] + 1);

            PREFETCH(Pjv + posidx);         // start read of Value (hopefully)
            if (Pjll1->jl1_Leaf[posidx] == index8)
            {
                DIRECTHITS;                     // Count direct hits
                goto CommonLeafExit;            // posidx & (only JudyL) Pjv
            }
            posidx = j__udySearchRawLeaf1(Pjll1->jl1_Leaf, Pop1, index8, posidx);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // ! WITHOUTPREFETCH
#endif  // noLEAF1STACKED


#ifndef noLEAF1STACKED
// Stacked Leaf1 with option of a Parallel search of sub-expanse.

#undef cbPK
#define cbPK    (8)            // bits per Key
        case cJL_JPLEAF1:
        {
//          Pointer to Leaf1
            Pjll1_t Pjll1       = P_JLL1(RawPntr);
            CHECKIT(Index, Pjll1->jl1_LastKey, 1);

            uint8_t index8      = Index;                        // trim off decoded bits
            int     subkey5      = index8 % (1 << (8 - 3));     // 0..31 Least 5 bits of Key
            int     subexp      = index8 >> (8 - 3);            // hi 3 bits = sub-expanse (0..7) to search
            subexp             *= 8;                            // convert shift bytes to shift bits
            Word_t  subLeafPops = Pjp->jp_subLeafPops;          // in-cache sub-expanse pop1

//          NOTE: max pop must be less than 256 !!!
            Pop1 = ju_LeafPop1(Pjp);                // leaf pop1 from jp_t
            assert(((subLeafPops * CHARSUMS) >> (64 - 8)) == Pop1);   // cute huh!!

            SEARCHPOPULATION(Pop1);     // enabled -DSEARCHMETRICS at compile time

//          get offset to current sub expanse
            int  leafOff  = j__udySubLeaf8Pops(subLeafPops & ((Word_t)1 << subexp) - 1);
            uint8_t *subLeaf8  = Pjll1->jl1_Leaf + leafOff;     // ^ to the subLeaf
            Pjv = JL_LEAF1VALUEAREA(Pjll1, Pop1) + leafOff;     // Sub-Value area ^ 

//          Get the population of the sub-expanse containing the subkey5
            uint8_t subpop8       = (subLeafPops >> subexp) /* & 0xFF */; // 0..32 - (mask 0..63)

#ifdef  SUB0CHECK
//          Note: this is slightly faster when not found, but slightly slower when found
            if (subpop8 == 0)            // if subexp empty, then not found
                break;
#endif  // SUB0CHECK
            
//          do a linear interpolation of the location (0..31) of key in the sub-expanse
//            int start         = (subpop8 * subkey5) / 32;        // 32 == sub-expanse size
            int start = LERP(subpop8, subkey5, 1 * (8 - 3));
//          cover 3 cache lines
#ifdef  JUDYL
//  no Effect          PREFETCH(Pjv + start - (64/8));     // start read of Value (hopefully)
//  no Effect          PREFETCH(Pjv + start + (64/8));     // start read of Value (hopefully)
            PREFETCH(Pjv + start);              // start read of Value (hopefully)
#endif  // JUDYL

#ifndef  noLEAF1STACKEDPARA
// NOTE: The Direct hits are > 90%, so leave out parallel miss code
//          Replicate the Lsb & Msb in every Key position (at compile time)
            Word_t repLsbKey = REPKEY(cbPK, 1);
            Word_t repMsbKey = repLsbKey << (cbPK - 1);
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
#endif  // noLEAF1STACKED

#endif  // JUDYL

#ifdef  noLEAF2STACKED
// the prefetch stuff is in j__udySearchLeaf2
// Linear interpolation is done in j__udySearchLeaf2()
        case cJU_JPLEAF2:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll2_t Pjll2 = P_JLL2(RawPntr);

//          Prepare for Stacked Leaf2 -- mpy does shifts and adds -- very fast
            assert(((Pjp->jp_subLeafPops * CHARSUMS) >> (64 - 8)) == Pop1); // cute huh!!

#ifdef  JUDYL
            Pjv = JL_LEAF2VALUEAREA(Pjll2, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf2(Pjll2, Pop1, Index, 2 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // noLEAF2STACKED


#ifndef noLEAF2STACKED
// Stacked Leaf2 with option of a Parallel search of sub-expanse.
// Need check if any good in JUDYL

#undef cbPK
#define cbPK    (16)            // bits per Key
// make any Pointer a PWord_t aligned pointer by masking off least bits
#define PWORDMASK(ANYPNTR)      ((PWord_t)(((Word_t)(ANYPNTR)) & (-(Word_t)8)))

        case cJU_JPLEAF2:
        {
//          Pointer to Leaf2
            Pjll2_t Pjll2       = P_JLL2(RawPntr);
            CHECKIT(Index, Pjll2->jl2_LastKey, 2);              // Check if in-leaf Dcd works

            Word_t   subLeafPops = Pjp->jp_subLeafPops;         // in-cache 8 sub-expanses pops
            uint16_t index16     = Index;                       // trim to 16 decoded bits
            int      subkey13    = index16 % (1 << (16 - 3));   // 0..8191 Least 13 bits of Key
            int      subexp      = index16 >> (16 - 3);         // hi 3 bits = sub-expanse (0..7) to search
            subexp              *= 8;                           // convert shift bytes to shift bits
            int      sDir;                                      // forward or backward search

//          NOTE: max pop is 255 !!! (8 bits)                           
            Pop1 = ju_LeafPop1(Pjp);                            // leaf pop1 from jp_t
            assert(((subLeafPops * CHARSUMS) >> (64 - 8)) == Pop1);     // cute huh!!

#ifdef  JUDYL
            Pjv = JL_LEAF2VALUEAREA(Pjll2, Pop1);               // start of Value area ^ 
#endif  // JUDYL

            SEARCHPOPULATION(Pop1);                             // enabled -DSEARCHMETRICS at compile time

//          Get population of the sub-expanse containing the key
            uint8_t subpop16 = (subLeafPops >> subexp);         // & 0xFF 0..32 - (mask 0..63)

#ifdef  SUB0CHECK
//          Note: this is about 1nS slower faster when found, but slightly faster when not found
            if (subpop16 == 0)                          // if subexp empty (no Pop), then not found
                break;
#endif  // SUB0CHECK

            subLeafPops  &= ((Word_t)1 << subexp) - 1;          // mask to just previous Pops
            int  leafOff  = (subLeafPops * CHARSUMS) >> 56;     // offset by sum of prior sub-expanses
#ifdef  JUDYL
            Pjv += leafOff;                                     // start of Sub-Value area ^ 
#endif  // JUDYL
//          offset start of search to sub expanse containing Key
            uint16_t *subLeaf16 = Pjll2->jl2_Leaf + leafOff;    // ^ to the subLeaf
#ifdef  JUDY1
            PREFETCH(subLeaf16);                                // start read of Key area(hopefully)
#endif  // JUDY1

//          linear interpolation of the location (0..31) of key in the sub-expanse
            posidx  = LERP(subpop16, subkey13, (16 - 3));       // 1st try of offset in sub-expanse
#ifdef  JUDYL
            PREFETCH(Pjv + posidx);                             // start read of Value (hopefully)
#endif  // JUDYL


#ifdef  diag
if (subpop16 > 16)
{
printf("\nKey = 0x%04x, start = %d, subLeaf16 = 0x%lx, subpop16 = %d\n", index16, start, (Word_t)subLeaf16 & 0x7, subpop16);
for (int loff = 0; loff < subpop16; loff++)
{
    printf("%3d = 0x%04x", loff, subLeaf16[loff]);
    if (subLeaf16[loff] == index16) printf("*");
    if (start == loff) printf("?");
    printf("\n");
}
}
#endif  // diag


#ifndef  noLEAF2STACKEDPARA
// Do a parallel search (4 Keys at a time)

//          Replicate the Lsb & Msb in every Key position (at compile time)
            Word_t repLsbKey = REPKEY(cbPK, 1);
            Word_t repMsbKey = repLsbKey << (cbPK - 1);
//          Make 8 copys of Key in Word_t
            Word_t repKey    = REPKEY(cbPK, index16);

//          Word_t aligned Begin pointer
//          Note:  This may cross a sub-expanse boundry, but that is OK
            PWord_t BucketPtr = PWORDMASK(subLeaf16 + posidx);  // ^ Word containg Starting Key

            Word_t newBucket  = BucketPtr[0] ^ repKey;
            newBucket         = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
            if (newBucket)                                      // Key found in Bucket?
            {
                DIRECTHITS;                             // Count direct hits
#ifdef  JUDYL
//              calculate position of Value from offset of Key in Leaf
                posidx = (uint16_t *)(BucketPtr) - subLeaf16;
                posidx += __builtin_ctzl(newBucket) / cbPK;
                return((PPvoid_t) (Pjv + posidx));
#endif  // JUDYL

#ifdef  JUDY1
                return(1);
#endif  // JUDY1
            }

#ifdef Broken_with_missing_key_and_a_little_bit_slower_when_found
// I think a more accurate LERP is where my effort should be put

            PWord_t SubLeaf16Limit;
//          Word_t aligned pointers for Begin and End words
            if (subLeaf16[posidx] > index16)
            {
                sDir = -1;                                      // search backward
                SubLeaf16Limit = PWORDMASK(subLeaf16 + 0);      // ^ Word containing Begin sub-expanse
            }
            else
            {
                sDir = 1;                                       // search forward
                SubLeaf16Limit = PWORDMASK(subLeaf16 + Pop1 - 1); // ^ Word containing End sub-expanse
            }
//          Begin search 4 Keys at a time
            for (int l4off = sDir; ;l4off += sDir)   // Inc or Dec
            {
//   printf("l4off = %d, Index = 0x%016lx\n", l4off, Index);
                Word_t newBucket    = BucketPtr[l4off] ^ repKey;
                newBucket           = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;

                if (newBucket)                                  // Key found in Bucket?
                {
                    if (sDir < 0) { MISCOMPARESM(-l4off); }     // search backward
                    else          { MISCOMPARESP(l4off);  }     // search forward
#ifdef  JUDYL
//              calculate position of Value from offset of Key in Leaf
                posidx = __builtin_ctzl(newBucket) / cbPK;
                posidx += (uint16_t *)(BucketPtr + l4off)- subLeaf16;
                return((PPvoid_t) (Pjv + posidx));
#endif  // JUDYL

#ifdef  JUDY1
                return(1);
#endif  // JUDY1
                }
//              reached end of sub-expanse
                if (SubLeaf16Limit == BucketPtr + l4off) 
                    goto NotFoundExit;
            }
            goto NotFoundExit;
#endif  // Broken_with_missing_key_and_a_little_bit_slower_when_found
//          search a hopefully small expanse
            posidx = j__udySearchRawLeaf2(subLeaf16, subpop16, index16, posidx);

#else   //  noLEAF2STACKEDPARA

//          search a hopefully small expanse
            posidx = j__udySearchRawLeaf2(subLeaf16, subpop16, index16, posidx);
#endif  // noLEAF2STACKEDPARA

            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // noLEAF2STACKED

        case cJU_JPLEAF3:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll3_t Pjll3 = P_JLL3(RawPntr);
            CHECKIT(Index, Pjll3->jl3_LastKey, 3);
#ifdef  JUDYL
            Pjv = JL_LEAF3VALUEAREA(Pjll3, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf3(Pjll3, Pop1, Index, 3 * 8);
            goto CommonLeafExit;        // posidx & JudyL Pjv

CommonLeafExit:         // with posidx & JudyL Pjv only
            if (posidx < 0) 
                break;
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
            CHECKIT(Index, Pjll4->jl4_LastKey, 4);
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
            CHECKIT(Index, Pjll5->jl5_LastKey, 5);
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
            CHECKIT(Index, Pjll6->jl6_LastKey, 6);
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
            CHECKIT(Index, Pjll7->jl7_LastKey, 7);
#ifdef  JUDYL
            Pjv = JL_LEAF7VALUEAREA(Pjll7, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf7(Pjll7, Pop1, Index, 7 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

// ****************************************************************************
// JPLEAF_B1:

        case cJU_JPLEAF_B1:
        {
            Pjlb_t Pjlb   = P_JLB1(RawPntr);
            CHECKIT(Index, Pjlb->jlb_LastKey, 1);
#ifdef  JUDYL
            Pjv = JL_JLB_PVALUE(Pjlb);
#endif  // JUDYL
            uint8_t digit = Index;
//  noEffect  PREFETCH(Pjv + digit);

            DIRECTHITS;       // not necessary, because always 100%
            SEARCHPOPULATION(ju_LEAF_POP1(Pjp)); // special one to handle 256

            Word_t  sube   = digit / cJU_BITSPERSUBEXPL;        // 0..3
            BITMAPL_t BitMap = JU_JLB_BITMAP(Pjlb, sube);
            BITMAPL_t BitMsk = JU_BITPOSMASKL(digit);

// No value in sub-expanse for Index => Index not found:

            if (! (BitMap & BitMsk)) 
                break;                          // not found
#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
//          offset to pointer of Uncompressed Value area
            return((PPvoid_t) (Pjv + digit));
#endif // JUDYL

        } // case cJU_JPLEAF_B1

#ifdef JUDY1
// ****************************************************************************
// JPFULLPOPU1:
//
// If the Index is in the expanse, it is necessarily valid (found).

        case cJ1_JPFULLPOPU1:
        {
//            CHECKIT(Index, Pjll1->jl1_LastKey, 1); not nec, always in a Branch
            DIRECTHITS;       // not necessary, because always 100%
            SEARCHPOPULATION(256);
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
            SEARCHPOPULATION(1);        // Too much overhead?
            DIRECTHITS;                 // Count direct hits

//          The "<< 8" makes a possible bad assumption!!!!, but
//          this version does not have an conditional branch
            if ((ju_IMM01Key(Pjp) ^ Index) << 8) 
                break;

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
            Pop1 = ju_Type(Pjp) - cJU_JPIMMED_1_02 + 2;
            uint8_t *Pleaf1 = ju_PImmed1(Pjp);  // Get ^ to Keys
#ifdef OLDWAY
            posidx = j__udySearchImmed1(Pleaf1, Pop1, Index, 1 * 8);
#else   // ! OLDWAY
            int start = LERP(Pop1, (uint8_t)Index, 1 * 8);
#ifdef  JUDYL
            PREFETCH(Pjv + start);              // start read of Value (hopefully)
#endif  // JUDYL
            SEARCHPOPULATION(Pop1);
            posidx = j__udySearchRawLeaf1(Pleaf1, Pop1, (uint8_t)Index, start);
#endif  // ! OLDWAY
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
            Pop1 = ju_Type(Pjp) - cJU_JPIMMED_2_02 + 2;
            uint16_t *Pleaf2 = ju_PImmed2(Pjp);     // Get ^ to Keys
#ifdef OLDWAY
            posidx = j__udySearchImmed2(Pleaf2, Pop1, Index, 2 * 8);
#else   // ! OLDWAY
            int start = LERP(Pop1, (uint16_t)Index, 1 * 16);
#ifdef  JUDYL
            PREFETCH(Pjv + start);              // start read of Value (hopefully)
#endif  // JUDYL
            SEARCHPOPULATION(Pop1);
            posidx = j__udySearchRawLeaf2(Pleaf2, Pop1, (uint16_t)Index, start);
#endif  // ! OLDWAY
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

#ifdef  JUDY1
        case cJ1_JPIMMED_3_05: 
        case cJ1_JPIMMED_3_04:
        case cJ1_JPIMMED_3_03:
#endif  // JUDY1

        case cJU_JPIMMED_3_02:
        {
            Pop1 = ju_Type(Pjp) - cJU_JPIMMED_3_02 + 2;
            uint8_t *Pleaf3 = ju_PImmed3(Pjp);     // Get ^ to Keys
#ifdef  JUDYL
            Pjv = P_JV(RawPntr);  // ^ immediate values area
#endif  // JUDYL
            posidx = j__udySearchImmed3(Pleaf3, Pop1, Index, 3 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

#ifdef  JUDYL
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
        case cJ1_JPIMMED_4_02:
        case cJ1_JPIMMED_4_03:
        {
            Pop1 = ju_Type(Pjp) - cJ1_JPIMMED_4_02 + 2;
            uint32_t *Pleaf4 = ju_PImmed4(Pjp);
            posidx = j__udySearchImmed4(Pleaf4, Pop1, Index, 4 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJ1_JPIMMED_5_02:
        case cJ1_JPIMMED_5_03:
        {
            Pop1 = ju_Type(Pjp) - cJ1_JPIMMED_5_02 + 2;
            uint8_t *Pleaf5 = ju_PImmed5(Pjp);  // Get ^ to Keys
            posidx = j__udySearchImmed5(Pleaf5, Pop1, Index, 5 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJ1_JPIMMED_6_02:
        {
            Pop1 = 2;
            uint8_t *Pleaf6 = ju_PImmed6(Pjp);  // Get ^ to Keys
            posidx = j__udySearchImmed6(Pleaf6, Pop1, Index, 6 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJ1_JPIMMED_7_02:
        {
            Pop1 = 2;
            uint8_t *Pleaf7 = ju_PImmed7(Pjp);  // Get ^ to Keys
            posidx = j__udySearchImmed7(Pleaf7, Pop1, Index, 7 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // JUDY1

// ****************************************************************************
// INVALID JP TYPE:

        default:
ReturnCorrupt:                  // return not found -- no error return now
            break;

//            JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
//            JUDY1CODE(return(JERRI );)
//            JUDYLCODE(return(PPJERR);)

        } // switch on JP type

NotFoundExit:

#ifdef TRACEJPG
//    if (startpop && (j__udyPopulation >= startpop))
    {
#ifdef JUDY1
        printf("---Judy1Test   Key = 0x%016lx NOT FOUND\n", Index);
#else /* JUDYL */
        printf("---JudyLGet    Key = 0x%016lx NOT FOUND\n", Index);
#endif /* JUDYL */
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
