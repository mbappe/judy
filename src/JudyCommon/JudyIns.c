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

#ifdef JUDY1
extern Word_t j__udy1BranchBfromParts(Pjp_t, uint8_t *, Word_t, Pjpm_t);
extern int j__udy1CreateBranchU(Pjp_t, Pjpm_t);
extern int j__udy1Cascade2(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1Cascade3(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1Cascade4(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1Cascade5(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1Cascade6(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1Cascade7(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1Cascade8(Pjp_t, Word_t, Pjpm_t);
extern int j__udy1InsertBranchL(Pjp_t Pjp, Word_t Index, Word_t Dcd, Word_t Btype, Pjpm_t);
#else /* JUDYL */
extern Word_t j__udyLBranchBfromParts(Pjp_t, uint8_t *, Word_t, Pjpm_t);
extern int j__udyLCreateBranchU(Pjp_t, Pjpm_t);
extern int j__udyLCascade2(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLCascade3(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLCascade4(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLCascade5(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLCascade6(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLCascade7(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLCascade8(Pjp_t, Word_t, Pjpm_t);
extern int j__udyLInsertBranchL(Pjp_t Pjp, Word_t Index, Word_t Dcd, Word_t Btype, Pjpm_t);
#endif /* JUDYL */

//
//                  64 Bit JudyIns.c
//
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
              Word_t Index,             // to insert.
              Pjpm_t Pjpm)              // for returning info to top Level.
{
    uint8_t   digit;                    // from Index, current offset into a branch.
    jp_t      newJP;                    // for creating a new Immed JP.
    Word_t    exppop1;                  // expanse (leaf) population.
    int       retcode;                  // return codes:  -1, 0, 1.
    uint8_t   jpLevel = 0;
    Word_t    RawJpPntr;                // Raw pointer in most jp_ts

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
        return (1);
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
        goto JudyBranchL;
    }

    case cJU_JPBRANCH_L3:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 3))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 3, Pjpm));
        digit = JU_DIGITATSTATE(Index, 3);
        exppop1 = ju_BranchPop0(Pjp, 3);
        goto JudyBranchL;
    }

    case cJU_JPBRANCH_L4:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 4))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 4, Pjpm));
        digit = JU_DIGITATSTATE(Index, 4);
        exppop1 = ju_BranchPop0(Pjp, 4);
        goto JudyBranchL;
    } 

    case cJU_JPBRANCH_L5:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 5))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 5, Pjpm));
        digit = JU_DIGITATSTATE(Index, 5);
        exppop1 = ju_BranchPop0(Pjp, 5);
        goto JudyBranchL;
    }

    case cJU_JPBRANCH_L6:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 6))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 6, Pjpm));
        digit = JU_DIGITATSTATE(Index, 6);
        exppop1 = ju_BranchPop0(Pjp, 6);
        goto JudyBranchL;
    }

    case cJU_JPBRANCH_L7:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 7))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 7, Pjpm));
        digit = JU_DIGITATSTATE(Index, 7);
        exppop1 = ju_BranchPop0(Pjp, 7);
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
        // fall through:
// COMMON CODE FOR LINEAR BRANCHES:
//
// Come here with digit and exppop1 already set.
      JudyBranchL:
        PjblRaw = RawJpPntr;
        Pjbl_t  Pjbl = P_JBL(PjblRaw);
        Word_t  numJPs = Pjbl->jbl_NumJPs;

//      If population under this branch greater than:
//      Convert to BranchU
        if ((exppop1 > JU_BRANCHL_MAX_POP) && (numJPs != 1))
        {
// Memory efficiency is no object because the branch pop1 is large enough, so
// speed up array access.  Come here with PjblRaw set.  Note:  This is goto
// code because the previous block used to fall through into it as well, but no
// longer.
            jpLevel = ju_Type(Pjp) - cJU_JPBRANCH_L2 + 2;
#ifdef  PCAS
            printf("\nConvertBranchLtoU ju_BranchPop0 = %ld, NumJPs = %d, Level = %d\n", exppop1, Pjbl->jbl_NumJPs, jpLevel);
#endif  // PCAS

//         Allocate memory for an uncompressed branch:
            PjbuRaw = j__udyAllocJBU(Pjpm);         // pointer to new uncompressed branch.
            if (PjbuRaw == 0)
                return (-1);
            Pjbu_t  Pjbu = P_JBU(PjbuRaw);

//          Set the proper NULL type for most of the uncompressed branchs JPs:
            ju_SetIMM01(&newJP, 0, 0, cJU_JPNULL1 - 1 + jpLevel - 1);

//          Initialize:  Pre-set uncompressed branch to JPNULL*s:
            for (numJPs = 0; numJPs < cJU_BRANCHUNUMJPS; ++numJPs)
                Pjbu->jbu_jp[numJPs] = newJP;
       
//          Copy JPs from linear branch to uncompressed branch:
            for (numJPs = 0; numJPs < Pjbl->jbl_NumJPs; numJPs++)
            {
                Pjp_t       Pjp1 = Pjbl->jbl_jp + numJPs;       // next Pjp
                int exp   = Pjbl->jbl_Expanse[numJPs];          // in jp_t ?
                Pjbu->jbu_jp[exp] = *Pjp1;
            }

//          Copy jp count to BranchU
            Pjbu->jbu_NumJPs = Pjbl->jbl_NumJPs;

            j__udyFreeJBL(PjblRaw, Pjpm);   // free old BranchL.
//          Plug new values into parent JP:
            ju_SetPntrInJp(Pjp, PjbuRaw);

//          Convert from BranchL to BranchU at the same level
            jpLevel = ju_Type(Pjp) - cJU_JPBRANCH_L2 + 2;
            ju_SetJpType(Pjp, cJU_JPBRANCH_U2 - 2 + jpLevel);
            goto ContinueInsWalk;           // now insert Index into BRANCH_U
        }
//      else
//      Search for a match to the digit:
        int offset = j__udySearchBranchL(Pjbl->jbl_Expanse, numJPs, digit);

//      If Index is found, offset is into an array of 1..cJU_BRANCHLMAXJPS JPs:
        if (offset >= 0)
        {
//          save Parent if special kind of Pjp
            Pjp_t PPjp = Pjp;
            Pjp  = Pjbl->jbl_jp + offset;       // address of next JP.

            if (numJPs == 1)
            {
                if (Pjp->jp_Type <= cJU_JPMAXBRANCH)        // is a Branch
                {
                    Word_t RawPntr = ju_PntrInJp(PPjp);
                    *PPjp = *Pjp;               // copy it higher in Tree
                    j__udyFreeJBL(RawPntr, Pjpm);   // free old BranchL.
                    Pjp = PPjp;
                }
            }
            break;                      // continue walk.
        }

