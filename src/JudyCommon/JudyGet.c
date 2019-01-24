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
    Word_t    Pop1;                     // leaf population (number of indexes).
    Pjll_t    Pjll;                     // pointer to LeafL.

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
//    if (startpop && (j__udyPopulation >= startpop))
    {
#ifdef JUDY1
        printf("\n0x%lx j__udy1Test, Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAFW_POP0(PArray) + 1));
#else /* JUDYL */
        printf("\n0x%lx j__udyLGet,  Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAFW_POP0(PArray) + 1));
#endif /* JUDYL */
    }
#endif  // TRACEJPG

//        Index = Index << 2;

// ****************************************************************************
// PROCESS TOP LEVEL BRANCHES AND LEAF:

        if (JU_LEAFW_POP0(PArray) < cJU_LEAFW_MAXPOP1) // must be a LEAFW
        {
            Pjllw_t Pjllw = P_JLLW(PArray);        // first word of leaf.
            Pop1   = Pjllw->jlw_Population0 + 1;

//printf("\n--j__judy_Get-LEAFW,  Key = 0x%016lx, Array Pop1 = %lu\n", Index, Pop1);

            if ((posidx = j__udySearchLeafW(Pjllw->jlw_Leaf, Pop1, Index)) < 0)
            {
                goto NotFoundExit;              // no jump to middle of switch
            }
#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
            return((PPvoid_t) (JL_LEAFWVALUEAREA(Pjllw, Pop1) + posidx));
#endif  // JUDYL
        }
//      else must be the tree under a jpm_t
        Pjpm = P_JPM(PArray);
        Pjp = Pjpm->jpm_JP + 0;  // top branch is below JPM.

#ifdef  TRACEJPG        // TBD: joke?
        j__udyIndex = Index;
        j__udyPopulation = Pjpm->jpm_Pop0;
#endif  // TRACEJPG

// ****************************************************************************
// WALK THE JUDY TREE USING A STATE MACHINE:

ContinueWalk:           // for going down one level in tree; come here with Pjp set.

#ifdef TRACEJPG
        JudyPrintJP(Pjp, "G", __LINE__);
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
// Note:  These are legitimate in a BranchU (only) and do not constitute a
// fault.

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
        case cJU_JPBRANCH_L:
        {
            Pjbl_t Pjbl;

            Digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);

// Common code for all BranchLs; come here with Digit set:

JudyBranchL:
//            Pjbl = P_JBL(Pjp->jp_Addr0);
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

        case cJU_JPBRANCH_B:
        {
            Pjbb_t    Pjbb;
            Word_t    subexp;   // in bitmap, 0..7[3]
            BITMAPB_t BitMap;   // for one subexpanse.
            BITMAPB_t BitMsk;   // bit in BitMap for Indexs Digit.

            Digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);

// Common code for all BranchBs; come here with Digit set:

JudyBranchB:
            Pjbb   = P_JBB(RawPntr);
            subexp = Digit / cJU_BITSPERSUBEXPB;

            BitMap = JU_JBB_BITMAP(Pjbb, subexp);
            BitMsk = JU_BITPOSMASKB(Digit);

//          No JP in subexpanse for Index, implies not found:
            if ((BitMap & BitMsk) == 0) 
                break;

//          start address of Pjp_t array
            Pjp    = P_JP(JU_JBB_PJP(Pjbb, subexp));

            Pjp += j__udyCountBitsB(BitMap & (BitMsk - 1));
            goto ContinueWalk;

        } // case cJU_JPBRANCH_B*
#endif  // noB
// ****************************************************************************
// JPBRANCH_U*:

        case cJU_JPBRANCH_U:
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
// ****************************************************************************
// JPLEAF*:
//
// Note:  Here the calls of ju_DcdNotMatchKey() are necessary and check
// whether Index is out of the expanse of a narrow pointer.

        case cJU_JPLEAF1:
        {
//            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
/////////            if (ju_DcdNotMatchKey(Index, Pjp, 1)) break;
            Pjll1_t Pjll1 = P_JLL1(RawPntr);
            if (ju_DcdNotMatchKeyLeaf(Index, Pjll1->jl1_DcdPop0, 1)) break;

            Pop1          = ju_LeafPop0(Pjp) + 1;
#ifdef  JUDYL
            Pjv  = JL_LEAF1VALUEAREA(Pjll1, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf1(Pjll1, Pop1, Index, 1 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF2:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 2)) break;
            Pop1        = ju_LeafPop0(Pjp) + 1;
            Pjll        = P_JLL(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF2VALUEAREA(Pjll, Pop1);
#endif  // JUDYL
            goto Leaf2Exit;

//          entry Pjll = Leaf1, Pjv = PValue, Pop1 = population, Key = Index
Leaf2Exit:
            posidx = j__udySearchLeaf2(Pjll, Pop1, Index, 2 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF3:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 3)) break;

            Pop1        = ju_LeafPop0(Pjp) + 1;
            Pjll        = P_JLL(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF3VALUEAREA(Pjll, Pop1);
#endif  // JUDYL
            goto Leaf3Exit;

Leaf3Exit: // entry Pjll = Leaf3, Pjv = Value, Pop1 = population
            posidx = j__udySearchLeaf3(Pjll, Pop1, Index, 3 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF4:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 4)) break;

            Pop1        = ju_LeafPop0(Pjp) + 1;
            Pjll        = P_JLL(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF4VALUEAREA(Pjll, Pop1);
#endif  // JUDYL
            goto Leaf4Exit;
            
Leaf4Exit: // entry Pjll = Leaf4, Pjv = Value, Pop1 = population
            posidx = j__udySearchLeaf4(Pjll, Pop1, Index, 4 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF5:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 5)) break;

            Pop1        = ju_LeafPop0(Pjp) + 1;
            Pjll        = P_JLL(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF5VALUEAREA(Pjll, Pop1);
#endif  // JUDYL
            goto Leaf5Exit;
            
Leaf5Exit: // entry Pjll = Leaf3, Pjv = Value, Pop1 = population
            posidx = j__udySearchLeaf5(Pjll, Pop1, Index, 5 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF6:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 6)) break;

            Pop1        = ju_LeafPop0(Pjp) + 1;
            Pjll        = P_JLL(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF6VALUEAREA(Pjll, Pop1);
#endif  // JUDYL
            goto Leaf6Exit;
            
Leaf6Exit: // entry Pjll = Leaf3, Pjv = Value, Pop1 = population
            posidx = j__udySearchLeaf6(Pjll, Pop1, Index, 6 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF7:
        {
//printf("_Get-F7: ju_LeafPop0(Pjp) = 0x%lx, ju_DcdPop0(Pjp) = 0x%016lx, Key = 0x%016lx\n", ju_LeafPop0(Pjp), ju_DcdPop0(Pjp), Index);
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 7)) break;

            Pop1        = ju_LeafPop0(Pjp) + 1;
            Pjll        = P_JLL(RawPntr);

#ifdef diag
printf("\n------j__Get---------LEAF7\n");
for(int ii = 0; ii < Pop1; ii++)
{
    Word_t key;
    JU_COPY7_PINDEX_TO_LONG(key, (uint8_t *)Pjll + (7 * ii));
    printf("%2d = 0x%016lx\n", ii, key);
}
#endif  // diag

#ifdef  JUDYL
            Pjv = JL_LEAF7VALUEAREA(Pjll, Pop1);
#endif  // JUDYL

            goto Leaf7Exit;
            
Leaf7Exit: // entry Pjll = Leaf3, Pjv = Value, Pop1 = population
            posidx = j__udySearchLeaf7(Pjll, Pop1, Index, 7 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

// ****************************************************************************
// JPLEAF_B1:

        case cJU_JPLEAF_B1:
        {
            Pjlb_t    Pjlb;
            Word_t    subexp;   // in bitmap, 0..7.
            BITMAPL_t BitMap;   // for one subexpanse.
            BITMAPL_t BitMsk;   // bit in BitMap for Indexs Digit.

            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 1)) break;

            DIRECTHITS;       // not necessary, because always 100%
            SEARCHPOPULATION(ju_LeafPop0(Pjp) + 1);

            Pjlb   = P_JLB(RawPntr);
            Digit  = JU_DIGITATSTATE(Index, 1);
            subexp = Digit / cJU_BITSPERSUBEXPL;

#ifdef EXP1BIT
            if (subexp == 0) return(1); // 7% faster
#endif  // EXP1BIT

            BitMap = JU_JLB_BITMAP(Pjlb, subexp);
            BitMsk = JU_BITPOSMASKL(Digit);

// No value in subexpanse for Index => Index not found:

            if (! (BitMap & BitMsk)) 
                break;
#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
// Count value areas in the subexpanse below the one for Index:
// JudyL is much more complicated because of Value area subarrays:

#ifdef BMVALUE
//          if population of subexpanse == 1, return ^ to Value^
            if (BitMap == BitMsk)
            {
//////            if (ju_DcdNotMatchKey(Index, Pjp, 1)) break;
                return((PPvoid_t)(&Pjlb->jLlb_jLlbs[subexp].jLlbs_PV_Raw));
            }
#endif /* BMVALUE */

//          Get raw pointer to Value area
//            Word_t PjvRaw = Pjlb->jLlb_jLlbs[subexp].jLlbs_PV_Raw;
            Word_t PjvRaw = JL_JLB_PVALUE(Pjlb, subexp);

#ifdef  UNCOMPVALUE
            if (PjvRaw & 1)     // check if uncompressed Value area
            {
                posidx = Digit % cJU_BITSPERSUBEXPL; // offset from Index
// done by P_JV()  PjvRaw -= 1;       // mask off least bit
            }
            else                                    // Compressed, calc offset
#endif  // UNCOMPVALUE
            {
                posidx = j__udyCountBitsL(BitMap & (BitMsk - 1));
            }
            return((PPvoid_t) (P_JV(PjvRaw) + posidx));
#endif // JUDYL

        } // case cJU_JPLEAF_B1

#ifdef JUDY1
// ****************************************************************************
// JPFULLPOPU1:
//
// If the Index is in the expanse, it is necessarily valid (found).

        case cJ1_JPFULLPOPU1:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 1)) break;
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

