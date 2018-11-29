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
#endif

#ifdef JUDY1
extern int j__udy1CreateBranchB(Pjp_t, Pjp_t, uint8_t *, Word_t, Pjpm_t);
extern int j__udy1CreateBranchU(Pjp_t, Pjpm_t);
extern int j__udy1Cascade2(Pjp_t, Pjpm_t);
extern int j__udy1Cascade3(Pjp_t, Pjpm_t);
extern int j__udy1Cascade4(Pjp_t, Pjpm_t);
extern int j__udy1Cascade5(Pjp_t, Pjpm_t);
extern int j__udy1Cascade6(Pjp_t, Pjpm_t);
extern int j__udy1Cascade7(Pjp_t, Pjpm_t);
extern int j__udy1CascadeL(Pjp_t, Pjpm_t);
extern int j__udy1InsertBranch(Pjp_t Pjp, Word_t Index, Word_t Btype, Pjpm_t);
#else /* JUDYL */
extern int j__udyLCreateBranchB(Pjp_t, Pjp_t, uint8_t *, Word_t, Pjpm_t);
extern int j__udyLCreateBranchU(Pjp_t, Pjpm_t);
extern int j__udyLCascade2(Pjp_t, Pjpm_t);
extern int j__udyLCascade3(Pjp_t, Pjpm_t);
extern int j__udyLCascade4(Pjp_t, Pjpm_t);
extern int j__udyLCascade5(Pjp_t, Pjpm_t);
extern int j__udyLCascade6(Pjp_t, Pjpm_t);
extern int j__udyLCascade7(Pjp_t, Pjpm_t);
extern int j__udyLCascadeL(Pjp_t, Pjpm_t);
extern int j__udyLInsertBranch(Pjp_t Pjp, Word_t Index, Word_t Btype, Pjpm_t);
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
j__udyInsWalk(Pjp_t Pjp,                // current JP to descend.
              Word_t Index,             // to insert.
              Pjpm_t Pjpm)              // for returning info to top Level.
{
    uint8_t   digit;                    // from Index, current offset into a branch.
    jp_t      newJP;                    // for creating a new Immed JP.
    Word_t    exppop1;                  // expanse (leaf) population.
    int       retcode;                  // return codes:  -1, 0, 1.
    uint8_t   jpLevel = 0;

  ContinueInsWalk:                     // for modifying state without recursing.

#ifdef TRACEJPI
        JudyPrintJP(Pjp, "i", __LINE__);
#endif

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
        assert((ju_BaLPntr(Pjp)) == 0);
        jpLevel = ju_Type(Pjp) - cJU_JPNULL1;
//        JU_JPSETADT(Pjp, 0, Index, cJU_JPIMMED_1_01 + jpLevel);
        ju_SetIMM01(Pjp, 0, Index, cJU_JPIMMED_1_01 + jpLevel);

#ifdef TRACEJPI
        JudyPrintJP(Pjp, "i", __LINE__);
#endif

#ifdef JUDYL
        // value area is first word of new Immed_01 JP:
//        Pjpm->jpm_PValue = (Pjv_t) &Pjp->jp_ValueI;
        Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);          // ^ to Immed Value
#endif /* JUDYL */
        return (1);
// ****************************************************************************
// JPBRANCH_L*:
//
// If the new Index is not an outlier to the branchs expanse, and the branch
// should not be converted to uncompressed, extract the digit and record the
// Immediate type to create for a new Immed JP, before going to common code.
//
// Note:  JU_CHECK_IF_OUTLIER() is a no-op for BranchB3[7] on 32[64]-bit.
    case cJU_JPBRANCH_L2:
        if (ju_DcdNonMatchKey(Index, Pjp, 2))
            return (j__udyInsertBranch(Pjp, Index, 2, Pjpm));
        digit = JU_DIGITATSTATE(Index, 2);
        (exppop1) = ju_BranchPop0(Pjp, 2);
        goto JudyBranchL;
    case cJU_JPBRANCH_L3:
        if (ju_DcdNonMatchKey(Index, Pjp, 3))
            return (j__udyInsertBranch(Pjp, Index, 3, Pjpm));
        digit = JU_DIGITATSTATE(Index, 3);
        (exppop1) = ju_BranchPop0(Pjp, 3);
        goto JudyBranchL;
    case cJU_JPBRANCH_L4:
        if (ju_DcdNonMatchKey(Index, Pjp, 4))
            return (j__udyInsertBranch(Pjp, Index, 4, Pjpm));
        digit = JU_DIGITATSTATE(Index, 4);
        (exppop1) = ju_BranchPop0(Pjp, 4);
        goto JudyBranchL;
    case cJU_JPBRANCH_L5:
        if (ju_DcdNonMatchKey(Index, Pjp, 5))
            return (j__udyInsertBranch(Pjp, Index, 5, Pjpm));
        digit = JU_DIGITATSTATE(Index, 5);
        (exppop1) = ju_BranchPop0(Pjp, 5);
        goto JudyBranchL;
    case cJU_JPBRANCH_L6:
        if (ju_DcdNonMatchKey(Index, Pjp, 6))
            return (j__udyInsertBranch(Pjp, Index, 6, Pjpm));
        digit = JU_DIGITATSTATE(Index, 6);
        (exppop1) = ju_BranchPop0(Pjp, 6);
        goto JudyBranchL;
    case cJU_JPBRANCH_L7:
        if (ju_DcdNonMatchKey(Index, Pjp, 7))
            return (j__udyInsertBranch(Pjp, Index, 7, Pjpm));
        digit = JU_DIGITATSTATE(Index, 7);
        (exppop1) = ju_BranchPop0(Pjp, 7);
        goto JudyBranchL;
// Similar to common code above, but no outlier check is needed, and the Immed
// type depends on the word size:
    case cJU_JPBRANCH_L:
    {
        Word_t    PjblRaw;              // pointer to old linear branch.
        Pjbl_t    Pjbl;
        Word_t    PjbuRaw;              // pointer to new uncompressed branch.
        Pjbu_t    Pjbu;
        Word_t    numJPs;               // number of JPs = populated expanses.
        int       offset;               // in branch.
        digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
        exppop1 = Pjpm->jpm_Pop0;
        // fall through:
// COMMON CODE FOR LINEAR BRANCHES:
//
// Come here with digit and exppop1 already set.
      JudyBranchL:
        PjblRaw = ju_BaLPntr(Pjp);

////printf("Entry1 into BranchL: PjblRaw = 0x%lx, type = %d\n", PjblRaw, ju_Type(Pjp));

        Pjbl = P_JBL(PjblRaw);
// If population under this branch greater than:
        if (exppop1 > JU_BRANCHL_MAX_POP)
            goto ConvertBranchLtoU;
        numJPs = Pjbl->jbl_NumJPs;
        if ((numJPs == 0) || (numJPs > cJU_BRANCHLMAXJPS))
        {
            JU_SET_ERRNO_NONNULL(Pjpm, JU_ERRNO_CORRUPT);
            return (-1);
        }
// Search for a match to the digit:
        offset = j__udySearchBranchL(Pjbl->jbl_Expanse, numJPs, digit);
// If Index is found, offset is into an array of 1..cJU_BRANCHLMAXJPS JPs:
        if (offset >= 0)
        {
            Pjp = Pjbl->jbl_jp + offset;      // address of next JP.
////printf("Entry2!! into BranchL: Pjp = 0x%lx, Type = %d, offset = %d, BaLPntr = 0x%lx\n", (Word_t)Pjp, ju_Type(Pjp), offset, ju_BaLPntr(Pjp));
            break;                      // continue walk.
        }
////printf("Entry3 into BranchL: offset = %d\n", offset);
// Expanse is missing (not populated) for the passed Index, so insert an Immed
// -- if theres room:
        if (numJPs < cJU_BRANCHLMAXJPS)
        {
            offset = ~offset;           // insertion offset.
            jpLevel = ju_Type(Pjp) - cJU_JPBRANCH_L2;

//            JU_JPSETADT(&newJP, 0, Index, cJU_JPIMMED_1_01 + jpLevel);
            ju_SetIMM01(&newJP, 0, Index, cJU_JPIMMED_1_01 + jpLevel);
            JU_INSERTINPLACE(Pjbl->jbl_Expanse, numJPs, offset, digit);
            JU_INSERTINPLACE(Pjbl->jbl_jp, numJPs, offset, newJP);
            Pjbl->jbl_NumJPs++;
#ifdef JUDYL
            // Value area is in the new Immed 01 JP:
            Pjp = Pjbl->jbl_jp + offset;
//            Pjpm->jpm_PValue = (Pjv_t) &Pjp->jp_ValueI;
            Pjpm->jpm_PValue =  ju_PImmVal_01(Pjp);        // ^ to Immed Values
#endif /* JUDYL */
            return (1);
        }
// MAXED OUT LINEAR BRANCH, CONVERT TO A BITMAP BRANCH, THEN INSERT:
//
// Copy the linear branch to a bitmap branch.
//
// TBD:  Consider renaming j__udyCreateBranchB() to j__udyConvertBranchLtoB().
        assert((numJPs) <= cJU_BRANCHLMAXJPS);
        if (j__udyCreateBranchB(Pjp, Pjbl->jbl_jp, Pjbl->jbl_Expanse,
                                numJPs, Pjpm) == -1)
        {
            return (-1);
        }
// Convert jp_Type from linear branch to equivalent bitmap branch:
//        Pjp->jp_Type += cJU_JPBRANCH_B - cJU_JPBRANCH_L;
        ju_SetJpType(Pjp, ju_Type(Pjp) + cJU_JPBRANCH_B - cJU_JPBRANCH_L);
        j__udyFreeJBL(PjblRaw, Pjpm);   // free old BranchL.
// Having changed branch types, now do the insert in the new branch type:
        goto ContinueInsWalk;
// OPPORTUNISTICALLY CONVERT FROM BRANCHL TO BRANCHU:
//
// Memory efficiency is no object because the branchs pop1 is large enough, so
// speed up array access.  Come here with PjblRaw set.  Note:  This is goto
// code because the previous block used to fall through into it as well, but no
// longer.
      ConvertBranchLtoU:
#ifdef  PCAS
        printf("\nConvertBranchLtoU\n");
#endif  // PCAS

// Allocate memory for an uncompressed branch:
        if ((PjbuRaw = j__udyAllocJBU(Pjpm)) == 0)
            return (-1);
        Pjbu = P_JBU(PjbuRaw);
// Set the proper NULL type for most of the uncompressed branchs JPs:
        jpLevel = (ju_Type(Pjp) - cJU_JPBRANCH_L2);
//        JU_JPSETADT(&newJP, 0, 0, cJU_JPNULL1 + jpLevel);
        ju_SetIMM01(&newJP, 0, 0, cJU_JPNULL1 + jpLevel);
// Initialize:  Pre-set uncompressed branch to mostly JPNULL*s:
        for (numJPs = 0; numJPs < cJU_BRANCHUNUMJPS; ++numJPs)
            Pjbu->jbu_jp[numJPs] = newJP;
// Copy JPs from linear branch to uncompressed branch:
        {
            for (numJPs = 0; numJPs < Pjbl->jbl_NumJPs; ++numJPs)
            {
                Pjp_t     Pjp1 = &(Pjbl->jbl_jp[numJPs]);
                offset = Pjbl->jbl_Expanse[numJPs];
                Pjbu->jbu_jp[offset] = *Pjp1;
            }
        }
        j__udyFreeJBL(PjblRaw, Pjpm);   // free old BranchL.
// Plug new values into parent JP:
//          Pjp->Jp_Addr0 = PjbuRaw;
        ju_SetBaLPntr(Pjp, PjbuRaw);

        jpLevel = ju_Type(Pjp) - cJU_JPBRANCH_L2;
//        Pjp->jp_Type = cJU_JPBRANCH_U2 + jpLevel;
        ju_SetJpType(Pjp, cJU_JPBRANCH_U2 + jpLevel);
// Save global population of last BranchU conversion:
        Pjpm->jpm_LastUPop0 = Pjpm->jpm_Pop0;
        goto ContinueInsWalk;
    }                                   // case cJU_JPBRANCH_L.
