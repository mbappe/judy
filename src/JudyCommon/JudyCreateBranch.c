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

// @(#) $Revision: 4.26 $ $Source: /judy/src/JudyCommon/JudyCreateBranch.c $

// Branch creation functions for Judy1 and JudyL.
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

#ifdef JUDY1
#define j__udyJPPop1 j__udy1JPPop1
#endif // JUDY1

#ifdef  JUDYL
#define j__udyJPPop1 j__udyLJPPop1
#endif // JUDYL

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

FUNCTION Word_t j__udyJPPop1(Pjp_t Pjp)                 // JP to count.
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
        return (0);

    case cJU_JPBRANCH_L2:
    case cJU_JPBRANCH_B2:
    case cJU_JPBRANCH_U2:
        return (ju_BranchPop0(Pjp, 2) + 1);

    case cJU_JPBRANCH_L3:
    case cJU_JPBRANCH_B3:
    case cJU_JPBRANCH_U3:
        return (ju_BranchPop0(Pjp, 3) + 1);

    case cJU_JPBRANCH_L4:
    case cJU_JPBRANCH_B4:
    case cJU_JPBRANCH_U4:
        return (ju_BranchPop0(Pjp, 4) + 1);

    case cJU_JPBRANCH_L5:
    case cJU_JPBRANCH_B5:
    case cJU_JPBRANCH_U5:
        return (ju_BranchPop0(Pjp, 5) + 1);

    case cJU_JPBRANCH_L6:
    case cJU_JPBRANCH_B6:
    case cJU_JPBRANCH_U6:
        return (ju_BranchPop0(Pjp, 6) + 1);

    case cJU_JPBRANCH_L7:
    case cJU_JPBRANCH_B7:
    case cJU_JPBRANCH_U7:
//      256 ^ 6 is as big of Pop possible (for now)
        return (ju_BranchPop0(Pjp, 7) + 1);

    case cJU_JPBRANCH_L8:
    case cJU_JPBRANCH_B8:
    case cJU_JPBRANCH_U8:
//      256 ^ 6 is as big of Pop possible (for now)
        return (ju_BranchPop0(Pjp, 8) + 1);

#ifdef  JUDYL
    case cJU_JPLEAF1:
#endif // JUDYL
    case cJU_JPLEAF2:
    case cJU_JPLEAF3:
    case cJU_JPLEAF4:
    case cJU_JPLEAF5:
    case cJU_JPLEAF6:
    case cJU_JPLEAF7:
        return (ju_LeafPop1(Pjp));
    case cJU_JPLEAF_B1U:
    {
        Word_t    pop1 = ju_LeafPop1(Pjp);
#ifdef  JUDYL
        if (pop1 == 0)
            pop1 = 256;
#endif // JUDYL
        return (pop1);
    }

#ifdef JUDY1
    case cJ1_JPFULLPOPU1:
        return (cJU_JPFULLPOPU1_POP0 + 1);
#endif // JUDY1
    case cJU_JPIMMED_1_01:
    case cJU_JPIMMED_2_01:
    case cJU_JPIMMED_3_01:
    case cJU_JPIMMED_4_01:
    case cJU_JPIMMED_5_01:
    case cJU_JPIMMED_6_01:
    case cJU_JPIMMED_7_01:
        return (1);

    case cJU_JPIMMED_1_02:
        return (2);
    case cJU_JPIMMED_1_03:
        return (3);
    case cJU_JPIMMED_1_04:
        return (4);
    case cJU_JPIMMED_1_05:
        return (5);
    case cJU_JPIMMED_1_06:
        return (6);
    case cJU_JPIMMED_1_07:
        return (7);
    case cJU_JPIMMED_1_08:
        return (8);
#ifdef  JUDY1
    case cJ1_JPIMMED_1_09:
        return (9);
    case cJ1_JPIMMED_1_10:
        return (10);
    case cJ1_JPIMMED_1_11:
        return (11);
    case cJ1_JPIMMED_1_12:
        return (12);
    case cJ1_JPIMMED_1_13:
        return (13);
    case cJ1_JPIMMED_1_14:
        return (14);
    case cJ1_JPIMMED_1_15:
        return (15);