//            if (JU_JPDCDPOP0(Pjp) != JU_TrimToIMM01(Index)) 
//            if (JU_TrimToIMM01(Index ^ ju_DcdPop0(Pjp)) != 0) 
//            if (ju_DcdPop0(Pjp) != JU_TrimToIMM01(Index)) 

//            SEARCHPOPULATION(1);      // Too much overhead
//            DIRECTHITS;               // Too much overhead
//          This version does not have an conditional branch
            if ((ju_IMM01Key(Pjp) ^ Index) << 8) // mask off high byte
                break;
#ifdef  JUDY1
        return(1);
#endif  // JUDY1

#ifdef  JUDYL
//        return((PPvoid_t) &(Pjp->jp_ValueI));  // ^ immediate value
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
            Pjll = (Pjll_t)ju_PImmed1(Pjp);     // Get ^ to Keys
            posidx = j__udySearchImmed1(Pjll, Pop1, Index, 1 * 8);
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
            Pjll = (Pjll_t)ju_PImmed2(Pjp);
            goto Leaf2Exit;
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
            Pjll = (Pjll_t)ju_PImmed1(Pjp);
            goto Leaf3Exit;
        }

#ifdef  JUDYL
        case cJL_JPIMMED_4_02:
        {
            Pop1 = 2;
            Pjv = P_JV(RawPntr);  // ^ immediate values area
            Pjll = (Pjll_t)ju_PImmed4(Pjp);
            goto Leaf4Exit;
        }
