// Chunky. The NFS chunk scanner.
//

#include "stdafx.h"
#include <stdlib.h>

struct ChunkStruct
{
	unsigned int ChunkID;
	unsigned long int Size;
	unsigned long int FullSize;
	unsigned long int FilePos;
	unsigned long int Padding;
	unsigned long int FullPadding;
}Chunk[0xFFFF];

typedef struct ChunkDefStruct
{
	unsigned int ChunkID;
	char ChunkName[255];
}*ChunkDef;

const static char UnknownChunkString[] = UNKNOWNCHUNKSTR;

unsigned int ChunkDefCount;

unsigned int CountLinesInFile(FILE *finput)
{
	unsigned long int OldOffset = ftell(finput);
	unsigned int LineCount = 0;
	char ReadCh;

	while (!feof(finput))
	{
		ReadCh = fgetc(finput);
		if (ReadCh == '\n')
			LineCount++;
	}
	fseek(finput, OldOffset, SEEK_SET);
	return LineCount+1;
}

const char* SearchChunkType(unsigned int ChunkID, ChunkDefStruct* InChunkDef)
{
	for (unsigned int i = 0; i < ChunkDefCount; i++)
	{
		if (InChunkDef[i].ChunkID == ChunkID)
			return InChunkDef[i].ChunkName;
	}

	return UnknownChunkString;
}

ChunkDef ReadChunkDefFile(const char* Filename)
{
	FILE* fin = fopen(Filename, "r");
	if (!fin)
	{
		printf("%s Can't open file for reading.\n", PRINTTYPEERROR);
		perror("ERROR");
		return NULL;
	}
	ChunkDefCount = CountLinesInFile(fin);

	ChunkDef TheOutChunkDef = (ChunkDefStruct*)calloc(ChunkDefCount, sizeof(ChunkDefStruct));
	for (unsigned int i = 0; i < ChunkDefCount; i++)
		fscanf(fin, "0x%x %s\n", &TheOutChunkDef[i].ChunkID, &TheOutChunkDef[i].ChunkName);

	fclose(fin);
	return TheOutChunkDef;
}

int ScanChunks(const char* OutTxtFilename, const char* InFilename)
{
	struct stat st;
	ChunkDef ScannerChunkDef;
	unsigned int Counter = 0;

	printf("%s Opening %s\n", PRINTTYPEINFO, InFilename);
	FILE* InFile = fopen(InFilename, "rb");
	if (!InFile)
	{
		printf("%s Can't open file for writing.\n", PRINTTYPEERROR);
		perror("ERROR");
		return -1;
	}

	ScannerChunkDef = ReadChunkDefFile(CHUNKDEFFILENAME);
	if (!ScannerChunkDef)
		printf("%s Can't read the chunk definition file\n", PRINTTYPEWARNING);

	stat(InFilename, &st);
	printf("%s Opening %s\n", PRINTTYPEINFO, OutTxtFilename);
	FILE* OutTxtFile = fopen(OutTxtFilename, "w");
	if (!OutTxtFile)
	{
		printf("%s Can't open file for writing, using stdout.\n", PRINTTYPEWARNING);
		perror("ERROR");
		OutTxtFile = stdout;
	}
	fputs(FORMATTHING1, OutTxtFile);
	while (ftell(InFile) < st.st_size)
	{
		Chunk[Counter].FilePos = ftell(InFile);
		fread(&Chunk[Counter].ChunkID, 1, 4, InFile);
		fread(&Chunk[Counter].Size, 1, 4, InFile);

		if (Chunk[Counter].ChunkID)
		{
			Chunk[Counter].FullSize = Chunk[Counter].Size + 8;
			fprintf(OutTxtFile, FORMATTHING2, Chunk[Counter].FilePos, Chunk[Counter].Size, Chunk[Counter].FullSize, Chunk[Counter].ChunkID, Chunk[Counter].Padding, Chunk[Counter].FullPadding, SearchChunkType(Chunk[Counter].ChunkID, ScannerChunkDef));
			fseek(InFile, Chunk[Counter].Size, SEEK_CUR);
			Counter++;
		}
		else
		{
			Chunk[Counter + 1].Padding = Chunk[Counter].Size;
			Chunk[Counter + 1].FullPadding = Chunk[Counter].Size + 8;
			fseek(InFile, Chunk[Counter].Size, SEEK_CUR);
			Counter++;
		}
		
	}
	fclose(OutTxtFile);
	fclose(InFile);
	printf("%s Scan complete!\n", PRINTTYPEINFO);
	return 0;
}


int main(int argc, char* argv[])
{
	printf("Chunky v%d by Xanvier\n", BUILDVER);

	if ((argv[1] == NULL) || argc < 2)
	{
		printf("%s Too few arguments passed.\nUSAGE: %s OutTextFile InFile", PRINTTYPEERROR, argv[0]);
		return -1;
	}

	return ScanChunks(argv[1], argv[2]);
}
