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

#ifdef  PREFETCH
#include <immintrin.h>
#endif  // PREFETCH

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

//#ifdef  noDCD
//#undef ju_DcdNotMatchKey
//#define ju_DcdNotMatchKey(INDEX,PJP,POP0BYTES) (0)
//#endif  // DCD

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
                        PJError_t PJError       // optional, for returning error info.
                         )
#else /* JUDYL */
FUNCTION PPvoid_t j__udyLGet (Pcvoid_t PArray,  // from which to retrieve.
                        Word_t Index,           // to retrieve.
                        PJError_t PJError       // optional, for returning error info.
                             )
#endif /* JUDYL */
{
    Pjp_t     Pjp;                      // current JP while walking the tree.
    Pjpm_t    Pjpm;                     // for global accounting.
    Word_t    Pop1 = 0;                 // leaf population (number of indexes).

#ifdef  JUDYL
    Pjv_t     Pjv;
#endif  // JUDYL

    Word_t    RawPntr;
    int       posidx;
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
#else /* JUDYL */
        printf("\n0x%lx j__udyLGet,  Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAF8_POP0(PArray) + 1));
#endif /* JUDYL */
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
// not necessary?            if (ju_DcdNotMatchKey(Index, Pjp, 7)) break;
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
#ifdef  LEAF1_UCOMP
        case cJL_JPLEAF1_UCOMP:             //       Uncompressed Leaf1
        {
//printf("\nj__udyGet:cJL_JPLEAF1_UCOMP UncompressedL1, key = 0x%lx\n", Index);
            Pjll1_t Pjll1 = P_JLL1(RawPntr);

            uint8_t index8 = Index;

            if (Pjll1->jl1_Leaf[index8] == 0)
                break;

            posidx = index8;
            Pjv   = JL_LEAF1VALUEAREA(Pjll1, 256);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // LEAF1_UCOMP
#endif  // JUDYL

        case cJU_JPLEAF1:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll1_t Pjll1 = P_JLL1(RawPntr);
#ifdef  JUDYL
            Pjv  = JL_LEAF1VALUEAREA(Pjll1, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf1(Pjll1, Pop1, Index, 1 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

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
#ifdef  JUDYL
        case cJL_JPLEAF_B1_UCOMP:
        {
            Pjlb_t    Pjlb   = P_JLB1(RawPntr);

            DIRECTHITS;       // not necessary, because always 100%
            int pop1 = ju_LeafPop1(Pjp);
            if (pop1 == 0) pop1 = 256;
            SEARCHPOPULATION(pop1); 

            uint8_t digit  = (uint8_t)Index;
            Word_t  sube   = digit / cJU_BITSPERSUBEXPL;        // 0..3

            BITMAPL_t BitMap = JU_JLB_BITMAP(Pjlb, sube);
            BITMAPL_t BitMsk = JU_BITPOSMASKL(digit);

// No value in sub-expanse for Index => Index not found:

            if (! (BitMap & BitMsk)) 
                break;                          // not found

//          offset to pointer of Value area
            return((PPvoid_t) (JL_JLB_PVALUE(Pjlb) + digit));

        } // case cJU_JPLEAF_B1
#endif // JUDYL

        case cJU_JPLEAF_B1:
        {
            Pjlb_t    Pjlb;
            Word_t    sub4exp;   // in bitmap, 0..7.
            BITMAPL_t BitMap;   // for one sub-expanse.
            BITMAPL_t BitMsk;   // bit in BitMap for Indexs Digit.

            DIRECTHITS;       // not necessary, because always 100%
            SEARCHPOPULATION(ju_LeafPop1(Pjp));

            Pjlb   = P_JLB1(RawPntr);
            Digit  = JU_DIGITATSTATE(Index, 1);
            sub4exp = Digit / cJU_BITSPERSUBEXPL;

            BitMap = JU_JLB_BITMAP(Pjlb, sub4exp);
            BitMsk = JU_BITPOSMASKL(Digit);

// No value in sub-expanse for Index => Index not found:

            if (! (BitMap & BitMsk)) 
                break;
#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
// Count value areas in the sub-expanse below the one for Index:
// JudyL is much more complicated because of Value area subarrays:

//          Get raw pointer to Value area
//            Word_t PjvRaw = Pjlb->jLlb_jLlbs[sub4exp].jLlbs_PV_Raw;
            Pjv_t Pjv = JL_JLB_PVALUE(Pjlb);

            posidx = j__udyCountBitsL(BitMap & (BitMsk - 1));
            return((PPvoid_t) (Pjv + posidx));
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
            DIRECTHITS;               // Too much overhead?
//          This version does not have an conditional branch
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

#else   // ! JUDYGETINLINE








// ****************************************************************************
// J U D Y   1   T E S T
// J U D Y   L   G E T
//
// See the manual entry for details.  Note support for "shortcut" entries to
// trees known to start with a JPM.

// See the manual entry for details.  Note support for "shortcut" entries to
// trees known to start with a JPM.
#ifdef JUDY1
FUNCTION int Judy1Test (Pcvoid_t PArray, // from which to retrieve.
                         Word_t Index,    // to retrieve.
                         PJError_t PJError        // optional, for returning error info.
                       )
#else /* JUDYL */
FUNCTION PPvoid_t JudyLGet (Pcvoid_t PArray,     // from which to retrieve.
                Word_t Index,        // to retrieve.
                PJError_t PJError    // optional, for returning error info.
                           )
#endif /* JUDYL */
{
    Pjp_t     Pjp;                      // current JP while walking the tree.
    Pjpm_t    Pjpm;                     // for global accounting.
    Word_t    Pop1 = 0;                 // leaf population (number of indexes).

#ifdef  JUDYL
    Pjv_t     Pjv;
#endif  // JUDYL

    Word_t    RawPntr;
    int       posidx;
    uint8_t   Digit = 0;                // byte just decoded from Index.

    (void) PJError;                     // no longer used

    if (PArray == (Pcvoid_t)NULL)  // empty array.
        goto NotFoundExit;

#ifdef  TRACEJPG
    if (startpop && (*(Word_t *)PArray >= startpop))
    {
//        printf("\n\n                *PArray = 0x%016lx, startpop = 0x%016lx\n", *(Word_t *)PArray, startpop);
#ifdef JUDY1
        printf("\n0x%lx Judy1Test, Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAF8_POP0(PArray) + 1));
#else /* JUDYL */
        printf("\n0x%lx JudyLGet,  Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAF8_POP0(PArray) + 1));
#endif /* JUDYL */
    }
#endif  // TRACEJPG


// ****************************************************************************
// PROCESS TOP LEVEL BRANCHES AND LEAF:

        if (JU_LEAF8_POP0(PArray) < cJU_LEAF8_MAXPOP1) // must be a LEAF8
        {
            Pjll8_t Pjll8 = P_JLL8(PArray);        // first word of leaf.
            Pop1          = Pjll8->jl8_Population0 + 1;

//printf("\n--JudyLGet-LEAF8,  Key = 0x%016lx, Array Pop1 = %lu\n", Index, Pop1);

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

#ifdef  COMPRESSU
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
#else   // ! COMPRESSU

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
#endif  // ! COMPRESSU

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

#ifdef  JUDYL
//  ***************************************************************
//  This is only for JudyL because Judy1 does not have a Leaf1
//  ***************************************************************

#ifdef  PARALLEL1
#undef cbPK
#undef cKPW
#define cbPK    (8)             // bits per Key
#define cKPW    (cbPW / cbPK)   // Keys per Word

        case cJU_JPLEAF1:
        {
            Pjll1_t Pjll1 = P_JLL1(RawPntr);
            CHECKIT(Index, Pjll1->jl1_LastKey, 1);

            Pop1           = ju_LeafPop1(Pjp);
            uint8_t index8 = (uint8_t)Index;
            posidx         = PSPLIT(Pop1, index8, 1 * 8); 
            int start      = posidx;

// Note: The Key array must be padded with replicas of last Key Ins, Del and Cascade

#ifndef MMX1
//          Replicate the Lsb & Msb in every Key position (at compile time)
            Word_t repLsbKey = REPKEY(cbPK, 1);
            Word_t repMsbKey = repLsbKey << (cbPK - 1);

            Word_t repKey    = REPKEY(cbPK, index8);
#endif  // MMX1

            Word_t  Bucket;
            Word_t  newBucket;


#ifdef  PADCHECK1       // TEMP
if ((Pop1 & 0x7))
{
    int roundupnextword = ((Pop1 + 7) / 8) * 8;
    for (int ii = Pop1; ii < roundupnextword; ii++)
    {
        if (Pjll1->jl1_Leaf[Pop1 - 1] != Pjll1->jl1_Leaf[ii])
        {
printf("\n---Oops-----------Pop1 = %ld, Key = 0x%2lx, posidx = %d\n", Pop1, index8, posidx);
    for (int ii = 0; ii < ((Pop1 + 7) & -cKPW); ii++)
        printf("%d=0x%02x ", ii, Pjll1->jl1_Leaf[ii]);
    printf("posidx = %d\n", posidx);
    break;
        }
    }
}
#endif  // PADCHECK1 end TEMP


//          make a zero the searched for Key in new Bucket
//          Magic, the Msb=1 is located in the matching Key position
            
#ifdef  JUDYL
            Pjv  = JL_LEAF1VALUEAREA(Pjll1, Pop1);
#endif  // JUDYL
//          It should be 16 byte aligned from malloc()
            PWord_t PWord = (PWord_t)Pjll1->jl1_Leaf;

#ifdef  MMX1
            PWord_t Bptr = PWord + (posidx/cKPW);
            newBucket = Hk64c(Bptr, index8);
#else   // ! MMX1
            Bucket = PWord[posidx / cKPW];     // KeysPerWord = 8
            newBucket = Bucket ^ repKey;
            newBucket = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
#endif  // ! MMX1
            if (newBucket)         // Key found in Bucket?
            {
                SEARCHPOPULATION(Pop1);
                DIRECTHITS;
                goto FoundL1Exit;
            }
//          Determine direction of search after miss
            if (index8 > Pjll1->jl1_Leaf[start])
            {
                for(;;)             // forward
                {
                    posidx += cKPW;
//                  Check if passed the last Bucket that can have Key
                    if ((posidx / cKPW) >= ((Pop1 + cKPW - 1) / cKPW)) 
                        goto NotFoundExit;
#ifdef  MMX1
                    PWord_t Bptr = PWord + (posidx / cKPW);
                    newBucket = Hk64c(Bptr, index8);
#else   // ! MMX1
                    Bucket = PWord[posidx / cKPW]; // KeysPerWord = 8
                    newBucket = Bucket ^ repKey;
                    newBucket = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
#endif  // ! MMX1
                    if (newBucket)         // Key found in Bucket?
                    {
                        SEARCHPOPULATION(Pop1);
                        MISCOMPARESP((posidx - start) / cKPW);
                        goto FoundL1Exit;
                    }
//                    if (Pjll1->jl1_Leaf[posidx] > index8) 
//                        goto NotFoundExit;
                }
            }
            else
            {
                for(;;)             // reverse
                {
                    posidx -= cKPW;
                    if (posidx < 0) 
                        goto NotFoundExit;
#ifdef  MMX1
                    PWord_t Bptr = PWord + (posidx / cKPW);
                    newBucket = Hk64c(Bptr, index8);
#else   // ! MMX1
                    Bucket = PWord[posidx / cKPW];      // KeysPerWord = 8
                    newBucket = Bucket ^ repKey;
                    newBucket = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
#endif  // ! MMX1
                    if (newBucket)         // Key found in Bucket?
                    {
                        SEARCHPOPULATION(Pop1);
                        MISCOMPARESM((start - posidx) / cKPW);
                        goto FoundL1Exit;
                    }
//                    if (Pjll1->jl1_Leaf[posidx] < index8) 
//                        goto NotFoundExit;
                }
            }
FoundL1Exit:

#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
            posidx &= -cKPW;
#ifdef  noCTZL
//          Lowest non-zero Key in Bucket is the posidx offset
            for (; (uint8_t)newBucket == 0; posidx++)
                newBucket >>= cbPK;
#else   // CTZL
            posidx += __builtin_ctzl(newBucket) / cbPK;
#endif  // CTZL
            return((PPvoid_t) (Pjv + posidx));
#endif  // JUDYL
        }

#else   // ! PARALLEL1

#undef cbPK
#define cbPK    (8)            // bits per Key
//        case cJU_JPLEAF1_SUB8:
        case cJU_JPLEAF1:
        {
//          Pointer to Leaf1
            Pjll1_t Pjll1       = P_JLL1(RawPntr);
            CHECKIT(Index, Pjll1->jl1_LastKey, 1);

            uint8_t index8      = Index;        // trim off decoded bits
            int     subexp      = (index8 / 32) * 8;    // sub-expanse (0..7) to look for Key
            int     subkey      =  index8 % 32;         // 0..31 Least 5 bits of Key
            Word_t  subLeafPops = Pjp->jp_subLeafPops;  // in-cache sub-expanse pop1

            Pop1 = ju_LeafPop1(Pjp);                // leaf pop1 from jp_t
//            if (Pop1 != 256) assert(Pop1 == j__udySubLeaf8Pops(subLeafPops));   // FAILS at 256
            if (Pop1 == 0)
            {
 printf("\nChange 0 to 256\n");
                Pop1 = 256;
            }

            SEARCHPOPULATION(Pop1);     // enabled -DSEARCHMETRICS at compile time

//          Sum populations of previous sub-expanses to Key - mask is from the -1
//            int  leafOff  = j__udySubLeaf8Pops(subLeafPops & (ADDTOSUBLEAF8(subexp) - 1));
           // Word_t mask = ((Word_t)1 << ((index8 / 32) * 8)) - 1;
           //  Word_t mask = ((Word_t)1 << subexp) - 1;
           
//          get offset to current sub expanse
            int  leafOff  = j__udySubLeaf8Pops(subLeafPops & ((Word_t)1 << subexp) - 1);
            uint8_t *subLeaf8  = Pjll1->jl1_Leaf + leafOff;     // ^ to the subLeaf
            Pjv = JL_LEAF1VALUEAREA(Pjll1, Pop1) + leafOff;     // Sub-Value area ^ 

//          Get the population of the sub-expanse containing the subkey
            uint8_t subpop8       = (subLeafPops >> subexp) /* & 0xFF */; // 0..32 - (mask 0..63)

//          Note: this is slightly faster when not found, but slightly slower when found
//            if (subpop8 == 0)            // if subexp empty, then not found
//                break;
            
//          do a linear approximation of the location (0..31) of key in the sub-expanse
            int start         = (subpop8 * subkey) / 32;

#ifdef  SUBPARALLEL1
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
                DIRECTHITS;
                posidx = RawBucketPtr - (Word_t)subLeaf8;
                posidx += __builtin_ctzl(newBucket) / cbPK;
                return((PPvoid_t) (Pjv + posidx));
            }
#endif  //  SUBPARALLEL1

//          search an expanse of maximum of 32 keys -- 4 Words max
            posidx = j__udySearchRawLeaf1(subLeaf8, subpop8, index8, start);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // ! PARALLEL1



#ifdef  later
#define cSubExp8  (256 / 16)              // 16 keys per sub-expanse
        case cJU_JPLEAF1_SUB16:
        {
//          Pointer to Leaf1
            Pjll1_t Pjll1     = P_JLL1(RawPntr);
            uint8_t index8    = Index;                 // trim off already decoded bits

             Word_t sub16msk = (Word_t)0xF << ((index8 >> 4) * 4);
            if ((Pjll1->jl1_subLeafPops & sub16msk) == sub16msk)    // sub pop1 = 15

//          Get the offset to start of 0..7 sub-expanses search
            int     subnumbit = (index8 / cSubExp16) * 4; // (0..15) * 4 of Key

//          Get the only sub-expanse (0..15) that could have Key
            Word_t  subLeafPops  = Pjp->jp_subLeafPops;        // in-cache sub-expanse pop1

//          The key only has 4 signifacant bits un-decoded
            int     subkey    = index8 % cSubExp16;      // 0..15

//          Get leaf population from jp_t
            Pop1              = ju_LeafPop1(Pjp);

//          SEARCHMETRICS conditional on -DSEARCHMETRICS at compile time
            SEARCHPOPULATION(Pop1);

//          Get start of Value area ^ from a table
            Pjv               = JL_LEAF1VALUEAREA(Pjll1, Pop1);

//          Make a mask of all sub-expanse populations preceeding the Keys population
            Word_t subkeymsk16  = ((Word_t)1 << subnumbit) - 1;    // upto 15 nibbles = 0xf

//          Sum previous sub-expanse populations of Leaf (use mpy trick to sum)
//////            int leafOff       = ((subLeafPops & subkeymsk16) * cMagic8) >> 56;      // 0..56

            Pjv += leafOff;             // update ^ to Value area of sub-expanse

//          Get the population of the sub-expanse containing the subkey
//////            int subpop16       = (subLeafPops >> subnumbit) & 0x3F; // 0..15 

//          Note: This is slightly faster when Key not found, but slightly slower if found
//            if (subpop16 == 0)            // if subexp empty, then not found
//                break;
            
//          Sum individual sub-expanse pops and check with jp_t
            assert(Pop1 == (subLeafPops * cMagic8) >> 56);     // verify with Pop0 in jp_t

//          Get ^ to the only sub-expanse that could have key
            uint8_t *subLeaf16 = Pjll1->jl1_Leaf + leafOff;

//          do a linear approximation of the location (0..31) of key in the sub-expanse
            int start         = (subpop16 * subkey) / cSubExp16;

//          search an expanse of maximum of 32 keys
            posidx = j__udySearchRawLeaf1(subLeaf16, subpop16, index8, start);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // later



//      Uncompressed, both Key and Value are offset by the LSByte bits of Key
//      And VERY fast
#ifdef  LEAF1_UCOMP
        case cJL_JPLEAF1_UCOMP:
        {
            uint8_t index8 = Index;     // trim off already decoded bits
            Pjll1_t Pjll1 = P_JLL1(RawPntr);
            Pjv   = JL_LEAF1VALUEAREA(Pjll1, 256);      // uncompressed

            Pop1 = ju_LeafPop1(Pjp);
            if (Pop1 == 0)
            {
// printf("\nChange 0 to 256 in UCOMP\n");
                Pop1 = 256;
            }
            SEARCHPOPULATION(Pop1);
            DIRECTHITS;

//          Note: Should really be a bitmap instead of a bytemap?
            if (Pjll1->jl1_Leaf[index8])     
                return((PPvoid_t) (Pjv + index8));
            break;
//            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // LEAF1_UCOMP
#endif  // JUDYL -- Judy1 does not have a Leaf1


#ifndef  noPARALLEL2

// Do a parallel (4) Key Leaf2 search
#undef cbPK
#undef cKPW
#define cbPK    (16)            // bits per Key
#define cKPW    (cbPW / cbPK)   // Keys per Word

        case cJU_JPLEAF2:
        {
            Pop1           = ju_LeafPop1(Pjp);
            Pjll2_t Pjll2  = P_JLL2(RawPntr);
            CHECKIT(Index, Pjll2->jl2_LastKey, 2);
#ifdef  JUDYL
            Pjv   = JL_LEAF2VALUEAREA(Pjll2, Pop1);
#endif  // JUDYL
            Word_t index16 = (uint16_t)Index;
            posidx         = PSPLIT(Pop1, index16, 2 * 8); 
            int start      = posidx;

// Note: The Key array must be padded with replicas of last Key Ins, Del and Cascade

//          replicate the Lsb in every Key position (at compile time)
            Word_t repLsbKey = REPKEY(cbPK, 1);

//          replicate the Msb in every Key position (at compile time)
            Word_t repMsbKey = repLsbKey << (cbPK - 1);

            Word_t  Bucket;
            Word_t  newBucket;


#ifdef  PADCHECK2 // TEMP
if ((Pop1 & 0x3))
{
    int roundupnextword = ((Pop1 + 3) / 4) * 4;
    for (int ii = Pop1; ii < roundupnextword; ii++)
    {
        if (Pjll2->jl2_Leaf[Pop1 -1] != Pjll2->jl2_Leaf[ii])
        {
printf("\n---Oops-----------Pop1 = %ld, Key = 0x%2lx, posidx = %d\n", Pop1, index16, posidx);
    for (int ii = 0; ii < ((Pop1 + 7) & -cKPW); ii++)
        printf("%d=0x%02x ", ii, Pjll2->jl2_Leaf[ii]);
    printf("posidx = %d\n", posidx);
    break;
        }
    }
}
#endif  // PADCHECK2 end TEMP




//          make a zero the searched for Key in new Bucket
//          Magic, the Msb=1 is located in the matching Key position

            PWord_t PWord = (PWord_t)Pjll2->jl2_Leaf;
            Bucket = PWord[posidx / cKPW];              // KeysPerWord = 4
            newBucket = Bucket ^ REPKEY(cbPK, index16);
            newBucket = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
//
            if (newBucket)         // Key found in Bucket
            {
                SEARCHPOPULATION(Pop1);
                DIRECTHITS;
                goto FoundL2Exit;
            }
            if (index16 > Pjll2->jl2_Leaf[start])
            {
                for(;;)  // search forward
                {
                    posidx += cKPW;
                    if ((posidx / cKPW) >= ((Pop1 + cKPW - 1) / cKPW)) 
                        goto NotFoundExit;              // past end

                    Bucket = PWord[posidx / cKPW];      // KeysPerWord = 4
                    newBucket = Bucket ^ REPKEY(cbPK, index16);
                    newBucket = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
                    if (newBucket)                      // Key found in Bucket
                    {
                        SEARCHPOPULATION(Pop1);
                        MISCOMPARESP((posidx - start) / cKPW); // divided by 4
                        goto FoundL2Exit;
                    }
                    if (Pjll2->jl2_Leaf[posidx] > index16) 
                        goto NotFoundExit;
                }
            }
            else
            {
                for(;;)    // search backward
                {
                    posidx -= cKPW;
                    if (posidx < 0) 
                        goto NotFoundExit;
                    Bucket = PWord[posidx / cKPW];      // KeysPerWord = 4
                    newBucket = Bucket ^ REPKEY(cbPK, index16);
                    newBucket = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
                    if (newBucket)                      // Key found in Bucket
                    {
                        SEARCHPOPULATION(Pop1);
                        MISCOMPARESM((start - posidx) / cKPW);  // divided by 4
                        goto FoundL2Exit;
                    }
                    if (Pjll2->jl2_Leaf[posidx] < index16) 
                        goto NotFoundExit;              // index16 too small
                }
            }

FoundL2Exit:    // found
#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
            posidx &= -cKPW;
#ifdef  noCTZL
//          Lowest non-zero Key in Bucket is the posidx offset
            for (; (uint16_t)newBucket == 0; posidx++)
                newBucket >>= cbPK;
#else   // CTZL
            posidx += __builtin_ctzl(newBucket) / cbPK;
#endif  // CTZL
            return((PPvoid_t) (Pjv + posidx));
#endif  // JUDYL
        }

#else   // ! noPARALLEL2

// Do NOT do a parallel (4) Key Leaf2 search
        case cJU_JPLEAF2:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll2_t Pjll2 = P_JLL2(RawPntr);
            CHECKIT(Index, Pjll2->jl2_LastKey, 2);
#ifdef  JUDYL
            Pjv = JL_LEAF2VALUEAREA(Pjll2, Pop1);
#endif  // JUDYL
//          entry Pjll = Leaf2, Pjv = Value, Pop1 = population
            posidx = j__udySearchLeaf2(Pjll2, Pop1, Index, 2 * 8);
            goto CommonLeafExit;         // posidx & (only JudyL) Pjv
        }
#endif  // ! noPARALLEL2

        case cJU_JPLEAF3:
        {
            Pop1          = ju_LeafPop1(Pjp);
            Pjll3_t Pjll3 = P_JLL3(RawPntr);
            CHECKIT(Index, Pjll3->jl3_LastKey, 3);
#ifdef  JUDYL
            Pjv = JL_LEAF3VALUEAREA(Pjll3, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf3(Pjll3, Pop1, Index, 3 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv

CommonLeafExit:         // posidx & (only JudyL) Pjv
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
            Pjlb_t    Pjlb   = P_JLB1(RawPntr);
            CHECKIT(Index, Pjlb->jlb_LastKey, 1);

            DIRECTHITS;       // not necessary, because always 100%
            SEARCHPOPULATION(ju_LeafPop1(Pjp)); // cannot be 256!!!!

            uint8_t digit  = JU_DIGITATSTATE(Index, 1);
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
//          offset of preceding subexpanses
            posidx = j__udySubLeaf8Pops(Pjp->jp_subLeafPops & ((Word_t)1 << (sube * 8)) - 1);

//          offset of sub-expanse that matches digit
            posidx +=  j__udyCountBitsL(BitMap & (BitMsk - 1)); 

//          offset to pointer of Value area
            return((PPvoid_t) (JL_JLB_PVALUE(Pjlb) + posidx));
#endif // JUDYL

        } // case cJU_JPLEAF_B1

#ifdef  JUDYL
        case cJL_JPLEAF_B1_UCOMP:
        {
            Pjlb_t    Pjlb   = P_JLB1(RawPntr);
            CHECKIT(Index, Pjlb->jlb_LastKey, 1);

            DIRECTHITS;       // not necessary, because always 100%
            int pop1 = ju_LeafPop1(Pjp);
            if (pop1 == 0) pop1 = 256;
            SEARCHPOPULATION(pop1); 

            uint8_t digit  = (uint8_t)Index;
            Word_t  sube   = digit / cJU_BITSPERSUBEXPL;        // 0..3

            BITMAPL_t BitMap = JU_JLB_BITMAP(Pjlb, sube);
            BITMAPL_t BitMsk = JU_BITPOSMASKL(digit);

// No value in sub-expanse for Index => Index not found:

            if (! (BitMap & BitMsk)) 
                break;                          // not found

//          offset to pointer of Value area
            return((PPvoid_t) (JL_JLB_PVALUE(Pjlb) + digit));

        } // case cJU_JPLEAF_B1
#endif // JUDYL

#ifdef JUDY1
// ****************************************************************************
// JPFULLPOPU1:
//
// If the Index is in the expanse, it is necessarily valid (found).

        case cJ1_JPFULLPOPU1:
        {
//            CHECKIT(Index, Pjll1->jl1_LastKey, 1); not nec, always in a Branch
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
            SEARCHPOPULATION(1);      // Too much overhead
            DIRECTHITS;               // Too much overhead

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
            Pop1 = ju_Type(Pjp) - cJU_JPIMMED_1_02 + 2;
            uint8_t *Pleaf1 = ju_PImmed1(Pjp);  // Get ^ to Keys
            posidx = j__udySearchImmed1(Pleaf1, Pop1, Index, 1 * 8);
#ifdef  JUDYL
            Pjv = P_JV(RawPntr);                // Get ^ to Values
#endif  // JUDYL
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
            uint16_t *Pleaf2 = ju_PImmed2(Pjp);     // Get ^ to Keys
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