// ****************************************************************************
// JPBRANCH_B*:
//
// If the new Index is not an outlier to the branchs expanse, extract the
// digit and record the Immediate type to create for a new Immed JP, before
// going to common code.
//
// Note:  JU_CHECK_IF_OUTLIER() is a no-op for BranchB3[7] on 32[64]-bit.
    case cJU_JPBRANCH_B2:
        if (ju_DcdNonMatchKey(Index, Pjp, 2))
            return (j__udyInsertBranch(Pjp, Index, 2, Pjpm));
        digit = JU_DIGITATSTATE(Index, 2);
        (exppop1) = ju_BranchPop0(Pjp, 2);
        goto JudyBranchB;
    case cJU_JPBRANCH_B3:
        if (ju_DcdNonMatchKey(Index, Pjp, 3))
            return (j__udyInsertBranch(Pjp, Index, 3, Pjpm));
        digit = JU_DIGITATSTATE(Index, 3);
        (exppop1) = ju_BranchPop0(Pjp, 3);
        goto JudyBranchB;
    case cJU_JPBRANCH_B4:
        if (ju_DcdNonMatchKey(Index, Pjp, 4))
            return (j__udyInsertBranch(Pjp, Index, 4, Pjpm));
        digit = JU_DIGITATSTATE(Index, 4);
        (exppop1) = ju_BranchPop0(Pjp, 4);
        goto JudyBranchB;
    case cJU_JPBRANCH_B5:
        if (ju_DcdNonMatchKey(Index, Pjp, 5))
            return (j__udyInsertBranch(Pjp, Index, 5, Pjpm));
        digit = JU_DIGITATSTATE(Index, 5);
        (exppop1) = ju_BranchPop0(Pjp, 5);
        goto JudyBranchB;
    case cJU_JPBRANCH_B6:
        if (ju_DcdNonMatchKey(Index, Pjp, 6))
            return (j__udyInsertBranch(Pjp, Index, 6, Pjpm));
        digit = JU_DIGITATSTATE(Index, 6);
        (exppop1) = ju_BranchPop0(Pjp, 6);
        goto JudyBranchB;
    case cJU_JPBRANCH_B7:
        if (ju_DcdNonMatchKey(Index, Pjp, 7))
            return (j__udyInsertBranch(Pjp, Index, 7, Pjpm));
        digit = JU_DIGITATSTATE(Index, 7);
        (exppop1) = ju_BranchPop0(Pjp, 7);
        goto JudyBranchB;
    case cJU_JPBRANCH_B:
    {
        Pjbb_t    Pjbb;                 // pointer to bitmap branch.
        Word_t    PjbbRaw;              // pointer to bitmap branch.
        Word_t    Pjp2Raw;              // 1 of N arrays of JPs.
        Pjp_t     Pjp2;                 // 1 of N arrays of JPs.
        Word_t    subexp;               // 1 of N subexpanses in bitmap.
        BITMAPB_t bitmap;               // for one subexpanse.
        BITMAPB_t bitmask;              // bit set for Indexs digit.
        Word_t    numJPs;               // number of JPs = populated expanses.
        int       offset;               // in bitmap branch.
// Similar to common code above, but no outlier check is needed, and the Immed
// type depends on the word size:
        digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
        exppop1 = Pjpm->jpm_Pop0;
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

                        Pjpm->jpm_LastUPop0 = Pjpm->jpm_Pop0;

                        goto ContinueInsWalk;
                    }
                }
            }
#endif  // ! noU

// CONTINUE TO USE BRANCHB:
//
// Get pointer to bitmap branch (JBB):
        PjbbRaw = ju_BaLPntr(Pjp);
        Pjbb = P_JBB(PjbbRaw);
// Form the Int32 offset, and Bit offset values:
//
// 8 bit Decode | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//              |SubExpanse |    Bit offset     |
//
// Get the 1 of 8 expanses from digit, Bits 5..7 = 1 of 8, and get the 32-bit
// word that may have a bit set:
        subexp = digit / cJU_BITSPERSUBEXPB;
        bitmap = JU_JBB_BITMAP(Pjbb, subexp);
        Pjp2Raw = JU_JBB_PJP(Pjbb, subexp);
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
        jpLevel = ju_Type(Pjp) - cJU_JPBRANCH_B2;
//        JU_JPSETADT(&newJP, 0, Index, cJU_JPIMMED_1_01 + jpLevel);
        ju_SetIMM01(&newJP, 0, Index,  cJU_JPIMMED_1_01 + jpLevel);

// Get 1 of the 8 JP arrays and calculate number of JPs in subexpanse array:
        Pjp2Raw = JU_JBB_PJP(Pjbb, subexp);
        Pjp2 = P_JP(Pjp2Raw);
        numJPs = j__udyCountBitsB(bitmap);
// Expand branch JP subarray in-place:
        if (JU_BRANCHBJPGROWINPLACE(numJPs))
        {
            assert(numJPs > 0);
            JU_INSERTINPLACE(Pjp2, numJPs, offset, newJP);
#ifdef JUDYL
            // value area is first word of new Immed 01 JP:
            Pjpm->jpm_PValue = (Pjv_t) (Pjp2 + offset);
//            Pjpm->jpm_PValue =  P_JV(ju_PImmVals(Pjp2 + offset));        // ^ to Immed Values
#endif /* JUDYL */
        }
// No room, allocate a bigger bitmap branch JP subarray:
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
                Pjpm->jpm_PValue = (Pjv_t) (Pjpnew + offset);
//                Pjpm->jpm_PValue =  P_JV(ju_PImmVals(Pjpnew + offset));        // ^ to Immed Values
#endif /* JUDYL */
            }
// New JP subarray; point to cJU_JPIMMED_*_01 and place it:
            else
            {
                assert(JU_JBB_PJP(Pjbb, subexp) == 0);
                Pjp = Pjpnew;
                *Pjp = newJP;           // copy to new memory.
#ifdef JUDYL
//                Pjpm->jpm_PValue = (Pjv_t) &(Pjp->jp_ValueI);
                Pjpm->jpm_PValue =  ju_PImmVal_01(Pjp);    // ^ to Immed Value
#endif /* JUDYL */
            }
// Place new JP subarray in BranchB:
            JU_JBB_PJP(Pjbb, subexp) = PjpnewRaw;
        }                               // else
// Set the new Indexs bit:
        JU_JBB_BITMAP(Pjbb, subexp) |= bitmask;
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
        if (ju_DcdNonMatchKey(Index, Pjp, 2))
            return (j__udyInsertBranch(Pjp, Index, 2, Pjpm));

        Pjbu_t    P_jbu = P_JBU(ju_BaLPntr(Pjp));
        Pjp     = P_jbu->jbu_jp + JU_DIGITATSTATE(Index, 2);
        break;
    }
    case cJU_JPBRANCH_U3:
    {
        if (ju_DcdNonMatchKey(Index, Pjp, 3))
            return (j__udyInsertBranch(Pjp, Index, 3, Pjpm));

        Pjbu_t    P_jbu = P_JBU(ju_BaLPntr(Pjp));
        Pjp     = P_jbu->jbu_jp + JU_DIGITATSTATE(Index, 3);
        break;
    }
    case cJU_JPBRANCH_U4:
    {
        if (ju_DcdNonMatchKey(Index, Pjp, 4))
            return (j__udyInsertBranch(Pjp, Index, 4, Pjpm));

        Pjbu_t    P_jbu = P_JBU(ju_BaLPntr(Pjp));
        Pjp     = P_jbu->jbu_jp + JU_DIGITATSTATE(Index, 4);
        break;
    }
    case cJU_JPBRANCH_U5:
    {
        if (ju_DcdNonMatchKey(Index, Pjp, 5))
            return (j__udyInsertBranch(Pjp, Index, 5, Pjpm));

        Pjbu_t    P_jbu = P_JBU(ju_BaLPntr(Pjp));
        Pjp     = P_jbu->jbu_jp + JU_DIGITATSTATE(Index, 5);
        break;
    }
    case cJU_JPBRANCH_U6:
    {
        if (ju_DcdNonMatchKey(Index, Pjp, 6))
            return (j__udyInsertBranch(Pjp, Index, 6, Pjpm));

        Pjbu_t    P_jbu = P_JBU(ju_BaLPntr(Pjp));
        Pjp     = P_jbu->jbu_jp + JU_DIGITATSTATE(Index, 6);
        break;
    }
    case cJU_JPBRANCH_U7:
    {
        Pjbu_t    P_jbu = P_JBU(ju_BaLPntr(Pjp));
        Pjp     = P_jbu->jbu_jp + JU_DIGITATSTATE(Index, 7);
        break;
    }
    case cJU_JPBRANCH_U:
    {
        Pjbu_t    P_jbu = P_JBU(ju_BaLPntr(Pjp));
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
//////////#ifdef  JUDYL
    case cJU_JPLEAF1:
    {
        if (ju_DcdNonMatchKey(Index, Pjp, 1))
            return (j__udyInsertBranch(Pjp, Index, 1, Pjpm));

        exppop1 = ju_LeafPop0(Pjp) + 1;

        assert(exppop1 <= cJU_LEAF1_MAXPOP1);

        Word_t Pleaf1Raw = ju_BaLPntr(Pjp);
        uint8_t *Pleaf1 = P_JLL1(Pleaf1Raw);

#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF1VALUEAREA(Pleaf1, exppop1);
#endif /* JUDYL */

        int offset = j__udySearchLeaf1(Pleaf1, exppop1, Index, 1 * 8);
        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (0);
        }
        offset = ~offset;
        if (JU_LEAF1GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE(Pleaf1, exppop1, offset, Index);
//            if (offset == exppop1)
            {
                JU_PADLEAF1(Pleaf1, exppop1 + 1);
            }

#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif // JUDYL
            return (1);
        }
        if (exppop1 < cJU_LEAF1_MAXPOP1)      /* grow to new leaf */
        {
            Word_t Pleaf1newRaw = j__udyAllocJLL1(exppop1 + 1, Pjpm);
            if (Pleaf1newRaw == 0)
                return (-1);

            uint8_t  *Pleaf1new = P_JLL1(Pleaf1newRaw);
            JU_INSERTCOPY(Pleaf1new, Pleaf1, exppop1, offset, Index);
            JU_PADLEAF1(Pleaf1new, exppop1 + 1);

#ifdef JUDYL
            Pjv_t Pjvnew = JL_LEAF1VALUEAREA(Pleaf1new, exppop1 + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif // JUDYL
            j__udyFreeJLL1(Pleaf1Raw, exppop1, Pjpm);
//            Pjp->Jp_Addr0 = Pleaf1newRaw;
            ju_SetBaLPntr(Pjp, Pleaf1newRaw);
            return (1);
        }
//      Max population, convert to LeafB1
        assert(exppop1 == (cJU_LEAF1_MAXPOP1));

#ifdef  PCAS
        printf("\n================== Leaf1 to LeafB1\n");
#endif  // PCAS

	Word_t PjlbRaw = j__udyAllocJLB1(Pjpm);
	if (PjlbRaw == 0)
            return(-1);

	Pjlb_t Pjlb  = P_JLB(PjlbRaw);
//	Copy 1 byte index Leaf to bitmap Leaf
	for (int ii = 0; ii < cJU_LEAF1_MAXPOP1; ii++)
            JU_BITMAPSETL(Pjlb, Pleaf1[ii]);

#ifdef JUDYL
//	Build 4 subexpanse Value leaves from bitmap
	for (int ii = 0; ii < cJU_NUMSUBEXPL; ii++)
	{
//	    Get number of Indexes in subexpanse
	    Word_t subpop1 = j__udyCountBitsL(JU_JLB_BITMAP(Pjlb, ii));
#ifdef BMVALUE 
	    if (subpop1 > 1)
#else
	    if (subpop1)
#endif  // ! BMVALUE 
            {
		Word_t PjvnewRaw;	// value area of new leaf.
		Pjv_t  Pjvnew;

		PjvnewRaw = j__udyLAllocJV(subpop1, Pjpm);
		if (PjvnewRaw == 0)	// out of memory.
		{
//                  Free prevously allocated LeafVs:
		    while(ii--)
		    {
			subpop1 = j__udyCountBitsL(JU_JLB_BITMAP(Pjlb, ii));
#ifdef BMVALUE 
			if (subpop1 > 1)
#else
			if (subpop1)
#endif  // ! BMVALUE 
			{
			    PjvnewRaw = JL_JLB_PVALUE(Pjlb, ii);
			    j__udyLFreeJV(PjvnewRaw, subpop1, Pjpm);
			}
		    }
//                  Free the bitmap leaf
		    j__udyLFreeJLB1(PjlbRaw,Pjpm);
		    return(-1);
		}
		Pjvnew    = P_JV(PjvnewRaw);
		JU_COPYMEM(Pjvnew, Pjv, subpop1);

		Pjv += subpop1;
		JL_JLB_PVALUE(Pjlb, ii) = PjvnewRaw;
	    }
	}
#endif  // JUDYL

        j__udyFreeJLL1(Pleaf1Raw, cJU_LEAF1_MAXPOP1, Pjpm);
//        Pjp->jp_Type = cJU_JPLEAF_B1;
        ju_SetJpType(Pjp, cJU_JPLEAF_B1);
//        Pjp->Jp_Addr0 = PjlbRaw;
        ju_SetBaLPntr(Pjp, PjlbRaw);
        goto ContinueInsWalk;   // now do the Insert
    }
///////////#endif  // JUDYL

    case cJU_JPLEAF2:
    {
        Word_t    PjllRaw;
        uint16_t *Pleaf;                /* specific type */
        int       offset;

        if (ju_DcdNonMatchKey(Index, Pjp, 2))
            return (j__udyInsertBranch(Pjp, Index, 2, Pjpm));
        exppop1 = ju_LeafPop0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF2_MAXPOP1));
        PjllRaw = ju_BaLPntr(Pjp);
        Pleaf = (uint16_t *) P_JLL(PjllRaw);

