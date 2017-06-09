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
// TBD:  Should some of the assertions here be converted to product code that
// returns JU_ERRNO_CORRUPT?
// Note:  Call JudyCheckPop() even before "already inserted" returns, to catch
// population errors; see fix in 4.84:
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
#ifndef JU_64BIT
extern int j__udy1Cascade1(Pjp_t, Pjpm_t);
#endif
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
extern int j__udyLCascade1(Pjp_t, Pjpm_t);
extern int j__udyLCascade2(Pjp_t, Pjpm_t);
extern int j__udyLCascade3(Pjp_t, Pjpm_t);
extern int j__udyLCascade4(Pjp_t, Pjpm_t);
extern int j__udyLCascade5(Pjp_t, Pjpm_t);
extern int j__udyLCascade6(Pjp_t, Pjpm_t);
extern int j__udyLCascade7(Pjp_t, Pjpm_t);
extern int j__udyLCascadeL(Pjp_t, Pjpm_t);
extern int j__udyLInsertBranch(Pjp_t Pjp, Word_t Index, Word_t Btype, Pjpm_t);
#endif /* JUDYL */

#ifdef  JU_64BIT
//
//                  64 Bit JudyIns.c
//
// ****************************************************************************
// MACROS FOR COMMON CODE:
//
// Check if Index is an outlier to (that is, not a member of) this expanse:
//
// An outlier is an Index in-the-expanse of the slot containing the pointer,
// but not-in-the-expanse of the "narrow" pointer in that slot.  (This means
// the Dcd part of the Index differs from the equivalent part of jp_DcdPopO.)
// Therefore, the remedy is to put a cJU_JPBRANCH_L* between the narrow pointer
// and the object to which it points, and add the outlier Index as an Immediate
// in the cJU_JPBRANCH_L*.  The "trick" is placing the cJU_JPBRANCH_L* at a
// Level that is as low as possible.  This is determined by counting the digits
// in the existing narrow pointer that are the same as the digits in the new
// Index (see j__udyInsertBranch()).
//
// Note:  At some high Levels, cJU_DCDMASK() is all zeros => dead code; assume
// the compiler optimizes this out.
// Check if an Index is already in a leaf or immediate, after calling
// j__udySearchLeaf*() to set Offset:
//
// A non-negative Offset means the Index already exists, so return 0; otherwise
// complement Offset to proceed.
#ifdef JUDYL
// For JudyL, also set the value area pointer in the Pjpm:
#endif /* JUDYL */
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

  ContinueInsWalk:                     // for modifying state without recursing.

#ifdef TRACEJPI
        JudyPrintJP(Pjp, "i", __LINE__);
#endif

    switch (JU_JPTYPE(Pjp))             // entry:  Pjp, Index.
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
        assert((Pjp->jp_Addr) == 0);
        JU_JPSETADT(Pjp, 0, Index,
                    JU_JPTYPE(Pjp) + cJU_JPIMMED_1_01 - cJU_JPNULL1);
#ifdef JUDYL
        // value area is first word of new Immed_01 JP:
        Pjpm->jpm_PValue = (Pjv_t) (&(Pjp->jp_Addr));
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
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            return (j__udyInsertBranch(Pjp, Index, 2, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 2);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 2);
        goto JudyBranchL;
    case cJU_JPBRANCH_L3:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 3))
            return (j__udyInsertBranch(Pjp, Index, 3, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 3);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 3);
        goto JudyBranchL;
    case cJU_JPBRANCH_L4:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 4))
            return (j__udyInsertBranch(Pjp, Index, 4, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 4);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 4);
        goto JudyBranchL;
    case cJU_JPBRANCH_L5:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 5))
            return (j__udyInsertBranch(Pjp, Index, 5, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 5);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 5);
        goto JudyBranchL;
    case cJU_JPBRANCH_L6:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 6))
            return (j__udyInsertBranch(Pjp, Index, 6, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 6);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 6);
        goto JudyBranchL;
    case cJU_JPBRANCH_L7:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 7))
            return (j__udyInsertBranch(Pjp, Index, 7, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 7);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 7);
        goto JudyBranchL;
// Similar to common code above, but no outlier check is needed, and the Immed
// type depends on the word size:
    case cJU_JPBRANCH_L:
    {
        Pjbl_t    PjblRaw;              // pointer to old linear branch.
        Pjbl_t    Pjbl;
        Pjbu_t    PjbuRaw;              // pointer to new uncompressed branch.
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
        PjblRaw = (Pjbl_t) (Pjp->jp_Addr);
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
        offset = j__udySearchLeaf1((Pjll_t) (Pjbl->jbl_Expanse), numJPs, digit);
// If Index is found, offset is into an array of 1..cJU_BRANCHLMAXJPS JPs:
        if (offset >= 0)
        {
            Pjp = (Pjbl->jbl_jp) + offset;      // address of next JP.
            break;                      // continue walk.
        }
// Expanse is missing (not populated) for the passed Index, so insert an Immed
// -- if theres room:
        if (numJPs < cJU_BRANCHLMAXJPS)
        {
            offset = ~offset;           // insertion offset.
            JU_JPSETADT(&newJP, 0, Index,
                        JU_JPTYPE(Pjp) + cJU_JPIMMED_1_01 - cJU_JPBRANCH_L2);
            JU_INSERTINPLACE(Pjbl->jbl_Expanse, numJPs, offset, digit);
            JU_INSERTINPLACE(Pjbl->jbl_jp, numJPs, offset, newJP);
            ++(Pjbl->jbl_NumJPs);
#ifdef JUDYL
            // value area is first word of new Immed 01 JP:
            Pjpm->jpm_PValue = (Pjv_t) ((Pjbl->jbl_jp) + offset);
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
        Pjp->jp_Type += cJU_JPBRANCH_B - cJU_JPBRANCH_L;
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
// Allocate memory for an uncompressed branch:
        if ((PjbuRaw = j__udyAllocJBU(Pjpm)) == (Pjbu_t) NULL)
            return (-1);
        Pjbu = P_JBU(PjbuRaw);
// Set the proper NULL type for most of the uncompressed branchs JPs:
        JU_JPSETADT(&newJP, 0, 0,
                    JU_JPTYPE(Pjp) - cJU_JPBRANCH_L2 + cJU_JPNULL1);
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
        Pjp->jp_Addr = (Word_t)PjbuRaw;
        Pjp->jp_Type += cJU_JPBRANCH_U - cJU_JPBRANCH_L;        // to BranchU.
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
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            return (j__udyInsertBranch(Pjp, Index, 2, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 2);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 2);
        goto JudyBranchB;
    case cJU_JPBRANCH_B3:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 3))
            return (j__udyInsertBranch(Pjp, Index, 3, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 3);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 3);
        goto JudyBranchB;
    case cJU_JPBRANCH_B4:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 4))
            return (j__udyInsertBranch(Pjp, Index, 4, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 4);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 4);
        goto JudyBranchB;
    case cJU_JPBRANCH_B5:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 5))
            return (j__udyInsertBranch(Pjp, Index, 5, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 5);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 5);
        goto JudyBranchB;
    case cJU_JPBRANCH_B6:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 6))
            return (j__udyInsertBranch(Pjp, Index, 6, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 6);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 6);
        goto JudyBranchB;
    case cJU_JPBRANCH_B7:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 7))
            return (j__udyInsertBranch(Pjp, Index, 7, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 7);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 7);
        goto JudyBranchB;
    case cJU_JPBRANCH_B:
    {
        Pjbb_t    Pjbb;                 // pointer to bitmap branch.
        Pjbb_t    PjbbRaw;              // pointer to bitmap branch.
        Pjp_t     Pjp2Raw;              // 1 of N arrays of JPs.
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

#ifndef  noINC
//          If population increment is greater than..  (150):
            if ((Pjpm->jpm_Pop0 - Pjpm->jpm_LastUPop0) > JU_BTOU_POP_INCREMENT)
#endif  // noINC

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

//                    if (exppop1 > JU_BRANCHB_MIN_POP)
//                    {
                        if (j__udyCreateBranchU(Pjp, Pjpm) == -1) return(-1);

// Save global population of last BranchU conversion:

                        Pjpm->jpm_LastUPop0 = Pjpm->jpm_Pop0;

                        goto ContinueInsWalk;
//                    }
                }
            }
#endif  // ! noU

// CONTINUE TO USE BRANCHB:
//
// Get pointer to bitmap branch (JBB):

        PjbbRaw = (Pjbb_t) (Pjp->jp_Addr);
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
        JU_JPSETADT(&newJP, 0, Index,
                    JU_JPTYPE(Pjp) + cJU_JPIMMED_1_01 - cJU_JPBRANCH_B2);
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
#endif /* JUDYL */
        }
// No room, allocate a bigger bitmap branch JP subarray:
        else
        {
            Pjp_t     PjpnewRaw;
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
#endif /* JUDYL */
            }
// New JP subarray; point to cJU_JPIMMED_*_01 and place it:
            else
            {
                assert(JU_JBB_PJP(Pjbb, subexp) == (Pjp_t) NULL);
                Pjp = Pjpnew;
                *Pjp = newJP;           // copy to new memory.
#ifdef JUDYL
                // value area is first word of new Immed 01 JP:
                Pjpm->jpm_PValue = (Pjv_t) (&(Pjp->jp_Addr));
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
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            return (j__udyInsertBranch(Pjp, Index, 2, Pjpm));
        {
            uint8_t   digit = JU_DIGITATSTATE(Index, 2);
            Pjbu_t    P_jbu = P_JBU((Pjp)->jp_Addr);
            (Pjp) = &(P_jbu->jbu_jp[digit]); /* null. */ ;
        };
        break;
    case cJU_JPBRANCH_U3:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 3))
            return (j__udyInsertBranch(Pjp, Index, 3, Pjpm));
        {
            uint8_t   digit = JU_DIGITATSTATE(Index, 3);
            Pjbu_t    P_jbu = P_JBU((Pjp)->jp_Addr);
            (Pjp) = &(P_jbu->jbu_jp[digit]); /* null. */ ;
        };
        break;
    case cJU_JPBRANCH_U4:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 4))
            return (j__udyInsertBranch(Pjp, Index, 4, Pjpm));
        {
            uint8_t   digit = JU_DIGITATSTATE(Index, 4);
            Pjbu_t    P_jbu = P_JBU((Pjp)->jp_Addr);
            (Pjp) = &(P_jbu->jbu_jp[digit]); /* null. */ ;
        };
        break;
    case cJU_JPBRANCH_U5:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 5))
            return (j__udyInsertBranch(Pjp, Index, 5, Pjpm));
        {
            uint8_t   digit = JU_DIGITATSTATE(Index, 5);
            Pjbu_t    P_jbu = P_JBU((Pjp)->jp_Addr);
            (Pjp) = &(P_jbu->jbu_jp[digit]); /* null. */ ;
        };
        break;
    case cJU_JPBRANCH_U6:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 6))
            return (j__udyInsertBranch(Pjp, Index, 6, Pjpm));
        {
            uint8_t   digit = JU_DIGITATSTATE(Index, 6);
            Pjbu_t    P_jbu = P_JBU((Pjp)->jp_Addr);
            (Pjp) = &(P_jbu->jbu_jp[digit]); /* null. */ ;
        };
        break;
    case cJU_JPBRANCH_U7:
    {
        uint8_t   digit = JU_DIGITATSTATE(Index, 7);
        Pjbu_t    P_jbu = P_JBU((Pjp)->jp_Addr);
        (Pjp) = &(P_jbu->jbu_jp[digit]); /* null. */ ;
    };
        break;
    case cJU_JPBRANCH_U:
    {
        uint8_t   digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
        Pjbu_t    P_jbu = P_JBU((Pjp)->jp_Addr);
        (Pjp) = &(P_jbu->jbu_jp[digit]); /* null. */ ;
    };
        break;
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
// 64-bit Judy1 does not have 1-byte leaves:
#ifdef JUDYL
    case cJU_JPLEAF1:
    {
        Pjll_t    PjllRaw;
        uint8_t  *Pleaf;                /* specific type */
        int       offset;
        Pjv_t     Pjv;
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 1))
            return (j__udyInsertBranch(Pjp, Index, 1, Pjpm));
        exppop1 = JU_JPLEAF_POP0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF1_MAXPOP1));
        PjllRaw = (Pjll_t) (Pjp->jp_Addr);
        Pleaf = (uint8_t *) P_JLL(PjllRaw);
        (Pjv) = JL_LEAF1VALUEAREA(Pleaf, exppop1);
        offset = j__udySearchLeaf1(Pleaf, exppop1, Index);
        {
            if ((offset) >= 0)
            {
                Pjpm->jpm_PValue = (Pjv) + (offset);
                return (0);
            }
            (offset) = ~(offset);
        };
        if (JU_LEAF1GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE(Pleaf, exppop1, offset, Index);
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = (Pjv) + (offset);
            return (1);
        }
        if (exppop1 < (cJU_LEAF1_MAXPOP1))      /* grow to new leaf */
        {
            Pjll_t    PjllnewRaw;
            uint8_t  *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL1(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint8_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY(Pleafnew, Pleaf, exppop1, offset, Index);
            {
                Pjv_t     Pjvnew = JL_LEAF1VALUEAREA(Pleafnew, (exppop1) + 1);
                JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
                Pjpm->jpm_PValue = (Pjvnew) + (offset);
            };
            j__udyFreeJLL1(PjllRaw, exppop1, Pjpm);
            (Pjp->jp_Addr) = (Word_t)PjllnewRaw;
            return (1);
        }
        assert(exppop1 == (cJU_LEAF1_MAXPOP1));
        if (j__udyCascade1(Pjp, Pjpm) == -1)
            return (-1);
        j__udyFreeJLL1(PjllRaw, cJU_LEAF1_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    };
#endif /* JUDYL */
    case cJU_JPLEAF2:
    {
        Pjll_t    PjllRaw;
        uint16_t *Pleaf;                /* specific type */
#ifdef JUDY1
        int       offset; /* null. */ ;
#else /* JUDYL */
        int       offset;
        Pjv_t     Pjv;
#endif /* JUDYL */
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            return (j__udyInsertBranch(Pjp, Index, 2, Pjpm));
        exppop1 = JU_JPLEAF_POP0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF2_MAXPOP1));
        PjllRaw = (Pjll_t) (Pjp->jp_Addr);
        Pleaf = (uint16_t *) P_JLL(PjllRaw);