#endif // JUDY1
    case cJU_JPIMMED_2_02:
        return (2);
    case cJU_JPIMMED_2_03:
        return (3);
    case cJU_JPIMMED_2_04:
        return (4);
#ifdef  JUDY1
    case cJ1_JPIMMED_2_05:
        return (5);
    case cJ1_JPIMMED_2_06:
        return (6);
    case cJ1_JPIMMED_2_07:
        return (7);
#endif // JUDY1

    case cJU_JPIMMED_3_02:
        return (2);
#ifdef  JUDY1
    case cJ1_JPIMMED_3_03:
        return (3);
    case cJ1_JPIMMED_3_04:
        return (4);
    case cJ1_JPIMMED_3_05:
        return (5);
#endif // JUDY1

    case cJU_JPIMMED_4_02:
        return (2);

#ifdef  JUDY1
    case cJ1_JPIMMED_4_03:
        return (3);

    case cJ1_JPIMMED_5_02:
    case cJ1_JPIMMED_6_02:
    case cJ1_JPIMMED_7_02:
        return (2);
#endif // JUDY1

    default:
//printf("in j__udyJPPop1 jp_Type = %d Line = %d\n", ju_Type(Pjp), (int)__LINE__);
        assert(FALSE);
        exit(-1);
    /*NOTREACHED*/}                        // j__udyJPPop1()
}

// ****************************************************************************
// J U D Y   C O N V E R T   B R A N C H   L  T O  U
//
// Build a BranchU from a BranchL.  
// Return with Pjp pointing to the BranchL.  Caller must deallocate passed arrays
// , if necessary.
//
// Return -1 if error (details in Pjpm), otherwise return 1.

FUNCTION int j__udyConvertBranchLtoU (Pjp_t Pjp, Pjpm_t Pjpm)
{
    Word_t      PjblRaw = ju_PntrInJp(Pjp);
    Pjbl_t      Pjbl    = P_JBL(PjblRaw);
    Word_t      NumLjp  = Pjbl->jbl_NumJPs;

//  Convert from BranchL to BranchU at the same level
    int jpLevel    = ju_Type(Pjp) - cJU_JPBRANCH_L2 + 2;
    assert((Word_t)jpLevel <= 8);

#ifdef  PCAS
printf("\n-----------------ConvertBranchLtoU ju_BranchPop0 = %ld, NumLjp = %zu, Level = %d\n", ju_BranchPop0(Pjp, jpLevel), NumLjp, jpLevel);
#endif // PCAS

//  Allocate memory for an uncompressed branch:
    Word_t PjbuRaw = j__udyAllocJBU(Pjpm);      // ^ to new uncompressed branch.
    if (PjbuRaw == 0)
        return(-1);
    Pjbu_t Pjbu    = P_JBU(PjbuRaw);

//  Make a null jp_t at proper level
    jp_t        NullJP;
    ju_SetIMM01(&NullJP, 0, 0, cJU_JPNULL1 - 1 + jpLevel - 1);

//  Initialize:  Pre-set uncompressed branch to JPNull*s:
//    for (int jpnU = 0; jpnU < 256; jpnU++)
    for (int jpnU = 0; jpnU < cJU_BRANCHUNUMJPS; jpnU++)
        Pjbu->jbu_jp[jpnU] = NullJP;

#ifndef noBigU
//  clear groupings of 8 populations * 32
    assert((cJU_BRANCHUNUMJPS / 8 * sizeof(Word_t)) == 256);
    memset((void *)Pjbu->jbu_sums8, 0, 256);
#endif // noBigU

//  Copy jp_ts from linear branch to uncompressed branch:
    for (int jpnL= 0; jpnL < NumLjp; jpnL++)
    {
        Pjp_t PjpL   = Pjbl->jbl_jp + jpnL;      // next Pjp
        uint8_t expU = Pjbl->jbl_Expanse[jpnL];  // in jp_t ?

        Pjbu->jbu_jp[expU] = *PjpL;             // copy to BranchU

#ifndef noBigU
//                      expU / 8
        Pjbu->jbu_sums8[expU >> 3] += j__udyJPPop1(PjpL);
#endif // noBigU
    }
//  Copy jp count to BranchU
    Pjbu->jbu_NumJPs = NumLjp;

    j__udyFreeJBL(PjblRaw, Pjpm);               // free old BranchL.

//  Plug new values into parent JP:
//  Note: DcdPop0 do not change in the Pjp
    ju_SetPntrInJp(Pjp, PjbuRaw);
    ju_SetJpType(Pjp, cJU_JPBRANCH_U2 - 2 + jpLevel);
    return(1);
}                                               // j__udyConvertBranchLtoU()                                    



