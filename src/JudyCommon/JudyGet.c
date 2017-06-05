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

#ifdef TRACEJPR                 // different macro name, for "retrieval" only.
#include "JudyPrintJP.c"
#endif


// ****************************************************************************
// J U D Y   1   T E S T
// J U D Y   L   G E T
//
// See the manual entry for details.  Note support for "shortcut" entries to
// trees known to start with a JPM.

// See the manual entry for details.  Note support for "shortcut" entries to
// trees known to start with a JPM.
#ifdef JUDY1
#ifdef JUDYGETINLINE
FUNCTION int j__udy1Test (Pcvoid_t PArray, // from which to retrieve.
                       Word_t Index,    // to retrieve.
                       PJError_t PJError        // optional, for returning error info.
        )
#else  // ! JUDYGETINLINE 
FUNCTION int Judy1Test (Pcvoid_t PArray, // from which to retrieve.
                         Word_t Index,    // to retrieve.
                         PJError_t PJError        // optional, for returning error info.
        )
#endif // ! JUDYGETINLINE 
#else /* JUDYL */
#ifdef JUDYGETINLINE
FUNCTION PPvoid_t j__udyLGet (Pcvoid_t PArray,     // from which to retrieve.
                Word_t Index,        // to retrieve.
                PJError_t PJError    // optional, for returning error info.
    )
#else  // ! JUDYGETINLINE 
FUNCTION PPvoid_t JudyLGet (Pcvoid_t PArray,     // from which to retrieve.
                Word_t Index,        // to retrieve.
                PJError_t PJError    // optional, for returning error info.
    )
#endif // ! JUDYGETINLINE 
#endif /* JUDYL */
{
    Pjp_t     Pjp;                      // current JP while walking the tree.
    Pjpm_t    Pjpm;                     // for global accounting.
    uint8_t   Digit;                    // byte just decoded from Index.
    Word_t    Pop1;                     // leaf population (number of indexes).
    Pjll_t    Pjll;                     // pointer to LeafL.
    if (PArray == (Pcvoid_t) NULL)      // empty array.
    {
#ifdef JUDY1
        return (0);
#else /* JUDYL */
        return ((PPvoid_t) NULL);
#endif /* JUDYL */
    }

// ****************************************************************************
// PROCESS TOP LEVEL BRANCHES AND LEAF:
    if (JU_LEAFW_POP0(PArray) < cJU_LEAFW_MAXPOP1)      // must be a LEAFW
    {
        Pjlw_t    Pjlw = P_JLW(PArray); // first word of leaf.
        int       posidx;               // signed offset in leaf.
        Pop1 = Pjlw[0] + 1;
        posidx = j__udySearchLeafW(Pjlw + 1, Pop1, Index);
        if (posidx >= 0)
        {
#ifdef JUDY1
            return (1);
#else /* JUDYL */
            return ((PPvoid_t) (JL_LEAFWVALUEAREA(Pjlw, Pop1) + posidx));
#endif /* JUDYL */
        }
#ifdef JUDY1
        return (0);
#else /* JUDYL */
        return ((PPvoid_t) NULL);
#endif /* JUDYL */
    }
    Pjpm = P_JPM(PArray);
    Pjp = &(Pjpm->jpm_JP);              // top branch is below JPM.


//  6 4  B I T  J U D Y L G E T  /  J U D Y 1 T E S T
//
// ****************************************************************************
// WALK THE JUDY TREE USING A STATE MACHINE:
  ContinueWalk:                        // for going down one level; come here with Pjp set.
    switch (JU_JPTYPE(Pjp))
    {

// Trace is different with j__ entry points
#ifdef  TRACEJPR
#ifdef  JUDYGETINLINE
        JudyPrintJP(Pjp, "_g", __LINE__);
#else   // ! JUDYGETINLINE 
        JudyPrintJP(Pjp, "g", __LINE__);
#endif  // ! JUDYGETINLINE 
#endif  // TRACEJPR

// NOTE: The Judy 32Bit code is in the 2nd half of the file

#ifdef  JU_64BIT
// Ensure the switch table starts at 0 for speed; otherwise more code is
// executed:
    case 0:
        goto ReturnCorrupt;             // save a little code.
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
        assert(ParentJPType >= cJU_JPBRANCH_U2);
        assert(ParentJPType <= cJU_JPBRANCH_U);
#ifdef JUDY1
        return (0);
#else /* JUDYL */
        return ((PPvoid_t) NULL);
#endif /* JUDYL */
// ****************************************************************************
// JPBRANCH_L*:
//
// Note:  The use of JU_DCDNOTMATCHINDEX() in branches is not strictly
// required,since this can be done at leaf level, but it costs nothing to do it
// sooner, and it aborts an unnecessary traversal sooner.
    case cJU_JPBRANCH_L2:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            break;
        Digit = JU_DIGITATSTATE(Index, 2);
        goto JudyBranchL;
    case cJU_JPBRANCH_L3:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 3))
            break;
        Digit = JU_DIGITATSTATE(Index, 3);
        goto JudyBranchL;
    case cJU_JPBRANCH_L4:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 4))
            break;
        Digit = JU_DIGITATSTATE(Index, 4);
        goto JudyBranchL;
    case cJU_JPBRANCH_L5:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 5))
            break;
        Digit = JU_DIGITATSTATE(Index, 5);
        goto JudyBranchL;
    case cJU_JPBRANCH_L6:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 6))
            break;
        Digit = JU_DIGITATSTATE(Index, 6);
        goto JudyBranchL;
    case cJU_JPBRANCH_L7:
        // JU_DCDNOTMATCHINDEX() would be a no-op.
        Digit = JU_DIGITATSTATE(Index, 7);
        goto JudyBranchL;
    case cJU_JPBRANCH_L:
    {
        Pjbl_t    Pjbl;
        int       posidx;
        Digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
// Common code for all BranchLs; come here with Digit set:
      JudyBranchL:
        Pjbl = P_JBL(Pjp->jp_Addr);
        posidx = 0;
        do
        {
            if (Pjbl->jbl_Expanse[posidx] == Digit)
            {                           // found Digit; continue traversal:
                Pjp = Pjbl->jbl_jp + posidx;
                goto ContinueWalk;
            }
        } while (++posidx != Pjbl->jbl_NumJPs);
        break;
    }