#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF2VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */

        offset = j__udySearchLeaf2(Pleaf, exppop1, Index, 2 * 8);
        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (0);
        }
        offset = ~offset;
        if (JU_LEAF2GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE(Pleaf, exppop1, offset, Index);
//            if (offset == exppop1)
            {
                JU_PADLEAF2(Pleaf, exppop1 + 1);
            }
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < cJU_LEAF2_MAXPOP1)      /* grow to new leaf */
        {
            Word_t    PjllnewRaw;
            uint16_t *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL2(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint16_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY(Pleafnew, Pleaf, exppop1, offset, Index);
            JU_PADLEAF2(Pleafnew, exppop1 + 1);
#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF2VALUEAREA(Pleafnew, (exppop1) + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL2(PjllRaw, exppop1, Pjpm);
//            Pjp->Jp_Addr0 = PjllnewRaw;
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            return (1);
        }
        assert(exppop1 == (cJU_LEAF2_MAXPOP1));
        if (j__udyCascade2(Pjp, Pjpm) == -1)
            return (-1);

////////////////////////        j__udyFreeJLL2(PjllRaw, cJU_LEAF2_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    }
    case cJU_JPLEAF3:
    {
        Word_t    PjllRaw;
        uint8_t  *Pleaf;                /* specific type */
        int       offset;

        if (ju_DcdNonMatchKey(Index, Pjp, 3))
            return (j__udyInsertBranch(Pjp, Index, 3, Pjpm));
        exppop1 = ju_LeafPop0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF3_MAXPOP1));
        PjllRaw = ju_BaLPntr(Pjp);
        Pleaf = (uint8_t *) P_JLL(PjllRaw);