// ****************************************************************************
// J U D Y   C R E A T E   B R A N C H   L
//
// Build a BranchL from an array of JPs and associated 1 byte digits
// (expanses).  Return with Pjp pointing to the BranchL.  Caller must
// deallocate passed arrays, if necessary.
//
// We have no idea what kind of BranchL it is, so caller must set the jp_Type.
//
// Return -1 if error (details in Pjpm), otherwise return 1.

FUNCTION int j__udyPartstoBranchL(
                     Pjp_t Pjp,         // Build JPs from this place
                     Pjp_t   PJPs,      // Array of JPs to put into Bitmap branch
                     uint8_t Exp[],     // Array of expanses to put into bitmap
                     Word_t  ExpCnt,    // Number of above JPs and Expanses
                     Pjpm_t  Pjpm)
{
    Word_t    PjblRaw;                  // pointer to linear branch.
    Pjbl_t    Pjbl;

    assert(ExpCnt <= cJU_BRANCHLMAXJPS);

#ifdef  PCAS
    printf("\n=========== j__udyPartstoBranchL (num jp_t = %d)\n", (int)ExpCnt);
#endif // PCAS

    PjblRaw = j__udyAllocJBL(Pjpm);
    if (PjblRaw == 0)
        return(-1);
    Pjbl = P_JBL(PjblRaw);

//  Build a Linear Branch
    Pjbl->jbl_NumJPs = ExpCnt;

//  Copy from the Linear branch from splayed leaves
    JU_COPYMEM(Pjbl->jbl_Expanse, Exp, ExpCnt);
    JU_COPYMEM(Pjbl->jbl_jp, PJPs, ExpCnt);

//  Pass back new pointer to the Linear branch in JP
    ju_SetPntrInJp(Pjp, PjblRaw);
    return(1);
}                                       // j__udyPartstoBranchL()

// ****************************************************************************
// J U D Y   C R E A T E   B R A N C H   B
//
// Build a BranchB from an array of JPs and associated 1 byte digits
// (expanses).  Return with Pjp pointing to the BranchB.  Caller must
// deallocate passed arrays, if necessary.
//
// We have no idea what kind of BranchB it is, so caller must set the jp_Type.
//
// Return 0 if error (details in Pjpm), otherwise return Pjbb_t RawPntr