//      Expanse is missing in BranchL for the passed Index, so insert an Immed
        if (numJPs < cJU_BRANCHLMAXJPS) // -- if theres room:
        {
            offset = ~offset;           // insertion offset.
            jpLevel = ju_Type(Pjp) - cJU_JPBRANCH_L2 + 2;
//          Value = 0 in both Judy1 and JudyL
            ju_SetIMM01(&newJP, 0, Index, cJU_JPIMMED_1_01 - 1 + jpLevel - 1);
            JU_INSERTINPLACE(Pjbl->jbl_Expanse, numJPs, offset, digit);
            JU_INSERTINPLACE(Pjbl->jbl_jp, numJPs, offset, newJP);
            Pjbl->jbl_NumJPs++;
#ifdef JUDYL
//          Value area is in the new Immed 01 JP:
            Pjp = Pjbl->jbl_jp + offset;
            Pjpm->jpm_PValue =  ju_PImmVal_01(Pjp);     // ^ to Immed_01 Value
#endif /* JUDYL */
            return (1);                                 // done with insert
        }
//      else 
//      MAXED OUT LINEAR BRANCH, CONVERT TO A BITMAP BRANCH, THEN INSERT:

//      Copy the linear branch to a bitmap branch.
        assert((numJPs) <= cJU_BRANCHLMAXJPS);
        Word_t PjbbRaw = j__udyBranchBfromParts(Pjbl->jbl_jp, Pjbl->jbl_Expanse, numJPs, Pjpm);
        if (PjbbRaw == 0)
            return (-1);
        j__udyFreeJBL(PjblRaw, Pjpm);   // free old BranchL.
//      Convert jp_Type from linear branch to equivalent bitmap branch:
        ju_SetJpType(Pjp, ju_Type(Pjp) + cJU_JPBRANCH_B2 - cJU_JPBRANCH_L2);
        ju_SetPntrInJp(Pjp, PjbbRaw);
//      Having changed branch types, now do the insert in the new BranchB:
        goto ContinueInsWalk;
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

#ifdef noB
//           Convert branchB to branchU
             if (j__udyCreateBranchU(Pjp, Pjpm) == -1) return(-1);
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
                        if (j__udyCreateBranchU(Pjp, Pjpm) == -1) 
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
        Pjbb = P_JBB(PjbbRaw);
// Form the Int32 offset, and Bit offset values:
//
// 8 bit Decode | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//              |SubExpanse |    Bit offset     |
//
// Get the 1 of 8 expanses from digit, Bits 5..7 = 1 of 8, and get the 32-bit
// word that may have a bit set:
        sub4exp = digit / cJU_BITSPERSUBEXPB;
        bitmap = JU_JBB_BITMAP(Pjbb, sub4exp);
        Pjp2Raw = JU_JBB_PJP(Pjbb, sub4exp);
        Pjp2 = P_JP(Pjp2Raw);
// Get the bit position that represents the desired expanse, and get the offset
// into the array of JPs for the JP that matches the bit.
        bitmask = JU_BITPOSMASKB(digit);
        offset = j__udyCountBitsB(bitmap & (bitmask - 1));
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
        return (1);
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
        if (ju_DcdNotMatchKey(Index, Pjp, 2))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 2, Pjpm));

        Pjbu_t    P_jbu = P_JBU(RawJpPntr);
        Pjp     = P_jbu->jbu_jp + JU_DIGITATSTATE(Index, 2);
        break;
    }

    case cJU_JPBRANCH_U3:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 3))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 3, Pjpm));

        Pjbu_t    P_jbu = P_JBU(RawJpPntr);
        Pjp     = P_jbu->jbu_jp + JU_DIGITATSTATE(Index, 3);
        break;
    }

    case cJU_JPBRANCH_U4:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 4))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 4, Pjpm));

        Pjbu_t    P_jbu = P_JBU(RawJpPntr);
        Pjp     = P_jbu->jbu_jp + JU_DIGITATSTATE(Index, 4);
        break;
    }

    case cJU_JPBRANCH_U5:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 5))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 5, Pjpm));

        Pjbu_t    P_jbu = P_JBU(RawJpPntr);
        Pjp     = P_jbu->jbu_jp + JU_DIGITATSTATE(Index, 5);
        break;
    }

    case cJU_JPBRANCH_U6:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 6))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 6, Pjpm));

        Pjbu_t    P_jbu = P_JBU(RawJpPntr);
        Pjp     = P_jbu->jbu_jp + JU_DIGITATSTATE(Index, 6);
        break;
    }

    case cJU_JPBRANCH_U7:
    {
        if (ju_DcdNotMatchKey(Index, Pjp, 7))
            return (j__udyInsertBranchL(Pjp, Index, ju_DcdPop0(Pjp), 7, Pjpm));

        Pjbu_t    P_jbu = P_JBU(RawJpPntr);
        Pjp     = P_jbu->jbu_jp + JU_DIGITATSTATE(Index, 7);
        break;
    }

    case cJU_JPBRANCH_U8:
    {
        Pjbu_t    P_jbu = P_JBU(RawJpPntr);
        Pjp     = P_jbu->jbu_jp + JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
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
            return (0);
        }
        offset = ~offset;

        if (JU_LEAF1GROWINPLACE(exppop1))       /* insert into current leaf? */
        {
            JU_INSERTINPLACE(Pjll1->jl1_Leaf, exppop1, offset, index8);
            JU_PADLEAF1(Pjll1->jl1_Leaf, exppop1 + 1);
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);

            Pjpm->jpm_PValue = Pjv + offset;
            ju_SetLeafPop1(Pjp, exppop1 + 1);               // increase Population0 by one
            Pjp->jp_subLeafPops += ((Word_t)1) << ((index8 >> (8 - 3)) * 8);    // and sum-exp
            return (1);
        }
        if (exppop1 < cJU_LEAF1_MAXPOP1)      /* grow to new leaf */
        {
            Word_t Pjll1newRaw = j__udyAllocJLL1(exppop1 + 1, Pjpm);
            if (Pjll1newRaw == 0)
                return (-1);
            Pjll1_t  Pjll1new = P_JLL1(Pjll1newRaw);

//          Put the LastKey in the Leaf1
            Pjll1new->jl1_LastKey = (Index & cJU_DCDMASK(1)); // + exppop1 + 0;
            Pjv_t Pjvnew = JL_LEAF1VALUEAREA(Pjll1new, exppop1 + 1);
            JU_INSERTCOPY(Pjll1new->jl1_Leaf, Pjll1->jl1_Leaf, exppop1, offset, index8);
            JU_PADLEAF1(  Pjll1new->jl1_Leaf, exppop1 + 1);     // only needed for Parallel search
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);

            j__udyFreeJLL1(Pjll1Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll1newRaw);

            Pjpm->jpm_PValue = Pjvnew + offset;
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            Pjp->jp_subLeafPops += ((Word_t)1) << ((index8 >> (8 - 3)) * 8);    // and sum-exp
            return (1);
        }