#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF3VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */

        offset = j__udySearchLeaf3(Pleaf, exppop1, Index, 3 * 8);
        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (0);
        }
        offset = ~offset;
        if (JU_LEAF3GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE3(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < (cJU_LEAF3_MAXPOP1))      /* grow to new leaf */
        {
            Word_t    PjllnewRaw;
            uint8_t  *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL3(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint8_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY3(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            Pjv_t Pjvnew = JL_LEAF3VALUEAREA(Pleafnew, (exppop1) + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL3(PjllRaw, exppop1, Pjpm);
//            Pjp->Jp_Addr0 = PjllnewRaw;
            ju_SetBaLPntr(Pjp, PjllnewRaw);

            return (1);
        }
        assert(exppop1 == (cJU_LEAF3_MAXPOP1));
        if (j__udyCascade3(Pjp, Pjpm) == -1)
            return (-1);
        j__udyFreeJLL3(PjllRaw, cJU_LEAF3_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    }
    case cJU_JPLEAF4:
    {
        Word_t    PjllRaw;
        uint32_t *Pleaf;                /* specific type */
        int       offset;

        if (ju_DcdNonMatchKey(Index, Pjp, 4))
            return (j__udyInsertBranch(Pjp, Index, 4, Pjpm));
        exppop1 = ju_LeafPop0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF4_MAXPOP1));
        PjllRaw = ju_BaLPntr(Pjp);
        Pleaf = (uint32_t *)P_JLL(PjllRaw);

#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF4VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */

        offset = j__udySearchLeaf4(Pleaf, exppop1, Index, 4 * 8);
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
            JU_INSERTINPLACE(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < cJU_LEAF4_MAXPOP1)      /* grow to new leaf */
        {
            Word_t    PjllnewRaw;
            uint32_t *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL4(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint32_t *)P_JLL(PjllnewRaw);
            JU_INSERTCOPY(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF4VALUEAREA(Pleafnew, (exppop1) + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL4(PjllRaw, exppop1, Pjpm);
//            Pjp->Jp_Addr0 = PjllnewRaw;
            ju_SetBaLPntr(Pjp, PjllnewRaw);

            return (1);
        }
        assert(exppop1 == cJU_LEAF4_MAXPOP1);
        if (j__udyCascade4(Pjp, Pjpm) == -1)
            return (-1);
        j__udyFreeJLL4(PjllRaw, cJU_LEAF4_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    }
    case cJU_JPLEAF5:
    {
        Word_t    PjllRaw;
        uint8_t  *Pleaf;                /* specific type */
        int       offset;

        if (ju_DcdNonMatchKey(Index, Pjp, 5))
            return (j__udyInsertBranch(Pjp, Index, 5, Pjpm));
        exppop1 = ju_LeafPop0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF5_MAXPOP1));
        PjllRaw = ju_BaLPntr(Pjp);
        Pleaf = (uint8_t *) P_JLL(PjllRaw);
#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF5VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */
        offset = j__udySearchLeaf5(Pleaf, exppop1, Index, 5 * 8);
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
            JU_INSERTINPLACE5(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < (cJU_LEAF5_MAXPOP1))      /* grow to new leaf */
        {
            Word_t    PjllnewRaw;
            uint8_t  *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL5(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint8_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY5(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF5VALUEAREA(Pleafnew, (exppop1) + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL5(PjllRaw, exppop1, Pjpm);
//            Pjp->Jp_Addr0 = PjllnewRaw;
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            return (1);
        }
        assert(exppop1 == (cJU_LEAF5_MAXPOP1));
        if (j__udyCascade5(Pjp, Pjpm) == -1)
            return (-1);
        j__udyFreeJLL5(PjllRaw, cJU_LEAF5_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    }
    case cJU_JPLEAF6:
    {
        Word_t    PjllRaw;
        uint8_t  *Pleaf;                /* specific type */
        int       offset;

        if (ju_DcdNonMatchKey(Index, Pjp, 6))
            return (j__udyInsertBranch(Pjp, Index, 6, Pjpm));
        exppop1 = ju_LeafPop0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF6_MAXPOP1));
        PjllRaw = ju_BaLPntr(Pjp);
        Pleaf = (uint8_t *) P_JLL(PjllRaw);
#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF6VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */
        offset = j__udySearchLeaf6(Pleaf, exppop1, Index, 6 * 8);
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
            JU_INSERTINPLACE6(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < cJU_LEAF6_MAXPOP1)      /* grow to new leaf */
        {
            Word_t    PjllnewRaw;
            uint8_t  *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL6(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint8_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY6(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF6VALUEAREA(Pleafnew, (exppop1) + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL6(PjllRaw, exppop1, Pjpm);
//            Pjp->Jp_Addr0 = PjllnewRaw;
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            return (1);
        }
        assert(exppop1 == (cJU_LEAF6_MAXPOP1));
        if (j__udyCascade6(Pjp, Pjpm) == -1)
            return (-1);
        j__udyFreeJLL6(PjllRaw, cJU_LEAF6_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    }
    case cJU_JPLEAF7:
    {
        Word_t    PjllRaw;
        uint8_t  *Pleaf;                /* specific type */
        int       offset;

        if (ju_DcdNonMatchKey(Index, Pjp, 7))
            return (j__udyInsertBranch(Pjp, Index, 7, Pjpm));
        exppop1 = ju_LeafPop0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF7_MAXPOP1));
        PjllRaw = ju_BaLPntr(Pjp);
        Pleaf = (uint8_t *) P_JLL(PjllRaw);
#ifdef JUDYL
        Pjv_t Pjv = JL_LEAF7VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */
        offset = j__udySearchLeaf7(Pleaf, exppop1, Index, 7 * 8);
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
            JU_INSERTINPLACE7(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < (cJU_LEAF7_MAXPOP1))      /* grow to new leaf */
        {
            Word_t    PjllnewRaw;
            uint8_t  *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL7(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint8_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY7(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            Pjv_t     Pjvnew = JL_LEAF7VALUEAREA(Pleafnew, (exppop1) + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
            j__udyFreeJLL7(PjllRaw, exppop1, Pjpm);
//            Pjp->Jp_Addr0 = PjllnewRaw;
            ju_SetBaLPntr(Pjp, PjllnewRaw);
            return (1);
        }
        assert(exppop1 == (cJU_LEAF7_MAXPOP1));
        if (j__udyCascade7(Pjp, Pjpm) == -1)
            return (-1);
        j__udyFreeJLL7(PjllRaw, cJU_LEAF7_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    }
// ****************************************************************************
// JPLEAF_B1:
//
// 8 bit Decode | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//              |SubExpanse |    Bit offset     |
//
// Note:  For JudyL, values are stored in 8[4] subexpanses, each a linear word
// array of up to 32[64] values each.

    case cJU_JPLEAF_B1:
    {
        if (ju_DcdNonMatchKey(Index, Pjp, 1))
            return (j__udyInsertBranch(Pjp, Index, 1, Pjpm));
#ifdef JUDY1
//      If Index (bit) is already set, return now:
        if (JU_BITMAPTESTL(P_JLB(ju_BaLPntr(Pjp)), Index)) 
            return(0);
//      If bitmap is not full, set the new Indexs bit; otherwise convert to a Full:
        exppop1 = ju_LeafPop0(Pjp) + 1;
        if (exppop1 < cJU_JPFULLPOPU1_POP0)
        {
            JU_BITMAPSETL(P_JLB(ju_BaLPntr(Pjp)), Index);
        }
        else
        {
            j__udyFreeJLB1(ju_BaLPntr(Pjp), Pjpm);  // free LeafB1.
//            Pjp->jp_Type = cJ1_JPFULLPOPU1;
            ju_SetJpType(Pjp, cJ1_JPFULLPOPU1);
//            Pjp->Jp_Addr0 = 0;
            ju_SetBaLPntr(Pjp, 0);
        }
        return(1);
#endif  // JUDY1

#ifdef  JUDYL
// This is very different from Judy1 because of the need to return a value area
// even for an existing Index, or manage the value area for a new Index, and
// because JudyL has no FULLPOPU1 type:

        Word_t    PjvRaw;           // pointer to value part of the leaf.
        Pjv_t     Pjv;              // pointer to value part of the leaf.
        Word_t    PjvnewRaw;        // new value area.
        Pjv_t     Pjvnew;           // new value area.
        Word_t    subexp;           // 1 of 8 subexpanses in bitmap.
        Pjlb_t    Pjlb;             // pointer to bitmap part of the leaf.
        BITMAPL_t bitmap;           // for one subexpanse.
        BITMAPL_t bitmask;          // bit set for Indexs digit.
        int       offset;           // of index in value area.

//      Get last byte to decode from Index, and pointer to bitmap leaf:
        digit = JU_DIGITATSTATE(Index, 1);
        Pjlb  = P_JLB(ju_BaLPntr(Pjp));

        subexp  = digit / cJU_BITSPERSUBEXPL;       // which subexpanse.
        bitmap  = JU_JLB_BITMAP(Pjlb, subexp);      // subexps 32-bit map.
        PjvRaw  = JL_JLB_PVALUE(Pjlb, subexp);      // corresponding values.
        bitmask = JU_BITPOSMASKL(digit);            // mask for Index.
        Pjv     = P_JV(PjvRaw);                     // corresponding values.

        offset  = j__udyCountBitsL(bitmap & (bitmask - 1)); // of Index.

//     If Index already exists, get value pointer and exit:
        if (bitmap & bitmask)
        {
// if (bitmap == bitmask) pop = 1; somdeay
            assert(Pjv);
            Pjpm->jpm_PValue = Pjv + offset;        // existing value.
            return(0);
        }

// Get the total bits set = expanse population of Value area:
        exppop1 = j__udyCountBitsL(bitmap);

//      If the value area can grow in place, do it:
        if (JL_LEAFVGROWINPLACE(exppop1))
        {
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            JU_JLB_BITMAP(Pjlb, subexp) |= bitmask;  // set Indexs bit.
            Pjpm->jpm_PValue = Pjv + offset;          // new value area.
            return(1);
        }

// Increase size of value area:
        if ((PjvnewRaw = j__udyLAllocJV(exppop1 + 1, Pjpm)) == 0) 
            return(-1);
        Pjvnew = P_JV(PjvnewRaw);

        if (exppop1)                // have existing value area.
        {
            assert(Pjv);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
            j__udyLFreeJV(PjvRaw, exppop1, Pjpm);   // free old values.
        }
        else                        // first index, new value area:
        {
// !! OOPs,if new value area, then use the pointer as Value
             Pjvnew[0] = 0;
             Pjpm->jpm_PValue   = Pjvnew;
        }

// Set bit for new Index and place new leaf value area in bitmap:

        JU_JLB_BITMAP(Pjlb, subexp) |= bitmask;
        JL_JLB_PVALUE(Pjlb, subexp)  = PjvnewRaw;
        return(1);
#endif // JUDYL
    } // case


#ifdef JUDY1
// ****************************************************************************
// JPFULLPOPU1:
//
// If Index is not an outlier, then by definition its already set.

     case cJ1_JPFULLPOPU1:

     if (ju_DcdNonMatchKey(Index, Pjp, 1))
         return (j__udyInsertBranch(Pjp, Index, 1, Pjpm));
     return(0);
#endif /* ! JUDY1 */
// JPIMMED*:
//
// This is some of the most complex code in Judy considering Judy1 versus JudyL
// and 32-bit versus 64-bit variations.  The following comments attempt to make
// this clearer.
//
// Of the 2 words in a JP, for immediate indexes Judy1 can use 2 words - 1 byte
// = 7 [15] bytes, but JudyL can only use 1 word - 1 byte = 3 [7] bytes because
// the other word is needed for a value area or a pointer to a value area.
//
// For both Judy1 and JudyL, cJU_JPIMMED_*_01 indexes are in word 2; otherwise
// for Judy1 only, a list of 2 or more indexes starts in word 1.  JudyL keeps
// the list in word 2 because word 1 is a pointer (to a LeafV, that is, a leaf
// containing only values).  Furthermore, cJU_JPIMMED_*_01 indexes are stored
// all-but-first-byte in jp_DcdPopO, not just the Index Sizes bytes.
//
// TBD:  This can be confusing because Doug didnt use data structures for it.
// Instead he often directly accesses Pjp for the first word and jp_DcdPopO for
// the second word.  It would be nice to use data structs, starting with
// jp_Index and jp_LIndex where possible.
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
#ifdef never
// This skips the JPIMMED_1_02..JPIMMED_1_07 !!!!!
#ifdef JUDYL
    case cJL_JPIMMED_1_01:
    {
#ifdef JUDYL
        Word_t    D_P0;
        Word_t    PjllRaw;
        Word_t    oldValue;
        Pjv_t     Pjv;
#endif /* JUDYL */
        uint8_t *Pjll;
        Word_t    oldIndex = ju_DcdPop0(Pjp);
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
//            Pjpm->jpm_PValue = (Pjv_t) &Pjp->jp_ValueI;
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);   // ^ to Immed_x_01 Value
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjllRaw = j__udyAllocJLL1(2, Pjpm)) == 0) 
            return (-1);
        Pjll = (uint8_t *)P_JLL(PjllRaw);
        Pjv = JL_LEAF1VALUEAREA(Pjll, 2);                                 //  Where is jp_Type for this

//        oldValue = Pjp->jp_ValueI;
        oldValue = ju_ImmVal_01(Pjp);
#endif /* JUDYL */

#ifdef JUDY1
        Pjll = ju_PImmed1(Pjp);
#endif /* JUDY1 */

        if (oldIndex < Index)
        {
            Pjll[0] = oldIndex;
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;              // point to inserted Value
#endif /* JUDYL */
            Pjll[1] = Index;
        }
        else
        {
            Pjll[0] = Index;
            Pjll[1] = oldIndex;
#ifdef JUDYL
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
#ifdef JUDYL
        *Pjv = 0;               // redundent?
        Pjpm->jpm_PValue = Pjv;
        D_P0 = Index & cJU_DCDMASK(1);  /* pop0 = 0 */
//        JU_JPSETADT(Pjp, PjllRaw, D_P0, cJU_JPLEAF1);
        ju_SetBaLPntr(Pjp, PjllRaw);
        ju_SetDcdPop0(Pjp, D_P0);     // pop0 == 0
//        ju_SetLeafPop0(Pjp, Pop1 - 1);
        ju_SetJpType(Pjp, cJU_JPLEAF1);
#endif /* JUDYL */
        return (1);
    }
#endif // JUDYL
#endif  // never

// #ifdef  JUDY1
    case cJU_JPIMMED_1_01:
    {
        uint8_t  *Pjll;
        Word_t    oldIndex = ju_DcdPop0(Pjp);
#ifdef JUDYL
        Word_t    oldValue;
        Word_t    PjvRaw;
        Pjv_t     Pjv;
#endif /* JUDYL */
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
//            Pjpm->jpm_PValue = (Pjv_t) &Pjp->jp_ValueI;
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);   // ^ to Immed_x_01 Value
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjvRaw = j__udyLAllocJV(2, Pjpm)) == 0)
            return (-1);
        Pjv = P_JV(PjvRaw);

//        oldValue = Pjp->jp_ValueI;      // whole word
        oldValue = ju_ImmVal_01(Pjp);

//        Pjp->jp_PValue = PjvRaw;
        ju_SetPjvPntr(Pjp, PjvRaw);     // put ^ to Value area in Pjp
#endif  // JUDYL

//        Pjll = Pjp->jp_1Index1;         // ^ IMMED_1 area
        Pjll = ju_PImmed1(Pjp);         // ^ IMMED_1 area
        if (oldIndex < Index)           // sort
        {
            Pjll[0] = oldIndex;
            Pjll[1] = Index;
#ifdef JUDYL
            Pjv[0] = oldValue;
            Pjv[1] = 0;
            Pjpm->jpm_PValue = &Pjv[1]; // return ^ to new Value
#endif /* JUDYL */
        }
        else
        {
            Pjll[0] = Index;
            Pjll[1] = oldIndex;
#ifdef JUDYL
            Pjv[0] = 0;
            Pjv[1] = oldValue;
            Pjpm->jpm_PValue = Pjv;         // return ^ to new Value
#endif /* JUDYL */
        }
//         Pjp->jp_Type = cJU_JPIMMED_1_02;
        ju_SetJpType(Pjp, cJU_JPIMMED_1_02);

#ifdef JUDYL
#endif /* JUDYL */
        return (1);
    }
//#endif /* JUDY1 */

// 2_01 leads to 2_02, and 3_01 leads to 3_02, except for JudyL 32-bit, where
// they lead to a leaf:
//
// (2_01 => [ 2_02 => 2_03 => [ 2_04..07 => ]] LeafL)
// (3_01 => [ 3_02 =>         [ 3_03..05 => ]] LeafL)
    case cJU_JPIMMED_2_01:
    {
        uint16_t *Pjll;
        Word_t    oldIndex = ju_DcdPop0(Pjp);
#ifdef JUDYL
        Word_t    oldValue;
        Word_t    PjvRaw;
        Pjv_t     Pjv;
#endif /* JUDYL */
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
//            Pjpm->jpm_PValue = (Pjv_t) &Pjp->jp_ValueI;
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);   // ^ to Immed_x_01 Value
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjvRaw = j__udyLAllocJV(2, Pjpm)) == 0)
            return (-1);
        Pjv = P_JV(PjvRaw);

//        oldValue = Pjp->jp_ValueI;
        oldValue = ju_ImmVal_01(Pjp);

//        Pjp->jp_PValue = PjvRaw;
        ju_SetPjvPntr(Pjp, PjvRaw);
#endif /* JUDYL */

//        Pjll = Pjp->jp_1Index2;
        Pjll = ju_PImmed2(Pjp);

        if (oldIndex < Index)           // sorr
        {
            Pjll[0] = oldIndex;
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;              // point to inserted Value
#endif /* JUDYL */
            Pjll[1] = Index;
        }
        else
        {
            Pjll[0] = Index;
            Pjll[1] = oldIndex;
#ifdef JUDYL
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
//        Pjp->jp_Type = (cJU_JPIMMED_2_02);
        ju_SetJpType(Pjp, cJU_JPIMMED_2_02);
#ifdef JUDYL
        *Pjv = 0;
        Pjpm->jpm_PValue = Pjv;
#endif /* JUDYL */
        return (1);
    }
    case cJU_JPIMMED_3_01:
    {
        uint8_t  *Pjll;
        Word_t    oldIndex = ju_DcdPop0(Pjp);
#ifdef JUDYL
        Word_t    oldValue;
        Word_t    PjvRaw;
        Pjv_t     Pjv;
#endif /* JUDYL */
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
//            Pjpm->jpm_PValue = (Pjv_t) &Pjp->jp_ValueI;
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);   // ^ to Immed_x_01 Value
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjvRaw = j__udyLAllocJV(2, Pjpm)) == 0)
            return (-1);
        Pjv = P_JV(PjvRaw);

//        oldValue = Pjp->jp_ValueI;
        oldValue = ju_ImmVal_01(Pjp);

//        Pjp->jp_PValue = PjvRaw;
        ju_SetPjvPntr(Pjp, PjvRaw);
//        Pjll = Pjp->jp_LIndex1;
#endif /* JUDYL */

//        Pjll = Pjp->jp_Index1;
        Pjll = ju_PImmed1(Pjp);
        if (oldIndex < Index)
        {
            JU_COPY3_LONG_TO_PINDEX(Pjll + 0, oldIndex);
            JU_COPY3_LONG_TO_PINDEX(Pjll + (3), Index);
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;              // point to inserted Value
#endif /* JUDYL */
        }
        else
        {
            JU_COPY3_LONG_TO_PINDEX(Pjll + 0, Index);
            JU_COPY3_LONG_TO_PINDEX(Pjll + 3, oldIndex);
#ifdef JUDYL
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
//        Pjp->jp_Type = cJU_JPIMMED_3_02;
        ju_SetJpType(Pjp, cJU_JPIMMED_3_02);
#ifdef JUDYL
        *Pjv = 0;
        Pjpm->jpm_PValue = Pjv;
#endif /* JUDYL */
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
#ifdef JUDYL
        Word_t    D_P0;
        Word_t    PjllRaw;
        Word_t    oldValue;
        Pjv_t     Pjv;
#endif /* JUDYL */
        uint32_t *Pjll;
        Word_t    oldIndex = ju_DcdPop0(Pjp);
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
//            Pjpm->jpm_PValue = (Pjv_t) &Pjp->jp_ValueI;
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);   // ^ to Immed_x_01 Value
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjllRaw = j__udyAllocJLL4(2, Pjpm)) == 0) 
            return (-1);
        Pjll = (uint32_t *)P_JLL(PjllRaw);
        Pjv = JL_LEAF4VALUEAREA(Pjll, 2);                                 //  Where is jp_Type for this