#ifdef JUDYL
        (Pjv) = JL_LEAF2VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */
        offset = j__udySearchLeaf2(Pleaf, exppop1, Index);
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
            (offset) = ~(offset);
        }
        if (JU_LEAF2GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < (cJU_LEAF2_MAXPOP1))      /* grow to new leaf */
        {
            Pjll_t    PjllnewRaw;
            uint16_t *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL2(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint16_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            {
                Pjv_t     Pjvnew = JL_LEAF2VALUEAREA(Pleafnew, (exppop1) + 1);
                JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
                Pjpm->jpm_PValue = (Pjvnew) + (offset);
            }
#endif /* JUDYL */
            j__udyFreeJLL2(PjllRaw, exppop1, Pjpm);
            (Pjp->jp_Addr) = (Word_t)PjllnewRaw;
            return (1);
        }
        assert(exppop1 == (cJU_LEAF2_MAXPOP1));
        if (j__udyCascade2(Pjp, Pjpm) == -1)
            return (-1);
        j__udyFreeJLL2(PjllRaw, cJU_LEAF2_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    }
    case cJU_JPLEAF3:
    {
        Pjll_t    PjllRaw;
        uint8_t  *Pleaf;                /* specific type */
        int       offset;
#ifdef JUDYL
        Pjv_t     Pjv;
#endif /* JUDYL */
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 3))
            return (j__udyInsertBranch(Pjp, Index, 3, Pjpm));
        exppop1 = JU_JPLEAF_POP0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF3_MAXPOP1));
        PjllRaw = (Pjll_t) (Pjp->jp_Addr);
        Pleaf = (uint8_t *) P_JLL(PjllRaw); /* null. */ ;
#ifdef JUDYL
        (Pjv) = JL_LEAF3VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */
        offset = j__udySearchLeaf3(Pleaf, exppop1, Index);
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
            (offset) = ~(offset);
        }
        if (JU_LEAF3GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE3(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < (cJU_LEAF3_MAXPOP1))      /* grow to new leaf */
        {
            Pjll_t    PjllnewRaw;
            uint8_t  *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL3(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint8_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY3(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            {
                Pjv_t     Pjvnew = JL_LEAF3VALUEAREA(Pleafnew, (exppop1) + 1);
                JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
                Pjpm->jpm_PValue = (Pjvnew) + (offset);
            }
#endif /* JUDYL */
            j__udyFreeJLL3(PjllRaw, exppop1, Pjpm);
            (Pjp->jp_Addr) = (Word_t)PjllnewRaw;
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
        Pjll_t    PjllRaw;
        uint32_t *Pleaf;                /* specific type */
        int       offset;
#ifdef JUDYL
        Pjv_t     Pjv;
#endif /* JUDYL */
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 4))
            return (j__udyInsertBranch(Pjp, Index, 4, Pjpm));
        exppop1 = JU_JPLEAF_POP0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF4_MAXPOP1));
        PjllRaw = (Pjll_t) (Pjp->jp_Addr);
        Pleaf = (uint32_t *)P_JLL(PjllRaw);
#ifdef JUDYL
        (Pjv) = JL_LEAF4VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */
        offset = j__udySearchLeaf4(Pleaf, exppop1, Index);
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
            (offset) = ~(offset);
        }
        if (JU_LEAF4GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < (cJU_LEAF4_MAXPOP1))      /* grow to new leaf */
        {
            Pjll_t    PjllnewRaw;
            uint32_t *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL4(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint32_t *)P_JLL(PjllnewRaw);
            JU_INSERTCOPY(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            {
                Pjv_t     Pjvnew = JL_LEAF4VALUEAREA(Pleafnew, (exppop1) + 1);
                JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
                Pjpm->jpm_PValue = (Pjvnew) + (offset);
            }
#endif /* JUDYL */
            j__udyFreeJLL4(PjllRaw, exppop1, Pjpm);
            (Pjp->jp_Addr) = (Word_t)PjllnewRaw;
            return (1);
        }
        assert(exppop1 == (cJU_LEAF4_MAXPOP1));
        if (j__udyCascade4(Pjp, Pjpm) == -1)
            return (-1);
        j__udyFreeJLL4(PjllRaw, cJU_LEAF4_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    };
    case cJU_JPLEAF5:
    {
        Pjll_t    PjllRaw;
        uint8_t  *Pleaf;                /* specific type */
        int       offset;
#ifdef JUDYL
        Pjv_t     Pjv;
#endif /* JUDYL */
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 5))
            return (j__udyInsertBranch(Pjp, Index, 5, Pjpm));
        exppop1 = JU_JPLEAF_POP0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF5_MAXPOP1));
        PjllRaw = (Pjll_t) (Pjp->jp_Addr);
        Pleaf = (uint8_t *) P_JLL(PjllRaw);
#ifdef JUDYL
        (Pjv) = JL_LEAF5VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */
        offset = j__udySearchLeaf5(Pleaf, exppop1, Index);
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
            (offset) = ~(offset);
        }
        if (JU_LEAF5GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE5(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < (cJU_LEAF5_MAXPOP1))      /* grow to new leaf */
        {
            Pjll_t    PjllnewRaw;
            uint8_t  *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL5(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint8_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY5(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            {
                Pjv_t     Pjvnew = JL_LEAF5VALUEAREA(Pleafnew, (exppop1) + 1);
                JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
                Pjpm->jpm_PValue = (Pjvnew) + (offset);
            }
#endif /* JUDYL */
            j__udyFreeJLL5(PjllRaw, exppop1, Pjpm);
            (Pjp->jp_Addr) = (Word_t)PjllnewRaw;
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
        Pjll_t    PjllRaw;
        uint8_t  *Pleaf;                /* specific type */
        int       offset; /* null. */ ;
#ifdef JUDYL
        Pjv_t     Pjv;
#endif /* JUDYL */
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 6))
            return (j__udyInsertBranch(Pjp, Index, 6, Pjpm));
        exppop1 = JU_JPLEAF_POP0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF6_MAXPOP1));
        PjllRaw = (Pjll_t) (Pjp->jp_Addr);
        Pleaf = (uint8_t *) P_JLL(PjllRaw);
#ifdef JUDYL
        (Pjv) = JL_LEAF6VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */
        offset = j__udySearchLeaf6(Pleaf, exppop1, Index);
        if ((offset) >= 0)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
            return (0);
        }
        (offset) = ~(offset);
        if (JU_LEAF6GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE6(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < (cJU_LEAF6_MAXPOP1))      /* grow to new leaf */
        {
            Pjll_t    PjllnewRaw;
            uint8_t  *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL6(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint8_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY6(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            {
                Pjv_t     Pjvnew = JL_LEAF6VALUEAREA(Pleafnew, (exppop1) + 1);
                JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
                Pjpm->jpm_PValue = (Pjvnew) + (offset);
            }
#endif /* JUDYL */
            j__udyFreeJLL6(PjllRaw, exppop1, Pjpm);
            (Pjp->jp_Addr) = (Word_t)PjllnewRaw;
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
        Pjll_t    PjllRaw;
        uint8_t  *Pleaf;                /* specific type */
        int       offset; /* null. */ ;
#ifdef JUDYL
        Pjv_t     Pjv;
#endif /* JUDYL */
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 7))
            return (j__udyInsertBranch(Pjp, Index, 7, Pjpm));
        exppop1 = JU_JPLEAF_POP0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF7_MAXPOP1));
        PjllRaw = (Pjll_t) (Pjp->jp_Addr);
        Pleaf = (uint8_t *) P_JLL(PjllRaw);
