// Copyright (C) 2019 Douglas L. Baskins
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

// @(#) $Revision: 1.3 $ $Source: /home/doug/judy-1.0.5_PSplit_goto_newLeaf3_U2_1K_1_L765_5th_cleanedup_again/src/JudyCommon/RCS/JudyTables.c,v $

#ifndef JU_WIN
#include <unistd.h>		// unavailable on win_*.
#endif

#include <stdlib.h>
#include <stdio.h>

#if (! (defined(JUDY1) || defined(JUDYL)))
#error:  One of -DJUDY1 or -DJUDYL must be specified.
#endif

#define	TERMINATOR 999		// terminator for Alloc tables

#define BPW sizeof(Word_t)	// define bytes per word

#ifdef JUDY1
#include "Judy1.h"
#else
#include "JudyL.h"
#endif
    
FILE *fd;

// Definitions come from header files Judy1.h and JudyL.h:

int AllocSizes[] = ALLOCSIZES;

#define	ROUNDUP(BYTES,BPW,OFFSETW) \
	((((BYTES) + (BPW) - 1) / (BPW)) + (OFFSETW))


// ****************************************************************************
// G E N   T A B L E
//
// Note:  "const" is required for newer compilers.

FUNCTION void GenTable(
    const char * TableName,	// name of table string
    const char * TableSize,	// dimentioned size string
    int		 IndexBytes,	// bytes per Index
    int		 LeafSize,	// number elements in object
    int		 ValueBytes,	// bytes per Value
    int		 OffsetWords)	// 1 for LEAF8
{
    int        * PAllocSizes = AllocSizes;
    int		 OWord;
    int		 CurWord;
    int		 IWord;
    int		 ii;
    int		 BytesOfIndex;
    int		 BytesOfObject;
    int		 Index;
    int		 LastWords;
    int		 Words [1000] = { 0 };
    int		 Offset[1000] = { 0 };
    int		 MaxWords;

    MaxWords  =	ROUNDUP((IndexBytes + ValueBytes) * LeafSize, BPW, OffsetWords);
    Words [0] = 0;
    Offset[0] = 0;
    CurWord   = TERMINATOR;

// Walk through all number of Indexes in table:

    for (Index = 1; /* null */; Index++)
    {

// Calculate byte required for next size:

	BytesOfIndex  = IndexBytes * Index;
	BytesOfObject = (IndexBytes + ValueBytes) * Index;

// Round up and calculate words required for next size:

        OWord =	ROUNDUP(BytesOfObject, BPW, OffsetWords);
        IWord =	ROUNDUP(BytesOfIndex,  BPW, OffsetWords);

// Root-level leaves of population of 1 and 2 do not have the 1 word offset:

// Save minimum value of offset:

        Offset[Index] = IWord;

// Round up to next available size of words:

	while (OWord > *PAllocSizes) PAllocSizes++;

        if (Index == LeafSize)
        {
	    CurWord = Words[Index] = OWord;
            break;
        }
//      end of available sizes ?

	if (*PAllocSizes == TERMINATOR)
        {
            fprintf(stderr, "BUG, in %sPopToWords, sizes not big enough for object\n", TableName);
	    exit(1);
        }

// Save words required and last word:

        if (*PAllocSizes < MaxWords) 
        { 
            CurWord = Words[Index] = *PAllocSizes; 
        }
        else                         
        { 
            CurWord = Words[Index] = MaxWords; 
        }

    } // for each index

    LastWords = TERMINATOR;

// Round up to largest size in each group of malloc sizes:

    for (ii = LeafSize; ii > 0; ii--)
    {
        if (LastWords > (Words[ii] - ii)) LastWords = Offset[ii];
        else                              Offset[ii] = LastWords;
    }

// Print the PopToWords[] table:

    fprintf(fd,"\n//\tobject uses %d words\n", CurWord);
    fprintf(fd,"//\t%s = %d\n", TableSize, LeafSize);

    fprintf(fd,"const uint16_t\n");
    fprintf(fd,"%sPopToWords[%s + 2] =\n", TableName, TableSize);
    fprintf(fd, "{\n       0,\t\t// saves subtracting 1");

    for (ii = 1; ii <= LeafSize; ii++)
    {

// 8 columns per line, starting with 1:

	if ((ii % 10) == 1) fprintf(fd,"  // %3d\n", ii - 1);

	fprintf(fd, " %3d,", Words[ii]);

// If not last number place comma:

//	if (ii != LeafSize) fprintf(fd,", ");
    }
    fprintf(fd, "\n       0\t\t// saves testing for MAX\n};\n");

// Print the Offset table if needed:

    if (! ValueBytes) return;

    fprintf(fd,"const uint16_t\n");
    fprintf(fd,"%sOffset[%s + 1] =\n", TableName, TableSize);
    fprintf(fd,"{\n");
    fprintf(fd,"       0,");

    for (ii = 1; ii <= LeafSize; ii++)
    {
        if ((ii % 10) == 1) fprintf(fd,"\n     ");

	fprintf(fd,"%3d", Offset[ii]);

	if (ii != LeafSize) fprintf(fd,", ");
    }
    fprintf(fd,"\n};\n");

} // GenTable()