#endif  // JUDYL

#ifdef  JUDY1
        case cJ1_JPIMMED_4_02:
        case cJ1_JPIMMED_4_03:
        {
            Pop1 = ju_Type(Pjp) - cJ1_JPIMMED_4_02 + 2;
            Pjll = (Pjll_t)ju_PImmed4(Pjp);
            goto Leaf4Exit;
        }

        case cJ1_JPIMMED_5_02:
        case cJ1_JPIMMED_5_03:
        {
            Pop1 = ju_Type(Pjp) - cJ1_JPIMMED_5_02 + 2;
            Pjll = (Pjll_t)ju_PImmed1(Pjp);
            goto Leaf5Exit;
        }

        case cJ1_JPIMMED_6_02:
        {
            Pop1 = 2;
            Pjll = (Pjll_t)ju_PImmed1(Pjp);
            goto Leaf6Exit;
        }

        case cJ1_JPIMMED_7_02:
        {
            Pop1 = 2;
            Pjll = (Pjll_t)ju_PImmed1(Pjp);
            goto Leaf7Exit;
        }
#endif  // JUDY1

// ****************************************************************************
// INVALID JP TYPE:

        default:
ReturnCorrupt:
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
    Word_t    Pop1;                     // leaf population (number of indexes).
    Pjll_t    Pjll;                     // pointer to LeafL.

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
//    if (startpop && (j__udyPopulation >= startpop))
    {
#ifdef JUDY1
        printf("\n0x%lx Judy1Test, Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAFW_POP0(PArray) + 1));
#else /* JUDYL */
        printf("\n0x%lx JudyLGet,  Key = 0x%016lx, Array Pop1 = %lu\n", 
            (Word_t)PArray, (Word_t)Index, (Word_t)(JU_LEAFW_POP0(PArray) + 1));
#endif /* JUDYL */
    }
#endif  // TRACEJPG


// ****************************************************************************
// PROCESS TOP LEVEL BRANCHES AND LEAF:

        if (JU_LEAFW_POP0(PArray) < cJU_LEAFW_MAXPOP1) // must be a LEAFW
        {
            Pjllw_t Pjllw = P_JLLW(PArray);        // first word of leaf.
            Pop1   = Pjllw->jlw_Population0 + 1;

//printf("\n--JudyLGet-LEAFW,  Key = 0x%016lx, Array Pop1 = %lu\n", Index, Pop1);

            if ((posidx = j__udySearchLeafW(Pjllw->jlw_Leaf, Pop1, Index)) < 0)
            {
                goto NotFoundExit;              // no jump to middle of switch
            }
#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
            return((PPvoid_t) (JL_LEAFWVALUEAREA(Pjllw, Pop1) + posidx));
#endif  // JUDYL
        }
//      else must be the tree under a jpm_t
        Pjpm = P_JPM(PArray);
        Pjp = Pjpm->jpm_JP + 0;  // top branch is below JPM.

#ifdef  TRACEJPG        // TBD: joke?
        j__udyIndex = Index;
        j__udyPopulation = Pjpm->jpm_Pop0;
#endif  // TRACEJPG

// ****************************************************************************
// WALK THE JUDY TREE USING A STATE MACHINE:

ContinueWalk:           // for going down one level in tree; come here with Pjp set.

#ifdef TRACEJPG
        JudyPrintJP(Pjp, "g", __LINE__);
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
// Note:  These are legitimate in a BranchU (only) and do not constitute a
// fault.

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

        case cJU_JPBRANCH_L:
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

        case cJU_JPBRANCH_B:
        {
            Pjbb_t    Pjbb;
            Word_t    subexp;   // in bitmap, 0..7[3]
            BITMAPB_t BitMap;   // for one subexpanse.
            BITMAPB_t BitMsk;   // bit in BitMap for Indexs Digit.

            Digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);

// Common code for all BranchBs; come here with Digit set:

JudyBranchB:
            Pjbb   = P_JBB(RawPntr);
            subexp = Digit / cJU_BITSPERSUBEXPB;

            BitMap = JU_JBB_BITMAP(Pjbb, subexp);
            BitMsk = JU_BITPOSMASKB(Digit);

//          No JP in subexpanse for Index, implies not found:
            if ((BitMap & BitMsk) == 0) 
                break;

//          start address of Pjp_t array
            Pjp    = P_JP(JU_JBB_PJP(Pjbb, subexp));

            Pjp += j__udyCountBitsB(BitMap & (BitMsk - 1));
            goto ContinueWalk;

        } // case cJU_JPBRANCH_B*