#ifdef JUDYL
        (Pjv) = JL_LEAF7VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */
        offset = j__udySearchLeaf7(Pleaf, exppop1, Index);
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
            (offset) = ~(offset);
        }
        if (JU_LEAF7GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE7(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < (cJU_LEAF7_MAXPOP1))      /* grow to new leaf */
        {
            Pjll_t    PjllnewRaw;
            uint8_t  *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL7(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint8_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY7(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            {
                Pjv_t     Pjvnew = JL_LEAF7VALUEAREA(Pleafnew, (exppop1) + 1);
                JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
                Pjpm->jpm_PValue = (Pjvnew) + (offset);
            }
#endif /* JUDYL */
            j__udyFreeJLL7(PjllRaw, exppop1, Pjpm);
            (Pjp->jp_Addr) = (Word_t)PjllnewRaw;
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
// Note:  For JudyL, values are stored in 8 subexpanses, each a linear word
// array of up to 32 values each.

        case cJU_JPLEAF_B1:
        {
#ifdef JUDYL
            Pjv_t     PjvRaw;           // pointer to value part of the leaf.
            Pjv_t     Pjv;              // pointer to value part of the leaf.
            Pjv_t     PjvnewRaw;        // new value area.
            Pjv_t     Pjvnew;           // new value area.
            Word_t    subexp;           // 1 of 8 subexpanses in bitmap.
            Pjlb_t    Pjlb;             // pointer to bitmap part of the leaf.
            BITMAPL_t bitmap;           // for one subexpanse.
            BITMAPL_t bitmask;          // bit set for Indexs digit.
            int       offset;           // of index in value area.
#endif

        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 1))
            return (j__udyInsertBranch(Pjp, Index, 1, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 1);

#ifdef JUDY1

// If Index (bit) is already set, return now:

            if (JU_BITMAPTESTL(P_JLB(Pjp->jp_Addr), Index)) return(0);

// If bitmap is not full, set the new Indexs bit; otherwise convert to a Full:

            if ((exppop1 = JU_JPLEAF_POP0(Pjp) + 1)
              < cJU_JPFULLPOPU1_POP0)
            {
                JU_BITMAPSETL(P_JLB(Pjp->jp_Addr), Index);
            }
            else
            {
                j__udyFreeJLB1((Pjlb_t) (Pjp->jp_Addr), Pjpm);  // free LeafB1.
                Pjp->jp_Type = cJ1_JPFULLPOPU1;
                Pjp->jp_Addr = 0;
            }

#else // JUDYL

// This is very different from Judy1 because of the need to return a value area
// even for an existing Index, or manage the value area for a new Index, and
// because JudyL has no Full type:

// Get last byte to decode from Index, and pointer to bitmap leaf:

            digit = JU_DIGITATSTATE(Index, 1);
            Pjlb  = P_JLB(Pjp->jp_Addr);

// Prepare additional values:

            subexp  = digit / cJU_BITSPERSUBEXPL;       // which subexpanse.
            bitmap  = JU_JLB_BITMAP(Pjlb, subexp);      // subexps 32-bit map.
            PjvRaw  = JL_JLB_PVALUE(Pjlb, subexp);      // corresponding values.
            Pjv     = P_JV(PjvRaw);                     // corresponding values.
            bitmask = JU_BITPOSMASKL(digit);            // mask for Index.
            offset  = j__udyCountBitsL(bitmap & (bitmask - 1)); // of Index.

// If Index already exists, get value pointer and exit:

            if (bitmap & bitmask)
            {
                assert(Pjv);
                Pjpm->jpm_PValue = Pjv + offset;        // existing value.
                return(0);
            }

// Get the total bits set = expanse population of Value area:

            exppop1 = j__udyCountBitsL(bitmap);

// If the value area can grow in place, do it:

            if (JL_LEAFVGROWINPLACE(exppop1))
            {
                JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
                JU_JLB_BITMAP(Pjlb, subexp) |= bitmask;  // set Indexs bit.
                Pjpm->jpm_PValue = Pjv + offset;          // new value area.
                return(1);
            }

// Increase size of value area:

            if ((PjvnewRaw = j__udyLAllocJV(exppop1 + 1, Pjpm))
             == (Pjv_t) NULL) return(-1);
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
                 Pjpm->jpm_PValue   = Pjvnew;
                *(Pjpm->jpm_PValue) = 0;
            }

// Set bit for new Index and place new leaf value area in bitmap:

            JU_JLB_BITMAP(Pjlb, subexp) |= bitmask;
            JL_JLB_PVALUE(Pjlb, subexp)  = PjvnewRaw;

#endif // JUDYL

            return(1);

        } // case


#ifdef JUDY1
// ****************************************************************************
// JPFULLPOPU1:
//
// If Index is not an outlier, then by definition its already set.

        case cJ1_JPFULLPOPU1:

        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 1))
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
//      |                  JUDY1 || JU_64BIT        JUDY1 && JU_64BIT
//      V
// 1_01 => 1_02 => 1_03 => [ 1_04 => ... => 1_07 => [ 1_08..15 => ]] Leaf1 (*)
// 2_01 =>                 [ 2_02 => 2_03 =>        [ 2_04..07 => ]] Leaf2
// 3_01 =>                 [ 3_02 =>                [ 3_03..05 => ]] Leaf3
// JU_64BIT only:
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
        uint8_t  *Pjll;
        Word_t    oldIndex = JU_JPDCDPOP0(Pjp);
#ifdef JUDYL
        Word_t    oldValue;
        Pjv_t     PjvRaw;
        Pjv_t     Pjv;
#endif /* JUDYL */
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = (Pjv_t) Pjp;
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjvRaw = j__udyLAllocJV(2, Pjpm)) == (Pjv_t) NULL)
            return (-1);
        Pjv = P_JV(PjvRaw);
        oldValue = Pjp->jp_Addr;
        (Pjp->jp_Addr) = (Word_t)PjvRaw;
        Pjll = Pjp->jp_LIndex1;
#endif  // JUDYL
#
#ifdef JUDY1
        Pjll = Pjp->jp_1Index1;
#endif /* JUDYL */
        if (oldIndex < Index)
        {
            Pjll[0] = oldIndex;
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;
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
        Pjp->jp_Type = (cJU_JPIMMED_1_02);
#ifdef JUDYL
        *Pjv = 0;
        Pjpm->jpm_PValue = Pjv;
#endif /* JUDYL */
        return (1);
    }
// 2_01 leads to 2_02, and 3_01 leads to 3_02, except for JudyL 32-bit, where
// they lead to a leaf:
//
// (2_01 => [ 2_02 => 2_03 => [ 2_04..07 => ]] LeafL)
// (3_01 => [ 3_02 =>         [ 3_03..05 => ]] LeafL)
    case cJU_JPIMMED_2_01:
    {
        uint16_t *Pjll;
        Word_t    oldIndex = JU_JPDCDPOP0(Pjp);
#ifdef JUDYL
        Word_t    oldValue;
        Pjv_t     PjvRaw;
        Pjv_t     Pjv;
#endif /* JUDYL */
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = (Pjv_t) Pjp;
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjvRaw = j__udyLAllocJV(2, Pjpm)) == (Pjv_t) NULL)
            return (-1);
        Pjv = P_JV(PjvRaw);
        oldValue = Pjp->jp_Addr;
        (Pjp->jp_Addr) = (Word_t)PjvRaw;
//        Pjll = (uint16_t *) (Pjp->jp_LIndex);
        Pjll = Pjp->jp_LIndex2;
#endif /* JUDYL */

#ifdef JUDY1
//        Pjll = (uint16_t *) (Pjp->jp_1Index);
        Pjll = Pjp->jp_1Index2;
#endif /* JUDY1 */
        if (oldIndex < Index)
        {
            Pjll[0] = oldIndex;
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;
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
        Pjp->jp_Type = (cJU_JPIMMED_2_02);
#ifdef JUDYL
        *Pjv = 0;
        Pjpm->jpm_PValue = Pjv;
#endif /* JUDYL */
        return (1);
    }
    case cJU_JPIMMED_3_01:
    {
        uint8_t  *Pjll;
        Word_t    oldIndex = JU_JPDCDPOP0(Pjp);
#ifdef JUDYL
        Word_t    oldValue;
        Pjv_t     PjvRaw;
        Pjv_t     Pjv;
#endif /* JUDYL */
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = (Pjv_t) Pjp;
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjvRaw = j__udyLAllocJV(2, Pjpm)) == (Pjv_t) NULL)
            return (-1);
        Pjv = P_JV(PjvRaw);
        oldValue = Pjp->jp_Addr;
        (Pjp->jp_Addr) = (Word_t)PjvRaw;
        Pjll = Pjp->jp_LIndex1;
#endif /* JUDYL */

#ifdef JUDY1
        Pjll = Pjp->jp_1Index1;
#endif /* JUDY1 */
        if (oldIndex < Index)
        {
            JU_COPY3_LONG_TO_PINDEX(Pjll + 0, oldIndex);
            JU_COPY3_LONG_TO_PINDEX(Pjll + (3), Index);
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;
#endif /* JUDYL */
        }
        else
        {
            JU_COPY3_LONG_TO_PINDEX(Pjll + 0, Index);
            JU_COPY3_LONG_TO_PINDEX(Pjll + (3), oldIndex);
#ifdef JUDYL
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
        Pjp->jp_Type = (cJU_JPIMMED_3_02);
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
        uint32_t *PjllRaw;
        Word_t    oldValue;
        Pjv_t     Pjv;
#endif /* JUDYL */
        uint32_t *Pjll;
        Word_t    oldIndex = JU_JPDCDPOP0(Pjp);
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = (Pjv_t) (&(Pjp->jp_Addr));
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjllRaw =
             (uint32_t *)j__udyAllocJLL4(2, Pjpm)) == (uint32_t *)NULL)
            return (-1);
        Pjll = (uint32_t *)P_JLL(PjllRaw);
        Pjv = JL_LEAF4VALUEAREA(Pjll, 2);
        oldValue = Pjp->jp_Addr;
#endif /* JUDYL */

#ifdef JUDY1
//        Pjll = (uint32_t *)(Pjp->jp_1Index);
        Pjll = Pjp->jp_1Index4;
#endif /* JUDY1 */
        if (oldIndex < Index)
        {
            Pjll[0] = oldIndex;
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;
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
        *Pjv = 0;
        Pjpm->jpm_PValue = Pjv;
        D_P0 = Index & cJU_DCDMASK(4);  /* pop0 = 0 */
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF4);
#endif /* JUDYL */
#ifdef JUDY1
        Pjp->jp_Type = (cJ1_JPIMMED_4_02);
#endif /* JUDY1 */
        return (1);
    }
    case cJU_JPIMMED_5_01:
    {
#ifdef JUDYL
        Word_t    D_P0;
        uint8_t  *PjllRaw;
        Word_t    oldValue;
        Pjv_t     Pjv;
#endif /* JUDYL */
        uint8_t  *Pjll;
        Word_t    oldIndex = JU_JPDCDPOP0(Pjp);
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = (Pjv_t) (&(Pjp->jp_Addr));
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjllRaw =
             (uint8_t *) j__udyAllocJLL5(2, Pjpm)) == (uint8_t *) NULL)
            return (-1);
        Pjll = (uint8_t *) P_JLL(PjllRaw);
        Pjv = JL_LEAF5VALUEAREA(Pjll, 2);
        oldValue = Pjp->jp_Addr;
#else /* JUDY1 */
        Pjll = Pjp->jp_1Index1;
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
            JU_COPY5_LONG_TO_PINDEX(Pjll + (5), oldIndex);
#ifdef JUDYL
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
#ifdef JUDYL
        *Pjv = 0;
        Pjpm->jpm_PValue = Pjv;
        D_P0 = Index & cJU_DCDMASK(5);  /* pop0 = 0 */
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF5);
#else /* JUDY1 */
        Pjp->jp_Type = (cJ1_JPIMMED_5_02);
#endif /* JUDY1 */
        return (1);
    }
    case cJU_JPIMMED_6_01:
    {
#ifdef JUDYL
        Word_t    D_P0;
        uint8_t  *PjllRaw;
        Word_t    oldValue;
        Pjv_t     Pjv;
#endif /* JUDYL */
        uint8_t  *Pjll;
        Word_t    oldIndex = JU_JPDCDPOP0(Pjp);
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = (Pjv_t) (&(Pjp->jp_Addr));
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjllRaw =
             (uint8_t *) j__udyAllocJLL6(2, Pjpm)) == (uint8_t *) NULL)
            return (-1);
        Pjll = (uint8_t *) P_JLL(PjllRaw);
        Pjv = JL_LEAF6VALUEAREA(Pjll, 2);
        oldValue = Pjp->jp_Addr;
#else /* JUDY1 */
        Pjll = Pjp->jp_1Index1;
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
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF6);
#else /* JUDY1 */
        Pjp->jp_Type = (cJ1_JPIMMED_6_02);
#endif /* JUDY1 */
        return (1);
    }
    case cJU_JPIMMED_7_01:
    {
#ifdef JUDYL
        Word_t    D_P0;
        uint8_t  *PjllRaw;
        Word_t    oldValue;
        Pjv_t     Pjv;
#endif /*  JUDYL */
        uint8_t  *Pjll;
        Word_t    oldIndex = JU_JPDCDPOP0(Pjp);
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = (Pjv_t) (&(Pjp->jp_Addr));
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDYL
        if ((PjllRaw =
             (uint8_t *) j__udyAllocJLL7(2, Pjpm)) == (uint8_t *) NULL)
            return (-1);
        Pjll = (uint8_t *) P_JLL(PjllRaw);
        Pjv = JL_LEAF7VALUEAREA(Pjll, 2);
        oldValue = Pjp->jp_Addr;
#else /* JUDY1 */
        Pjll = Pjp->jp_1Index1;
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
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF7);
#else /* JUDY1 */
        Pjp->jp_Type = (cJ1_JPIMMED_7_02);
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
#ifdef JUDYL
        uint8_t  *Pleaf;