//      Max population, convert to LeafB1
        assert(exppop1 == (cJU_LEAF1_MAXPOP1));

#ifdef  PCAS
        printf("\n====JudyIns======= Leaf1 to LeafB1, Pop1 = %d\n", (int)cJU_LEAF1_MAXPOP1 + 1);
#endif  // PCAS

	Word_t PjlbRaw = j__udyAllocJLB1(Pjpm);
	if (PjlbRaw == 0)
            return(-1);

	Pjlb_t Pjlb  = P_JLB1(PjlbRaw);

//      This area for DCD with uncompressed Value area -- LeafB1
//      NOTE:  this is distructive of jp_subLeafPops!!!
        ju_SetDcdPop0(Pjp, Pjll1->jl1_LastKey); // + cJU_LEAF1_MAXPOP1 - 1;

//	Build all 4 bitmaps from 1 byte index Leaf1 -- slow?
	for (int off = 0; off < cJU_LEAF1_MAXPOP1; off++)
            JU_BITMAPSETL(Pjlb, Pjll1->jl1_Leaf[off]);

        Pjv_t Pjvnew = JL_JLB_PVALUE(Pjlb);

//      Copy Values to B1 in uncompressed style
	for (int loff = 0; loff < cJU_LEAF1_MAXPOP1; loff++)
            Pjvnew[Pjll1->jl1_Leaf[loff]] = Pjv[loff];

        j__udyFreeJLL1(Pjll1Raw, cJU_LEAF1_MAXPOP1, Pjpm);
//        Word_t DcdP0 = (Index & cJU_DCDMASK(1)) + cJU_LEAF1_MAXPOP1 - 1;
//        ju_SetDcdPop0(Pjp, DcdP0);  Done above
        ju_SetJpType  (Pjp, cJL_JPLEAF_B1);
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
            return (0);
        }
        offset = ~offset;

//      bump sub-expanse pop from high 3 bits in Key -- imminent growth
        Pjp->jp_subLeafPops += ((Word_t)1) << ((index16 >> (16 - 3)) * 8);