// ****************************************************************************
// JPBRANCH_B*:
    case cJU_JPBRANCH_B2:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            break;
        Digit = JU_DIGITATSTATE(Index, 2);
        goto JudyBranchB;
    case cJU_JPBRANCH_B3:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 3))
            break;
        Digit = JU_DIGITATSTATE(Index, 3);
        goto JudyBranchB;
    case cJU_JPBRANCH_B4:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 4))
            break;
        Digit = JU_DIGITATSTATE(Index, 4);
        goto JudyBranchB;
    case cJU_JPBRANCH_B5:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 5))
            break;
        Digit = JU_DIGITATSTATE(Index, 5);
        goto JudyBranchB;
    case cJU_JPBRANCH_B6:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 6))
            break;
        Digit = JU_DIGITATSTATE(Index, 6);
        goto JudyBranchB;
    case cJU_JPBRANCH_B7:
        // JU_DCDNOTMATCHINDEX() would be a no-op.
        Digit = JU_DIGITATSTATE(Index, 7);
        goto JudyBranchB;
    case cJU_JPBRANCH_B:
    {
        Pjbb_t    Pjbb;
        Word_t    subexp;               // in bitmap, 0..7.
        BITMAPB_t BitMap;               // for one subexpanse.
        BITMAPB_t BitMask;              // bit in BitMap for Indexs Digit.
        Digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
// Common code for all BranchBs; come here with Digit set:
      JudyBranchB:
        Pjbb = P_JBB(Pjp->jp_Addr);
        subexp = Digit / cJU_BITSPERSUBEXPB;
        BitMap = JU_JBB_BITMAP(Pjbb, subexp);
        Pjp = P_JP(JU_JBB_PJP(Pjbb, subexp));
        BitMask = JU_BITPOSMASKB(Digit);
// No JP in subexpanse for Index => Index not found:
        if (!(BitMap & BitMask))
            break;
// Count JPs in the subexpanse below the one for Index:
        Pjp += j__udyCountBitsB(BitMap & (BitMask - 1));
        goto ContinueWalk;
    }                                   // case cJU_JPBRANCH_B*
// ****************************************************************************
// JPBRANCH_U*:
//
// Notice the reverse order of the cases, and falling through to the next case,
// for performance.
    case cJU_JPBRANCH_U:
        Pjp = JU_JBU_PJP(Pjp, Index, cJU_ROOTSTATE);
// If not a BranchU, traverse; otherwise fall into the next case, which makes
// this very fast code for a large Judy array (mainly BranchUs), especially
// when branches are already in the cache, such as for prev/next:
        if (JU_JPTYPE(Pjp) != cJU_JPBRANCH_U7)
            goto ContinueWalk;
    case cJU_JPBRANCH_U7:
        // JU_DCDNOTMATCHINDEX() would be a no-op.
        Pjp = JU_JBU_PJP(Pjp, Index, 7);
        if (JU_JPTYPE(Pjp) != cJU_JPBRANCH_U6)
            goto ContinueWalk;
        // and fall through.
    case cJU_JPBRANCH_U6:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 6))
            break;
        Pjp = JU_JBU_PJP(Pjp, Index, 6);
        if (JU_JPTYPE(Pjp) != cJU_JPBRANCH_U5)
            goto ContinueWalk;
        // and fall through.
    case cJU_JPBRANCH_U5:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 5))
            break;
        Pjp = JU_JBU_PJP(Pjp, Index, 5);
        if (JU_JPTYPE(Pjp) != cJU_JPBRANCH_U4)
            goto ContinueWalk;
        // and fall through.
    case cJU_JPBRANCH_U4:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 4))
            break;
        Pjp = JU_JBU_PJP(Pjp, Index, 4);
        if (JU_JPTYPE(Pjp) != cJU_JPBRANCH_U3)
            goto ContinueWalk;
        // and fall through.
    case cJU_JPBRANCH_U3:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 3))
            break;
        Pjp = JU_JBU_PJP(Pjp, Index, 3);
        if (JU_JPTYPE(Pjp) != cJU_JPBRANCH_U2)
            goto ContinueWalk;
        // and fall through.
    case cJU_JPBRANCH_U2:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            break;
        Pjp = JU_JBU_PJP(Pjp, Index, 2);