FUNCTION Word_t j__udyPartstoBranchB(
                     Pjp_t PJPs,        // Array of JPs to put into Bitmap branch
                     uint8_t Exp[],     // Array of expanses to put into bitmap
                     Word_t ExpCnt,     // Number of above JPs and Expanses
                     Pjpm_t Pjpm)
{
    Word_t    PjbbRaw;                  // pointer to bitmap branch.
    Pjbb_t    Pjbb;
    Word_t    exp, jj;                   // Temps
    uint8_t   CurrSubExp;               // Current sub expanse for BM

#ifdef  PCAS
    printf("\n=========== j__udyPartstoBranchB (num jp_t = %d)\n", (int)ExpCnt);
#endif // PCAS

// This assertion says the number of populated subexpanses is not too large.
// This function is only called when a BranchL overflows to a BranchB or when a
// cascade occurs, meaning a leaf overflows.  Either way ExpCnt cant be very
// large, in fact a lot smaller than cJU_BRANCHBMAXJPS.  (Otherwise a BranchU
// would be used.)  Popping this assertion means something (unspecified) has
// gone very wrong, or else Judys design criteria have changed, although in
// fact there should be no HARM in creating a BranchB with higher actual
// fanout.

    assert(ExpCnt <= cJU_BRANCHBMAXJPS);

//  Get memory for a Bitmap branch
    PjbbRaw = j__udyAllocJBB(Pjpm);
    if (PjbbRaw == 0)
        return(0);

    Pjbb = P_JBB(PjbbRaw);

    Pjbb->jbb_NumJPs = ExpCnt;

//  Get 1st "sub" expanse (0..7) of bitmap branch
    CurrSubExp = Exp[0] / cJU_BITSPERSUBEXPB;

//  Index thru all 1 byte sized expanses:

    for (jj = exp = 0; exp <= ExpCnt; exp++)
    {
        Word_t    SubExp;               // Cannot be a uint8_t

//      Make sure we cover the last one
        if (exp == ExpCnt)
        {
            SubExp = cJU_ALLONES;       // Force last one
        }
        else
        {
//          Calculate the "sub" expanse of the byte expanse
            SubExp = Exp[exp] / cJU_BITSPERSUBEXPB;      // Bits 5..7.

//          Set the bit that represents the expanse in Exp[]
            JU_JBB_BITMAP(Pjbb, SubExp) |= JU_BITPOSMASKB(Exp[exp]);
        }
//      Check if a new "sub" expanse range needed
        if (SubExp != CurrSubExp)
        {
//          Get number of JPs in this sub expanse
            Word_t    NumJP = exp - jj;
            Word_t    PjpRaw;
            Pjp_t     Pjp;

            PjpRaw = j__udyAllocJBBJP(NumJP, Pjpm);

            if (PjpRaw == 0)            // out of memory.
            {
//              Free any previous allocations:
                while (CurrSubExp--)
                {
                    NumJP = j__udyCountBitsB(JU_JBB_BITMAP(Pjbb, CurrSubExp));
                    if (NumJP)
                    {
                        j__udyFreeJBBJP(JU_JBB_PJP(Pjbb, CurrSubExp), NumJP, Pjpm);
                    }
                }
                j__udyFreeJBB(PjbbRaw, Pjpm);
                return (0);             // out of memory
            }
            Pjp = P_JP(PjpRaw);

//          Place the array of JPs in bitmap branch:
            JU_JBB_PJP(Pjbb, CurrSubExp) = PjpRaw;

//          Copy the JPs to new allocation:
            JU_COPYMEM(Pjp, PJPs + jj, NumJP);

//          On to the next bitmap branch "sub" expanse:
            jj = exp;
            CurrSubExp = SubExp;
        }
    }                                   // for each 1-byte expanse
//      Pjp->Jp_Addr0 = PjbbRaw;
// now done by caller        ju_SetPntrInJp(Pjp, PjbbRaw);
    return (PjbbRaw);
}                                       // j__udyPartstoBranchB()

// ****************************************************************************
// J U D Y   C R E A T E   B R A N C H   U
//
// Build a BranchU from a BranchB.  Return with Pjp pointing to the BranchU.
// Free the BranchB and its JP subarrays.
//
// Return -1 if error (details in Pjpm), otherwise return 1.