#endif  // noB
// ****************************************************************************
// JPBRANCH_U*:

        case cJU_JPBRANCH_U:
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
// ****************************************************************************
// JPLEAF*:
//
// Note:  Here the calls of ju_DcdNotMatchKey() are necessary and check
// whether Index is out of the expanse of a narrow pointer.

#ifdef  JUDY1
#ifndef  noPARALLEL1

//  This needs fixing
#undef cbPK
#undef cKPW
#define cbPK    (8)            // bits per Key
#define cKPW    (cbPW / cbPK)   // Keys per Word

        case cJU_JPLEAF1:
        {
//            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
//////////////            if (ju_DcdNotMatchKey(Index, Pjp, 1)) break;
            Pjll1_t Pjll1 = P_JLL1(RawPntr);
            if (ju_DcdNotMatchKeyLeaf(Index, Pjll1->jl1_DcdPop0, 1)) break;

//            Pop1          = ju_LeafPop0(Pjp) + 1;
            Index     = Index & 0xFF;
            posidx    = PSPLIT(Pop1, Index, 1 * 8); 
            int start = posidx;

// Note: The Key array must be padded with replicas of last Key Ins, Del and Cascade

#ifndef MMX1
//          replicate the Lsb in every Key position (at compile time)
            Word_t repLsbKey = REPKEY(cbPK, 1);

//          replicate the Msb in every Key position (at compile time)
            Word_t repMsbKey = repLsbKey << (cbPK - 1);
            Word_t repKey    = REPKEY(cbPK, Index);
#endif  // MMX1

            Word_t  Bucket;
            Word_t  newBucket;

#ifdef  PADCHECK1
if ((Pop1 & 0x7))
{
    int roundupnextword = ((Pop1 + 7) / 8) * 8;
    for (int ii = Pop1; ii < roundupnextword; ii++)
    {
        if (Pjll1->jl1_Leaf[Pop1 -1] != Pjll1->jl1_Leaf[ii])
        {
printf("\n---Oops-----------Pop1 = %ld, Key = 0x%2lx, posidx = %d\n", Pop1, Index, posidx);
    for (int ii = 0; ii < ((Pop1 + 7) & -cKPW); ii++)
        printf("%d=0x%02x ", ii, Pjll1->jl1_Leaf[ii]);
    printf("posidx = %d\n", posidx);
    break;
        }
    }
}
#endif  // PADCHECK1

//---Dir-----------Pop1 = 16, Key = 0x8d, NewBucket = 0x               0, posidx = 8
//0=0x0a 1=0x1f 2=0x24 3=0x31 4=0x39 5=0x47 6=0x5f 7=0x6e 8=0x71 9=0x92 10=0x99 11=0xc3 12=0xc8 13=0xde 14=0xf0 15=0xf4 posidx = 8

//-----Plus--------Pop1 = 16, Key = 0x8d, NewBucket = 0x               0, posidx = 8
//0=0x0a 1=0x1f 2=0x24 3=0x31 4=0x39 5=0x47 6=0x5f 7=0x6e 8=0x71 9=0x92 10=0x99 11=0xc3 12=0xc8 13=0xde 14=0xf0 15=0xf4 posidx = 8


//              make zero the searched for Key in new Bucket
//              Magic, the Msb=1 is located in the matching Key position
            {
#ifdef  MMX1
                PWord_t Bptr = ((PWord_t)Pjll1->jl1_Leaf) + (posidx/cKPW);
                newBucket = Hk64c(Bptr, Index);
#else   // ! MMX1
                Bucket = ((PWord_t)Pjll1->jl1_Leaf)[posidx / cKPW];     // KeysPerWord = 8
                newBucket = Bucket ^ repKey;
                newBucket = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
#endif  // ! MMX1
                if (newBucket)         // Key found in Bucket?
                {
                    SEARCHPOPULATION(Pop1);
                    DIRECTHITS;
                    return(1);              // found
                }
                if (Index > Pjll1->jl1_Leaf[start])
                {
                    for(;;)
                    {
                        posidx += cKPW;
                        if ((posidx / 8) >= ((Pop1 + cKPW - 1) / 8)) goto NotFoundExit;
#ifdef  MMX1
                        PWord_t Bptr = ((PWord_t)Pjll1->jl1_Leaf) + posidx/cKPW;
                        newBucket = Hk64c(Bptr, Index);
#else   // ! MMX1
                        Bucket = ((PWord_t)Pjll1->jl1_Leaf)[posidx / cKPW];     // KeysPerWord = 4
                        newBucket = Bucket ^ repKey;
                        newBucket = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
#endif  // ! MMX1
                        if (newBucket)         // Key found in Bucket?
                        {
                            SEARCHPOPULATION(Pop1);
                            MISCOMPARESP((posidx - start) / cKPW);
                            return(1);              // found
                        }
                        if (((uint8_t *)(Pjll1->jl1_Leaf))[posidx] > Index) 
                            goto NotFoundExit;
                    }
                }
                else
                {
                    for(;;)
                    {
                        posidx -= cKPW;
                        if (posidx < 0) goto NotFoundExit;
#ifdef  MMX1
                        PWord_t Bptr = ((PWord_t)Pjll1->jl1_Leaf) + posidx/cKPW;
                        newBucket = Hk64c(Bptr, Index);
#else   // ! MMX1
                        Bucket = ((PWord_t)Pjll1->jl1_Leaf)[posidx / cKPW];     // KeysPerWord = 4
                        newBucket = Bucket ^ repKey;
                        newBucket = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
#endif  // ! MMX1
                        if (newBucket)         // Key found in Bucket?
                        {
                            SEARCHPOPULATION(Pop1);
                            MISCOMPARESM((start - posidx) / cKPW);
                            return(1);              // found
                        }
                        if (Pjll1->jl1_Leaf[posidx] < Index) 
                            goto NotFoundExit;
                    }
                }
            }
        }