// Note:  BranchU2 is a special case that must continue traversal to a leaf,
// immed, full, or null type:
        goto ContinueWalk;
// ****************************************************************************
// JPLEAF*:
//
// Note:  Here the calls of JU_DCDNOTMATCHINDEX() are necessary and check
// whether Index is out of the expanse of a narrow pointer.
#ifdef JUDYL
    case cJU_JPLEAF1:
    {
        int       posidx;               // signed offset in leaf.
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 1))
            break;
        Pop1 = JU_JPLEAF_POP0(Pjp) + 1;
        Pjll = P_JLL(Pjp->jp_Addr);
        if ((posidx = j__udySearchLeaf1(Pjll, Pop1, Index)) < 0)
            break;
        return ((PPvoid_t) (JL_LEAF1VALUEAREA(Pjll, Pop1) + posidx));
    }
#endif /* JUDYL */
    case cJU_JPLEAF2:
    {
        int       posidx;               // signed offset in leaf.
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            break;
        Pop1 = JU_JPLEAF_POP0(Pjp) + 1;
        Pjll = P_JLL(Pjp->jp_Addr);
        if ((posidx = j__udySearchLeaf2(Pjll, Pop1, Index)) < 0)
            break;
#ifdef JUDY1
        return (1);
#else /* JUDYL */
        return ((PPvoid_t) (JL_LEAF2VALUEAREA(Pjll, Pop1) + posidx));
#endif /* JUDYL */
    }
    case cJU_JPLEAF3:
    {
        int       posidx;               // signed offset in leaf.
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 3))
            break;
        Pop1 = JU_JPLEAF_POP0(Pjp) + 1;
        Pjll = P_JLL(Pjp->jp_Addr);
        if ((posidx = j__udySearchLeaf3(Pjll, Pop1, Index)) < 0)
            break;
#ifdef JUDY1
        return (1);
#else /* JUDYL */
        return ((PPvoid_t) (JL_LEAF3VALUEAREA(Pjll, Pop1) + posidx));
#endif /* JUDYL */
    }
    case cJU_JPLEAF4:
    {
        int       posidx;               // signed offset in leaf.
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 4))
            break;
        Pop1 = JU_JPLEAF_POP0(Pjp) + 1;
        Pjll = P_JLL(Pjp->jp_Addr);
        if ((posidx = j__udySearchLeaf4(Pjll, Pop1, Index)) < 0)
            break;
#ifdef JUDY1
        return (1);
#else /* JUDYL */
        return ((PPvoid_t) (JL_LEAF4VALUEAREA(Pjll, Pop1) + posidx));
#endif /* JUDYL */
    }
    case cJU_JPLEAF5:
    {
        int       posidx;               // signed offset in leaf.
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 5))
            break;
        Pop1 = JU_JPLEAF_POP0(Pjp) + 1;
        Pjll = P_JLL(Pjp->jp_Addr);
        if ((posidx = j__udySearchLeaf5(Pjll, Pop1, Index)) < 0)
            break;
#ifdef JUDY1
        return (1);
#else /* JUDYL */
        return ((PPvoid_t) (JL_LEAF5VALUEAREA(Pjll, Pop1) + posidx));
#endif /* JUDYL */
    }
    case cJU_JPLEAF6:
    {
        int       posidx;               // signed offset in leaf.
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 6))
            break;
        Pop1 = JU_JPLEAF_POP0(Pjp) + 1;
        Pjll = P_JLL(Pjp->jp_Addr);
        if ((posidx = j__udySearchLeaf6(Pjll, Pop1, Index)) < 0)
            break;
#ifdef JUDY1
        return (1);
#else /* JUDYL */
        return ((PPvoid_t) (JL_LEAF6VALUEAREA(Pjll, Pop1) + posidx));
#endif /* JUDYL */
    }
    case cJU_JPLEAF7:
    {
        int       posidx;               // signed offset in leaf.
        // JU_DCDNOTMATCHINDEX() would be a no-op.
        Pop1 = JU_JPLEAF_POP0(Pjp) + 1;
        Pjll = P_JLL(Pjp->jp_Addr);
        if ((posidx = j__udySearchLeaf7(Pjll, Pop1, Index)) < 0)
            break;
#ifdef JUDY1
        return (1);
#else /* JUDYL */
        return ((PPvoid_t) (JL_LEAF7VALUEAREA(Pjll, Pop1) + posidx));
#endif /* JUDYL */
    }
// ****************************************************************************
// JPLEAF_B1:
    case cJU_JPLEAF_B1:
    {
        Pjlb_t    Pjlb;
#ifdef JUDYL
        int       posidx;
        Word_t    subexp;               // in bitmap, 0..7.
        BITMAPL_t BitMap;               // for one subexpanse.
        BITMAPL_t BitMask;              // bit in BitMap for Indexs Digit.
        Pjv_t     Pjv;
#endif /* JUDYL */
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 1))
            break;
        Pjlb = P_JLB(Pjp->jp_Addr);