//        oldValue = Pjp->jp_ValueI;
        oldValue = ju_ImmVal_01(Pjp);
#endif /* JUDYL */

#ifdef JUDY1
        Pjll = ju_PImmed4(Pjp);
#endif /* JUDY1 */
        if (oldIndex < Index)
        {
            Pjll[0] = oldIndex;
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;              // point to inserted Value
#endif /* JUDYL */
            Pjll[1] = Index;
        }
        else
        {
            Pjll[0] = Index;
            Pjll[1] = oldIndex;
#ifdef JUDYL
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
#ifdef JUDYL
        *Pjv = 0;               // redundent?
        Pjpm->jpm_PValue = Pjv;
        D_P0 = Index & cJU_DCDMASK(4);  /* pop0 = 0 */
//        JU_JPSETADT(Pjp, PjllRaw, D_P0, cJU_JPLEAF4);
        ju_SetBaLPntr(Pjp, PjllRaw);
        ju_SetDcdPop0(Pjp, D_P0);     // pop0 == 0
//        ju_SetLeafPop0(Pjp, Pop1 - 1);
        ju_SetJpType(Pjp, cJU_JPLEAF4);
#endif /* JUDYL */

#ifdef JUDY1
//         Pjp->jp_Type = cJ1_JPIMMED_4_02;
        ju_SetJpType(Pjp, cJ1_JPIMMED_4_02);
#endif /* JUDY1 */
        return (1);
    }
    case cJU_JPIMMED_5_01:
    {
#ifdef JUDYL
        Word_t    D_P0;
        Word_t    PjllRaw;
        Word_t    oldValue;
        Pjv_t     Pjv;
#endif /* JUDYL */
        uint8_t  *Pjll;
        Word_t    oldIndex = ju_DcdPop0(Pjp);
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
//            Pjpm->jpm_PValue = (Pjv_t) &Pjp->jp_ValueI;
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);   // ^ to Immed_x_01 Value
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjllRaw = j__udyAllocJLL5(2, Pjpm)) == 0)
            return (-1);
        Pjll = (uint8_t *) P_JLL(PjllRaw);
        Pjv = JL_LEAF5VALUEAREA(Pjll, 2);

//        oldValue = Pjp->jp_ValueI;
        oldValue = ju_ImmVal_01(Pjp);
#else /* JUDY1 */
//        Pjll = Pjp->jp_1Index1;
        Pjll = ju_PImmed1(Pjp);
#endif /* JUDY1 */
        if (oldIndex < Index)
        {
            JU_COPY5_LONG_TO_PINDEX(Pjll + 0, oldIndex);
            JU_COPY5_LONG_TO_PINDEX(Pjll + (5), Index);
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;
#endif /* JUDYL */
        }
        else
        {
            JU_COPY5_LONG_TO_PINDEX(Pjll + 0, Index);
            JU_COPY5_LONG_TO_PINDEX(Pjll + 5, oldIndex);
#ifdef JUDYL
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
#ifdef JUDYL
        *Pjv = 0;
        Pjpm->jpm_PValue = Pjv;
        D_P0 = Index & cJU_DCDMASK(5);  /* pop0 = 0 */
//        JU_JPSETADT(Pjp, PjllRaw, D_P0, cJU_JPLEAF5);
        ju_SetBaLPntr(Pjp, PjllRaw);
        ju_SetDcdPop0(Pjp, D_P0);     // pop0 == 0
//        ju_SetLeafPop0(Pjp, Pop1 - 1);
        ju_SetJpType(Pjp, cJU_JPLEAF5);
#else /* JUDY1 */
//        Pjp->jp_Type = cJ1_JPIMMED_5_02;
        ju_SetJpType(Pjp, cJ1_JPIMMED_5_02);
#endif /* JUDY1 */
        return (1);
    }
    case cJU_JPIMMED_6_01:
    {
#ifdef JUDYL
        Word_t    D_P0;
        Word_t    PjllRaw;
        Word_t    oldValue;
        Pjv_t     Pjv;
#endif /* JUDYL */
        uint8_t  *Pjll;
        Word_t    oldIndex = ju_DcdPop0(Pjp);
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
//            Pjpm->jpm_PValue = (Pjv_t) &Pjp->jp_ValueI;
            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);   // ^ to Immed_x_01 Value
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjllRaw = j__udyAllocJLL6(2, Pjpm)) == 0)
            return (-1);
        Pjll = (uint8_t *) P_JLL(PjllRaw);
        Pjv = JL_LEAF6VALUEAREA(Pjll, 2);

//        oldValue = Pjp->jp_ValueI;
        oldValue = ju_ImmVal_01(Pjp);
#else /* JUDY1 */
//        Pjll = Pjp->jp_1Index1;
        Pjll = ju_PImmed1(Pjp);
#endif /* JUDY1 */
        if (oldIndex < Index)
        {
            JU_COPY6_LONG_TO_PINDEX(Pjll + 0, oldIndex);
            JU_COPY6_LONG_TO_PINDEX(Pjll + (6), Index);
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;
#endif /* JUDYL */
        }
        else
        {
            JU_COPY6_LONG_TO_PINDEX(Pjll + 0, Index);
            JU_COPY6_LONG_TO_PINDEX(Pjll + (6), oldIndex);
#ifdef JUDYL
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
#ifdef JUDYL
        *Pjv = 0;
        Pjpm->jpm_PValue = Pjv;
        D_P0 = Index & cJU_DCDMASK(6);  /* pop0 = 0 */
//        JU_JPSETADT(Pjp, PjllRaw, D_P0, cJU_JPLEAF6);
        ju_SetBaLPntr(Pjp, PjllRaw);
        ju_SetDcdPop0(Pjp, D_P0);     // pop0 == 0
//        ju_SetLeafPop0(Pjp, Pop1 - 1);
        ju_SetJpType(Pjp, cJU_JPLEAF6);
#else /* JUDY1 */
//        Pjp->jp_Type = (cJ1_JPIMMED_6_02);
        ju_SetJpType(Pjp, cJ1_JPIMMED_6_02);
#endif /* JUDY1 */
        return (1);
    }
    case cJU_JPIMMED_7_01:
    {
#ifdef JUDYL
        Word_t    D_P0;
        Word_t    PjllRaw;
        Word_t    oldValue;
        Pjv_t     Pjv;
#endif /*  JUDYL */
        uint8_t  *Pjll;
        Word_t    oldIndex = ju_DcdPop0(Pjp);
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = (Pjv_t) &Pjp->jp_ValueI;
//            Pjpm->jpm_PValue = ju_PImmVal_01(Pjp);   // ^ to Immed_x_01 Value
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjllRaw = j__udyAllocJLL7(2, Pjpm)) == 0)
            return (-1);
        Pjll = (uint8_t *) P_JLL(PjllRaw);
        Pjv = JL_LEAF7VALUEAREA(Pjll, 2);

//        oldValue = Pjp->jp_ValueI;
        oldValue = ju_ImmVal_01(Pjp);
#else /* JUDY1 */
//        Pjll = Pjp->jp_1Index1;
        Pjll = ju_PImmed1(Pjp);