#else   // ! noPARALLEL1

        case cJU_JPLEAF1:
        {
//            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
/////            if (ju_DcdNotMatchKey(Index, Pjp, 1)) break;
            Pjll1_t Pjll1 = P_JLL1(RawPntr);
            if (ju_DcdNotMatchKeyLeaf(Index, Pjll1->jl1_DcdPop0, 1)) break;

            Pop1          = ju_LeafPop0(Pjp) + 1;
#ifdef  JUDYL
            Pjv  = JL_LEAF1VALUEAREA(Pjll1, Pop1);
#endif  // JUDYL
            posidx = j__udySearchLeaf1(Pjll1, Pop1, Index, 1 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

#endif  // ! noPARALLEL1
#endif  //  JUDY1

#define cMagic  0x0101010101010100;
#define cSubExp  (256 / 8)
#define SPLIT(POP, KEY, EXP)  (((Word_t)(KEY) * (POP)) / (EXP))

#ifdef  JUDYL
        case cJU_JPLEAF1:
        {
            typedef union {
                Word_t  d_Offsets;
                uint8_t d_OctAccum[8];
            } d_t;
            d_t d;      // declare
// make these the same memory location
#define Offsets  d.d_Offsets
#define OctAccum d.d_OctAccum

            Pjll1_t Pjll1 = P_JLL1(RawPntr);
//            if (ju_DcdNotMatchKey(Index, Pjp, 1)) break;
//            if (ju_DcdNotMatchKeyLeaf(Index, Pjll1->jl1_DcdPop0, 1)) break;

            SEARCHPOPULATION(ju_LeafPop0(Pjp) + 1);

//          calculate octant (0..7) that may contains Index
            int       Oct = (uint8_t)Index / cSubExp;  
            Offsets = Pjp->jp_Octants * cMagic; // form octant populations

#ifdef  JUDYL
#ifdef  POPINJP
            Pop1 = ju_LeafPop0(Pjp) + 1;
#else   // POPINJP from octant
            Pop1  = OctAccum[8 - 1] + Pjp->jp_OctPop1[8 - 1];
            assert((ju_LeafPop0(Pjp) + 1) == Pop1);
#endif  // POPINJP in octant
            Pjv   =JL_LEAF1VALUEAREA(Pjll1, Pop1);
#endif  // JUDYL

            uint8_t *SubLeaf = Pjll1->jl1_Leaf + OctAccum[Oct];
            int      pop1   = Pjp->jp_OctPop1[Oct];        // right out of jp_t
            int      Start = SPLIT(pop1, (uint8_t)Index - (Oct * cSubExp), cSubExp);

            posidx = j__udySearchRawLeaf1(SubLeaf, pop1, Index, Start) + OctAccum[Oct];
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }
#endif  // JUDYL

#ifdef  JUDY1
#ifndef  noPARALLEL2

//  This needs fixing
#undef cbPK
#undef cKPW
#define cbPK    (16)            // bits per Key
#define cKPW    (cbPW / cbPK)   // Keys per Word

        case cJU_JPLEAF2:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFFFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 2)) break;

            Pjll        = P_JLL(RawPntr);
            Pop1        = ju_LeafPop0(Pjp) + 1;