#ifdef JUDY1
// Simply check if Indexs bit is set in the bitmap:
        if (JU_BITMAPTESTL(Pjlb, Index))
            return (1);
        break;
#else /* JUDYL */
// JudyL is much more complicated because of value area subarrays:
        Digit = JU_DIGITATSTATE(Index, 1);
        subexp = Digit / cJU_BITSPERSUBEXPL;
        BitMap = JU_JLB_BITMAP(Pjlb, subexp);
        BitMask = JU_BITPOSMASKL(Digit);
// No value in subexpanse for Index => Index not found:
        if (!(BitMap & BitMask))
            break;
// Count value areas in the subexpanse below the one for Index:
        Pjv = P_JV(JL_JLB_PVALUE(Pjlb, subexp));
        assert(Pjv != (Pjv_t) NULL);
        posidx = j__udyCountBitsL(BitMap & (BitMask - 1));
        return ((PPvoid_t) (Pjv + posidx));
#endif /* JUDYL */
    }                                   // case cJU_JPLEAF_B1

#ifdef JUDY1
// ****************************************************************************
// JPFULLPOPU1:
//
// If the Index is in the expanse, it is necessarily valid (found).
    case cJ1_JPFULLPOPU1:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 1)) break;
        return (1);
#endif /* JUDY1 */

// ****************************************************************************
// JPIMMED*:
//
// Note that the contents of jp_DcdPopO are different for cJU_JPIMMED_*_01:
    case cJU_JPIMMED_1_01:
    case cJU_JPIMMED_2_01:
    case cJU_JPIMMED_3_01:
    case cJU_JPIMMED_4_01:
    case cJU_JPIMMED_5_01:
    case cJU_JPIMMED_6_01:
    case cJU_JPIMMED_7_01:
        if (JU_JPDCDPOP0(Pjp) != JU_TRIMTODCDSIZE(Index))
            break;
#ifdef JUDY1
        return (1);
#else /* JUDYL */
        return ((PPvoid_t) & (Pjp->jp_Addr));
#endif /* JUDYL */
//   Macros to make code more readable and avoid dup errors
#ifdef JUDY1
    case cJ1_JPIMMED_1_15:
        if (Pjp->jp_1Index1[15 - 1] == (uint8_t) Index) return (1);
    case cJ1_JPIMMED_1_14:
        if (Pjp->jp_1Index1[14 - 1] == (uint8_t) Index) return (1);
    case cJ1_JPIMMED_1_13:
        if (Pjp->jp_1Index1[13 - 1] == (uint8_t) Index) return (1);
    case cJ1_JPIMMED_1_12:
        if (Pjp->jp_1Index1[12 - 1] == (uint8_t) Index) return (1);
    case cJ1_JPIMMED_1_11:
        if (Pjp->jp_1Index1[11 - 1] == (uint8_t) Index) return (1);
    case cJ1_JPIMMED_1_10:
        if (Pjp->jp_1Index1[10 - 1] == (uint8_t) Index) return (1);
    case cJ1_JPIMMED_1_09:
        if (Pjp->jp_1Index1[9 - 1] == (uint8_t) Index) return (1);
    case cJ1_JPIMMED_1_08:
        if (Pjp->jp_1Index1[8 - 1] == (uint8_t) Index) return (1);
#endif /* ! JUDYL */
    case cJU_JPIMMED_1_07:
#ifdef JUDY1
        if (Pjp->jp_1Index1[7 - 1] == (uint8_t) Index) return (1);
#else /* JUDYL */
        if (Pjp->jp_LIndex1[7 - 1] == (uint8_t) Index)
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + 7 - 1));
#endif /* JUDYL */
    case cJU_JPIMMED_1_06:
#ifdef JUDY1
        if (Pjp->jp_1Index1[6 - 1] == (uint8_t) Index) return (1);
#else /* JUDYL */
        if (Pjp->jp_LIndex1[6 - 1] == (uint8_t) Index)
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + 6 - 1));
#endif /* JUDYL */
    case cJU_JPIMMED_1_05:
#ifdef JUDY1
        if (Pjp->jp_1Index1[5 - 1] == (uint8_t) Index) return (1);
#else /* JUDYL */
        if (Pjp->jp_LIndex1[5 - 1] == (uint8_t) Index)
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + 5 - 1));
#endif /* JUDYL */
    case cJU_JPIMMED_1_04:
#ifdef JUDY1
        if (Pjp->jp_1Index1[4 - 1] == (uint8_t) Index) return (1);
#else /* JUDYL */
        if (Pjp->jp_LIndex1[4 - 1] == (uint8_t) Index)
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + 4 - 1));
#endif /* JUDYL */
    case cJU_JPIMMED_1_03:
#ifdef JUDY1
        if (Pjp->jp_1Index1[3 - 1] == (uint8_t) Index) return (1);
#else /* JUDYL */
        if (Pjp->jp_LIndex1[3 - 1] == (uint8_t) Index)
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + 3 - 1));
#endif /* JUDYL */
    case cJU_JPIMMED_1_02:
#ifdef JUDY1
        if (Pjp->jp_1Index1[2 - 1] == (uint8_t) Index) return (1);
        if (Pjp->jp_1Index1[1 - 1] == (uint8_t) Index) return (1);
#else /* JUDYL */
        if (Pjp->jp_LIndex1[2 - 1] == (uint8_t) Index)
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + 2 - 1));
        if (Pjp->jp_LIndex1[1 - 1] == (uint8_t) Index)
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + 1 - 1));
#endif /* JUDYL */
        break;
#ifdef JUDY1
    case cJ1_JPIMMED_2_07:
        if (Pjp->jp_1Index2[7 - 1] == (uint16_t) Index) return (1);
    case cJ1_JPIMMED_2_06:
        if (Pjp->jp_1Index2[6 - 1] == (uint16_t) Index) return (1);
    case cJ1_JPIMMED_2_05:
        if (Pjp->jp_1Index2[5 - 1] == (uint16_t) Index) return (1);
    case cJ1_JPIMMED_2_04:
        if (Pjp->jp_1Index2[4 - 1] == (uint16_t) Index) return (1);
#endif /* ! JUDYL */
    case cJU_JPIMMED_2_03:
#ifdef JUDY1
        if (Pjp->jp_1Index2[3 - 1] == (uint16_t) Index) return (1);
#else /* JUDYL */
        if (Pjp->jp_LIndex2[3 - 1] == (uint16_t) Index)
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + (3) - 1));
#endif /* JUDYL */
    case cJU_JPIMMED_2_02:
#ifdef JUDY1
        if (Pjp->jp_1Index2[2 - 1] == (uint16_t) Index) return (1);
#else /* JUDYL */
        if (Pjp->jp_LIndex2[2 - 1] == (uint16_t) Index)
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + (2) - 1));
#endif /* JUDYL */
#ifdef JUDY1
        if (Pjp->jp_1Index2[0] == (uint16_t) Index) return (1);
#else /* JUDYL */
        if (Pjp->jp_LIndex2[0] == (uint16_t) Index)
            return ((PPvoid_t) (P_JV(Pjp->jp_Addr) + 1 - 1));
#endif /* JUDYL */
        break;
#ifdef JUDY1
    case cJ1_JPIMMED_3_05:
    {
        Word_t    i_ndex;
        uint8_t  *a_ddr;
        a_ddr = Pjp->jp_1Index1 + ((5 - 1) * 3);
        JU_COPY3_PINDEX_TO_LONG(i_ndex, a_ddr);
        if (i_ndex == JU_LEASTBYTES(Index, 3)) return (1);
    }
    case cJ1_JPIMMED_3_04:
    {
        Word_t    i_ndex;
        uint8_t  *a_ddr;
        a_ddr = Pjp->jp_1Index1 + ((4 - 1) * 3);
        JU_COPY3_PINDEX_TO_LONG(i_ndex, a_ddr);
        if (i_ndex == JU_LEASTBYTES(Index, 3)) return (1);
    }
    case cJ1_JPIMMED_3_03:
    {
        Word_t    i_ndex;
        uint8_t  *a_ddr;
        a_ddr = Pjp->jp_1Index1 + ((3 - 1) * 3);
        JU_COPY3_PINDEX_TO_LONG(i_ndex, a_ddr);
        if (i_ndex == JU_LEASTBYTES(Index, 3)) return (1);
    }
#endif /* ! JUDYL */
    case cJU_JPIMMED_3_02:
    {
        Word_t    i_ndex;
        uint8_t  *a_ddr;
#ifdef JUDY1
        a_ddr = Pjp->jp_1Index1 + ((2 - 1) * 3);
#else /* JUDYL */
        a_ddr = Pjp->jp_LIndex1 + ((2 - 1) * 3);
#endif /* JUDYL */
        JU_COPY3_PINDEX_TO_LONG(i_ndex, a_ddr);
        if (i_ndex == JU_LEASTBYTES(Index, 3))
#ifdef JUDY1
            return (1);
         a_ddr = Pjp->jp_1Index1 + (1 - 1) * 3;
#else /* JUDYL */
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + 2 - 1));
         a_ddr = Pjp->jp_LIndex1 + (1 - 1) * 3;
#endif /* JUDYL */
         JU_COPY3_PINDEX_TO_LONG(i_ndex, a_ddr);
         if (i_ndex == JU_LEASTBYTES(Index, 3))
#ifdef JUDY1
           return (1);
#else /* JUDYL */
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + 1 - 1));
#endif /* JUDYL */
        break;
    }