#else /* JUDY1 */
        uint8_t  *Pjll;
#endif /* JUDY1 */
        int       offset;
#ifdef JUDYL
        Pjv_t     PjvRaw;
        Pjv_t     Pjv;
        Pjv_t     PjvnewRaw;
        Pjv_t     Pjvnew;
#endif /* JUDYL */
        exppop1 = JU_JPTYPE(Pjp) - (cJU_JPIMMED_1_02) + 2;
#ifdef JUDYL
        offset = j__udySearchLeaf1((Pjll_t) Pjp->jp_LIndex1, exppop1, Index);
        PjvRaw = (Pjv_t) (Pjp->jp_Addr);
        Pjv = P_JV(PjvRaw);
#else /* JUDY1 */
        offset = j__udySearchLeaf1((Pjll_t) Pjp->jp_1Index1, exppop1, Index);
#endif /* JUDY1 */
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
            (offset) = ~(offset);
        }
#ifdef JUDYL
        if ((PjvnewRaw = j__udyLAllocJV(exppop1 + 1, Pjpm)) == (Pjv_t) NULL)
            return (-1);
        Pjvnew = P_JV(PjvnewRaw);
        Pleaf = Pjp->jp_LIndex1;
        JU_INSERTINPLACE(Pleaf, exppop1, offset, Index);        /* see TBD above about this: */
        JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
        j__udyLFreeJV(PjvRaw, exppop1, Pjpm);
        Pjp->jp_Addr = (Word_t)PjvnewRaw;
        Pjpm->jpm_PValue = Pjvnew + offset;
#else /* JUDY1 */
        Pjll = Pjp->jp_1Index1;
        JU_INSERTINPLACE(Pjll, exppop1, offset, Index);
#endif /* JUDY1 */
        ++(Pjp->jp_Type);
        return (1);
    }
// cJU_JPIMMED_1_* cases that must cascade:
//
// (1_01 => 1_02 => 1_03 => [ 1_04 => ... => 1_07 => [ 1_08..15 => ]] LeafL)
#ifdef JUDYL
    case cJU_JPIMMED_1_07:
#else /* JUDY1 */
// Special case, as described above, go directly from Immed to LeafB1:
    case cJ1_JPIMMED_1_15:
#endif /* JUDY1 */
    {
#ifdef JUDYL
        Word_t    D_P0;
        Pjll_t    PjllRaw;
        Pjll_t    Pjll;
#else /* JUDY1 */
        Word_t    DcdP0;
#endif /* JUDY1 */
        int       offset;
#ifdef JUDYL
        Pjv_t     PjvRaw;
        Pjv_t     Pjv;
        Pjv_t     Pjvnew;
        PjvRaw = (Pjv_t) (Pjp->jp_Addr);
        Pjv = P_JV(PjvRaw);
        offset = j__udySearchLeaf1((Pjll_t) Pjp->jp_LIndex1, (7), Index);
#endif /* JUDYL */

#ifdef JUDY1
        Pjlb_t    PjlbRaw;
        Pjlb_t    Pjlb;
        offset = j__udySearchLeaf1((Pjll_t) Pjp->jp_1Index1, 15, Index);
#endif /* JUDY1 */
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
            (offset) = ~(offset);
        }
#ifdef JUDYL
        if ((PjllRaw = j__udyAllocJLL1((7) + 1, Pjpm)) == 0)
            return (-1);
        Pjll = P_JLL(PjllRaw);
        JU_INSERTCOPY((uint8_t *) Pjll, Pjp->jp_LIndex1, 7, offset,
                      Index);
        Pjvnew = JL_LEAF1VALUEAREA(Pjll, (7) + 1);
        JU_INSERTCOPY(Pjvnew, Pjv, 7, offset, 0);
        j__udyLFreeJV(PjvRaw, (7), Pjpm);
        Pjpm->jpm_PValue = Pjvnew + offset;
        D_P0 = (Index & cJU_DCDMASK(1)) + (7) - 1;
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF1);
#endif /* JUDYL */

#ifdef JUDY1
// Create a bitmap leaf (special case for Judy1 64-bit only, see usage):  Set
// new Index in bitmap, copy an Immed1_15 to the bitmap, and set the parent JP
// EXCEPT jp_DcdPopO, leaving any followup to the caller:
        if ((PjlbRaw = j__udyAllocJLB1(Pjpm)) == (Pjlb_t) NULL)
            return (-1);
        Pjlb = P_JLB(PjlbRaw);
        JU_BITMAPSETL(Pjlb, Index);
        for (offset = 0; offset < 15; ++offset)
            JU_BITMAPSETL(Pjlb, Pjp->jp_1Index1[offset]);
//          Set jp_DcdPopO including the current pop0; incremented later:
        DcdP0 = (Index & cJU_DCDMASK(1)) + 15 - 1;
        JU_JPSETADT(Pjp, (Word_t)PjlbRaw, DcdP0, cJU_JPLEAF_B1);
#endif /* JUDY1 */
        return (1);
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
        Pjv_t     PjvRaw;
        Pjv_t     Pjv;
        Pjv_t     PjvnewRaw;
        Pjv_t     Pjvnew;
#endif /* JUDYL */
        exppop1 = JU_JPTYPE(Pjp) - (cJU_JPIMMED_2_02) + 2;
#ifdef JUDYL
        offset = j__udySearchLeaf2(Pjp->jp_LIndex2, exppop1, Index);
        PjvRaw = (Pjv_t) (Pjp->jp_Addr);
        Pjv = P_JV(PjvRaw);
#else /* JUDY1 */
        offset = j__udySearchLeaf2(Pjp->jp_1Index2, exppop1, Index);
#endif /* JUDY1 */
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
            (offset) = ~(offset);
        }
#ifdef JUDYL
        if ((PjvnewRaw = j__udyLAllocJV(exppop1 + 1, Pjpm)) == (Pjv_t) NULL)
            return (-1);
        Pjvnew = P_JV(PjvnewRaw);
        Pleaf = Pjp->jp_LIndex2;
        JU_INSERTINPLACE(Pleaf, exppop1, offset, Index);        /* see TBD above about this: */
        JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
        j__udyLFreeJV(PjvRaw, exppop1, Pjpm);
        Pjp->jp_Addr = (Word_t)PjvnewRaw;
        Pjpm->jpm_PValue = Pjvnew + offset;
#else /* JUDY1 */
        Pjll = Pjp->jp_1Index2;
        JU_INSERTINPLACE(Pjll, exppop1, offset, Index);
#endif /* JUDY1 */
        ++(Pjp->jp_Type);
        return (1);
    }
#ifdef JUDYL
    case cJU_JPIMMED_2_03:
#else /* JUDY1 */
    case cJ1_JPIMMED_2_07:
#endif /* JUDY1 */
    {
        Word_t    D_P0;
        Pjll_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
#ifdef JUDYL
        Pjv_t     PjvRaw;
        Pjv_t     Pjv;
        Pjv_t     Pjvnew;
        PjvRaw = (Pjv_t) (Pjp->jp_Addr);
        Pjv = P_JV(PjvRaw);
        offset = j__udySearchLeaf2(Pjp->jp_LIndex2, 3, Index);
#else /* JUDY1 */
        offset = j__udySearchLeaf2(Pjp->jp_1Index2, 7, Index);
#endif /* JUDY1 */
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = Pjv + offset;
#endif /* JUDYL */
                return (0);
            }
            offset = ~offset;
        }
#ifdef JUDYL
        if ((PjllRaw = j__udyAllocJLL2((3) + 1, Pjpm)) == 0)
#else /* JUDY1 */
        if ((PjllRaw = j__udyAllocJLL2((7) + 1, Pjpm)) == 0)
#endif /* JUDY1 */
            return (-1);
        Pjll = P_JLL(PjllRaw);
#ifdef JUDYL
        JU_INSERTCOPY((uint16_t *) Pjll, Pjp->jp_LIndex2, 3, offset, Index);
#else /* JUDY1 */
        JU_INSERTCOPY((uint16_t *) Pjll, Pjp->jp_1Index2, 7,
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
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF2);
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
        exppop1 = JU_JPTYPE(Pjp) - (cJU_JPIMMED_3_02) + 2;
        offset = j__udySearchLeaf3(Pjp->jp_1Index1, exppop1, Index);
        {
            if ((offset) >= 0)
                return (0);
            (offset) = ~(offset);
        }
        Pjll = Pjp->jp_1Index1;
        JU_INSERTINPLACE3(Pjll, exppop1, offset, Index);
        ++(Pjp->jp_Type);
        return (1);
    }
    case cJ1_JPIMMED_3_05:
#endif /* JUDY1 */
    {
        Word_t    D_P0;
        Pjll_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
#ifdef JUDYL
        Pjv_t     PjvRaw;
        Pjv_t     Pjv;
        Pjv_t     Pjvnew;
        PjvRaw = (Pjv_t) (Pjp->jp_Addr);
        Pjv = P_JV(PjvRaw);
        offset = j__udySearchLeaf3(Pjp->jp_LIndex1, (2), Index);
#else /* JUDY1 */
        offset = j__udySearchLeaf3(Pjp->jp_1Index1, (5), Index);
#endif /* JUDY1 */
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
            (offset) = ~(offset);
        }
#ifdef JUDYL
        if ((PjllRaw = j__udyAllocJLL3((2) + 1, Pjpm)) == 0)
#else /* JUDY1 */
        if ((PjllRaw = j__udyAllocJLL3((5) + 1, Pjpm)) == 0)
#endif /* JUDY1 */
            return (-1);
        Pjll = P_JLL(PjllRaw);
#ifdef JUDYL
        JU_INSERTCOPY3((uint8_t *) Pjll, Pjp->jp_LIndex1, 2,
                       offset, Index);
#else /* JUDY1 */
        JU_INSERTCOPY3((uint8_t *) Pjll, Pjp->jp_1Index1, 5,
                       offset, Index);
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
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF3);
        return (1);
    }
#ifdef JUDY1
// (4_01 => [[ 4_02..03 => ]] LeafL)
    case cJ1_JPIMMED_4_02:
    {
        uint32_t *Pjll;
        int       offset;
        exppop1 = JU_JPTYPE(Pjp) - (cJ1_JPIMMED_4_02) + 2;
        offset = j__udySearchLeaf4(Pjp->jp_1Index4, exppop1, Index);
        {
            if ((offset) >= 0)
                return (0);
            (offset) = ~(offset);
        };
        Pjll = Pjp->jp_1Index4;
        JU_INSERTINPLACE(Pjll, exppop1, offset, Index);
        ++(Pjp->jp_Type);
        return (1);
    }
    case cJ1_JPIMMED_4_03:
    {
        Word_t    D_P0;
        Pjll_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
        offset = j__udySearchLeaf4(Pjp->jp_1Index4, (3), Index);
        {
            if ((offset) >= 0)
                return (0);
            (offset) = ~(offset);
        }
        if ((PjllRaw = j__udyAllocJLL4((3) + 1, Pjpm)) == 0)
            return (-1);
        Pjll = P_JLL(PjllRaw);
        JU_INSERTCOPY((uint32_t *)Pjll, Pjp->jp_1Index4, 3, offset,
                      Index);
        D_P0 = (Index & cJU_DCDMASK(4)) + (3) - 1;
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF4);
        return (1);
    }