#endif /* JUDY1 */
        if (oldIndex < Index)
        {
            JU_COPY7_LONG_TO_PINDEX(Pjll + 0, oldIndex);
            JU_COPY7_LONG_TO_PINDEX(Pjll + (7), Index);
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;
#endif /* JUDYL */
        }
        else
        {
            JU_COPY7_LONG_TO_PINDEX(Pjll + 0, Index);
            JU_COPY7_LONG_TO_PINDEX(Pjll + (7), oldIndex);
#ifdef JUDYL
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
#ifdef JUDYL
        *Pjv = 0;
        Pjpm->jpm_PValue = Pjv;
        D_P0 = Index & cJU_DCDMASK(7);  /* pop0 = 0 */

////   JU_JPSETADT(Pjp, PjllRaw, D_P0, cJU_JPLEAF7);
        ju_SetBaLPntr(Pjp, PjllRaw);
        ju_SetDcdPop0(Pjp, D_P0);     // pop0 == 0
        ju_SetLeafPop0(Pjp, 0);
        ju_SetJpType(Pjp, cJU_JPLEAF7);
#else /* JUDY1 */
//        Pjp->jp_Type = (cJ1_JPIMMED_7_02);
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
#ifdef JUDY1
    case cJU_JPIMMED_1_07:
    case cJ1_JPIMMED_1_08:
    case cJ1_JPIMMED_1_09:
    case cJ1_JPIMMED_1_10:
    case cJ1_JPIMMED_1_11:
    case cJ1_JPIMMED_1_12:
    case cJ1_JPIMMED_1_13:
    case cJ1_JPIMMED_1_14:
#endif /* JUDY1 */
    {
        int       offset;

#ifdef JUDYL
        Word_t    PjvRaw;
        Pjv_t     Pjv;
        Word_t    PjvnewRaw;
        Pjv_t     Pjvnew;
#endif // JUDYL

        exppop1 = ju_Type(Pjp) - (cJU_JPIMMED_1_02) + 2;
        offset  = j__udySearchLeaf1((uint8_t *)ju_PImmed1(Pjp), 
                                exppop1, Index, 1 * 8);
#ifdef JUDYL
//        PjvRaw = Pjp->jp_ValueI;
        PjvRaw = ju_PImmVals(Pjp);    // Raw ^ to Immed Values
        Pjv = P_JV(PjvRaw);
#endif  // JUDYL

        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (0);
        }
        offset = ~offset;
        JU_INSERTINPLACE(ju_PImmed1(Pjp), exppop1, offset, Index);

#ifdef JUDYL
        if (JL_LEAFVGROWINPLACE(exppop1))
        {
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
        }
        else
        {
//          Increase size of value area:
            if ((PjvnewRaw = j__udyLAllocJV(exppop1 + 1, Pjpm)) == 0) 
                return(-1);
            Pjvnew = P_JV(PjvnewRaw);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
//            Pleaf = Pjp->jp_LIndex1;
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            j__udyLFreeJV(PjvRaw, exppop1, Pjpm);
//            Pjp->jp_PValue = PjvnewRaw;
            ju_SetPjvPntr(Pjp, PjvnewRaw);
            Pjv = Pjvnew;
        }
        Pjpm->jpm_PValue = Pjv + offset;                // new value area.
#endif // JUDYL
//        ++(Pjp->jp_Type);
        ju_SetJpType(Pjp, ju_Type(Pjp) + 1);

        return (1);
    }

// cJU_JPIMMED_1_* cases that must cascade:
// (1_01 => 1_02 => 1_03 => [ 1_04 => ... => 1_07 => [ 1_08..15 => ]] LeafL)

#ifdef  JUDY1
    case cJ1_JPIMMED_1_15:
#endif  // JUDY1
#ifdef  JUDYL
    case cJL_JPIMMED_1_07:
#endif  // JUDYL

    {
        uint8_t *Pjll1 = ju_PImmed1(Pjp);

//      Check if duplicate Key
        int offset = j__udySearchLeaf1(Pjll1, cJU_IMMED1_MAXPOP1, Index, 1 * 8);

#ifdef  JUDYL
        Word_t PjvRaw = ju_PImmVals(Pjp);
        Pjv_t  Pjv    = P_JV(PjvRaw);
#endif  // JUDYL

        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (0);         // Duplicate
        }

///////#ifdef  JUDYL
        if (cJU_IMMED1_MAXPOP1 >= cJU_LEAF1_MAXPOP1)    // produce LeafB1?
///////#endif  // JUDYL
        {
//          Then produce a LeafB1

	    Word_t PjlbRaw = j__udyAllocJLB1(Pjpm);
	    if (PjlbRaw == 0)
                return(-1);

	    Pjlb_t Pjlb  = P_JLB(PjlbRaw);

//	    Copy 1 byte index Leaf to bitmap Leaf
	    for (int ii = 0; ii < cJU_IMMED1_MAXPOP1; ii++)
                JU_BITMAPSETL(Pjlb, Pjll1[ii]);
//          Were done for Judy1

#ifdef JUDYL
//#ifdef BMVALUE 
//#else 
//	    Build 4 subexpanse Value leaves from bitmap
	    for (int ii = 0; ii < cJU_NUMSUBEXPL; ii++)
	    {
                Word_t pop1;
//	        Get number of Indexes in subexpanse
	        pop1 = j__udyCountBitsL(JU_JLB_BITMAP(Pjlb, ii));
	        if (pop1)
                {
		    Word_t PjvnewRaw;	// value area of new leaf.
		    Pjv_t  Pjvnew;

		    PjvnewRaw = j__udyLAllocJV(pop1, Pjpm);
		    if (PjvnewRaw == 0)	// out of memory.
		    {
//                      Free prevously allocated LeafVs:
		        while(ii--)
		        {
			    if ((pop1 = j__udyCountBitsL(JU_JLB_BITMAP(Pjlb, ii))))
			    {
			        PjvnewRaw = JL_JLB_PVALUE(Pjlb, ii);
			        j__udyLFreeJV(PjvnewRaw, pop1, Pjpm);
			    }
		        }
//                      Free the bitmap leaf
		        j__udyLFreeJLB1(PjlbRaw,Pjpm);
		        return(-1);
		    }
		    Pjvnew    = P_JV(PjvnewRaw);
		    JU_COPYMEM(Pjvnew, Pjv, pop1);

		    Pjv += pop1;
	    	    JL_JLB_PVALUE(Pjlb, ii) = PjvnewRaw;
	        }
	    }
            j__udyLFreeJV(PjvRaw, cJU_IMMED1_MAXPOP1, Pjpm);
//#endif  // BMVALUE
#endif  // JUDYL

            Word_t DcdP0 = (Index & cJU_DCDMASK(1)) + cJU_IMMED1_MAXPOP1 - 1;
//            JU_JPSETADT(Pjp, PjlbRaw, DcdP0, cJU_JPLEAF_B1);
            ju_SetBaLPntr(Pjp, PjlbRaw);
            ju_SetDcdPop0(Pjp, DcdP0);
            ju_SetLeafPop0(Pjp, cJU_IMMED1_MAXPOP1 - 1);
            ju_SetJpType(Pjp, cJU_JPLEAF_B1);
        }
//////////#ifdef  JUDYL
        else //      no, produce a Leaf1
        {
            Word_t Pjll1newRaw = j__udyAllocJLL1(cJU_IMMED1_MAXPOP1, Pjpm);
            if (Pjll1newRaw == 0)
                return (-1);
            uint8_t *Pjll1new = P_JLL1(Pjll1newRaw);

            JU_COPYMEM(Pjll1new, Pjll1, cJU_IMMED1_MAXPOP1);

            JU_PADLEAF1(Pjll1new, cJU_IMMED1_MAXPOP1);

#ifdef JUDYL
            Pjv_t  Pjvnew = JL_LEAF1VALUEAREA(Pjll1new, cJU_IMMED1_MAXPOP1);
            JU_COPYMEM(Pjvnew, Pjv, cJU_IMMED1_MAXPOP1);
            j__udyLFreeJV(PjvRaw, cJU_IMMED1_MAXPOP1, Pjpm);
#endif /* JUDYL */

            Word_t DcdP0 = (Index & cJU_DCDMASK(1)) + cJU_IMMED1_MAXPOP1 - 1;
//            JU_JPSETADT(Pjp, Pjll1newRaw, DcdP0, cJU_JPLEAF1);
            ju_SetBaLPntr(Pjp, Pjll1newRaw);
            ju_SetDcdPop0(Pjp, DcdP0);
            ju_SetLeafPop0(Pjp, cJU_IMMED1_MAXPOP1 - 1);
            ju_SetJpType(Pjp, cJU_JPLEAF1);
        }
//////#endif  // JUDYL

        goto ContinueInsWalk;          // go insert the Key
    }
// cJU_JPIMMED_[2..7]_[02..15] cases that grow in place or cascade:
//
// (2_01 => [ 2_02 => 2_03 => [ 2_04..07 => ]] LeafL)
    case cJU_JPIMMED_2_02:
#ifdef JUDY1
    case cJU_JPIMMED_2_03:
    case cJ1_JPIMMED_2_04:
    case cJ1_JPIMMED_2_05:
    case cJ1_JPIMMED_2_06:
#endif /* JUDY1 */
    {
#ifdef JUDYL
        uint16_t *Pleaf;
#else /* JUDY1 */
        uint16_t *Pjll;
#endif /* JUDY1 */
        int       offset;
#ifdef JUDYL
        Word_t    PjvRaw;
        Pjv_t     Pjv;
        Word_t    PjvnewRaw;
        Pjv_t     Pjvnew;
#endif /* JUDYL */
        exppop1 = ju_Type(Pjp) - (cJU_JPIMMED_2_02) + 2;
#ifdef JUDYL
//        offset = j__udySearchLeaf2(Pjp->jp_LIndex2, exppop1, Index);
        offset = j__udySearchLeaf2(ju_PImmed2(Pjp), exppop1, Index, 2 * 8);
        PjvRaw = ju_PImmVals(Pjp);
        Pjv = P_JV(PjvRaw);
#else /* JUDY1 */
//        offset = j__udySearchLeaf2(Pjp->jp_1Index2, exppop1, Index);
        offset = j__udySearchLeaf2(ju_PImmed2(Pjp), exppop1, Index, 2 * 8);
#endif /* JUDY1 */
        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (0);
        }
        offset = ~offset;
#ifdef JUDYL
        if ((PjvnewRaw = j__udyLAllocJV(exppop1 + 1, Pjpm)) == 0)
            return (-1);
        Pjvnew = P_JV(PjvnewRaw);
//        Pleaf = Pjp->jp_LIndex2;
        Pleaf = ju_PImmed2(Pjp);
        JU_INSERTINPLACE(Pleaf, exppop1, offset, Index);        /* see TBD above about this: */
        JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
        j__udyLFreeJV(PjvRaw, exppop1, Pjpm);
//        Pjp->jp_PValue = PjvnewRaw;
        ju_SetPjvPntr(Pjp, PjvnewRaw);
        Pjpm->jpm_PValue = Pjvnew + offset;
#else /* JUDY1 */
//        Pjll = Pjp->jp_1Index2;
        Pjll = ju_PImmed2(Pjp);
        JU_INSERTINPLACE(Pjll, exppop1, offset, Index);
#endif /* JUDY1 */
//        ++(Pjp->jp_Type);
        ju_SetJpType(Pjp, ju_Type(Pjp) + 1);
        return (1);
    }
#ifdef JUDYL
    case cJU_JPIMMED_2_03:
#else /* JUDY1 */
    case cJ1_JPIMMED_2_07:
#endif /* JUDY1 */
    {
        Word_t    D_P0;
        Word_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
#ifdef JUDYL
        Word_t    PjvRaw;
        Pjv_t     Pjv;
        Pjv_t     Pjvnew;
//        PjvRaw =  Pjp->jp_PValue;
        PjvRaw = ju_PImmVals(Pjp);
        Pjv = P_JV(PjvRaw);
//        offset = j__udySearchLeaf2(Pjp->jp_LIndex2, 3, Index);
        offset = j__udySearchLeaf2(ju_PImmed2(Pjp), 3, Index, 2 * 8);
#else /* JUDY1 */
//        offset = j__udySearchLeaf2(Pjp->jp_1Index2, 7, Index);
        offset = j__udySearchLeaf2(ju_PImmed2(Pjp), 7, Index, 2 * 8);
#endif /* JUDY1 */
        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (0);
        }
        offset = ~offset;
#ifdef JUDYL
        if ((PjllRaw = j__udyAllocJLL2((3) + 1, Pjpm)) == 0)
#else /* JUDY1 */
        if ((PjllRaw = j__udyAllocJLL2((7) + 1, Pjpm)) == 0)
#endif /* JUDY1 */
            return (-1);
        Pjll = P_JLL(PjllRaw);
#ifdef JUDYL
//        JU_INSERTCOPY((uint16_t *) Pjll, Pjp->jp_LIndex2, 3, offset, Index);
        JU_INSERTCOPY((uint16_t *) Pjll, ju_PImmed2(Pjp), 3, offset, Index);
#else /* JUDY1 */
//        JU_INSERTCOPY((uint16_t *) Pjll, Pjp->jp_1Index2, 7,
        JU_INSERTCOPY((uint16_t *) Pjll, ju_PImmed2(Pjp), 7,
                      offset, Index);
#endif /* JUDY1 */

#ifdef JUDYL
        Pjvnew = JL_LEAF2VALUEAREA(Pjll, (3) + 1);
        JU_INSERTCOPY(Pjvnew, Pjv, 3, offset, 0);
        j__udyLFreeJV(PjvRaw, (3), Pjpm);
        Pjpm->jpm_PValue = Pjvnew + offset;
        D_P0 = (Index & cJU_DCDMASK(2)) + (3) - 1;
#else /* JUDY1 */
        D_P0 = (Index & cJU_DCDMASK(2)) + (7) - 1;
#endif /* JUDY1 */

//        JU_JPSETADT(Pjp, PjllRaw, D_P0, cJU_JPLEAF2);
        ju_SetBaLPntr(Pjp, PjllRaw);
        ju_SetDcdPop0(Pjp, D_P0);
        ju_SetLeafPop0(Pjp, D_P0 & 0x7);
        ju_SetJpType(Pjp, cJU_JPLEAF2);
        return (1);
    }
// (3_01 => [ 3_02 => [ 3_03..05 => ]] LeafL)
    case cJU_JPIMMED_3_02:
#ifdef JUDY1
    case cJ1_JPIMMED_3_03:
    case cJ1_JPIMMED_3_04:
    {
        uint8_t  *Pjll;
        int       offset;
        exppop1 = ju_Type(Pjp) - (cJU_JPIMMED_3_02) + 2;
//        offset = j__udySearchLeaf3(Pjp->jp_1Index1, exppop1, Index);
        offset = j__udySearchLeaf3(ju_PImmed1(Pjp), exppop1, Index, 3 * 8);
        if (offset >= 0)
            return (0);
        offset = ~offset;
//        Pjll = Pjp->jp_1Index1;
        Pjll = ju_PImmed1(Pjp);
        JU_INSERTINPLACE3(Pjll, exppop1, offset, Index);
//        ++(Pjp->jp_Type);
        ju_SetJpType(Pjp, ju_Type(Pjp) + 1);
        return (1);
    }
    case cJ1_JPIMMED_3_05:
#endif /* JUDY1 */
    {
        Word_t    D_P0;
        Word_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
#ifdef JUDYL                    // Dead code?
        Word_t    PjvRaw;
        Pjv_t     Pjv;
        Pjv_t     Pjvnew;
//        PjvRaw =  Pjp->jp_PValue;
        PjvRaw = ju_PImmVals(Pjp);
        Pjv = P_JV(PjvRaw);
//        offset = j__udySearchLeaf3(Pjp->jp_LIndex1, (2), Index);
        offset = j__udySearchLeaf3(ju_PImmed1(Pjp), (2), Index, 3 * 8);
#else /* JUDY1 */
//        offset = j__udySearchLeaf3(Pjp->jp_1Index1, (5), Index);
        offset = j__udySearchLeaf3(ju_PImmed1(Pjp), (5), Index, 3 * 8);
#endif /* JUDY1 */
        if (offset >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
            return (0);
        }
        offset = ~offset;
#ifdef JUDYL
        if ((PjllRaw = j__udyAllocJLL3((2) + 1, Pjpm)) == 0)
#else /* JUDY1 */
        if ((PjllRaw = j__udyAllocJLL3((5) + 1, Pjpm)) == 0)
#endif /* JUDY1 */
            return (-1);
        Pjll = P_JLL(PjllRaw);
#ifdef JUDYL
//        JU_INSERTCOPY3((uint8_t *) Pjll, Pjp->jp_LIndex1, 2, offset, Index);
        JU_INSERTCOPY3((uint8_t *) Pjll, ju_PImmed1(Pjp), 2, offset, Index);
#else /* JUDY1 */
//        JU_INSERTCOPY3((uint8_t *) Pjll, Pjp->jp_1Index1, 5, offset, Index);
        JU_INSERTCOPY3((uint8_t *) Pjll, ju_PImmed1(Pjp), 5, offset, Index);
#endif /* JUDY1 */

#ifdef JUDYL
        Pjvnew = JL_LEAF3VALUEAREA(Pjll, (2) + 1);
        JU_INSERTCOPY(Pjvnew, Pjv, 2, offset, 0);
        j__udyLFreeJV(PjvRaw, (2), Pjpm);
        Pjpm->jpm_PValue = Pjvnew + offset;
        D_P0 = (Index & cJU_DCDMASK(3)) + (2) - 1;
#else /* JUDY1 */
        D_P0 = (Index & cJU_DCDMASK(3)) + (5) - 1;
#endif /* JUDY1 */
//        JU_JPSETADT(Pjp, PjllRaw, D_P0, cJU_JPLEAF3);
        ju_SetBaLPntr(Pjp, PjllRaw);
        ju_SetDcdPop0(Pjp, D_P0);
        ju_SetLeafPop0(Pjp, D_P0 & 0x7);
        ju_SetJpType(Pjp, cJU_JPLEAF3);
        return (1);
    }
#ifdef JUDY1
// (4_01 => [[ 4_02..03 => ]] LeafL)
    case cJ1_JPIMMED_4_02:
    {
        uint32_t *Pjll;
        int       offset;
        exppop1 = ju_Type(Pjp) - (cJ1_JPIMMED_4_02) + 2;
//        offset = j__udySearchLeaf4(Pjp->jp_1Index4, exppop1, Index);
        offset = j__udySearchLeaf4(ju_PImmed4(Pjp), exppop1, Index, 4 * 8);
        if (offset >= 0)
            return (0);
        offset = ~offset;
//        Pjll = Pjp->jp_1Index4;
        Pjll = ju_PImmed4(Pjp);
        JU_INSERTINPLACE(Pjll, exppop1, offset, Index);
//        ++(Pjp->jp_Type);
        ju_SetJpType(Pjp, ju_Type(Pjp) + 1);
        return (1);
    }
    case cJ1_JPIMMED_4_03:
    {
        Word_t    D_P0;
        Word_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
//        offset = j__udySearchLeaf4(Pjp->jp_1Index4, (3), Index);
        offset = j__udySearchLeaf4(ju_PImmed4(Pjp), (3), Index, 4 * 8);
        if (offset >= 0)
            return (0);
        offset = ~offset;
        if ((PjllRaw = j__udyAllocJLL4((3) + 1, Pjpm)) == 0)
            return (-1);
        Pjll = P_JLL(PjllRaw);
//        JU_INSERTCOPY((uint32_t *)Pjll, Pjp->jp_1Index4, 3, offset, Index);
        JU_INSERTCOPY((uint32_t *)Pjll, ju_PImmed4(Pjp), 3, offset, Index);
        D_P0 = (Index & cJU_DCDMASK(4)) + (3) - 1;
//        JU_JPSETADT(Pjp, PjllRaw, D_P0, cJU_JPLEAF4);
        ju_SetBaLPntr(Pjp, PjllRaw);
        ju_SetDcdPop0(Pjp, D_P0);
        ju_SetLeafPop0(Pjp, D_P0 & 0x7);
        ju_SetJpType(Pjp, cJU_JPLEAF4);
        return (1);
    }
// (5_01 => [[ 5_02..03 => ]] LeafL)
    case cJ1_JPIMMED_5_02:
    {
        uint8_t  *Pjll;
        int       offset;
        exppop1 = ju_Type(Pjp) - (cJ1_JPIMMED_5_02) + 2;
//        offset = j__udySearchLeaf5((Pjll_t) Pjp->jp_1Index1, exppop1, Index);
        offset = j__udySearchLeaf5((Pjll_t) ju_PImmed1(Pjp), exppop1, Index, 5 * 8);
        if (offset >= 0)
            return (0);
        offset = ~offset;
//        Pjll = Pjp->jp_1Index1;
        Pjll = ju_PImmed1(Pjp);
        JU_INSERTINPLACE5(Pjll, exppop1, offset, Index);
//        ++(Pjp->jp_Type);
        ju_SetJpType(Pjp, ju_Type(Pjp) + 1);
        return (1);
    }
    case cJ1_JPIMMED_5_03:
    {
        Word_t    D_P0;
        Word_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
//        offset = j__udySearchLeaf5((Pjll_t) Pjp->jp_1Index1, (3), Index);
        offset = j__udySearchLeaf5((Pjll_t) ju_PImmed1(Pjp), (3), Index, 5 * 8);
        if (offset >= 0)
            return (0);
        offset = ~offset;
        if ((PjllRaw = j__udyAllocJLL5((3) + 1, Pjpm)) == 0)
            return (-1);
        Pjll = P_JLL(PjllRaw);
//        JU_INSERTCOPY5((uint8_t *) Pjll, Pjp->jp_1Index1, 3, offset, Index);
        JU_INSERTCOPY5((uint8_t *) Pjll, ju_PImmed1(Pjp), 3, offset, Index);
        D_P0 = (Index & cJU_DCDMASK(5)) + (3) - 1;
//        JU_JPSETADT(Pjp, PjllRaw, D_P0, cJU_JPLEAF5);
        ju_SetBaLPntr(Pjp, PjllRaw);
        ju_SetDcdPop0(Pjp, D_P0);
        ju_SetLeafPop0(Pjp, D_P0 & 0x7);
        ju_SetJpType(Pjp, cJU_JPLEAF5);
        return (1);
    }