//            Index       &= KEYMASK(2 * 8);
            Index       = Index & 0xFFFF;
            posidx      = PSPLIT(Pop1, Index, 2 * 8); 
            int start   = posidx;

// Note: The Key array must be padded with replicas of last Key Ins, Del and Cascade

//          replicate the Lsb in every Key position (at compile time)
            Word_t repLsbKey = REPKEY(cbPK, 1);

//          replicate the Msb in every Key position (at compile time)
            Word_t repMsbKey = repLsbKey << (cbPK - 1);

            Word_t  Bucket;
            Word_t  newBucket;

#ifdef  PADCHECK2
if ((Pop1 & 0x3))
{
    int roundupnextword = ((Pop1 + 3) / 4) * 4;
    for (int ii = Pop1; ii < roundupnextword; ii++)
    {
        if (((uint16_t *)Pjll)[Pop1 -1] != (((uint16_t *)Pjll)[ii]))
        {
printf("\n---Oops-----------Pop1 = %ld, Key = 0x%2lx, posidx = %d\n", Pop1, Index, posidx);
    for (int ii = 0; ii < ((Pop1 + 7) & -cKPW); ii++)
        printf("%d=0x%02x ", ii, ((uint16_t *)(Pjll))[ii]);
    printf("posidx = %d\n", posidx);
    break;
        }
    }
}
#endif  // PADCHECK2

//              make zero the searched for Key in new Bucket
//              Magic, the Msb=1 is located in the matching Key position
            for (;;)
            {
                Bucket = ((PWord_t)Pjll)[posidx / cKPW];     // KeysPerWord = 4
                newBucket = Bucket ^ REPKEY(cbPK, Index);
                newBucket = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
//printf("\n-----------------Pop1 = %ld, Key = 0x%4lx, Newket = 0x%16lx, posidx = %d\n", Pop1, Index, newBucket, posidx);
//
                if (newBucket)         // Key found in Bucket
                {
                    SEARCHPOPULATION(Pop1);
                    DIRECTHITS;
                    return(1);              // found
                }
                if (Index > ((uint16_t *)Pjll)[start])
                {
                    for(;;)
                    {
                        posidx += cKPW;
//                        if (posidx >= (Pop1 + 3)) goto NotFoundExit;
                        if ((posidx / 4) >= ((Pop1 + cKPW - 1) / 4)) goto NotFoundExit;

                        Bucket = ((PWord_t)Pjll)[posidx / cKPW];     // KeysPerWord = 4
                        newBucket = Bucket ^ REPKEY(cbPK, Index);
                        newBucket = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
                        if (newBucket)         // Key found in Bucket
                        {
                            SEARCHPOPULATION(Pop1);
                            MISCOMPARESP((posidx - start) / 4);
                            return(1);              // found
                        }
                        if (((uint16_t *)(Pjll))[posidx] > Index) goto NotFoundExit;
                    }
                }
                else
                {
                    for(;;)
                    {
                        posidx -= cKPW;
                        if (posidx < 0) goto NotFoundExit;
                        Bucket = ((PWord_t)Pjll)[posidx / cKPW];     // KeysPerWord = 4
                        newBucket = Bucket ^ REPKEY(cbPK, Index);
                        newBucket = (newBucket - repLsbKey) & (~newBucket) & repMsbKey;
                        if (newBucket)         // Key found in Bucket
                        {
                            SEARCHPOPULATION(Pop1);
                            MISCOMPARESM((start - posidx) / cKPW);
                            return(1);              // found
                        }
                        if (((uint16_t* )(Pjll))[posidx] < Index) goto NotFoundExit;
                    }
                }
            }
        }

//          else search for Key to slow way
//          entry PLeaf2 = Leaf2, Pjv = Value, Pop1 = population
//            posidx = j__udySearchLeaf2(PLeaf2, Pop1, Index, 2 * 8);
//            printf("-------------posidx from j__udySearchLeaf2 = %d\n", posidx);
//            if (posidx >= 0) 
//                return(1);
//            break;
#else   // ! noPARALLEL2

        case cJU_JPLEAF2:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 2)) break;

            Pop1        = ju_LeafPop0(Pjp) + 1;
            Pjll        = P_JLL(RawPntr);

//          entry Pjll = Leaf2, Pjv = Value, Pop1 = population
            posidx = j__udySearchLeaf2(Pjll, Pop1, Index, 2 * 8);
            if (posidx < 0) 
                break;
            return(1);
        }
#endif  // ! noPARALLEL2
#endif  //  JUDY1

#ifdef  JUDYL
        case cJU_JPLEAF2:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 2)) break;

            Pop1 = ju_LeafPop0(Pjp) + 1;
            Pjll = P_JLL(RawPntr);