FUNCTION int j__udyConvertBranchBtoU(Pjp_t Pjp, Pjpm_t Pjpm)
{

//  Allocate memory for a BranchU:
    Word_t    PjbuRaw = j__udyAllocJBU(Pjpm);
    if (PjbuRaw == 0)
        return (-1);
    Pjbu_t    Pjbu = P_JBU(PjbuRaw);

//  find (Level = 2..8) of branchB
    uint8_t   jpLevel = ju_Type(Pjp) - cJU_JPBRANCH_B2 + 2;

//  create a JPNull at the correct level - 1
    jp_t      JPNull;
    ju_SetIMM01(&JPNull, 0, 0, cJU_JPNULL1 + jpLevel - 2);

//  Initialize: branchU to all cJU_JPNULLs:
//    for (int jpnU = 0; jpnU < cJU_BRANCHUNUMJPS; jpnU++)
//        Pjbu->jbu_jp[jpnU] = JPNull;

#ifndef noBigU
//  Zero out the (256/8 = 32) subexp counts
//  memset((void *)Pjbu->jbu_sums8, 0, cJU_BRANCHUNUMJPS / 8 * sizeof(Word_t));
    memset((void *)Pjbu->jbu_sums8, 0, 256);
#endif // noBigU

    Word_t    PjbbRaw   = ju_PntrInJp(Pjp);
    Pjbb_t    Pjbb      = P_JBB(PjbbRaw);

#ifdef  PCAS
printf("\n==== j__udyConvertBranchBtoU%d, (num jp_t = %d)\n", jpLevel, (int)Pjbb->jbb_NumJPs);
#endif // PCAS

//  Copy over number pointers in the struct
    Pjbu->jbu_NumJPs  = Pjbb->jbb_NumJPs;
    Pjbu->jbu_DCDPop0 = Pjbb->jbb_DCDPop0;

//  BranchB subexpanse:                4
    int       subexpB;
//    for (subexpB = 0; subexpB < cJU_NUMSUBEXPB; subexpB++)
//    printf("\n");

    Word_t BranchUsum = 0;
    for (subexpB = 0; subexpB < 4; subexpB++)
    {
        uint16_t jpUnum = subexpB * cbPW;       // adjust on every subexpanse
//      Next (0..3) Pjp to BranchB jp_t array
        Pjp_t     PjpB = P_JP(JU_JBB_PJP(Pjbb, subexpB));

//      Get the bitmap for this subexpanse
        BITMAPB_t Bitmap = JU_JBB_BITMAP(Pjbb, subexpB);

#ifdef  DEBUG
        int       Bitcount = j__udyCountBitsB(Bitmap);
#endif // DEBUG

//        if (Bitmap == 0)
//            continue;                           // next BranchB subexpanse

        Word_t BPop1;
//      Copy Valid jp_ts to BranchU
        uint8_t jpBnum = 0;                     // cannot exceed 64 
//        for ( ; Bitmap != 0; jpUnum++)          // inc BrancU jp for every bit
        for (int ii = 0; ii < 64; ii++, jpUnum++)          // inc BrancU jp for every bit
        {
            if (Bitmap & ((Word_t)1))
            {
                jpBnum++;                       // Valid jp_t
#ifndef noBigU
                assert((jpUnum >> 3) < 32);
//              every 8 BranchU jp_t sums
                BPop1 = j__udyJPPop1(PjpB);
                Pjbu->jbu_sums8[jpUnum >> 3] += BPop1;
#endif // noBigU
                Pjbu->jbu_jp[jpUnum] = *PjpB++; // next jp_t
            }
            else
            {
                Pjbu->jbu_jp[jpUnum] = JPNull;
            }
//if ((jpUnum % 8) == 7) printf("%5ld ", Pjbu->jbu_sums8[jpUnum >> 3]);
            Bitmap >>= 1;
        }
        assert(jpBnum == Bitcount);
//      Free the BranchB subexpanse of jp_t
        if (jpBnum)
            j__udyFreeJBBJP(JU_JBB_PJP(Pjbb, subexpB), jpBnum, Pjpm);
    }                                           // for each JP in BranchU
//    printf("\n");

#ifdef  DEBUG_COUNT
    BranchUsum = 0;
    for (int subnum = 0; subnum < 32; subnum++)
    {
//       printf("%5ld ", Pjbu->jbu_sums8[subnum]);
       BranchUsum += Pjbu->jbu_sums8[subnum];
    }

    if (BranchUsum != (ju_BranchPop0(Pjp, jpLevel) + 1))
    {
printf("\n---OOps4 j__udyConvertBranchBtoU: BranchUsum = %lu, ju_BranchPop0(Pjp, jpLevel) + 1) = %lu\n", BranchUsum, ju_BranchPop0(Pjp, jpLevel) + 1);
    assert(BranchUsum == (ju_BranchPop0(Pjp, jpLevel) + 1));
    }
#endif  // DEBUG_COUNT

//  Free the BranchB and put the BranchU in its place:
    j__udyFreeJBB(PjbbRaw, Pjpm);

    ju_SetPntrInJp(Pjp, PjbuRaw);
    ju_SetJpType(Pjp, cJU_JPBRANCH_U2 - 2 + jpLevel);
    return (1);
}                                               // j__udyConvertBranchBtoU()