// ****************************************************************************
// M A I N

FUNCTION int main()
{
    int ii;

#ifdef JUDY1
    if ((fd = fopen("Judy1Tables.c", "w")) == NULL)
    {
	perror("FATAL ERROR: could not write to Judy1Tables.c file\n");
	return (-1);
    }
#endif  // JUDY1

#ifdef JUDYL
    if ((fd = fopen("JudyLTables.c", "w")) == NULL)
    {
	perror("FATAL ERROR: could not write to JudyLTables.c file\n");
	return (-1);
    }
#endif  // JUDYL

    fprintf(fd,"// This file is created by src/JudyTables.c -- DO NOT EDIT!!\n");
    fprintf(fd,"//\n\n");

#ifdef JUDY1
// ================================ Judy1 =================================

    fprintf(fd,"#include \"Judy1.h\"\n");

    fprintf(fd,"// Leave the malloc() sizes readable in the binary (via strings(1)):\n");
    fprintf(fd,"const char * Judy1MallocSizes = \"Judy1MallocSizes =");

    for (ii = 0; AllocSizes[ii] != TERMINATOR; ii++)
	fprintf(fd,"%d,", AllocSizes[ii]);

    fprintf(fd," L8=%d L7=%d L6=%d L5=%d L4=%d L3=%d L2=%d L1=%d\";\n\n", 
            (int)cJ1_LEAF8_MAXPOP1,
            (int)cJ1_LEAF7_MAXPOP1,
            (int)cJ1_LEAF6_MAXPOP1,
            (int)cJ1_LEAF5_MAXPOP1,
            (int)cJ1_LEAF4_MAXPOP1,
            (int)cJ1_LEAF3_MAXPOP1,
            (int)cJ1_LEAF2_MAXPOP1,
            (int)cJ1_LEAF1_MAXPOP1);

    GenTable("j__1_BranchBJP","cJU_BITSPERSUBEXPB",sizeof(jp_t), cJU_BITSPERSUBEXPB, 0, 0);

//    GenTable("j__1_Leaf1", "cJ1_LEAF1_MAXPOP1", 1, cJ1_LEAF1_MAXPOP1, 0, 1);
    GenTable("j__1_Leaf2", "cJ1_LEAF2_MAXPOP1", 2, cJ1_LEAF2_MAXPOP1, 0, 1);
    GenTable("j__1_Leaf3", "cJ1_LEAF3_MAXPOP1", 4, cJ1_LEAF3_MAXPOP1, 0, 1);
    GenTable("j__1_Leaf4", "cJ1_LEAF4_MAXPOP1", 4, cJ1_LEAF4_MAXPOP1, 0, 1);
    GenTable("j__1_Leaf5", "cJ1_LEAF5_MAXPOP1", 8, cJ1_LEAF5_MAXPOP1, 0, 1);
    GenTable("j__1_Leaf6", "cJ1_LEAF6_MAXPOP1", 8, cJ1_LEAF6_MAXPOP1, 0, 1);
    GenTable("j__1_Leaf7", "cJ1_LEAF7_MAXPOP1", 8, cJ1_LEAF7_MAXPOP1, 0, 1);
    GenTable("j__1_Leaf8", "cJ1_LEAF8_MAXPOP1", 8, cJ1_LEAF8_MAXPOP1, 0, 1);
#endif  // JUDY1

#ifdef JUDYL
// ================================ JudyL =================================

    fprintf(fd,"#include \"JudyL.h\"\n");

    fprintf(fd,"// Leave the malloc() sizes readable in the binary (via "
	   "strings(1)):\n");
    fprintf(fd,"const char * JudyLMallocSizes = \"JudyLMallocSizes =");

    for (ii = 0; AllocSizes[ii] != TERMINATOR; ii++)
	fprintf(fd,"%d,", AllocSizes[ii]);

    fprintf(fd," L8=%d L7=%d L6=%d L5=%d L4=%d L3=%d L2=%d L1=%d V=%d\";\n\n", 
            (int)cJL_LEAF8_MAXPOP1,
            (int)cJL_LEAF7_MAXPOP1,
            (int)cJL_LEAF6_MAXPOP1,
            (int)cJL_LEAF5_MAXPOP1,
            (int)cJL_LEAF4_MAXPOP1,
            (int)cJL_LEAF3_MAXPOP1,
            (int)cJL_LEAF2_MAXPOP1,
            (int)cJL_LEAF1_MAXPOP1,
            (int)cJU_BITSPERSUBEXPL);

    GenTable("j__L_BranchBJP","cJU_BITSPERSUBEXPB",sizeof(jp_t), cJU_BITSPERSUBEXPB, 0, 0);

//exp    GenTable("j__L_Leaf1", "cJL_LEAF1_MAXPOP1",  0, cJL_LEAF1_MAXPOP1,  8, 33);
//    GenTable("j__L_LeafB1", "cJL_LEAFB1_MAXPOP1",0, cJL_LEAFB1_MAXPOP1,  BPW, cJL_WORDSPERLEAFB1);
//    GenTable("j__L_LeafB1", "cJL_LEAFB1_MAXPOP1",0,              256,   BPW, cJL_WORDSPERLEAFB1);
    GenTable("j__L_Leaf1", "cJL_LEAF1_MAXPOP1",  1, cJL_LEAF1_MAXPOP1,  BPW, 1);
    GenTable("j__L_Leaf2", "cJL_LEAF2_MAXPOP1",  2, cJL_LEAF2_MAXPOP1,  BPW, 1);
    GenTable("j__L_Leaf3", "cJL_LEAF3_MAXPOP1",  4, cJL_LEAF3_MAXPOP1,  BPW, 1);
    GenTable("j__L_Leaf4", "cJL_LEAF4_MAXPOP1",  4, cJL_LEAF4_MAXPOP1,  BPW, 1);
    GenTable("j__L_Leaf5", "cJL_LEAF5_MAXPOP1",  8, cJL_LEAF5_MAXPOP1,  BPW, 1);
    GenTable("j__L_Leaf6", "cJL_LEAF6_MAXPOP1",  8, cJL_LEAF6_MAXPOP1,  BPW, 1);
    GenTable("j__L_Leaf7", "cJL_LEAF7_MAXPOP1",  8, cJL_LEAF7_MAXPOP1,  BPW, 1);
    GenTable("j__L_Leaf8", "cJL_LEAF8_MAXPOP1",  8, cJL_LEAF8_MAXPOP1  ,BPW, 1);
//    GenTable("j__L_LeafV", "cJU_BITSPERSUBEXPL", 8, cJU_BITSPERSUBEXPL, 0, 0);
    GenTable("j__L_LeafV", "cJU_BITSPERSUBEXPL", 8, 8 /* ImmedL max pop */, 0, 0);
#endif // JUDYL

    fclose(fd);
    return(0);

} // main()
