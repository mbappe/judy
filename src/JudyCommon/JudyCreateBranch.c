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

FUNCTION int j__udyCreateBranchL(
	Pjp_t	Pjp,		// Build JPs from this place
	Pjp_t	PJPs,		// Array of JPs to put into Bitmap branch
	uint8_t Exp[],		// Array of expanses to put into bitmap
	Word_t  ExpCnt,		// Number of above JPs and Expanses
	Pjpm_t	Pjpm)
{
	Word_t	PjblRaw;	// pointer to linear branch.
	Pjbl_t	Pjbl;

	assert(ExpCnt <= cJU_BRANCHLMAXJPS);

#ifdef  PCAS
        printf("\n=========== j__udyCreateBranchL (num jp_t = %d)\n", (int)ExpCnt);
#endif  // PCAS

	PjblRaw	= j__udyAllocJBL(Pjpm);
	if (PjblRaw == 0) return(-1);
        Pjbl    = P_JBL(PjblRaw);

//	Build a Linear Branch
	Pjbl->jbl_NumJPs = ExpCnt;

//	Copy from the Linear branch from splayed leaves
	JU_COPYMEM(Pjbl->jbl_Expanse, Exp,  ExpCnt);
	JU_COPYMEM(Pjbl->jbl_jp,      PJPs, ExpCnt);

////printf("Create BranchL: S Type = %d\n", ju_Type(&PJPs[0]));
////printf("Create BranchL: D Type = %d\n", ju_Type(&Pjbl->jbl_jp[0]));

////        for (int ii = 0; ii < ExpCnt; ii++)
////        {
////	     Pjbl->jbl_Expanse[ii] = Exp[ii];
////	     Pjbl->jbl_jp[ii] = PJPs[ii];

////printf("Create BranchL: S Type = %d\n", ju_Type(&PJPs[ii]));
////printf("Create BranchL: D Type = %d\n", ju_Type(&Pjbl->jbl_jp[ii]));
////printf("Create BranchL: S BaL = 0x%lx\n", ju_PntrInJp(PJPs + ii));
////printf("Create BranchL: D BaL = 0x%lx\n", ju_PntrInJp(Pjbl->jbl_jp + ii));

////        }

//	Pass back new pointer to the Linear branch in JP
        ju_SetPntrInJp(Pjp, PjblRaw);

	return(1);

} // j__udyCreateBranchL()


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

FUNCTION Word_t j__udyBranchBfromParts(
	Pjp_t	PJPs,		// Array of JPs to put into Bitmap branch
	uint8_t Exp[],		// Array of expanses to put into bitmap
	Word_t  ExpCnt,		// Number of above JPs and Expanses
	Pjpm_t	Pjpm)
{
	Word_t	PjbbRaw;	// pointer to bitmap branch.
	Pjbb_t	Pjbb;
	Word_t  ii, jj;		// Temps
	uint8_t CurrSubExp;	// Current sub expanse for BM

#ifdef  PCAS
        printf("\n=========== j__udyBranchBfromParts (num jp_t = %d)\n", (int)ExpCnt);
#endif  // PCAS

// This assertion says the number of populated subexpanses is not too large.
// This function is only called when a BranchL overflows to a BranchB or when a
// cascade occurs, meaning a leaf overflows.  Either way ExpCnt cant be very
// large, in fact a lot smaller than cJU_BRANCHBMAXJPS.  (Otherwise a BranchU
// would be used.)  Popping this assertion means something (unspecified) has
// gone very wrong, or else Judys design criteria have changed, although in
// fact there should be no HARM in creating a BranchB with higher actual
// fanout.

	assert(ExpCnt <= cJU_BRANCHBMAXJPS);

//	Get memory for a Bitmap branch
	PjbbRaw	= j__udyAllocJBB(Pjpm);
	if (PjbbRaw == 0) 
            return(0);

	Pjbb = P_JBB(PjbbRaw);

        Pjbb->jbb_NumJPs = ExpCnt;

//	Get 1st "sub" expanse (0..7) of bitmap branch
	CurrSubExp = Exp[0] / cJU_BITSPERSUBEXPB;

// Index thru all 1 byte sized expanses:

	for (jj = ii = 0; ii <= ExpCnt; ii++)
	{
		Word_t SubExp;	// Cannot be a uint8_t

//		Make sure we cover the last one
		if (ii == ExpCnt)
		{
			SubExp = cJU_ALLONES;	// Force last one
		}
		else
		{
//			Calculate the "sub" expanse of the byte expanse
			SubExp = Exp[ii] / cJU_BITSPERSUBEXPB;  // Bits 5..7.

//			Set the bit that represents the expanse in Exp[]
			JU_JBB_BITMAP(Pjbb, SubExp) |= JU_BITPOSMASKB(Exp[ii]);
		}
//		Check if a new "sub" expanse range needed
		if (SubExp != CurrSubExp)
		{
//			Get number of JPs in this sub expanse
			Word_t NumJP = ii - jj;
			Word_t PjpRaw;
			Pjp_t  Pjp;

			PjpRaw = j__udyAllocJBBJP(NumJP, Pjpm);

			if (PjpRaw == 0)	// out of memory.
			{
//                          Free any previous allocations:
			    while(CurrSubExp--)
			    {
				NumJP = j__udyCountBitsB(JU_JBB_BITMAP(Pjbb,
								  CurrSubExp));
				if (NumJP)
				{
				    j__udyFreeJBBJP(JU_JBB_PJP(Pjbb,
						    CurrSubExp), NumJP, Pjpm);
				}
			    }
			    j__udyFreeJBB(PjbbRaw, Pjpm);
			    return(0);          // out of memory
			}
                        Pjp    = P_JP(PjpRaw);

//                      Place the array of JPs in bitmap branch:
			JU_JBB_PJP(Pjbb, CurrSubExp) = PjpRaw;

//                      Copy the JPs to new allocation:
			JU_COPYMEM(Pjp, PJPs + jj, NumJP);

//                      On to the next bitmap branch "sub" expanse:
			jj	   = ii;
			CurrSubExp = SubExp;
		}
	} // for each 1-byte expanse

//	Pjp->Jp_Addr0 = PjbbRaw;
// now done by caller        ju_SetPntrInJp(Pjp, PjbbRaw);
	return(PjbbRaw);

} // j__udyBranchBfromParts()