#ifdef JUDY1
    case cJ1_JPIMMED_4_03:
        if (Pjp->jp_1Index4[3 - 1] == (uint32_t)Index)
            return (1);
    case cJ1_JPIMMED_4_02:
        if (Pjp->jp_1Index4[2 - 1] == (uint32_t)Index)
            return (1);
        if (Pjp->jp_1Index4[1 - 1] == (uint32_t)Index)
            return (1);
        break;
    case cJ1_JPIMMED_5_03:
    {
        Word_t    i_ndex;
        uint8_t  *a_ddr;
        a_ddr = Pjp->jp_1Index1 + ((3 - 1) * 5);
        JU_COPY5_PINDEX_TO_LONG(i_ndex, a_ddr);
        if (i_ndex == JU_LEASTBYTES(Index, 5)) return (1);
    }
    case cJ1_JPIMMED_5_02:
    {
        Word_t    i_ndex;
        uint8_t  *a_ddr;
        a_ddr = Pjp->jp_1Index1 + ((2 - 1) * 5);
        JU_COPY5_PINDEX_TO_LONG(i_ndex, a_ddr);
        if (i_ndex == JU_LEASTBYTES(Index, 5)) return (1);
        a_ddr = Pjp->jp_1Index1 + (1 - 1) * 5;
        JU_COPY5_PINDEX_TO_LONG(i_ndex, a_ddr);
        if (i_ndex == JU_LEASTBYTES(Index, 5)) return (1);
        break;
    }
    case cJ1_JPIMMED_6_02:
    {
        Word_t    i_ndex;
        uint8_t  *a_ddr;
        a_ddr = Pjp->jp_1Index1 + (2 - 1) * 6;
        JU_COPY6_PINDEX_TO_LONG(i_ndex, a_ddr);
        if (i_ndex == JU_LEASTBYTES(Index, 6)) return (1);
        a_ddr = Pjp->jp_1Index1 + ((1 - 1) * 6);
        JU_COPY6_PINDEX_TO_LONG(i_ndex, a_ddr);
        if (i_ndex == JU_LEASTBYTES((Index), (6))) return (1);
        break;
    }
    case cJ1_JPIMMED_7_02:
    {
        Word_t    i_ndex;
        uint8_t  *a_ddr;
        a_ddr = Pjp->jp_1Index1 + (((2) - 1) * (7));
        JU_COPY7_PINDEX_TO_LONG(i_ndex, a_ddr);
        if (i_ndex == JU_LEASTBYTES(Index, 7)) return (1);
        a_ddr = Pjp->jp_1Index1 + ((1 - 1) * 7);
        JU_COPY7_PINDEX_TO_LONG(i_ndex, a_ddr);
        if (i_ndex == JU_LEASTBYTES(Index, 7)) return (1);
        break;
    }
#endif /* JUDY1 */
// ****************************************************************************
// INVALID JP TYPE:
    default:
      ReturnCorrupt:
        JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
#ifdef JUDY1
        return (JERRI);
#else /* JUDYL */
        return (PPJERR);
#endif /* JUDYL */
    }                                   // switch on JP type
#ifdef JUDY1
    return (0);
#else /* JUDYL */
    return ((PPvoid_t) NULL);
#endif /* JUDYL */
}                                       // Judy1Test() / JudyLGet()


//  3 2  B I T  J U D Y L G E T  /  J U D Y 1 T E S T

// ****************************************************************************
// WALK THE JUDY TREE USING A STATE MACHINE:
//  ContinueWalk:                        // for going down one level; come here with Pjp set.
//    switch (JU_JPTYPE(Pjp))
//    {

// Trace is different with j__* entry points
//#ifdef  TRACEJPR
//#ifdef  JUDYGETINLINE
//        JudyPrintJP(Pjp, "_g", __LINE__);
//#else   // ! JUDYGETINLINE 
//        JudyPrintJP(Pjp, "g", __LINE__);
//#endif  // ! JUDYGETINLINE 
//#endif  // TRACEJPR

#else   // JU_32BIT

// Ensure the switch table starts at 0 for speed; otherwise more code is
// executed:
    case 0:
        goto ReturnCorrupt;             // save a little code.
// ****************************************************************************
// JPNULL*:
//
// Note:  These are legitimate in a BranchU (only) and do not constitute a
// fault.
    case cJU_JPNULL1:
    case cJU_JPNULL2:
    case cJU_JPNULL3:
        assert(ParentJPType >= cJU_JPBRANCH_U2);
        assert(ParentJPType <= cJU_JPBRANCH_U);
#ifdef JUDY1
        return (0);
#else /* JUDYL */
        return ((PPvoid_t) NULL);
#endif /* JUDYL */
// ****************************************************************************
// JPBRANCH_L*:
//
// Note:  The use of JU_DCDNOTMATCHINDEX() in branches is not strictly
// required,since this can be done at leaf level, but it costs nothing to do it
// sooner, and it aborts an unnecessary traversal sooner.
    case cJU_JPBRANCH_L2:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            break;
        Digit = JU_DIGITATSTATE(Index, 2);
        goto JudyBranchL;
    case cJU_JPBRANCH_L3:
        Digit = JU_DIGITATSTATE(Index, 3);
        goto JudyBranchL;
    case cJU_JPBRANCH_L:
    {
        Pjbl_t    Pjbl;
        int       posidx;
        Digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
// Common code for all BranchLs; come here with Digit set:
      JudyBranchL:
        Pjbl = P_JBL(Pjp->jp_Addr);
        posidx = 0;
        do
        {
            if (Pjbl->jbl_Expanse[posidx] == Digit)
            {                           // found Digit; continue traversal:
                Pjp = Pjbl->jbl_jp + posidx;
                goto ContinueWalk;
            }
        } while (++posidx != Pjbl->jbl_NumJPs);
        break;
    }