//          entry Pjll = Leaf2, Pjv = Value, Pop1 = population
            posidx = j__udySearchLeaf2(Pjll, Pop1, Index, 2 * 8);
            if (posidx < 0) 
                break;
            Pjv = JL_LEAF2VALUEAREA(Pjll, Pop1);
            return((PPvoid_t) (Pjv + posidx));
        }
#endif  // JUDYL

        case cJU_JPLEAF3:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 3)) break;

            Pop1        = ju_LeafPop0(Pjp) + 1;
            Pjll        = P_JLL(RawPntr);

#ifdef  diag
if (Pop1 == 255)
{
printf("\n---------------------LEAF3\n");
for(int ii = 0; ii < Pop1; ii++)
{
    Word_t key;
    JU_COPY3_PINDEX_TO_LONG(key, (uint8_t *)Pjll + (3 * ii));
    printf("%2d = 0x%06lx\n", ii, key);
}
}
#endif  // diag

#ifdef  JUDYL
            Pjv = JL_LEAF3VALUEAREA(Pjll, Pop1);
#endif  // JUDYL
            goto Leaf3Exit;

Leaf3Exit: // entry Pjll = Leaf3, Pjv = Value, Pop1 = population
            posidx = j__udySearchLeaf3(Pjll, Pop1, Index, 3 * 8);
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
//printf("Get-F4: ju_LeafPop0(Pjp) = 0x%lx, ju_DcdPop0(Pjp) = 0x%016lx, Key = 0x%016lx\n", ju_LeafPop0(Pjp), ju_DcdPop0(Pjp), Index);
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 4)) break;

            Pop1        = ju_LeafPop0(Pjp) + 1;
            Pjll        = P_JLL(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF4VALUEAREA(Pjll, Pop1);
#endif  // JUDYL
            goto Leaf4Exit;
            
Leaf4Exit:
            posidx = j__udySearchLeaf4(Pjll, Pop1, Index, 4 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF5:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 5)) break;

            Pop1        = ju_LeafPop0(Pjp) + 1;
            Pjll        = P_JLL(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF5VALUEAREA(Pjll, Pop1);
#endif  // JUDYL
            goto Leaf5Exit;
            
Leaf5Exit:
            posidx = j__udySearchLeaf5(Pjll, Pop1, Index, 5 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF6:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 6)) break;

            Pop1        = ju_LeafPop0(Pjp) + 1;
            Pjll        = P_JLL(RawPntr);
#ifdef  JUDYL
            Pjv = JL_LEAF6VALUEAREA(Pjll, Pop1);
#endif  // JUDYL
            goto Leaf6Exit;
            
Leaf6Exit:
            posidx = j__udySearchLeaf6(Pjll, Pop1, Index, 6 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

        case cJU_JPLEAF7:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 7)) break;

            Pop1        = ju_LeafPop0(Pjp) + 1;
            Pjll        = P_JLL(RawPntr);

#ifdef diag
printf("\n--------Get----------LEAF7\n");
for(int ii = 0; ii < Pop1; ii++)
{
    Word_t key;
    JU_COPY7_PINDEX_TO_LONG(key, (uint8_t *)Pjll + (7 * ii));
    printf("%2d = 0x%016lx\n", ii, key);
}
#endif  // diag




#ifdef  JUDYL
            Pjv = JL_LEAF7VALUEAREA(Pjll, Pop1);
#endif  // JUDYL

            goto Leaf7Exit;
            
Leaf7Exit:
            posidx = j__udySearchLeaf7(Pjll, Pop1, Index, 7 * 8);
            goto CommonLeafExit;                // posidx & (only JudyL) Pjv
        }

// ****************************************************************************
// JPLEAF_B1:

        case cJU_JPLEAF_B1:
        {
            Pjlb_t    Pjlb;
            Word_t    subexp;   // in bitmap, 0..7.
            BITMAPL_t BitMap;   // for one subexpanse.
            BITMAPL_t BitMsk;   // bit in BitMap for Indexs Digit.

            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 1)) break;

            DIRECTHITS;       // not necessary, because always 100%
            SEARCHPOPULATION(ju_LeafPop0(Pjp) + 1);

            Pjlb   = P_JLB(RawPntr);
            Digit  = JU_DIGITATSTATE(Index, 1);
            subexp = Digit / cJU_BITSPERSUBEXPL;

#ifdef EXP1BIT
            if (subexp == 0) return(1); // 7% faster
#endif  // EXP1BIT

            BitMap = JU_JLB_BITMAP(Pjlb, subexp);
            BitMsk = JU_BITPOSMASKL(Digit);

// No value in subexpanse for Index => Index not found:

            if (! (BitMap & BitMsk)) 
                break;

#ifdef  JUDY1
            return(1);
#endif  // JUDY1

#ifdef  JUDYL
// Count value areas in the subexpanse below the one for Index:
// JudyL is much more complicated because of Value area subarrays:

#ifdef BMVALUE
//          if population of subexpanse == 1, return ^ to Value^
            if (BitMap == BitMsk)
            {
//////            if (ju_DcdNotMatchKey(Index, Pjp, 1)) break;
                return((PPvoid_t)(&Pjlb->jLlb_jLlbs[subexp].jLlbs_PV_Raw));
            }
#endif /* BMVALUE */

//          Get raw pointer to Value area
//            Word_t PjvRaw = Pjlb->jLlb_jLlbs[subexp].jLlbs_PV_Raw;
            Word_t PjvRaw = JL_JLB_PVALUE(Pjlb, subexp);

#ifdef  UNCOMPVALUE
            if (PjvRaw & 1)     // check if uncompressed Value area
            {
                posidx = Digit % cJU_BITSPERSUBEXPL; // offset from Index
// done by P_JV()  PjvRaw -= 1;       // mask off least bit
            }
            else                                    // Compressed, calc offset
#endif  // UNCOMPVALUE
            {
                posidx = j__udyCountBitsL(BitMap & (BitMsk - 1));
//               posidx = BitMap & 255;
            }
            return((PPvoid_t) (P_JV(PjvRaw) + posidx));
//#ifdef  POSIDX
//            return((PPvoid_t) (Pjlb->jLlb_PV_Raw + posidx));
//#else   //  POSIDX
//            return((PPvoid_t) (Pjlb->jLlb_PV_Raw + Digit));
//#endif  //  POSIDX

#endif // JUDYL

        } // case cJU_JPLEAF_B1

#ifdef JUDY1
// ****************************************************************************
// JPFULLPOPU1:
//
// If the Index is in the expanse, it is necessarily valid (found).

        case cJ1_JPFULLPOPU1:
        {
            assert(ju_LeafPop0(Pjp) == (ju_DcdPop0(Pjp) & 0xFF));
            if (ju_DcdNotMatchKey(Index, Pjp, 1)) break;
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
//            SEARCHPOPULATION(1);      // Too much overhead
//            DIRECTHITS;               // Too much overhead
//
//            if (JU_JPDCDPOP0(Pjp) != JU_TrimToIMM01(Index)) 
//            if (JU_TrimToIMM01(Index ^ ju_DcdPop0(Pjp)) != 0) 
//            if (ju_DcdPop0(Pjp) != JU_TrimToIMM01(Index)) 

//          The "<< 8" makes a possible bad assumption!!!!, but
//          this version does not have an conditional branch
            if ((ju_IMM01Key(Pjp) ^ Index) << 8) 
                break;

#ifdef  JUDY1
        return(1);
#endif  // JUDY1

#ifdef  JUDYL
//        return((PPvoid_t) &(Pjp->jp_ValueI));   // ^ immediate value
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
            Pjll = (Pjll_t)ju_PImmed1(Pjp);     // Get ^ to Keys

            posidx = j__udySearchImmed1(Pjll, Pop1, Index, 1 * 8);
#ifdef  JUDYL
            Pjv = P_JV(RawPntr);       // Get ^ to Values
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
            Pjll = (Pjll_t)ju_PImmed2(Pjp);     // Get ^ to Keys
            posidx = j__udySearchLeaf2(Pjll, Pop1, Index, 2 * 8);
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
            Pjll = (Pjll_t)ju_PImmed3(Pjp);     // Get ^ to Keys
#ifdef  JUDYL
            Pjv = P_JV(RawPntr);  // ^ immediate values area
#endif  // JUDYL
            goto Leaf3Exit;
        }

#ifdef  JUDYL
        case cJL_JPIMMED_4_02:
        {
            Pop1 = 2;
            Pjv = P_JV(RawPntr);  // ^ immediate values area
            Pjll = (Pjll_t)ju_PImmed4(Pjp);
            goto Leaf4Exit;
        }
#endif  // JUDYL

#ifdef  JUDY1
        case cJ1_JPIMMED_4_02:
        case cJ1_JPIMMED_4_03:
        {
            Pop1 = ju_Type(Pjp) - cJ1_JPIMMED_4_02 + 2;
            Pjll = (Pjll_t)ju_PImmed4(Pjp);     // Get ^ to Keys
            goto Leaf4Exit;
        }

        case cJ1_JPIMMED_5_02:
        case cJ1_JPIMMED_5_03:
        {
            Pop1 = ju_Type(Pjp) - cJ1_JPIMMED_5_02 + 2;
            Pjll = (Pjll_t)ju_PImmed5(Pjp);     // Get ^ to Keys
            goto Leaf5Exit;
        }

        case cJ1_JPIMMED_6_02:
        {
            Pop1 = 2;
            Pjll = (Pjll_t)ju_PImmed6(Pjp);     // Get ^ to Keys
            goto Leaf6Exit;
        }

        case cJ1_JPIMMED_7_02:
        {
            Pop1 = 2;
            Pjll = (Pjll_t)ju_PImmed7(Pjp);     // Get ^ to Keys
            goto Leaf7Exit;
        }
#endif  // JUDY1

// ****************************************************************************
// INVALID JP TYPE:

        default:

ReturnCorrupt:
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