//printf("\nIn ins Leaf2 subLeafPops = 0x%016lx, Key = 0x%04x\n", Pjp->jp_subLeafPops, index16);

        if (JU_LEAF2GROWINPLACE(exppop1))       // add to current leaf
        {
            JU_INSERTINPLACE(Pjll2->jl2_Leaf, exppop1, offset, index16);
            JU_PADLEAF2(Pjll2->jl2_Leaf, exppop1 + 1);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            return (1);
        }
        if (exppop1 < cJU_LEAF2_MAXPOP1)        // grow to new leaf
        {
            Word_t Pjll2newRaw = j__udyAllocJLL2(exppop1 + 1, Pjpm);
            if (Pjll2newRaw == 0)
                return (-1);
            Pjll2_t Pjll2new = P_JLL2(Pjll2newRaw);

//          Put the Dcd in the Leaf2
            Pjll2new->jl2_LastKey = Index & cJU_DCDMASK(2);

            JU_INSERTCOPY(Pjll2new->jl2_Leaf, Pjll2->jl2_Leaf, exppop1, offset, Index);
            JU_PADLEAF2(Pjll2new->jl2_Leaf, exppop1 + 1);
#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF2VALUEAREA(Pjll2new, exppop1 + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL2(Pjll2Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll2newRaw);
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            return (1);
        }
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
            return (0);
        }
        offset = ~offset;
        if (JU_LEAF3GROWINPLACE(exppop1))       // add to current leaf
        {
            JU_INSERTINPLACE3(Pjll3->jl3_Leaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            ju_SetLeafPop1(Pjp, exppop1 + 1);       // increase Population0 by one
            return (1);
        }
        if (exppop1 < cJU_LEAF3_MAXPOP1)        // grow to new leaf
        {
            Word_t Pjll3newRaw = j__udyAllocJLL3(exppop1 + 1, Pjpm);
            if (Pjll3newRaw == 0)
                return (-1);
            Pjll3_t Pjll3new = P_JLL3(Pjll3newRaw);

//          Put the LastKey in the Leaf3
            Pjll3new->jl3_LastKey = Index & cJU_DCDMASK(3);

            JU_INSERTCOPY3(Pjll3new->jl3_Leaf, Pjll3->jl3_Leaf, exppop1, offset, Index);
#ifdef JUDYL
            Pjv_t Pjvnew = JL_LEAF3VALUEAREA(Pjll3new, exppop1 + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL3(Pjll3Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll3newRaw);
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            return (1);
        }
        assert(exppop1 == (cJU_LEAF3_MAXPOP1));
        if (j__udyCascade3(Pjp, Index, Pjpm) == -1)
            return (-1);
//      Now Pjp->jp_Type is a BranchLBorU
        j__udyFreeJLL3(Pjll3Raw, cJU_LEAF3_MAXPOP1, Pjpm);
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
            return (0);
        }
        offset = ~offset;
        if (JU_LEAF4GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE(Pjll4->jl4_Leaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            return (1);
        }
        if (exppop1 < cJU_LEAF4_MAXPOP1)      /* grow to new leaf */
        {
            Word_t Pjll4newRaw = j__udyAllocJLL4(exppop1 + 1, Pjpm);
            if (Pjll4newRaw == 0)
                return (-1);
            Pjll4_t Pjll4new = P_JLL4(Pjll4newRaw);

//          Put the LastKey in the Leaf4
            Pjll4new->jl4_LastKey = Index & cJU_DCDMASK(4);

            JU_INSERTCOPY(Pjll4new->jl4_Leaf, Pjll4->jl4_Leaf, exppop1, offset, Index);
#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF4VALUEAREA(Pjll4new, exppop1 + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL4(Pjll4Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll4newRaw);
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            return (1);
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
            return (0);
        }
        offset = ~offset;
        if (JU_LEAF5GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE5(Pjll5->jl5_Leaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            return (1);
        }
        if (exppop1 < cJU_LEAF5_MAXPOP1)      /* grow to new leaf */
        {
            Word_t Pjll5newRaw = j__udyAllocJLL5(exppop1 + 1, Pjpm);
            if (Pjll5newRaw == 0)
                return (-1);
            Pjll5_t Pjll5new = P_JLL5(Pjll5newRaw);

//          Put the LastKey in the Leaf5
            Pjll5new->jl5_LastKey = Index & cJU_DCDMASK(5);

            JU_INSERTCOPY5(Pjll5new->jl5_Leaf, Pjll5->jl5_Leaf, exppop1, offset, Index);
#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF5VALUEAREA(Pjll5new, (exppop1) + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL5(Pjll5Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll5newRaw);
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            return (1);
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
            return (0);
        }
        offset = ~offset;
        if (JU_LEAF6GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE6(Pjll6->jl6_Leaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            return (1);
        }
        if (exppop1 < cJU_LEAF6_MAXPOP1)      /* grow to new leaf */
        {
            Word_t Pjll6newRaw = j__udyAllocJLL6(exppop1 + 1, Pjpm);
            if (Pjll6newRaw == 0)
                return (-1);
            Pjll6_t Pjll6new = P_JLL6(Pjll6newRaw);

//          Put the LastKey in the Leaf6
            Pjll6new->jl6_LastKey = Index & cJU_DCDMASK(6);

            JU_INSERTCOPY6(Pjll6new->jl6_Leaf, Pjll6->jl6_Leaf, exppop1, offset, Index);
#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF6VALUEAREA(Pjll6new, (exppop1) + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL6(Pjll6Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll6newRaw);
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one
            return (1);
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
            return (0);
        }
        offset = ~offset;
        if (JU_LEAF7GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE7(Pjll7->jl7_Leaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif  // JUDYL
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one

//for (int ii = 0; ii < (exppop1 + 1); ii++)
//{
//    Word_t Key;
//    JU_COPY7_PINDEX_TO_LONG(Key, Pjll7->jl7_Leaf + (ii * 7));
//    printf("%3d = 0x%016lx\n", ii, Key);
//}


            return (1);
        }
        if (exppop1 < (cJU_LEAF7_MAXPOP1))      /* grow to new leaf */
        {
            Word_t    Pjll7newRaw = j__udyAllocJLL7(exppop1 + 1, Pjpm);
            if (Pjll7newRaw == 0)
                return (-1);
            Pjll7_t Pjll7new = P_JLL7(Pjll7newRaw);

//          Put the LastKey in the Leaf6
            Pjll7new->jl7_LastKey = Index & cJU_DCDMASK(7);

            JU_INSERTCOPY7(Pjll7new->jl7_Leaf, Pjll7->jl7_Leaf, exppop1, offset, Index);
#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF7VALUEAREA(Pjll7new, (exppop1) + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif  // JUDYL
            j__udyFreeJLL7(Pjll7Raw, exppop1, Pjpm);
            ju_SetPntrInJp(Pjp, Pjll7newRaw);
            ju_SetLeafPop1(Pjp, exppop1 + 1);   // increase Population0 by one

//for (int ii = 0; ii < (exppop1 + 1); ii++)
//{
//    Word_t Key;
//    JU_COPY7_PINDEX_TO_LONG(Key, Pjll7->jl7_Leaf + (ii * 7));
//    printf("%3d = 0x%016lx\n", ii, Key);
//}

            return (1);
        }
        assert(exppop1 == cJU_LEAF7_MAXPOP1);
        if (j__udyCascade7(Pjp, Index, Pjpm) == -1)
            return (-1);
//      Now Pjp->jp_Type is a BranchLorB

        j__udyFreeJLL7(Pjll7Raw, cJU_LEAF7_MAXPOP1, Pjpm);
        goto ContinueInsWalk;   // go insert into Leaf6
    }

// ****************************************************************************
// JPLEAF_B1:
//
// 8 bit Decode | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//              |SubExpanse |    Bit offset     |
//
// Note:  For JudyL, values are stored in 4 subexpanses, each a linear word
// array of up to 32[64] values each.

#ifdef  JUDYL
    case cJL_JPLEAF_B1:
    {
//      Get last byte to decode from Index, and pointer to bitmap leaf:
        int       key     = JU_DIGITATSTATE(Index, 1);
        Word_t    sube    = key / cJU_BITSPERSUBEXPL;   // 0..3 which subexpanse.
        BITMAPL_t bitmask = JU_BITPOSMASKL(key);        // mask for Index.
        Pjlb_t    Pjlb    = P_JLB1(RawJpPntr);          // ^ to bitmap leaf
        BITMAPL_t bitmap  = JU_JLB_BITMAP(Pjlb, sube);  // 64 bit map.
        Pjv_t     Pjv     = JL_JLB_PVALUE(Pjlb);        // start of Value area

//      If Index already exists, value pointer and exit:
        Pjpm->jpm_PValue  = Pjv + key;                  // Value location
        if (bitmap & bitmask)
            return(0);

        Pjv[key] = 0;                                   // Value
        JU_JLB_BITMAP(Pjlb, sube) |= bitmask;           // set Key bit.
        exppop1 = ju_LeafPop1(Pjp) + 1;

        ju_SetLeafPop1(Pjp, exppop1);                   // increase Pop by one
        return(1);
    }
#endif /* JUDYL */

#ifdef JUDY1
    case cJ1_JPLEAF_B1:
    {
        Pjlb_t    Pjlb    = P_JLB1(RawJpPntr);          // ^ to bitmap leaf

        if (JU_BITMAPTESTL(Pjlb, Index)) 
            return(0);                                  // duplicate Key

        exppop1 = ju_LeafPop1(Pjp) + 1;                 // bump population
        ju_SetLeafPop1(Pjp, exppop1);                   // set

//      If bitmap is not full, set the new Indexs bit; otherwise convert to a Full:
        if (exppop1 == 256)                             // full
        {
            j__udyFreeJLB1(RawJpPntr, Pjpm);            // free LeafB1.
            ju_SetJpType(Pjp, cJ1_JPFULLPOPU1);         // mark at full population
            ju_SetPntrInJp(Pjp, 0);
            ju_SetDcdPop0(Pjp, Index & cJU_DCDMASK(1));
        }
        else
        {
            JU_BITMAPSETL(Pjlb, Index);
        }
        return(1);
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
        return (1);
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
        return (1);
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
        uint8_t  *PImmed3 = ju_PImmed3(Pjp);
        if (oldIndex < newIndex)           // sort
        {
            JU_COPY3_LONG_TO_PINDEX(PImmed3 + 0, oldIndex);
            JU_COPY3_LONG_TO_PINDEX(PImmed3 + 3, newIndex);
#ifdef JUDYL
            Pjv[0] = oldValue;
            Pjv[1] = 0;
            Pjpm->jpm_PValue = Pjv + 1; // return ^ to new Value
#endif /* JUDYL */
        }
        else
        {
            JU_COPY3_LONG_TO_PINDEX(PImmed3 + 0, newIndex);
            JU_COPY3_LONG_TO_PINDEX(PImmed3 + 3, oldIndex);
#ifdef JUDYL
            Pjv[0] = 0;
            Pjpm->jpm_PValue = Pjv + 0;
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
        ju_SetJpType(Pjp, cJU_JPIMMED_3_02);
        return (1);
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
#ifdef JUDYL
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);      // ^ to Immed_x_01 Value
#endif  // JUDYL
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
        return (1);
    }

    case cJU_JPIMMED_5_01:
    {
        Word_t  oldIndex = ju_IMM01Key(Pjp);
        Word_t  newIndex = JU_TrimToIMM01(Index);

        if (oldIndex == newIndex)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);
#endif  // JUDYL
            return (0);
        }
#ifdef JUDYL
        Word_t  Pjll5Raw = j__udyAllocJLL5(2, Pjpm);
        if (Pjll5Raw  == 0)
            return (-1);
        Pjll5_t Pjll5    = P_JLL5(Pjll5Raw);

//      Put the LastKey in the Leaf5
        Pjll5->jl5_LastKey = Index & cJU_DCDMASK(5);

        uint8_t *PImmed5 = Pjll5->jl5_Leaf;     // bad name
        Pjv_t    Pjv     = JL_LEAF5VALUEAREA(Pjll5, 2);
        Word_t  oldValue = ju_ImmVal_01(Pjp);
#else   //JUDY1
        uint8_t *PImmed5 = ju_PImmed5(Pjp);     // good name
#endif  // JUDY1
        if (oldIndex < newIndex)
        {
            JU_COPY5_LONG_TO_PINDEX(PImmed5 + 0, oldIndex);
            JU_COPY5_LONG_TO_PINDEX(PImmed5 + 5, newIndex);
#ifdef JUDYL
            Pjv[0] = oldValue;
            Pjv[1] = 0;
            Pjpm->jpm_PValue = Pjv + 1;
#endif /* JUDYL */
        }
        else
        {
            JU_COPY5_LONG_TO_PINDEX(PImmed5 + 0, newIndex);
            JU_COPY5_LONG_TO_PINDEX(PImmed5 + 5, oldIndex);
#ifdef JUDYL
            Pjv[0] = 0;
            Pjv[1] = oldValue;
            Pjpm->jpm_PValue = Pjv + 0;
#endif /* JUDYL */
        }
#ifdef JUDYL
        ju_SetLeafPop1(Pjp, 2);
        ju_SetJpType(Pjp, cJU_JPLEAF5);
        ju_SetPntrInJp(Pjp, Pjll5Raw);
#else /* JUDY1 */
        ju_SetJpType(Pjp, cJ1_JPIMMED_5_02);
#endif /* JUDY1 */
        return (1);
    }

    case cJU_JPIMMED_6_01:
    {
        Word_t  oldIndex = ju_IMM01Key(Pjp);
        Word_t  newIndex = JU_TrimToIMM01(Index);

        if (oldIndex == newIndex)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);   // ^ to Immed_x_01 Value
#endif  // JUDYL
            return (0);
        }
#ifdef JUDYL
        Word_t Pjll6Raw = j__udyAllocJLL6(2, Pjpm);
        if (Pjll6Raw == 0)
            return (-1);
        Pjll6_t Pjll6 = P_JLL6(Pjll6Raw);

//      Put the LastKey in the Leaf6
        Pjll6->jl6_LastKey = Index & cJU_DCDMASK(6);

        uint8_t *PImmed6 = Pjll6->jl6_Leaf;     // bad name

        Pjv_t   Pjv   = JL_LEAF6VALUEAREA(Pjll6, 2);
        Word_t  oldValue = ju_ImmVal_01(Pjp);
#else   // JUDY1
        uint8_t *PImmed6 = ju_PImmed6(Pjp);     // good name
#endif  // JUDY1

        if (oldIndex < newIndex)
        {
            JU_COPY6_LONG_TO_PINDEX(PImmed6 + 0, oldIndex);
            JU_COPY6_LONG_TO_PINDEX(PImmed6 + 6, newIndex);
#ifdef JUDYL
            Pjv[0] = oldValue;
            Pjv[1] = 0;
            Pjpm->jpm_PValue = Pjv + 1;
#endif /* JUDYL */
        }
        else
        {
            JU_COPY6_LONG_TO_PINDEX(PImmed6 + 0, newIndex);
            JU_COPY6_LONG_TO_PINDEX(PImmed6 + 6, oldIndex);
#ifdef JUDYL
            Pjv[0] = 0;
            Pjpm->jpm_PValue = Pjv + 0;
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
#ifdef JUDYL
        ju_SetLeafPop1(Pjp, 2);         // pop1 == 2
        ju_SetJpType  (Pjp, cJU_JPLEAF6);
        ju_SetPntrInJp(Pjp, Pjll6Raw);
#else /* JUDY1 */
        ju_SetJpType(Pjp, cJ1_JPIMMED_6_02);
#endif /* JUDY1 */
        return (1);
    }

    case cJU_JPIMMED_7_01:
    {
        Word_t  oldIndex = ju_IMM01Key(Pjp);
        Word_t  newIndex = JU_TrimToIMM01(Index);

        if (oldIndex == newIndex)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);   // ^ to Immed_x_01 Value
#endif  // JUDYL
            return (0);
        }