// (5_01 => [[ 5_02..03 => ]] LeafL)
    case cJ1_JPIMMED_5_02:
    {
        uint8_t  *Pjll;
        int       offset;
        exppop1 = JU_JPTYPE(Pjp) - (cJ1_JPIMMED_5_02) + 2;
        offset = j__udySearchLeaf5((Pjll_t) Pjp->jp_1Index1, exppop1, Index);
        {
            if ((offset) >= 0)
                return (0);
            (offset) = ~(offset);
        }
        Pjll = Pjp->jp_1Index1;
        JU_INSERTINPLACE5(Pjll, exppop1, offset, Index);
        ++(Pjp->jp_Type);
        return (1);
    }
    case cJ1_JPIMMED_5_03:
    {
        Word_t    D_P0;
        Pjll_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
        offset = j__udySearchLeaf5((Pjll_t) Pjp->jp_1Index1, (3), Index);
        {
            if ((offset) >= 0)
                return (0);
            (offset) = ~(offset);
        };
        if ((PjllRaw = j__udyAllocJLL5((3) + 1, Pjpm)) == 0)
            return (-1);
        Pjll = P_JLL(PjllRaw);
        JU_INSERTCOPY5((uint8_t *) Pjll, Pjp->jp_1Index1, 3,
                       offset, Index);
        D_P0 = (Index & cJU_DCDMASK(5)) + (3) - 1;
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF5);
        return (1);
    }
// (6_01 => [[ 6_02 => ]] LeafL)
    case cJ1_JPIMMED_6_02:
    {
        Word_t    D_P0;
        Pjll_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
        offset = j__udySearchLeaf6((Pjll_t) Pjp->jp_1Index1, (2), Index);
        {
            if ((offset) >= 0)
                return (0);
            (offset) = ~(offset);
        };
        if ((PjllRaw = j__udyAllocJLL6((2) + 1, Pjpm)) == 0)
            return (-1);
        Pjll = P_JLL(PjllRaw);
        JU_INSERTCOPY6((uint8_t *) Pjll, Pjp->jp_1Index1, 2,
                       offset, Index);
        D_P0 = (Index & cJU_DCDMASK(6)) + (2) - 1;
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF6);
        return (1);
    }
// (7_01 => [[ 7_02 => ]] LeafL)
    case cJ1_JPIMMED_7_02:
    {
        Word_t    D_P0;
        Pjll_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
        offset = j__udySearchLeaf7((Pjll_t) Pjp->jp_1Index1, (2), Index);
        {
            if ((offset) >= 0)
                return (0);
            (offset) = ~(offset);
        };
        if ((PjllRaw = j__udyAllocJLL7((2) + 1, Pjpm)) == 0)
            return (-1);
        Pjll = P_JLL(PjllRaw);
        JU_INSERTCOPY7((uint8_t *) Pjll, Pjp->jp_1Index1, 2,
                       offset, Index);
        D_P0 = (Index & cJU_DCDMASK(7)) + (2) - 1;
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF7);
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
        if ((JU_JPTYPE(Pjp) < cJU_JPIMMED_1_01) && (retcode == 1))
        {
            jp_t      JP;
            Word_t    DcdP0;
            JP = *Pjp;
            DcdP0 = JU_JPDCDPOP0(Pjp) + 1;
            JU_JPSETADT(Pjp, JP.jp_Addr, DcdP0, JU_JPTYPE(&JP));
        }
    }
    return (retcode);
}                                       // j__udyInsWalk()

// ****************************************************************************
// J U D Y   1   S E T
// J U D Y   L   I N S
//
// Main entry point.  See the manual entry for details.
#ifdef JUDYL
FUNCTION PPvoid_t JudyLIns(PPvoid_t PPArray,    // in which to insert.
                           Word_t Index,        // to insert.
                           PJError_t PJError    // optional, for returning error info.
    )
#else /* JUDY1 */
FUNCTION int Judy1Set(PPvoid_t PPArray, // in which to insert.
                      Word_t Index,     // to insert.
                      PJError_t PJError // optional, for returning error info.
    )
#endif /* JUDY1 */
{
#ifdef JUDYL
    Pjv_t     Pjv;                      // value area in old leaf.
    Pjv_t     Pjvnew;                   // value area in new leaf.
#endif /* JUDYL */
    Pjpm_t    Pjpm;                     // array-global info.
    int       offset;                   // position in which to store new Index.
    Pjlw_t    Pjlw;

#ifdef  TRACEJPI
#ifdef JUDY1
    printf("\nJudy1Set, Index = 0x%lx\n", (unsigned long)Index);
#else /* JUDYL */
    printf("\nJudyLIns, Index = 0x%lx\n", (unsigned long)Index);
#endif /* JUDYL */
#endif  // TRACEJPI

// CHECK FOR NULL POINTER (error by caller):
    if (PPArray == (PPvoid_t) NULL)
    {
        JU_SET_ERRNO(PJError, JU_ERRNO_NULLPPARRAY);
#ifdef JUDYL
        return (PPJERR);
#else /* JUDY1 */
        return (JERRI);
#endif /* JUDY1 */
    }
    Pjlw = P_JLW(*PPArray);             // first word of leaf.
// ****************************************************************************
// PROCESS TOP LEVEL "JRP" BRANCHES AND LEAVES:
// ****************************************************************************
// JRPNULL (EMPTY ARRAY):  BUILD A LEAFW WITH ONE INDEX:
// if a valid empty array (null pointer), so create an array of population == 1:
    if (Pjlw == (Pjlw_t) NULL)
    {
        Pjlw_t    Pjlwnew;
        Pjlwnew = j__udyAllocJLW(1);
#ifdef JUDYL
        JU_CHECKALLOC(Pjlw_t, Pjlwnew, PPJERR);
#else /* JUDY1 */
        JU_CHECKALLOC(Pjlw_t, Pjlwnew, JERRI);
#endif /* JUDY1 */
        Pjlwnew[0] = 1 - 1;             // pop0 = 0.
        Pjlwnew[1] = Index;
        *PPArray = (Pvoid_t)Pjlwnew;
#ifdef JUDYL
        Pjlwnew[2] = 0;                 // value area.
        return ((PPvoid_t) (Pjlwnew + 2));
#else /* JUDY1 */
        return (1);
#endif /* JUDY1 */
    }                                   // NULL JRP
// ****************************************************************************
// LEAFW, OTHER SIZE:
    if (JU_LEAFW_POP0(*PPArray) < cJU_LEAFW_MAXPOP1)    // must be a LEAFW
    {
        Pjlw_t    Pjlwnew;
        Word_t    pop1;
        Pjlw = P_JLW(*PPArray);         // first word of leaf.
        pop1 = Pjlw[0] + 1;
#ifdef JUDYL
        Pjv = JL_LEAFWVALUEAREA(Pjlw, pop1);
#endif /* JUDYL */
        offset = j__udySearchLeafW(Pjlw + 1, pop1, Index);
        if (offset >= 0)                // index is already valid:
        {
#ifdef JUDYL
            return ((PPvoid_t) (Pjv + offset));
#else /* JUDY1 */
            return (0);
#endif /* JUDY1 */
        }
        offset = ~offset;
// Insert index in cases where no new memory is needed:
        if (JU_LEAFWGROWINPLACE(pop1))
        {
            ++Pjlw[0];                  // increase population.
            JU_INSERTINPLACE(Pjlw + 1, pop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, pop1, offset, 0);
            return ((PPvoid_t) (Pjv + offset));
#else /* JUDY1 */
            return (1);
#endif /* JUDY1 */
        }
// Insert index into a new, larger leaf:
        if (pop1 < cJU_LEAFW_MAXPOP1)   // can grow to a larger leaf.
        {
            Pjlwnew = j__udyAllocJLW(pop1 + 1);
#ifdef JUDYL
            JU_CHECKALLOC(Pjlw_t, Pjlwnew, PPJERR);
#else /* JUDY1 */
            JU_CHECKALLOC(Pjlw_t, Pjlwnew, JERRI);
#endif /* JUDY1 */
            Pjlwnew[0] = pop1;          // set pop0 in new leaf.
            JU_INSERTCOPY(Pjlwnew + 1, Pjlw + 1, pop1, offset, Index);
#ifdef JUDYL
            Pjvnew = JL_LEAFWVALUEAREA(Pjlwnew, pop1 + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, pop1, offset, 0);
#endif /* JUDYL */
            j__udyFreeJLW(Pjlw, pop1, NULL);
            *PPArray = (Pvoid_t)Pjlwnew;
#ifdef JUDYL
            return ((PPvoid_t) (Pjvnew + offset));
#else /* JUDY1 */
            return (1);
#endif /* JUDY1 */
        }
        assert(pop1 == cJU_LEAFW_MAXPOP1);
// Leaf at max size => cannot insert new index, so cascade instead:
//
// Upon cascading from a LEAFW leaf to the first branch, must allocate and
// initialize a JPM.
        Pjpm = j__udyAllocJPM();
#ifdef JUDYL
        JU_CHECKALLOC(Pjpm_t, Pjpm, PPJERR);
#else /* JUDY1 */
        JU_CHECKALLOC(Pjpm_t, Pjpm, JERRI);
#endif /* JUDY1 */
        (Pjpm->jpm_Pop0) = cJU_LEAFW_MAXPOP1 - 1;
        (Pjpm->jpm_JP.jp_Addr) = (Word_t)Pjlw;
        if (j__udyCascadeL(&(Pjpm->jpm_JP), Pjpm) == -1)
        {
            JU_COPY_ERRNO(PJError, Pjpm);
#ifdef JUDYL
            return (PPJERR);
#else /* JUDY1 */
            return (JERRI);
#endif /* JUDY1 */
        }
// Note:  No need to pass Pjpm for memory decrement; LEAFW memory is never
// counted in a JPM at all:
        j__udyFreeJLW(Pjlw, cJU_LEAFW_MAXPOP1, NULL);
        *PPArray = (Pvoid_t)Pjpm;
    }                                   // JU_LEAFW
// ****************************************************************************
// BRANCH:
    {
        int       retcode;              // really only needed for Judy1, but free for JudyL.
        Pjpm = P_JPM(*PPArray);
        retcode = j__udyInsWalk(&(Pjpm->jpm_JP), Index, Pjpm);
        if (retcode == -1)
        {
            JU_COPY_ERRNO(PJError, Pjpm);
#ifdef JUDYL
            return (PPJERR);
#else /* JUDY1 */
            return (JERRI);
#endif /* JUDY1 */
        }
        if (retcode == 1)
            ++(Pjpm->jpm_Pop0);         // incr total array popu.
        assert(((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_L)
               || ((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_B)
               || ((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_U));
#ifdef JUDYL
        assert(Pjpm->jpm_PValue != (Pjv_t) NULL);
        return ((PPvoid_t) Pjpm->jpm_PValue);
#else /* JUDY1 */
        assert((retcode == 0) || (retcode == 1));
        return (retcode);               // == JU_RET_*_JPM().
#endif /* JUDY1 */
    }
 /*NOTREACHED*/}                        // Judy1Set() / JudyLIns()

#else   // JU_32BIT
//
//                  32 Bit JudyIns.c
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

  ContinueInsWalk:                     // for modifying state without recursing.

#ifdef TRACEJPI
        JudyPrintJP(Pjp, "i", __LINE__);
#endif

    switch (JU_JPTYPE(Pjp))             // entry:  Pjp, Index.
    {
// ****************************************************************************
// JPNULL*:
//
// Convert JP in place from current null type to cJU_JPIMMED_*_01 by
// calculating new JP type.
    case cJU_JPNULL1:
    case cJU_JPNULL2:
    case cJU_JPNULL3:
        assert((Pjp->jp_Addr) == 0);
        JU_JPSETADT(Pjp, 0, Index,
                    JU_JPTYPE(Pjp) + cJU_JPIMMED_1_01 - cJU_JPNULL1);
#ifdef JUDYL
        // value area is first word of new Immed_01 JP:
        Pjpm->jpm_PValue = (Pjv_t) (&(Pjp->jp_Addr));
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
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            return (j__udyInsertBranch(Pjp, Index, 2, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 2);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 2);
        goto JudyBranchL;
    case cJU_JPBRANCH_L3:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 3))
            return (j__udyInsertBranch(Pjp, Index, 3, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 3);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 3);
        goto JudyBranchL;
// Similar to common code above, but no outlier check is needed, and the Immed
// type depends on the word size:
    case cJU_JPBRANCH_L:
    {
        Pjbl_t    PjblRaw;              // pointer to old linear branch.
        Pjbl_t    Pjbl;
        Pjbu_t    PjbuRaw;              // pointer to new uncompressed branch.
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
        PjblRaw = (Pjbl_t) (Pjp->jp_Addr);
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
        offset = j__udySearchLeaf1((Pjll_t) (Pjbl->jbl_Expanse), numJPs, digit);
// If Index is found, offset is into an array of 1..cJU_BRANCHLMAXJPS JPs:
        if (offset >= 0)
        {
            Pjp = (Pjbl->jbl_jp) + offset;      // address of next JP.
            break;                      // continue walk.
        }
// Expanse is missing (not populated) for the passed Index, so insert an Immed
// -- if theres room:
        if (numJPs < cJU_BRANCHLMAXJPS)
        {
            offset = ~offset;           // insertion offset.
            JU_JPSETADT(&newJP, 0, Index,
                        JU_JPTYPE(Pjp) + cJU_JPIMMED_1_01 - cJU_JPBRANCH_L2);
            JU_INSERTINPLACE(Pjbl->jbl_Expanse, numJPs, offset, digit);
            JU_INSERTINPLACE(Pjbl->jbl_jp, numJPs, offset, newJP);
            ++(Pjbl->jbl_NumJPs);
#ifdef JUDYL
            // value area is first word of new Immed 01 JP:
            Pjpm->jpm_PValue = (Pjv_t) ((Pjbl->jbl_jp) + offset);
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
        Pjp->jp_Type += cJU_JPBRANCH_B - cJU_JPBRANCH_L;
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
// Allocate memory for an uncompressed branch:
        if ((PjbuRaw = j__udyAllocJBU(Pjpm)) == (Pjbu_t) NULL)
            return (-1);
        Pjbu = P_JBU(PjbuRaw);
// Set the proper NULL type for most of the uncompressed branchs JPs:
        JU_JPSETADT(&newJP, 0, 0,
                    JU_JPTYPE(Pjp) - cJU_JPBRANCH_L2 + cJU_JPNULL1);
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
        Pjp->jp_Addr = (Word_t)PjbuRaw;
        Pjp->jp_Type += cJU_JPBRANCH_U - cJU_JPBRANCH_L;        // to BranchU.
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
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            return (j__udyInsertBranch(Pjp, Index, 2, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 2);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 2);
        goto JudyBranchB;
    case cJU_JPBRANCH_B3:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 3))
            return (j__udyInsertBranch(Pjp, Index, 3, Pjpm));
        (digit) = JU_DIGITATSTATE(Index, 3);
        (exppop1) = JU_JPBRANCH_POP0(Pjp, 3);
        goto JudyBranchB;
    case cJU_JPBRANCH_B:
    {
        Pjbb_t    Pjbb;                 // pointer to bitmap branch.
        Pjbb_t    PjbbRaw;              // pointer to bitmap branch.
        Pjp_t     Pjp2Raw;              // 1 of N arrays of JPs.
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

#ifndef  noINC
//          If population increment is greater than..  (150):
            if ((Pjpm->jpm_Pop0 - Pjpm->jpm_LastUPop0) > JU_BTOU_POP_INCREMENT)
#endif  // noINC

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

//                    if (exppop1 > JU_BRANCHB_MIN_POP)
//                    {
                        if (j__udyCreateBranchU(Pjp, Pjpm) == -1) return(-1);

// Save global population of last BranchU conversion:

                        Pjpm->jpm_LastUPop0 = Pjpm->jpm_Pop0;

                        goto ContinueInsWalk;
//                    }
                }
            }
#endif  // ! noU

// CONTINUE TO USE BRANCHB:
//
// Get pointer to bitmap branch (JBB):
        PjbbRaw = (Pjbb_t) (Pjp->jp_Addr);
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
        JU_JPSETADT(&newJP, 0, Index,
                    JU_JPTYPE(Pjp) + cJU_JPIMMED_1_01 - cJU_JPBRANCH_B2);
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
#endif /* JUDYL */
        }