// (6_01 => [[ 6_02 => ]] LeafL)
    case cJ1_JPIMMED_6_02:
    {
        Word_t    D_P0;
        Word_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
//        offset = j__udySearchLeaf6((Pjll_t) Pjp->jp_1Index1, (2), Index);
        offset = j__udySearchLeaf6((Pjll_t) ju_PImmed1(Pjp), (2), Index, 6 * 8);
        if (offset >= 0)
            return (0);
        offset = ~offset;
        if ((PjllRaw = j__udyAllocJLL6((2) + 1, Pjpm)) == 0)
            return (-1);
        Pjll = P_JLL(PjllRaw);
//        JU_INSERTCOPY6((uint8_t *) Pjll, Pjp->jp_1Index1, 2, offset, Index);
        JU_INSERTCOPY6((uint8_t *) Pjll, ju_PImmed1(Pjp), 2, offset, Index);
        D_P0 = (Index & cJU_DCDMASK(6)) + (2) - 1;
//        JU_JPSETADT(Pjp, PjllRaw, D_P0, cJU_JPLEAF6);
        ju_SetBaLPntr(Pjp, PjllRaw);
        ju_SetDcdPop0(Pjp, D_P0);
        ju_SetLeafPop0(Pjp, D_P0 & 0x7);
        ju_SetJpType(Pjp, cJU_JPLEAF6);
        return (1);
    }
// (7_01 => [[ 7_02 => ]] LeafL)
    case cJ1_JPIMMED_7_02:
    {
        Word_t    D_P0;
        Word_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
//        offset = j__udySearchLeaf7((Pjll_t) Pjp->jp_1Index1, (2), Index);
        offset = j__udySearchLeaf7((Pjll_t) ju_PImmed1(Pjp), (2), Index, 7 * 8);
        if (offset >= 0)
            return (0);
        offset = ~offset;
        if ((PjllRaw = j__udyAllocJLL7((2) + 1, Pjpm)) == 0)
            return (-1);
        Pjll = P_JLL(PjllRaw);
//        JU_INSERTCOPY7((uint8_t *) Pjll, Pjp->jp_1Index1, 2, offset, Index);
        JU_INSERTCOPY7((uint8_t *) Pjll, ju_PImmed1(Pjp), 2, offset, Index);
        D_P0 = (Index & cJU_DCDMASK(7)) + (2) - 1;
//        JU_JPSETADT(Pjp, PjllRaw, D_P0, cJU_JPLEAF7);
        ju_SetBaLPntr(Pjp, PjllRaw);
        ju_SetDcdPop0(Pjp, D_P0);
        ju_SetLeafPop0(Pjp, D_P0 & 0x7);
        ju_SetJpType(Pjp, cJU_JPLEAF7);
        return (1);
    }
#endif /* JUDY1 */
// ****************************************************************************
// INVALID JP TYPE:
    default:
        JU_SET_ERRNO_NONNULL(Pjpm, JU_ERRNO_CORRUPT);
        return (-1);
    }                                   // switch on JP type
    {
// PROCESS JP -- RECURSIVELY:
//
// For non-Immed JP types, if successful, post-increment the population count
// at this Level.
        retcode = j__udyInsWalk(Pjp, Index, Pjpm);
// Successful insert, increment JP and subexpanse count:
        if ((ju_Type(Pjp) < cJU_JPIMMED_1_01) && (retcode == 1))
        {
//            jp_t      JP;
            Word_t    DcdP0;
//            JP = *Pjp;
            DcdP0 = ju_DcdPop0(Pjp) + 1;
//          only update DcdP0 by + 1
//            JU_JPSETADT(Pjp, JP.Jp_Addr0, DcdP0, ju_Type(&JP));
            ju_SetDcdPop0(Pjp, DcdP0);
        }
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
#ifdef JUDYL
    Pjv_t     Pjv;                      // value area in old leaf.
    Pjv_t     Pjvnew;                   // value area in new leaf.
#endif /* JUDYL */
    Pjpm_t    Pjpm;                     // array-global info.
    int       offset;                   // position in which to store new Index.
    Pjllw_t    Pjllw;


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

    Pjllw = P_JLLW(*PPArray);             // first word of leaf.

// ****************************************************************************
// PROCESS TOP LEVEL "JRP" BRANCHES AND LEAVES:
// ****************************************************************************
// JRPNULL (EMPTY ARRAY):  BUILD A LEAFW WITH ONE INDEX:
// if a valid empty array (null pointer), so create an array of population == 1:
    if (Pjllw == (Pjllw_t) NULL)
    {
        Pjllw_t    Pjllwnew;
        Pjllwnew = j__udyAllocJLLW(1);
#ifdef JUDY1
        JU_CHECKALLOC(Pjllw_t, Pjllwnew, JERRI);
#else /* JUDYL */
        JU_CHECKALLOC(Pjllw_t, Pjllwnew, PPJERR);
#endif /* JUDYL */
        Pjllwnew->jlw_Population0 = 1 - 1;             // pop0 = 0.
        Pjllwnew->jlw_Leaf[0] = Index;
        *PPArray = (Pvoid_t)Pjllwnew;
#ifdef JUDY1
        return (1);
#else /* JUDYL */
        Pjllwnew->jlw_Leaf[1] = 0;                 // value area.
        return ((PPvoid_t) (Pjllwnew->jlw_Leaf + 1));
#endif /* JUDYL */
    }                                   // NULL JRP

#ifdef  TRACEJPI
#ifdef JUDY1
    printf("\n---Judy1Set, Key = 0x%lx, Array Pop1 = %lu\n", 
            (Word_t)Index, (Word_t)(JU_LEAFW_POP0(*PPArray) + 1));
#else /* JUDYL */
    printf("\n---JudyLIns, Key = 0x%lx, Array Pop1 = %lu\n", 
        (Word_t)Index, (Word_t)(JU_LEAFW_POP0(*PPArray) + 1));
#endif /* JUDYL */
#endif  // TRACEJPI

// ****************************************************************************
// LEAFW, OTHER SIZE:
    if (JU_LEAFW_POP0(*PPArray) < cJU_LEAFW_MAXPOP1)    // must be a LEAFW
    {
        Pjllw_t    Pjllwnew;
        Word_t    pop1;
        Pjllw = P_JLLW(*PPArray);         // first word of leaf.
        pop1 = Pjllw->jlw_Population0 + 1;
#ifdef JUDYL
        Pjv = JL_LEAFWVALUEAREA(Pjllw, pop1);
#endif /* JUDYL */
        offset = j__udySearchLeafW(Pjllw->jlw_Leaf, pop1, Index);
        if (offset >= 0)                // index is already valid:
        {
#ifdef JUDY1
            return (0);
#else /* JUDYL */
            return ((PPvoid_t) (Pjv + offset));
#endif /* JUDYL */
        }
        offset = ~offset;
// Insert index in cases where no new memory is needed:
        if (JU_LEAFWGROWINPLACE(pop1))
        {
//            ++Pjllw[0];                  // increase population.
            (Pjllw->jlw_Population0)++;
//             JU_INSERTINPLACE(Pjllw + 1, pop1, offset, Index);
             JU_INSERTINPLACE(Pjllw->jlw_Leaf, pop1, offset, Index);
#ifdef JUDY1
            return (1);
#else /* JUDYL */
            JU_INSERTINPLACE(Pjv, pop1, offset, 0);
            return ((PPvoid_t) (Pjv + offset));
#endif /* JUDYL */
        }
// Insert index into a new, larger leaf:
        if (pop1 < cJU_LEAFW_MAXPOP1)   // can grow to a larger leaf.
        {
            Pjllwnew = j__udyAllocJLLW(pop1 + 1);
#ifdef JUDY1
            JU_CHECKALLOC(Pjllw_t, Pjllwnew, JERRI);
#else /* JUDYL */
            JU_CHECKALLOC(Pjllw_t, Pjllwnew, PPJERR);
#endif /* JUDYL */
//            Pjllwnew[0] = pop1;          // set pop0 in new leaf.
            Pjllwnew->jlw_Population0 = pop1;          // set pop0 in new leaf.
            JU_INSERTCOPY(Pjllwnew->jlw_Leaf, Pjllw->jlw_Leaf, pop1, offset, Index);
#ifdef JUDYL
            Pjvnew = JL_LEAFWVALUEAREA(Pjllwnew, pop1 + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, pop1, offset, 0);
#endif /* JUDYL */
            j__udyFreeJLLW(Pjllw, pop1, NULL);
            *PPArray = (Pvoid_t)Pjllwnew;
#ifdef JUDY1
            return (1);
#else /* JUDYL */
            return ((PPvoid_t) (Pjvnew + offset));
#endif /* JUDYL */
        }
        assert(pop1 == cJU_LEAFW_MAXPOP1);
// Leaf at max size => cannot insert new index, so cascade instead:
//
// Upon cascading from a LEAFW leaf to the first branch, must allocate and
// initialize a JPM.
        Pjpm = j__udyAllocJPM();
#ifdef JUDY1
        JU_CHECKALLOC(Pjpm_t, Pjpm, JERRI);
#else /* JUDYL */
        JU_CHECKALLOC(Pjpm_t, Pjpm, PPJERR);
#endif /* JUDYL */
        (Pjpm->jpm_Pop0) = cJU_LEAFW_MAXPOP1 - 1;

////printf("sizeof(jp_t) = %d, sizeof(jpm_t) = %d, Pjllw = 0x%lx\n", (int)sizeof(jp_t), (int)sizeof(jpm_t), (Word_t)Pjllw);

//        (Pjpm->jpm_JP.Jp_Addr0) = (Word_t)Pjllw;
        Pjp_t Pjpt = &Pjpm->jpm_JP;
        ju_SetBaLPntr(Pjpt, (Word_t)Pjllw);

//        if (j__udyCascadeL(&(Pjpm->jpm_JP), Pjpm) == -1)
        if (j__udyCascadeL(Pjpt, Pjpm) == -1)
        {
            JU_COPY_ERRNO(PJError, Pjpm);
#ifdef JUDY1
            return (JERRI);
#else /* JUDYL */
            return (PPJERR);
#endif /* JUDYL */
        }

////printf("After1 JudyCascadeL: Type = %d\n", ju_Type(Pjpt));

// Note:  No need to pass Pjpm for memory decrement; LEAFW memory is never
// counted in a JPM at all:
        j__udyFreeJLLW(Pjllw, cJU_LEAFW_MAXPOP1, NULL);
        *PPArray = (Pvoid_t)Pjpm;
    }                                   // JU_LEAFW
// ****************************************************************************

#ifdef TRACEJPI
        Pjpm = P_JPM(*PPArray);
        j__udyIndex = Index;
        j__udyPopulation = Pjpm->jpm_Pop0;
#endif

// BRANCH:
    {
        int       retcode;              // really only needed for Judy1, but free for JudyL.
        Pjpm = P_JPM(*PPArray);
        Pjp_t Pjpt = &Pjpm->jpm_JP;
////printf("After2 JudyCascadeL: Type = %d\n", ju_Type(Pjpt));
        retcode = j__udyInsWalk(Pjpt, Index, Pjpm);
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
            ++(Pjpm->jpm_Pop0);         // incr total array popu.

//        assert(((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_L)
//               || ((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_B)
//               || ((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_U));

#ifdef JUDY1
        assert((retcode == 0) || (retcode == 1));
        return (retcode);               // == JU_RET_*_JPM().
#else /* JUDYL */
        assert(Pjpm->jpm_PValue != (Pjv_t) NULL);
        return ((PPvoid_t) Pjpm->jpm_PValue);
#endif /* JUDYL */
    }
 /*NOTREACHED*/
}                        // Judy1Set() / JudyLIns()