// ****************************************************************************
// J U D Y   C R E A T E   B R A N C H   U
//
// Build a BranchU from a BranchB.  Return with Pjp pointing to the BranchU.
// Free the BranchB and its JP subarrays.
//
// Return -1 if error (details in Pjpm), otherwise return 1.

FUNCTION int j__udyCreateBranchU(
	Pjp_t	  Pjp,
	Pjpm_t	  Pjpm)
{
	jp_t	  JPNull;
        Word_t    PjbuRaw;
        Pjbu_t    Pjbu;
	Word_t	  PjbbRaw;
	Pjbb_t	  Pjbb;
	Word_t	  ii, jj;
	BITMAPB_t BitMap;
	Pjp_t	  PDstJP;
        uint8_t   jpLevel;

#ifdef JU_STAGED_EXP
	jbu_t	  BranchU;	// Staged uncompressed branch
#else

// Allocate memory for a BranchU:

	PjbuRaw = j__udyAllocJBU(Pjpm);
	if (PjbuRaw == 0) return(-1);
        Pjbu = P_JBU(PjbuRaw);
#endif

//        jpLevel = JU_JPTYPE(Pjp) - cJU_JPBRANCH_B2;
        jpLevel = ju_Type(Pjp) - cJU_JPBRANCH_B2;

//        JU_JPSETADT(&JPNull, 0, 0, cJU_JPNULL1 + jpLevel);
        ju_SetIMM01(&JPNull, 0, 0, cJU_JPNULL1 + jpLevel);

// Get the pointer to the BranchB:

	PjbbRaw	= ju_PntrInJp(Pjp);
        assert(PjbbRaw);
	Pjbb	= P_JBB(PjbbRaw);

#ifdef  PCAS
        printf("\n=========== j__udyCreateBranch_U%d, (num jp_t = %d)\n", jpLevel + 2, (int)Pjbb->jbb_NumJPs);
#endif  // PCAS

//	Set the pointer to the Uncompressed branch
#ifdef JU_STAGED_EXP
	PDstJP = BranchU.jbu_jp;
#else
        PDstJP = Pjbu->jbu_jp;
#endif

//      Copy over number pointers in the struct
        Pjbu->jbu_NumJPs = Pjbb->jbb_NumJPs;
        
	for (ii = 0; ii < cJU_NUMSUBEXPB; ii++)
	{
		Pjp_t	PjpA;
		Pjp_t	PjpB;

		PjpB = PjpA = P_JP(JU_JBB_PJP(Pjbb, ii));

//		Get the bitmap for this subexpanse
		BitMap	= JU_JBB_BITMAP(Pjbb, ii);

//		NULL empty subexpanses
		if (BitMap == 0)
		{
//			But, fill with NULLs
			for (jj = 0; jj < cJU_BITSPERSUBEXPB; jj++)
			{
				PDstJP[jj] = JPNull;
			}
			PDstJP += cJU_BITSPERSUBEXPB;
			continue;
		}
//		Check if Uncompressed subexpanse
		if (BitMap == cJU_FULLBITMAPB)
		{
//			Copy subexpanse to the Uncompressed branch intact
			JU_COPYMEM(PDstJP, PjpA, cJU_BITSPERSUBEXPB);

//			Bump to next subexpanse
			PDstJP += cJU_BITSPERSUBEXPB;

//			Set length of subexpanse
			jj = cJU_BITSPERSUBEXPB;
		}
		else
		{
			for (jj = 0; jj < cJU_BITSPERSUBEXPB; jj++)
			{
//				Copy JP or NULLJP depending on bit
				if (BitMap & 1) { *PDstJP = *PjpA++; }
				else		{ *PDstJP = JPNull; }

				PDstJP++;	// advance to next JP
				BitMap >>= 1;
			}
			jj = PjpA - PjpB;
		}

// Free the subexpanse:

		j__udyFreeJBBJP(JU_JBB_PJP(Pjbb, ii), jj, Pjpm);

	} // for each JP in BranchU

#ifdef JU_STAGED_EXP

// Allocate memory for a BranchU:

	PjbuRaw = j__udyAllocJBU(Pjpm);
	if (PjbuRaw == 0) return(-1);
        Pjbu = P_JBU(PjbuRaw);

// Copy staged branch to newly allocated branch:
//
// TBD:  I think this code is broken.

	*Pjbu = BranchU;

#endif // JU_STAGED_EXP

// Finally free the BranchB and put the BranchU in its place:

	j__udyFreeJBB(PjbbRaw, Pjpm);

//	Pjp->Jp_Addr0  = PjbuRaw;
        ju_SetPntrInJp(Pjp, PjbuRaw);

//        jpLevel = JU_JPTYPE(Pjp) - cJU_JPBRANCH_B2;
//        jpLevel = ju_Type(Pjp) - cJU_JPBRANCH_B2;
//	  Pjp->jp_Type = cJU_JPBRANCH_U2 + jpLevel;
        ju_SetJpType(Pjp, cJU_JPBRANCH_U2 + jpLevel);

	return(1);

} // j__udyCreateBranchU()