#ifdef JUDYL
        Word_t  Pjll7Raw = j__udyAllocJLL7(2, Pjpm);
        if (Pjll7Raw == 0)
            return (-1);
        Pjll7_t Pjll7  = P_JLL7(Pjll7Raw);

//      Put the LastKey in the Leaf7
        Pjll7->jl7_LastKey = Index & cJU_DCDMASK(7);

        uint8_t *PImmed7  = Pjll7->jl7_Leaf;    // bad name
        Pjv_t    Pjv      = JL_LEAF7VALUEAREA(Pjll7, 2);
        Word_t   oldValue = ju_ImmVal_01(Pjp);
#else   // JUDY1
        uint8_t *PImmed7 = ju_PImmed7(Pjp);       // good name
#endif  // JUDY1
        if (oldIndex < newIndex)
        {
            JU_COPY7_LONG_TO_PINDEX(PImmed7 + 0, oldIndex);
            JU_COPY7_LONG_TO_PINDEX(PImmed7 + 7, newIndex);
#ifdef JUDYL
            Pjv[0] = oldValue;
            Pjv[1] = 0;
            Pjpm->jpm_PValue = Pjv + 1;
#endif /* JUDYL */
        }
        else
        {
            JU_COPY7_LONG_TO_PINDEX(PImmed7 + 0, newIndex);
            JU_COPY7_LONG_TO_PINDEX(PImmed7 + 7, oldIndex);
#ifdef JUDYL
            Pjv[0] = 0;
            Pjpm->jpm_PValue = Pjv + 0;
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
#ifdef JUDYL
        ju_SetLeafPop1(Pjp, 2);
        ju_SetJpType  (Pjp, cJU_JPLEAF7);
        ju_SetPntrInJp(Pjp, Pjll7Raw);
#else /* JUDY1 */
        ju_SetJpType(Pjp, cJ1_JPIMMED_7_02);
#endif /* JUDY1 */
        return (1);
    }

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
        return (1);
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
        Word_t PjvRaw = RawJpPntr;              // Value area
        Pjv_t  Pjv    = P_JV(PjvRaw);
        Pjpm->jpm_PValue = Pjv + offset;