// ****************************************************************************
// JPBRANCH_B*:
    case cJU_JPBRANCH_B2:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            break;
        Digit = JU_DIGITATSTATE(Index, 2);
        goto JudyBranchB;
    case cJU_JPBRANCH_B3:
        Digit = JU_DIGITATSTATE(Index, 3);
        goto JudyBranchB;
    case cJU_JPBRANCH_B:
    {
        Pjbb_t    Pjbb;
        Word_t    subexp;               // in bitmap, 0..7.
        BITMAPB_t BitMap;               // for one subexpanse.
        BITMAPB_t BitMask;              // bit in BitMap for Indexs Digit.
        Digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
// Common code for all BranchBs; come here with Digit set:
      JudyBranchB:
        Pjbb = P_JBB(Pjp->jp_Addr);
        subexp = Digit / cJU_BITSPERSUBEXPB;
        BitMap = JU_JBB_BITMAP(Pjbb, subexp);
        Pjp = P_JP(JU_JBB_PJP(Pjbb, subexp));
        BitMask = JU_BITPOSMASKB(Digit);
// No JP in subexpanse for Index => Index not found:
        if (!(BitMap & BitMask))
            break;
// Count JPs in the subexpanse below the one for Index:
        Pjp += j__udyCountBitsB(BitMap & (BitMask - 1));
        goto ContinueWalk;
    }                                   // case cJU_JPBRANCH_B*
// ****************************************************************************
// JPBRANCH_U*:
//
// Notice the reverse order of the cases, and falling through to the next case,
// for performance.
    case cJU_JPBRANCH_U:
        Pjp = JU_JBU_PJP(Pjp, Index, cJU_ROOTSTATE);
// If not a BranchU, traverse; otherwise fall into the next case, which makes
// this very fast code for a large Judy array (mainly BranchUs), especially
// when branches are already in the cache, such as for prev/next:
        if (JU_JPTYPE(Pjp) != cJU_JPBRANCH_U3)
            goto ContinueWalk;
    case cJU_JPBRANCH_U3:
        Pjp = JU_JBU_PJP(Pjp, Index, 3);
        if (JU_JPTYPE(Pjp) != cJU_JPBRANCH_U2)
            goto ContinueWalk;
        // and fall through.
    case cJU_JPBRANCH_U2:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            break;
        Pjp = JU_JBU_PJP(Pjp, Index, 2);
// Note:  BranchU2 is a special case that must continue traversal to a leaf,
// immed, full, or null type:
        goto ContinueWalk;
// ****************************************************************************
// JPLEAF*:
//
// Note:  Here the calls of JU_DCDNOTMATCHINDEX() are necessary and check
// whether Index is out of the expanse of a narrow pointer.
    case cJU_JPLEAF1:
    {
        int       posidx;               // signed offset in leaf.
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 1))
            break;
        Pop1 = JU_JPLEAF_POP0(Pjp) + 1;
        Pjll = P_JLL(Pjp->jp_Addr);
        if ((posidx = j__udySearchLeaf1(Pjll, Pop1, Index)) < 0)
            break;
#ifdef JUDY1
        return (1);
#else /* JUDYL */
        return ((PPvoid_t) (JL_LEAF1VALUEAREA(Pjll, Pop1) + posidx));
#endif /* JUDYL */
    }
    case cJU_JPLEAF2:
    {
        int       posidx;               // signed offset in leaf.
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            break;
        Pop1 = JU_JPLEAF_POP0(Pjp) + 1;
        Pjll = P_JLL(Pjp->jp_Addr);
        if ((posidx = j__udySearchLeaf2(Pjll, Pop1, Index)) < 0)
            break;
#ifdef JUDY1
        return (1);
#else /* JUDYL */
        return ((PPvoid_t) (JL_LEAF2VALUEAREA(Pjll, Pop1) + posidx));
#endif /* JUDYL */
    }
    case cJU_JPLEAF3:
    {
        int       posidx;               // signed offset in leaf.
        Pop1 = JU_JPLEAF_POP0(Pjp) + 1;
        Pjll = P_JLL(Pjp->jp_Addr);
        if ((posidx = j__udySearchLeaf3(Pjll, Pop1, Index)) < 0)
            break;
#ifdef JUDY1
        return (1);
#else /* JUDYL */
        return ((PPvoid_t) (JL_LEAF3VALUEAREA(Pjll, Pop1) + posidx));
#endif /* JUDYL */
    }
// ****************************************************************************
// JPLEAF_B1:
    case cJU_JPLEAF_B1:
    {
        Pjlb_t    Pjlb;
#ifdef JUDYL
        int       posidx;
        Word_t    subexp;               // in bitmap, 0..7.
        BITMAPL_t BitMap;               // for one subexpanse.
        BITMAPL_t BitMask;              // bit in BitMap for Indexs Digit.
        Pjv_t     Pjv;
#endif /* JUDYL */
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 1))
            break;
        Pjlb = P_JLB(Pjp->jp_Addr);