// No room, allocate a bigger bitmap branch JP subarray:
        else
        {
            Pjp_t     PjpnewRaw;
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
#endif /* JUDYL */
            }
// New JP subarray; point to cJU_JPIMMED_*_01 and place it:
            else
            {
                assert(JU_JBB_PJP(Pjbb, subexp) == (Pjp_t) NULL);
                Pjp = Pjpnew;
                *Pjp = newJP;           // copy to new memory.
#ifdef JUDYL
                // value area is first word of new Immed 01 JP:
                Pjpm->jpm_PValue = (Pjv_t) (&(Pjp->jp_Addr));
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
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            return (j__udyInsertBranch(Pjp, Index, 2, Pjpm));
        {
            uint8_t   digit = JU_DIGITATSTATE(Index, 2);
            Pjbu_t    P_jbu = P_JBU((Pjp)->jp_Addr);
            (Pjp) = &(P_jbu->jbu_jp[digit]); /* null. */ ;
        };
        break;
    case cJU_JPBRANCH_U3:
    {
        uint8_t   digit = JU_DIGITATSTATE(Index, 3);
        Pjbu_t    P_jbu = P_JBU((Pjp)->jp_Addr);
        (Pjp) = &(P_jbu->jbu_jp[digit]); /* null. */ ;
    };
        break;
    case cJU_JPBRANCH_U:
    {
        uint8_t   digit = JU_DIGITATSTATE(Index, cJU_ROOTSTATE);
        Pjbu_t    P_jbu = P_JBU((Pjp)->jp_Addr);
        (Pjp) = &(P_jbu->jbu_jp[digit]); /* null. */ ;
    };
        break;
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
// 64-bit Judy1 does not have 1-byte leaves:
    case cJU_JPLEAF1:
    {
        Pjll_t    PjllRaw;
        uint8_t  *Pleaf;                /* specific type */
        int       offset;
#ifdef JUDYL
        Pjv_t     Pjv;
#endif /* JUDYL */
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 1))
            return (j__udyInsertBranch(Pjp, Index, 1, Pjpm));
        exppop1 = JU_JPLEAF_POP0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF1_MAXPOP1));
        PjllRaw = (Pjll_t) (Pjp->jp_Addr);
        Pleaf = (uint8_t *) P_JLL(PjllRaw);
#ifdef JUDYL
        (Pjv) = JL_LEAF1VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */
        offset = j__udySearchLeaf1(Pleaf, exppop1, Index);
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
            (offset) = ~(offset);
        }
        if (JU_LEAF1GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < (cJU_LEAF1_MAXPOP1))      /* grow to new leaf */
        {
            Pjll_t    PjllnewRaw;
            uint8_t  *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL1(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint8_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            {
                Pjv_t     Pjvnew = JL_LEAF1VALUEAREA(Pleafnew, (exppop1) + 1);
                JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
                Pjpm->jpm_PValue = (Pjvnew) + (offset);
            }
#endif /* JUDYL */
            j__udyFreeJLL1(PjllRaw, exppop1, Pjpm);
            (Pjp->jp_Addr) = (Word_t)PjllnewRaw;
            return (1);
        }
        assert(exppop1 == (cJU_LEAF1_MAXPOP1));
        if (j__udyCascade1(Pjp, Pjpm) == -1)
            return (-1);
        j__udyFreeJLL1(PjllRaw, cJU_LEAF1_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    };
    case cJU_JPLEAF2:
    {
        Pjll_t    PjllRaw;
        uint16_t *Pleaf;                /* specific type */
        int       offset;
#ifdef JUDYL
        Pjv_t     Pjv;
#endif /* JUDYL */
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 2))
            return (j__udyInsertBranch(Pjp, Index, 2, Pjpm));
        exppop1 = JU_JPLEAF_POP0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF2_MAXPOP1));
        PjllRaw = (Pjll_t) (Pjp->jp_Addr);
        Pleaf = (uint16_t *) P_JLL(PjllRaw);
#ifdef JUDYL
        (Pjv) = JL_LEAF2VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */
        offset = j__udySearchLeaf2(Pleaf, exppop1, Index);
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
            (offset) = ~(offset);
        };
        if (JU_LEAF2GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < (cJU_LEAF2_MAXPOP1))      /* grow to new leaf */
        {
            Pjll_t    PjllnewRaw;
            uint16_t *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL2(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint16_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            {
                Pjv_t     Pjvnew = JL_LEAF2VALUEAREA(Pleafnew, (exppop1) + 1);
                JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
                Pjpm->jpm_PValue = (Pjvnew) + (offset);
            }
#endif /* JUDYL */
            j__udyFreeJLL2(PjllRaw, exppop1, Pjpm);
            (Pjp->jp_Addr) = (Word_t)PjllnewRaw;
            return (1);
        }
        assert(exppop1 == (cJU_LEAF2_MAXPOP1));
        if (j__udyCascade2(Pjp, Pjpm) == -1)
            return (-1);
        j__udyFreeJLL2(PjllRaw, cJU_LEAF2_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    };
    case cJU_JPLEAF3:
    {
        Pjll_t    PjllRaw;
        uint8_t  *Pleaf;                /* specific type */
        int       offset;
#ifdef JUDYL
        Pjv_t     Pjv;
#endif /* JUDYL */
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 3))
            return (j__udyInsertBranch(Pjp, Index, 3, Pjpm));
        exppop1 = JU_JPLEAF_POP0(Pjp) + 1;
        assert(exppop1 <= (cJU_LEAF3_MAXPOP1));
        PjllRaw = (Pjll_t) (Pjp->jp_Addr);
        Pleaf = (uint8_t *) P_JLL(PjllRaw);
#ifdef JUDYL
        (Pjv) = JL_LEAF3VALUEAREA(Pleaf, exppop1);
#endif /* JUDYL */
        offset = j__udySearchLeaf3(Pleaf, exppop1, Index);
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
            (offset) = ~(offset);
        }
        if (JU_LEAF3GROWINPLACE(exppop1))       /* add to current leaf */
        {
            JU_INSERTINPLACE3(Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
            return (1);
        }
        if (exppop1 < (cJU_LEAF3_MAXPOP1))      /* grow to new leaf */
        {
            Pjll_t    PjllnewRaw;
            uint8_t  *Pleafnew;
            if ((PjllnewRaw = j__udyAllocJLL3(exppop1 + 1, Pjpm)) == 0)
                return (-1);
            Pleafnew = (uint8_t *) P_JLL(PjllnewRaw);
            JU_INSERTCOPY3(Pleafnew, Pleaf, exppop1, offset, Index);
#ifdef JUDYL
            {
                Pjv_t     Pjvnew = JL_LEAF3VALUEAREA(Pleafnew, (exppop1) + 1);
                JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
                Pjpm->jpm_PValue = (Pjvnew) + (offset);
            }
#endif /* JUDYL */
            j__udyFreeJLL3(PjllRaw, exppop1, Pjpm);
            (Pjp->jp_Addr) = (Word_t)PjllnewRaw;
            return (1);
        }
        assert(exppop1 == (cJU_LEAF3_MAXPOP1));
        if (j__udyCascade3(Pjp, Pjpm) == -1)
            return (-1);
        j__udyFreeJLL3(PjllRaw, cJU_LEAF3_MAXPOP1, Pjpm);
        goto ContinueInsWalk;
    }