#endif  // JUDYL

        if (offset >= 0)
            return (0);         // Duplicate

#ifdef  JUDY1
//      Produce a LeafB1
	Word_t PjlbRaw = j__udyAllocJLB1(Pjpm);
	if (PjlbRaw == 0)
            return(-1);
	Pjlb_t Pjlb    = P_JLB1(PjlbRaw);

//	Copy 1 byte index Leaf to bitmap Leaf
	for (int off = 0; off < cJU_IMMED1_MAXPOP1; off++)
            JU_BITMAPSETL(Pjlb, PImmed1[off]);

        ju_SetDcdPop0(Pjp, Index & cJU_DCDMASK(1));     // NOTE: no Pop0
        ju_SetPntrInJp(Pjp, PjlbRaw);
        ju_SetJpType(Pjp, cJU_JPLEAF_B1);
#endif  // JUDY1

#ifdef  JUDYL
//      Produce Leaf1 from IMMED_1_08 JudyL=8
        Word_t Pjll1newRaw = j__udyAllocJLL1(cJU_IMMED1_MAXPOP1, Pjpm);
        if (Pjll1newRaw == 0)
            return (-1);
        Pjll1_t Pjll1new = P_JLL1(Pjll1newRaw);

//      Put the LastKey in the Leaf1
        Pjll1new->jl1_LastKey = Index & cJU_DCDMASK(1);

        JU_COPYMEM(Pjll1new->jl1_Leaf, PImmed1, cJU_IMMED1_MAXPOP1);
//        JU_PADLEAF1(Pjll1new->jl1_Leaf, cJU_IMMED1_MAXPOP1); we havent done the Insert yet

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
        return (1);
    }

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
            return (0);
        }
        offset = ~offset;
        Word_t Pjll2newRaw = j__udyAllocJLL2(exppop1 + 1, Pjpm);
        if (Pjll2newRaw == 0)
            return (-1);
        Pjll2_t Pjll2new = P_JLL2(Pjll2newRaw);

//      Put the Dcd in the Leaf2
        Pjll2new->jl2_LastKey = Index & cJU_DCDMASK(2);

        JU_INSERTCOPY(Pjll2new->jl2_Leaf, PImmed2, exppop1, offset, Index);
        JU_PADLEAF2(Pjll2new->jl2_Leaf, exppop1 + 1);

//      fill sub-expanse counts to the new Leaf1, NOT old PImmed2 (overwrite!)
        Word_t Counts = 0;
        for (int loff = 0; loff < (exppop1 + 1); loff++) 
        {       // bump the sub-expanse determined by hi 3 bits of Key
            uint16_t  subexp = Pjll2new->jl2_Leaf[loff] >> (16 - 3);
            Counts += ((Word_t)1) << (subexp * 8);      // add 1 to sub-expanse
        }
        Pjp->jp_subLeafPops = Counts;      // place all 8 SubExps populations
#ifdef JUDYL
        Pjv_t   Pjvnew = JL_LEAF2VALUEAREA(Pjll2new, exppop1 + 1);
        JU_INSERTCOPY(Pjvnew, PjvOld, exppop1, offset, 0);
        j__udyLFreeJV(PjvOldRaw, /* 4 */ exppop1, Pjpm);
        Pjpm->jpm_PValue = Pjvnew + offset;
#endif  // JUDYL

// not nec?        Word_t D_P0 = (Index & cJU_DCDMASK(2)) + exppop1 - 1;
//        JU_JPSETADT(Pjp, PjllRaw, D_P0, cJU_JPLEAF2);
// not nec?        ju_SetDcdPop0(Pjp, D_P0);
        ju_SetLeafPop1(Pjp, exppop1 + 1);
        ju_SetJpType(Pjp, cJU_JPLEAF2);
        ju_SetPntrInJp(Pjp, Pjll2newRaw);      // Combine these 3 !!!!!!!!!!!!!!
        return (1);
    }

// (3_01 => [ 3_02 => [ 3_03..05 => ]] LeafL)
#ifdef JUDY1
    case cJ1_JPIMMED_3_02:
    case cJ1_JPIMMED_3_03:
    case cJ1_JPIMMED_3_04:
    {   // increase Pop of IMMED_3
        uint8_t *PImmed3 = ju_PImmed3(Pjp);

        exppop1 = ju_Type(Pjp) - (cJU_JPIMMED_3_02) + 2;
        int offset = j__udySearchImmed3(PImmed3, exppop1, Index, 3 * 8);
        if (offset >= 0)
            return (0);
        offset = ~offset;
        JU_INSERTINPLACE3(PImmed3, exppop1, offset, Index);
        ju_SetJpType(Pjp, ju_Type(Pjp) + 1);
        return (1);
    }

    case cJ1_JPIMMED_3_05:
#endif  //JUDY1

#ifdef JUDYL
    case cJL_JPIMMED_3_02:
#endif  // JUDYL
    {   // Convert IMMED to Leaf3
        uint8_t *PImmed3 = ju_PImmed3(Pjp);

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
            return (0);
        }
        offset = ~offset;
        Word_t Pjll3newRaw = j__udyAllocJLL3(exppop1 + 1, Pjpm);
        if (Pjll3newRaw == 0)
            return (-1);
        Pjll3_t Pjll3new = P_JLL3(Pjll3newRaw);

//      Put the LastKey in the Leaf3
        Pjll3new->jl3_LastKey = Index & cJU_DCDMASK(3);

        JU_INSERTCOPY3(Pjll3new->jl3_Leaf, PImmed3, exppop1, offset, Index);
#ifdef JUDYL
        Pjv_t Pjvnew = JL_LEAF3VALUEAREA(Pjll3newRaw, exppop1 + 1);
        JU_INSERTCOPY(Pjvnew, PjvOld, exppop1, offset, 0);
        j__udyLFreeJV(PjvOldRaw, exppop1, Pjpm);
        Pjpm->jpm_PValue = Pjvnew + offset;
#endif  // JUDYL
// not nec?        Word_t  D_P0 = (Index & cJU_DCDMASK(3)) + exppop1 - 1;
//        JU_JPSETADT(Pjp, PjllRaw, D_P0, cJU_JPLEAF3);
        ju_SetPntrInJp(Pjp, Pjll3newRaw);      // Combine all 3?