#ifdef  JUDY1
// Simply check if Indexs bit is set in the bitmap:
        if (JU_BITMAPTESTL(Pjlb, Index))
            return (1);
        break;

#else /* JUDYL */

// JudyL is much more complicated because of value area subarrays:
        Digit = JU_DIGITATSTATE(Index, 1);
        subexp = Digit / cJU_BITSPERSUBEXPL;
        BitMap = JU_JLB_BITMAP(Pjlb, subexp);
        BitMask = JU_BITPOSMASKL(Digit);
// No value in subexpanse for Index => Index not found:
        if (!(BitMap & BitMask))
            break;
// Count value areas in the subexpanse below the one for Index:
        Pjv = P_JV(JL_JLB_PVALUE(Pjlb, subexp));
        assert(Pjv != (Pjv_t) NULL);
        posidx = j__udyCountBitsL(BitMap & (BitMask - 1));
        return ((PPvoid_t) (Pjv + posidx));
#endif /* JUDYL */
    }                                   // case cJU_JPLEAF_B1
#ifdef  JUDY1
// ****************************************************************************
// JPFULLPOPU1:
//
// If the Index is in the expanse, it is necessarily valid (found).
    case cJ1_JPFULLPOPU1:
    {
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 1))
            break;
        return (1);
    }
#endif /* JUDY1 */
// ****************************************************************************
// JPIMMED*:
//
// Note that the contents of jp_DcdPopO are different for cJU_JPIMMED_*_01:
    case cJU_JPIMMED_1_01:
    case cJU_JPIMMED_2_01:
    case cJU_JPIMMED_3_01:
        if (JU_JPDCDPOP0(Pjp) != JU_TRIMTODCDSIZE(Index))
            break;
#ifdef  JUDY1
        return (1);
#else /* JUDYL */
        return ((PPvoid_t) & (Pjp->jp_Addr));
#endif /* JUDYL */

#ifdef  JUDY1
    case cJ1_JPIMMED_1_07:
        if (Pjp->jp_1Index1[7 - 1] == (uint8_t)Index) return (1);
    case cJ1_JPIMMED_1_06:
        if (Pjp->jp_1Index1[6 - 1] == (uint8_t)Index) return (1);
    case cJ1_JPIMMED_1_05:
        if (Pjp->jp_1Index1[5 - 1] == (uint8_t)Index) return (1);
    case cJ1_JPIMMED_1_04:
        if (Pjp->jp_1Index1[4 - 1] == (uint8_t)Index) return (1);
    case cJ1_JPIMMED_1_03:
        if (Pjp->jp_1Index1[3 - 1] == (uint8_t)Index) return (1);
    case cJ1_JPIMMED_1_02:
        if (Pjp->jp_1Index1[2 - 1] == (uint8_t)Index) return (1);
        if (Pjp->jp_1Index1[1 - 1] == (uint8_t)Index) return (1);
        break;
    case cJ1_JPIMMED_2_03:
        if (Pjp->jp_1Index2[3 - 1] == (uint16_t)Index) return (1);
    case cJ1_JPIMMED_2_02:
        if (Pjp->jp_1Index2[2 - 1] == (uint16_t)Index) return (1);
        if (Pjp->jp_1Index2[1 - 1] == (uint16_t)Index) return (1);
        break;
    case cJ1_JPIMMED_3_02:
    {
        Word_t    i_ndex;

        uint8_t  *a_ddr;

        a_ddr = Pjp->jp_1Index1 + ((2 - 1) * 3);
        JU_COPY3_PINDEX_TO_LONG(i_ndex, a_ddr);
        if (i_ndex == JU_LEASTBYTES(Index, 3)) return (1);
        a_ddr = Pjp->jp_1Index1 + ((1 - 1) * 3);
        JU_COPY3_PINDEX_TO_LONG(i_ndex, a_ddr);
        if (i_ndex == JU_LEASTBYTES(Index, 3)) return (1);
        break;
    }
#endif /* JUDY1 */

#ifdef  JUDYL
    case cJL_JPIMMED_1_03:
        if (Pjp->jp_LIndex1[3 - 1] == (uint8_t)Index)
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + 3 - 1));
    case cJL_JPIMMED_1_02:
        if (Pjp->jp_LIndex1[2 - 1] == (uint8_t)Index)
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + 2 - 1));

        if (Pjp->jp_LIndex1[1 - 1] == (uint8_t)Index)
            return ((PPvoid_t) (P_JV((Pjp)->jp_Addr) + 1 - 1));
        break;
#endif /* JUDYL */

// ****************************************************************************
// INVALID JP TYPE:
    default:
      ReturnCorrupt:
        JU_SET_ERRNO(PJError, JU_ERRNO_CORRUPT);
#ifdef JUDY1
        return (JERRI);
#else /* JUDYL */
        return (PPJERR);
#endif /* JUDYL */
    }                                   // switch on JP type

#ifdef JUDY1
    return (0);
#else /* JUDYL */
    return ((PPvoid_t) NULL);
#endif /* JUDYL */
}                                       // Judy1Test() / JudyLGet()
#endif  // JU_32BIT