// ****************************************************************************
// JPLEAF_B1:
//
// 8 bit Decode | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
//              |SubExpanse |    Bit offset     |
//
// Note:  For JudyL, values are stored in 8 subexpanses, each a linear word
// array of up to 32 values each.
    case cJU_JPLEAF_B1:
    {
#ifdef JUDYL
        Pjv_t     PjvRaw;               // pointer to value part of the leaf.
        Pjv_t     Pjv;                  // pointer to value part of the leaf.
        Pjv_t     PjvnewRaw;            // new value area.
        Pjv_t     Pjvnew;               // new value area.
        Word_t    subexp;               // 1 of 8 subexpanses in bitmap.
        Pjlb_t    Pjlb;                 // pointer to bitmap part of the leaf.
        BITMAPL_t bitmap;               // for one subexpanse.
        BITMAPL_t bitmask;              // bit set for Indexs digit.
        int       offset;               // of index in value area.
#endif /* JUDYL */
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 1))
            return (j__udyInsertBranch(Pjp, Index, 1, Pjpm));
#ifdef JUDY1
// If Index (bit) is already set, return now:
        if (JU_BITMAPTESTL(P_JLB(Pjp->jp_Addr), Index))
            return (0);
// If bitmap is not full, set the new Indexs bit; otherwise convert to a Full:
        if ((exppop1 = JU_JPLEAF_POP0(Pjp) + 1) < cJU_JPFULLPOPU1_POP0)
        {
            JU_BITMAPSETL(P_JLB(Pjp->jp_Addr), Index);
        }
        else
        {
            j__udyFreeJLB1((Pjlb_t) (Pjp->jp_Addr), Pjpm);      // free LeafB1.
            Pjp->jp_Type = cJ1_JPFULLPOPU1;
            Pjp->jp_Addr = 0;
        }
        return (1);
#endif  // JUDY1

#ifdef JUDYL
// This is very different from Judy1 because of the need to return a value area
// even for an existing Index, or manage the value area for a new Index, and
// because JudyL has no Full type:
// Get last byte to decode from Index, and pointer to bitmap leaf:
        digit = JU_DIGITATSTATE(Index, 1);
        Pjlb = P_JLB(Pjp->jp_Addr);
// Prepare additional values:
        subexp = digit / cJU_BITSPERSUBEXPL;    // which subexpanse.
        bitmap = JU_JLB_BITMAP(Pjlb, subexp);   // subexps 32-bit map.
        PjvRaw = JL_JLB_PVALUE(Pjlb, subexp);   // corresponding values.
        Pjv = P_JV(PjvRaw);             // corresponding values.
        bitmask = JU_BITPOSMASKL(digit);        // mask for Index.
        offset = j__udyCountBitsL(bitmap & (bitmask - 1));      // of Index.
// If Index already exists, get value pointer and exit:
        if (bitmap & bitmask)
        {
            assert(Pjv);
            Pjpm->jpm_PValue = Pjv + offset;    // existing value.
            return (0);
        }
// Get the total bits set = expanse population of Value area:
        exppop1 = j__udyCountBitsL(bitmap);
// If the value area can grow in place, do it:
        if (JL_LEAFVGROWINPLACE(exppop1))
        {
            JU_INSERTINPLACE(Pjv, exppop1, offset, 0);
            JU_JLB_BITMAP(Pjlb, subexp) |= bitmask;     // set Indexs bit.
            Pjpm->jpm_PValue = Pjv + offset;    // new value area.
            return (1);
        }
// Increase size of value area:
        if ((PjvnewRaw = j__udyLAllocJV(exppop1 + 1, Pjpm)) == (Pjv_t) NULL)
            return (-1);
        Pjvnew = P_JV(PjvnewRaw);
        if (exppop1)                    // have existing value area.
        {
            assert(Pjv);
            JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
            Pjpm->jpm_PValue = Pjvnew + offset;
            j__udyLFreeJV(PjvRaw, exppop1, Pjpm);       // free old values.
        }
        else                            // first index, new value area:
        {
            Pjpm->jpm_PValue = Pjvnew;
            *(Pjpm->jpm_PValue) = 0;
        }
// Set bit for new Index and place new leaf value area in bitmap:
        JU_JLB_BITMAP(Pjlb, subexp) |= bitmask;
        JL_JLB_PVALUE(Pjlb, subexp) = PjvnewRaw;
        return (1);
#endif /* JUDYL */
    }                                   // case
// ****************************************************************************
#ifdef JUDY1
// JPFULLPOPU1:
//
// If Index is not an outlier, then by definition its already set.
    case cJ1_JPFULLPOPU1:
        if (JU_DCDNOTMATCHINDEX(Index, Pjp, 1))
            return (j__udyInsertBranch(Pjp, Index, 1, Pjpm));
        return (0);
// ****************************************************************************
#endif /* JUDYL */
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
// jp_1Index and jp_LIndex where possible.
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
//      |                  JUDY1 || JU_64BIT        JUDY1 && JU_64BIT
//      V
// 1_01 => 1_02 => 1_03 => [ 1_04 => ... => 1_07 => [ 1_08..15 => ]] Leaf1 (*)
// 2_01 =>                 [ 2_02 => 2_03 =>        [ 2_04..07 => ]] Leaf2
// 3_01 =>                 [ 3_02 =>                [ 3_03..05 => ]] Leaf3
// JU_64BIT only:
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
#ifndef JUDYL
// The "real" *_01 Copy macro:
//
// Trim the high byte off Index, look for a match with the old Index, and if
// none, insert the new Index in the leaf in the correct place, given Pjp and
// Index in the context.
#else /* JUDYL */
// Variations to also handle value areas; see comments above:
#endif /* JUDYL */
//
#ifndef JUDYL
// Note:  A single immediate index lives in the jp_DcdPopO field, but two or
// more reside starting at Pjp->jp_1Index.
#else /* JUDYL */
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
#endif /* JUDYL */
// Handle growth of cJU_JPIMMED_*_[02..15]:
#ifndef JUDYL
// Insert an Index into an immediate JP that has room for more, if the Index is
// not already present; given Pjp, Index, exppop1, Pjv, and Pjpm in the
// context:
//
// Note:  Use this only when the JP format doesnt change, that is, going from
// cJU_JPIMMED_X_0Y to cJU_JPIMMED_X_0Z, where X >= 2 and Y+1 = Z.
#else /* JUDYL */
// Variations to also handle value areas; see comments above:
#endif /* JUDYL */
//
#ifndef JUDYL
// Note:  Incrementing jp_Type is how to increase the Index population.
// Insert an Index into an immediate JP that has no room for more:
#else /* JUDYL */
// For JudyL, Pjv (start of value area) is also in the context.
#endif /* JUDYL */
//
#ifndef JUDYL
// If the Index is not already present, do a cascade (to a leaf); given Pjp,
// Index, Pjv, and Pjpm in the context.
#else /* JUDYL */
// TBD:  This code makes a true but weak assumption that a JudyL 32-bit 2-index
// value area must be copied to a new 3-index value area.  AND it doesnt know
// anything about JudyL 64-bit cases (cJU_JPIMMED_1_0[3-7] only) where the
// value area can grow in place!  However, this should not break it, just slow
// it down.
#endif /* JUDYL */
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
        uint8_t  *Pjll;
        Word_t    oldIndex = JU_JPDCDPOP0(Pjp);
#ifdef JUDYL
        Word_t    oldValue;
        Pjv_t     PjvRaw;
        Pjv_t     Pjv;
#endif /* JUDYL */
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = (Pjv_t) Pjp;
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDY1
        Pjll = Pjp->jp_1Index1;
#endif /* JUDY1 */

#ifdef JUDYL
        if ((PjvRaw = j__udyLAllocJV(2, Pjpm)) == (Pjv_t) NULL)
            return (-1);
        Pjv = P_JV(PjvRaw);
        oldValue = Pjp->jp_Addr;
        (Pjp->jp_Addr) = (Word_t)PjvRaw;
        Pjll = Pjp->jp_LIndex1;
#endif /* JUDYL */
        if (oldIndex < Index)
        {
            Pjll[0] = oldIndex;
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;
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
        Pjp->jp_Type = (cJU_JPIMMED_1_02);
#ifdef JUDYL
        *Pjv = 0;
        Pjpm->jpm_PValue = Pjv;
#endif /* JUDYL */
        return (1);
    }
// 2_01 leads to 2_02, and 3_01 leads to 3_02, except for JudyL 32-bit, where
// they lead to a leaf:
//
// (2_01 => [ 2_02 => 2_03 => [ 2_04..07 => ]] LeafL)
// (3_01 => [ 3_02 =>         [ 3_03..05 => ]] LeafL)
    case cJU_JPIMMED_2_01:
    {
#ifdef JUDYL
        Word_t    D_P0;
        uint16_t *PjllRaw;
#endif /* JUDYL */
        uint16_t *Pjll;
        Word_t    oldIndex = JU_JPDCDPOP0(Pjp);
#ifdef JUDYL
        Word_t    oldValue;
        Pjv_t     Pjv;
#endif /* JUDYL */
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = (Pjv_t) (&(Pjp->jp_Addr));
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDY1
        Pjll = Pjp->jp_1Index2;
#endif /* JUDY1 */

#ifdef JUDYL
        if ((PjllRaw =
             (uint16_t *) j__udyAllocJLL2(2, Pjpm)) == (uint16_t *) NULL)
            return (-1);
        Pjll = (uint16_t *) P_JLL(PjllRaw);
        Pjv = JL_LEAF2VALUEAREA(Pjll, 2);
        oldValue = Pjp->jp_Addr;
#endif /* JUDYL */
        if (oldIndex < Index)
        {
            Pjll[0] = oldIndex;
            Pjll[1] = Index;
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;
#endif /* JUDYL */
        }
        else
        {
            Pjll[0] = Index;
            Pjll[1] = oldIndex;
#ifdef JUDYL
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
#ifdef JUDY1
        Pjp->jp_Type = (cJU_JPIMMED_2_02);
#else /* JUDYL */
        *Pjv = 0;
        Pjpm->jpm_PValue = Pjv;
        D_P0 = Index & cJU_DCDMASK(2);  /* pop0 = 0 */
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF2);
#endif /* JUDYL */
        return (1);
    }
    case cJU_JPIMMED_3_01:
    {
#ifdef JUDYL
        Word_t    D_P0;
        uint8_t  *PjllRaw;
#endif /* JUDYL */
        uint8_t  *Pjll;
        Word_t    oldIndex = JU_JPDCDPOP0(Pjp);
#ifdef JUDYL
        Word_t    oldValue;
        Pjv_t     Pjv;
#endif /* JUDYL */
        Index = JU_TRIMTODCDSIZE(Index);
        if (oldIndex == Index)
        {
#ifdef JUDYL
            Pjpm->jpm_PValue = (Pjv_t) (&(Pjp->jp_Addr));
#endif /* JUDYL */
            return (0);
        }
#ifdef JUDY1
        Pjll = Pjp->jp_1Index1;
#else /* JUDYL */
        if ((PjllRaw =
             (uint8_t *) j__udyAllocJLL3(2, Pjpm)) == (uint8_t *) NULL)
            return (-1);
        Pjll = (uint8_t *) P_JLL(PjllRaw);
        Pjv = JL_LEAF3VALUEAREA(Pjll, 2);
        oldValue = Pjp->jp_Addr;
#endif /* JUDYL */
        if (oldIndex < Index)
        {
            JU_COPY3_LONG_TO_PINDEX(Pjll + 0, oldIndex);
            JU_COPY3_LONG_TO_PINDEX(Pjll + (3), Index);
#ifdef JUDYL
            Pjv[0] = oldValue;
            ++Pjv;
#endif /* JUDYL */
        }
        else
        {
            JU_COPY3_LONG_TO_PINDEX(Pjll + 0, Index);
            JU_COPY3_LONG_TO_PINDEX(Pjll + (3), oldIndex);
#ifdef JUDYL
            Pjv[1] = oldValue;
#endif /* JUDYL */
        }
#ifdef JUDY1
        Pjp->jp_Type = (cJU_JPIMMED_3_02);
#else /* JUDYL */
        *Pjv = 0;
        Pjpm->jpm_PValue = Pjv;
        D_P0 = Index & cJU_DCDMASK(3);  /* pop0 = 0 */
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF3);
#endif /* JUDYL */
        return (1);
    }