// not nec?        ju_SetDcdPop0(Pjp, D_P0);
        ju_SetLeafPop1(Pjp, exppop1 + 1);  // New Leaf
        ju_SetJpType(Pjp, cJU_JPLEAF3);
        return (1);
    }

// (4_01 => [[ 4_02..03 => ]] LeafL)
    case cJU_JPIMMED_4_02:
    {
        uint32_t *PImmed4 = ju_PImmed4(Pjp);
        int       offset  = j__udySearchImmed4(PImmed4, 2, Index, 4 * 8);
#ifdef  JUDYL
        Word_t PjvRaw     = RawJpPntr;
        Pjv_t  Pjv        = P_JV(PjvRaw);               // corresponding values.
#endif  // JUDYL
        if (offset >= 0)
        {
#ifdef  JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif  // JUDYL
            return (0);
        }
        offset = ~offset;
#ifdef JUDYL
        Word_t Pjll4newRaw = j__udyAllocJLL4(2 + 1, Pjpm);
        if (Pjll4newRaw == 0)
            return (-1);
        Pjll4_t Pjll4new = P_JLL4(Pjll4newRaw);

//      Put the LastKey in the Leaf4
        Pjll4new->jl4_LastKey = Index & cJU_DCDMASK(4);

        JU_INSERTCOPY(Pjll4new->jl4_Leaf, PImmed4, 2, offset, Index);
        Pjv_t Pjvnew = JL_LEAF4VALUEAREA(Pjll4new, 2 + 1);
        JU_INSERTCOPY(Pjvnew, Pjv, 2, offset, 0);
        Pjpm->jpm_PValue = Pjvnew + offset;                // new value area.
        j__udyLFreeJV(PjvRaw, 2, Pjpm);

        ju_SetPntrInJp(Pjp, Pjll4newRaw);
        ju_SetLeafPop1(Pjp, 3);
        ju_SetJpType(Pjp, cJU_JPLEAF4);
#else   // JUDY1
        JU_INSERTINPLACE(PImmed4, 2, offset, Index);
        ju_SetJpType(Pjp, cJ1_JPIMMED_4_03);
#endif  // JUDY1
        return (1);
    }

#ifdef  JUDY1
    case cJ1_JPIMMED_4_03:
    {
        uint32_t *PImmed4 = ju_PImmed4(Pjp);
        int offset = j__udySearchImmed4(PImmed4, 3, Index, 4 * 8);
        if (offset >= 0)
            return (0);
        offset = ~offset;
        Word_t Pjll4newRaw = j__udyAllocJLL4((3) + 1, Pjpm);
        if (Pjll4newRaw == 0)
            return (-1);
        Pjll4_t Pjll4new = P_JLL4(Pjll4newRaw);

//      Put the LastKey in the Leaf4
        Pjll4new->jl4_LastKey = Index & cJU_DCDMASK(4);

        JU_INSERTCOPY(Pjll4new->jl4_Leaf, PImmed4, 3, offset, Index);
        ju_SetPntrInJp(Pjp, Pjll4newRaw);
        ju_SetLeafPop1(Pjp, 3 + 1);
        ju_SetJpType(Pjp, cJ1_JPLEAF4);
        return (1);
    }

// (5_01 => [[ 5_02..03 => ]] LeafL)
    case cJ1_JPIMMED_5_02:
    {
        uint8_t *PImmed5 = ju_PImmed5(Pjp);
        exppop1 = 2;
        int offset = j__udySearchImmed5(PImmed5, exppop1, Index, 5 * 8);
        if (offset >= 0)
            return (0);
        offset = ~offset;
        JU_INSERTINPLACE5(PImmed5, exppop1, offset, Index);
        ju_SetJpType(Pjp, cJ1_JPIMMED_5_03);
        return (1);
    }

    case cJ1_JPIMMED_5_03:
    {
        uint8_t *PImmed5 = ju_PImmed5(Pjp);
        int offset = j__udySearchImmed5(PImmed5, 3, Index, 5 * 8);
        if (offset >= 0)
            return (0);
        offset = ~offset;
        Word_t Pjll5Raw = j__udyAllocJLL5(3 + 1, Pjpm);
        if (Pjll5Raw == 0)
            return (-1);
        Pjll5_t Pjll5 = P_JLL5(Pjll5Raw);

//      Put the LastKey in the Leaf5
        Pjll5->jl5_LastKey = Index & cJU_DCDMASK(5);

        JU_INSERTCOPY5(Pjll5->jl5_Leaf, PImmed5, 3, offset, Index);
        ju_SetPntrInJp(Pjp, Pjll5Raw);
        ju_SetLeafPop1(Pjp, 3 + 1);
        ju_SetJpType(Pjp, cJU_JPLEAF5);
        return (1);
    }

// (6_01 => [[ 6_02 => ]] LeafL)
    case cJ1_JPIMMED_6_02:
    {
        uint8_t *PImmed6 = ju_PImmed6(Pjp);
        int offset = j__udySearchImmed6(PImmed6, 2, Index, 6 * 8);
        if (offset >= 0)
            return (0);
        offset = ~offset;
        Word_t Pjll6Raw = j__udyAllocJLL6(2 + 1, Pjpm);
        if (Pjll6Raw == 0)
            return (-1);
        Pjll6_t Pjll6 = P_JLL6(Pjll6Raw);

//      Put the LastKey in the Leaf6
        Pjll6->jl6_LastKey = Index & cJU_DCDMASK(6);

        JU_INSERTCOPY6(Pjll6->jl6_Leaf, PImmed6, 2, offset, Index);
        ju_SetPntrInJp(Pjp, Pjll6Raw);
        ju_SetLeafPop1(Pjp, 2 + 1);
        ju_SetJpType(Pjp, cJU_JPLEAF6);
        return (1);
    }

// (7_01 => [[ 7_02 => ]] LeafL)
    case cJ1_JPIMMED_7_02:
    {
        uint8_t *PImmed7 = ju_PImmed7(Pjp);
        int      offset = j__udySearchImmed7(PImmed7, 2, Index, 7 * 8);
        if (offset >= 0)
            return (0);
        offset = ~offset;
        Word_t Pjll7Raw = j__udyAllocJLL7(2 + 1, Pjpm);
        if (Pjll7Raw == 0)
            return (-1);
        Pjll7_t Pjll7 = P_JLL7(Pjll7Raw);
//      Put the LastKey in the Leaf7
        Pjll7->jl7_LastKey = Index & cJU_DCDMASK(7);
        JU_INSERTCOPY7(Pjll7->jl7_Leaf, PImmed7, 2, offset, Index);
        ju_SetPntrInJp(Pjp, Pjll7Raw);
        ju_SetLeafPop1(Pjp, 2 + 1);
        ju_SetJpType(Pjp, cJU_JPLEAF7);
        return (7);
    }