// cJU_JPIMMED_1_* cases that can grow in place:
//
// (1_01 => 1_02 => 1_03 => [ 1_04 => ... => 1_07 => [ 1_08..15 => ]] LeafL)
    case cJU_JPIMMED_1_02:
#ifdef JUDY1
    case cJU_JPIMMED_1_03:
    case cJU_JPIMMED_1_04:
    case cJU_JPIMMED_1_05:
    case cJU_JPIMMED_1_06:
#endif /* ! JUDYL */
    {
#ifdef JUDY1
        uint8_t  *Pjll;
#else /* JUDYL */
        uint8_t  *Pleaf;
#endif /* JUDYL */
        int       offset;
#ifdef JUDYL
        Pjv_t     PjvRaw;
        Pjv_t     Pjv;
        Pjv_t     PjvnewRaw;
        Pjv_t     Pjvnew;
#endif /* JUDYL */
        exppop1 = JU_JPTYPE(Pjp) - (cJU_JPIMMED_1_02) + 2;
#ifdef JUDY1
        offset = j__udySearchLeaf1(Pjp->jp_1Index1, exppop1, Index);
#else /* JUDYL */
        offset = j__udySearchLeaf1(Pjp->jp_LIndex1, exppop1, Index);
        PjvRaw = (Pjv_t) (Pjp->jp_Addr);
        Pjv = P_JV(PjvRaw);
#endif /* JUDYL */
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
        }
        (offset) = ~(offset);
#ifdef JUDY1
        Pjll = Pjp->jp_1Index1;
        JU_INSERTINPLACE(Pjll, exppop1, offset, Index);
#else /* JUDYL */
        if ((PjvnewRaw = j__udyLAllocJV(exppop1 + 1, Pjpm)) == (Pjv_t) NULL)
            return (-1);
        Pjvnew = P_JV(PjvnewRaw);
        Pleaf = Pjp->jp_LIndex1;
        JU_INSERTINPLACE(Pleaf, exppop1, offset, Index);        /* see TBD above about this: */
        JU_INSERTCOPY(Pjvnew, Pjv, exppop1, offset, 0);
        j__udyLFreeJV(PjvRaw, exppop1, Pjpm);
        Pjp->jp_Addr = (Word_t)PjvnewRaw;
        Pjpm->jpm_PValue = Pjvnew + offset;
#endif /* JUDYL */
        ++(Pjp->jp_Type);
        return (1);
    }
// cJU_JPIMMED_1_* cases that must cascade:
//
// (1_01 => 1_02 => 1_03 => [ 1_04 => ... => 1_07 => [ 1_08..15 => ]] LeafL)
#ifdef JUDY1
    case cJU_JPIMMED_1_07:
#else /* JUDYL */
    case cJU_JPIMMED_1_03:
#endif /* JUDYL */
    {
        Word_t    D_P0;
        Pjll_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
#ifdef JUDY1
        offset = j__udySearchLeaf1(Pjp->jp_1Index1, (7), Index);
#else /* JUDYL */
        Pjv_t     PjvRaw;
        Pjv_t     Pjv;
        Pjv_t     Pjvnew;
        PjvRaw = (Pjv_t) (Pjp->jp_Addr);
        Pjv = P_JV(PjvRaw);
        offset = j__udySearchLeaf1(Pjp->jp_LIndex1, (3), Index);
#endif /* JUDYL */
        {
            if ((offset) >= 0)
            {
#ifdef JUDYL
                Pjpm->jpm_PValue = (Pjv) + (offset);
#endif /* JUDYL */
                return (0);
            }
        }
        (offset) = ~(offset);
#ifdef JUDY1
        if ((PjllRaw = j__udyAllocJLL1((7) + 1, Pjpm)) == 0)
            return (-1);
#else /* JUDYL */
        if ((PjllRaw = j__udyAllocJLL1((3) + 1, Pjpm)) == 0)
            return (-1);
#endif /* JUDYL */
        Pjll = P_JLL(PjllRaw);
#ifdef JUDY1
        JU_INSERTCOPY((uint8_t *) Pjll, Pjp->jp_1Index1, 7, offset,
                      Index);
#else /* JUDYL */
        JU_INSERTCOPY((uint8_t *) Pjll, Pjp->jp_LIndex1, 3, offset,
                      Index);
#endif /* JUDYL */
#ifdef JUDY1
        D_P0 = (Index & cJU_DCDMASK(1)) + (7) - 1;
#else /* JUDYL */
        Pjvnew = JL_LEAF1VALUEAREA(Pjll, (3) + 1);
        JU_INSERTCOPY(Pjvnew, Pjv, 3, offset, 0);
        j__udyLFreeJV(PjvRaw, (3), Pjpm);
        Pjpm->jpm_PValue = Pjvnew + offset;
        D_P0 = (Index & cJU_DCDMASK(1)) + (3) - 1;
#endif /* JUDYL */
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF1);
        return (1);
    }
// cJU_JPIMMED_[2..7]_[02..15] cases that grow in place or cascade:
//
// (2_01 => [ 2_02 => 2_03 => [ 2_04..07 => ]] LeafL)
#ifdef JUDY1
    case cJU_JPIMMED_2_02:
    {
        uint16_t *Pjll;
        int       offset;
        exppop1 = JU_JPTYPE(Pjp) - (cJU_JPIMMED_2_02) + 2;
        offset = j__udySearchLeaf2((Pjll_t) Pjp->jp_1Index2, exppop1, Index);
        {
            if ((offset) >= 0)
                return (0);
            (offset) = ~(offset);
        }
        Pjll = Pjp->jp_1Index2;
        JU_INSERTINPLACE(Pjll, exppop1, offset, Index);
        ++(Pjp->jp_Type);
        return (1);
    }
    case cJU_JPIMMED_2_03:
    {
        Word_t    D_P0;
        Pjll_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
        offset = j__udySearchLeaf2((Pjll_t) Pjp->jp_1Index2, (3), Index);
        {
            if ((offset) >= 0)
                return (0);
            (offset) = ~(offset);
        }
        if ((PjllRaw = j__udyAllocJLL2((3) + 1, Pjpm)) == 0)
            return (-1);
        Pjll = P_JLL(PjllRaw);
        JU_INSERTCOPY((uint16_t *) Pjll, Pjp->jp_1Index2, 3,
                      offset, Index);
        D_P0 = (Index & cJU_DCDMASK(2)) + (3) - 1;
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF2);
        return (1);
    }
// (3_01 => [ 3_02 => [ 3_03..05 => ]] LeafL)
    case cJU_JPIMMED_3_02:
    {
        Word_t    D_P0;
        Pjll_t    PjllRaw;
        Pjll_t    Pjll;
        int       offset;
        offset = j__udySearchLeaf3(Pjp->jp_1Index1, (2), Index);
        {
            if ((offset) >= 0)
                return (0);
            (offset) = ~(offset);
        };
        if ((PjllRaw = j__udyAllocJLL3((2) + 1, Pjpm)) == 0)
            return (-1);
        Pjll = P_JLL(PjllRaw);
        JU_INSERTCOPY3((uint8_t *) Pjll, Pjp->jp_1Index1, 2, offset, Index);
        D_P0 = (Index & cJU_DCDMASK(3)) + (2) - 1;
        JU_JPSETADT(Pjp, (Word_t)PjllRaw, D_P0, cJU_JPLEAF3);
        return (1);
    };
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
        if ((JU_JPTYPE(Pjp) < cJU_JPIMMED_1_01) && (retcode == 1))
        {
            jp_t      JP;
            Word_t    DcdP0;
            JP = *Pjp;
            DcdP0 = JU_JPDCDPOP0(Pjp) + 1;
            JU_JPSETADT(Pjp, JP.jp_Addr, DcdP0, JU_JPTYPE(&JP));
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
    Pjlw_t    Pjlw;
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
    Pjlw = P_JLW(*PPArray);             // first word of leaf.
// ****************************************************************************
// PROCESS TOP LEVEL "JRP" BRANCHES AND LEAVES:
// ****************************************************************************
// JRPNULL (EMPTY ARRAY):  BUILD A LEAFW WITH ONE INDEX:
// if a valid empty array (null pointer), so create an array of population == 1:
    if (Pjlw == (Pjlw_t) NULL)
    {
        Pjlw_t    Pjlwnew;
        Pjlwnew = j__udyAllocJLW(1);
#ifdef JUDY1
        JU_CHECKALLOC(Pjlw_t, Pjlwnew, JERRI);
#else /* JUDYL */
        JU_CHECKALLOC(Pjlw_t, Pjlwnew, PPJERR);
#endif /* JUDYL */
        Pjlwnew[0] = 1 - 1;             // pop0 = 0.
        Pjlwnew[1] = Index;
        *PPArray = (Pvoid_t)Pjlwnew;
#ifdef JUDY1
        return (1);
#else /* JUDYL */
        Pjlwnew[2] = 0;                 // value area.
        return ((PPvoid_t) (Pjlwnew + 2));
#endif /* JUDYL */
    }                                   // NULL JRP
// ****************************************************************************
// LEAFW, OTHER SIZE:
    if (JU_LEAFW_POP0(*PPArray) < cJU_LEAFW_MAXPOP1)    // must be a LEAFW
    {
        Pjlw_t    Pjlwnew;
        Word_t    pop1;
        Pjlw = P_JLW(*PPArray);         // first word of leaf.
        pop1 = Pjlw[0] + 1;
#ifdef JUDYL
        Pjv = JL_LEAFWVALUEAREA(Pjlw, pop1);
#endif /* JUDYL */
        offset = j__udySearchLeafW(Pjlw + 1, pop1, Index);
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
            ++Pjlw[0];                  // increase population.
            JU_INSERTINPLACE(Pjlw + 1, pop1, offset, Index);
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
            Pjlwnew = j__udyAllocJLW(pop1 + 1);
#ifdef JUDY1
            JU_CHECKALLOC(Pjlw_t, Pjlwnew, JERRI);
#else /* JUDYL */
            JU_CHECKALLOC(Pjlw_t, Pjlwnew, PPJERR);
#endif /* JUDYL */
            Pjlwnew[0] = pop1;          // set pop0 in new leaf.
            JU_INSERTCOPY(Pjlwnew + 1, Pjlw + 1, pop1, offset, Index);
#ifdef JUDYL
            Pjvnew = JL_LEAFWVALUEAREA(Pjlwnew, pop1 + 1);
            JU_INSERTCOPY(Pjvnew, Pjv, pop1, offset, 0);
#endif /* JUDYL */
            j__udyFreeJLW(Pjlw, pop1, NULL);
            *PPArray = (Pvoid_t)Pjlwnew;
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
        (Pjpm->jpm_JP.jp_Addr) = (Word_t)Pjlw;
        if (j__udyCascadeL(&(Pjpm->jpm_JP), Pjpm) == -1)
        {
            JU_COPY_ERRNO(PJError, Pjpm);
#ifdef JUDY1
            return (JERRI);
#else /* JUDYL */
            return (PPJERR);
#endif /* JUDYL */
        }
// Note:  No need to pass Pjpm for memory decrement; LEAFW memory is never
// counted in a JPM at all:
        j__udyFreeJLW(Pjlw, cJU_LEAFW_MAXPOP1, NULL);
        *PPArray = (Pvoid_t)Pjpm;
    }                                   // JU_LEAFW
// ****************************************************************************
// BRANCH:
    {
        int       retcode;              // really only needed for Judy1, but free for JudyL.
        Pjpm = P_JPM(*PPArray);
        retcode = j__udyInsWalk(&(Pjpm->jpm_JP), Index, Pjpm);
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
        assert(((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_L)
               || ((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_B)
               || ((Pjpm->jpm_JP.jp_Type) == cJU_JPBRANCH_U));
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
#endif  // JU_32BIT