#endif /* JUDY1 */

// ****************************************************************************
// INVALID JP TYPE:
    default:
        JU_SET_ERRNO_NONNULL(Pjpm, JU_ERRNO_CORRUPT);
        return (-1);
    } // end of switch on JP type

// PROCESS JP -- RECURSIVELY:
//
// For non-Immed JP types, if successful, post-increment the population count
// at this Level.
    retcode = j__udyInsWalk(Pjp, Index, Pjpm);

//  Successful insert, increment JP and subexpanse count:
    if (retcode > 0)
    {
//      Need to increment population of every Branch up the tree
        if (ju_Type(Pjp) <= cJU_JPMAXBRANCH)    
        {
            Word_t DcdP0 = ju_DcdPop0(Pjp) + 1;
            ju_SetDcdPop0(Pjp, DcdP0);  // update DcdP0 by + 1
        }
        retcode = 1;
    }
    return (retcode);
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
#ifdef JUDY1
        return (JERRI);
#else /* JUDYL */
        return (PPJERR);
#endif /* JUDYL */
    }






    Pjll8_t Pjll8 = P_JLL8(*PPArray);   // first word of leaf (always TotalPop0)

// ****************************************************************************
// PROCESS TOP LEVEL "JRP" BRANCHES AND LEAVES:
// ****************************************************************************

// ****************************************************************************
// JRPNULL (EMPTY ARRAY):  BUILD A LEAF8 WITH ONE INDEX:
// ****************************************************************************

// if a valid empty array (null pointer), so create an array of population == 1:
    if (Pjll8 == (Pjll8_t) NULL)
    {
        Word_t    Pjll8Raw = (Word_t)j__udyAllocJLL8(1);        // currently 3 words
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
//  ELSE more Keys than 1
// ****************************************************************************


#ifdef MAYBE    // Notes for future:
//  normally *PPArray is a ^ to Leaf8 or ^ to root structure  
//  Currently the root structure does not have a ju_Type
//  If it was to a jp_t then the 2nd word could have the 
//  Total Pop, if it was a branch (in DcdPop0), else if a Leaf8 or
//  any Leaf, then in the LeafPop field (in the Pointer to Leaf) or
//  in the LastKey field (not necessarly in a Leaf8).

    Pjp_t Pjp = (Pjp_t)PPArray;
    RawJpPntr = ju_PntrInJp(Pjp);
    switch(ju_Type(Pjp))
    {
    case cJU_JPJJPM:            // Root structure ^
    case cJU_JPLEAF8:           // or just Leaf8 ^ (no Root struct yet)
        etc...
#endif  // MAYBE


#ifdef  TRACEJPI
    if (startpop && (Pjll8->jl8_Population0 >= startpop))
    {
//        assert(Pjll8->jl8_Population0 == **(PWord_t)PPArray);   // first word is TotalPop0
#ifdef JUDY1
        printf("---Judy1Set, Key = 0x%016lx, Array TotalPop1 = %lu\n", 
            (Word_t)Index, Pjll8->jl8_Population0 + 1); // TotalPop0 = 0 (== 1 at base 1)
#else /* JUDYL */
        printf("---JudyLIns, Key = 0x%016lx(%ld), Array Pop1 = %lu\n", Index,
            (Word_t)Index, Pjll8->jl8_Population0 + 1); // TotalPop0 = 0 (== 1 at base 1)
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
            Pjll8_t  Pjll8new;
            Word_t   Pjll8newRaw;
//          Insert index into a new, larger leaf:
            Pjll8newRaw = (Word_t)j__udyAllocJLL8(pop1 + 1);
            if (Pjll8newRaw == 0)
            { // out of memory
                JU_SET_ERRNO(PJError, JU_ERRNO_NOMEM);
#ifdef JUDY1
                return(JERRI);
#else /* JUDYL */
                return(PPJERR);
#endif /* JUDYL */
            }
            Pjll8new = P_JLL8(Pjll8newRaw);
            JU_INSERTCOPY(Pjll8new->jl8_Leaf, Pjll8->jl8_Leaf, pop1, offset, Index);
#ifdef JUDYL
            Pjv_t Pjvnew = JL_LEAF8VALUEAREA(Pjll8new, pop1 + 1);
            JU_INSERTCOPY(Pjvnew, PjvOld, pop1, offset, 0);
#endif /* JUDYL */
            j__udyFreeJLL8(Pjll8, pop1, NULL);  // no jpm_t
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
        ju_SetLeafPop1(Pjp, cJU_LEAF8_MAXPOP1);
//        ju_SetDcdPop0 (Pjp, cJU_LEAF8_MAXPOP1 - 1);     // Level = 8, no pre-Key bytes
        ju_SetPntrInJp(Pjp, (Word_t)Pjll8);
//        ju_SetJpType(Pjp, cJU_JPLEAF8); Cascade8 knows what kind of LEAF
        if (j__udyCascade8(Pjp, Index, Pjpm) == -1)
        {
            JU_COPY_ERRNO(PJError, Pjpm);
#ifdef JUDY1
            return (JERRI);
#else /* JUDYL */
            return (PPJERR);
#endif /* JUDYL */
        }
//      Now Pjp->jp_Type is a BranchL*]

// Note:  No need to pass Pjpm for memory decrement; LEAF8 memory is never
// counted in the jpm_t because there is no jpm_t.
        j__udyFreeJLL8(Pjll8, cJU_LEAF8_MAXPOP1, NULL);

        *PPArray = (Pvoid_t)Pjpm;       // put Cascade8 results in Root pointer
    }   // fall thru
//    assert(Pjpm->jpm_Pop0 == (cJU_LEAF8_MAXPOP1 - 1));

// ****************************************************************************
// Now do the Insert in the Tree Walk code
    {
        Pjpm = P_JPM(*PPArray); // Caution: this is necessary

        int retcode = j__udyInsWalk(Pjpm->jpm_JP, Index, Pjpm);
        if (retcode == -1)
        {
            JU_COPY_ERRNO(PJError, Pjpm);
#ifdef JUDY1
            return (JERRI);
#else /* JUDYL */
            return (PPJERR);
#endif /* JUDYL */
        }
        if (retcode == 1)
        {
            ++Pjpm->jpm_Pop0;         // incr total array popu.

            Word_t DcdP0 = ju_DcdPop0(Pjpm->jpm_JP) + 1;
            ju_SetDcdPop0(Pjpm->jpm_JP, DcdP0);  // update DcdP0 by + 1

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
#else /* JUDYL */
        assert(Pjpm->jpm_PValue != (Pjv_t) NULL);
        return ((PPvoid_t) Pjpm->jpm_PValue);
#endif /* JUDYL */
    }
 /*NOTREACHED*/
}                        // Judy1Set() / JudyLIns()
