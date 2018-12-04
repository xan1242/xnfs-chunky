// Chunky. The NFS chunk scanner.
//

#include "stdafx.h"
#include "KnownChunkDefs.h"
#include <stdlib.h>
#include <string.h>

bool bExtract = false;
bool bCombine = false;
bool bPadding = true;
unsigned int ScannedChunksCount = 0;
char ExecBuf[255];

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
ChunkDef ScannerChunkDef;

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

int WriteChunkTypeAndSize(FILE *fout, unsigned int ChunkMagic, unsigned int ChunkSize)
{
	fwrite(&ChunkMagic, 4, 1, fout);
	fwrite(&ChunkSize, 4, 1, fout);
	return 1;
}

int ZeroChunkWriter(FILE *fout, unsigned int ChunkSize)
{
	if (ChunkSize)
	{
		WriteChunkTypeAndSize(fout, 0, ChunkSize);
		for (unsigned int i = 0; i < ChunkSize; i++)
			fputc(0, fout);
	}
	else
	{
		for (unsigned int i = 0; i < 8; i++)
			fputc(0, fout);
	}
	return 1;
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

int ScanChunks(const char* InFilename)
{
	struct stat st;
	//ChunkDef ScannerChunkDef;
	unsigned int Counter = 0;

	//printf("%s Opening %s\n\n", PRINTTYPEINFO, InFilename);
	FILE* InFile = fopen(InFilename, "rb");
	if (!InFile)
	{
		printf("%s Can't open file %s for reading.\n", PRINTTYPEERROR, InFilename);
		perror("ERROR");
		return -1;
	}

	
	if (!ScannerChunkDef)
		printf("%s Can't read the chunk definition file\n", PRINTTYPEWARNING);

	stat(InFilename, &st);
	//printf("%s Opening %s\n", PRINTTYPEINFO, OutTxtFilename);
	//FILE* OutTxtFile = fopen(OutTxtFilename, "w");
	FILE* OutTxtFile = stdout;
	/*if (!OutTxtFile)
	{
		printf("%s Can't open file for writing, using stdout.\n", PRINTTYPEWARNING);
		perror("ERROR");
		OutTxtFile = stdout;
	}*/
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
//	fclose(OutTxtFile);
	fclose(InFile);
	printf("%s Scan complete!\n", PRINTTYPEINFO);
	return Counter;
}

int ExtractChunks(const char* InFilename, unsigned int ChunkCount)
{
	FILE* InFile = fopen(InFilename, "rb");
	FILE* TxtFile;
	FILE* OutFile;

	unsigned int FileCounter = 0;
	void* FileBuf;

	char TxtFileName[255];
	char BaseFileName[255];
	char OutFileName[255];

	if (!InFile)
	{
		printf("%s Can't open file %s for reading.\n", PRINTTYPEERROR, InFilename);
		perror("ERROR");
		return -1;
	}

	// Create a folder by using the bundle's filename
	strncpy(BaseFileName, InFilename, strrchr(InFilename, '.') - InFilename);
	BaseFileName[strrchr(InFilename, '.') - InFilename] = '\0';
	printf("%s Creating directory %s\n", PRINTTYPEINFO, BaseFileName);
	sprintf(ExecBuf, "md %s\0", BaseFileName);
	system(ExecBuf);

	// Also create a file list so we can keep track of the order of chunks (maybe important for the game)
	// We're using the bundle's filename as the basis and attaching a .txt at the end of it
	// This is reused for repacking.
	strcpy(TxtFileName, BaseFileName);
	strcat(TxtFileName, ".txt\0");
	TxtFile = fopen(TxtFileName, "w");

	for (unsigned int i = 0; i < ChunkCount; i++)
	{
		// for every non pad chunk
		if (Chunk[i].ChunkID)
		{
			// generate a filename (+ fileext if possible, defaults to .bin)
			sprintf(OutFileName, "%s\\%d_%s", BaseFileName, FileCounter, SearchChunkType(Chunk[i].ChunkID, ScannerChunkDef));
			switch (Chunk[i].ChunkID)
			{
			case BCHUNK_SPEED_TEXTURE_PACK_LIST_CHUNKS:
				strcat(OutFileName, ".tpk");
				break;
			case BCHUNK_FENG_PACKAGE:
				strcat(OutFileName, ".fng");
				break;
			case BCHUNK_FENG_PACKAGE_COMPRESSED:
				strcat(OutFileName, ".fng.lzc");
				break;
			default:
				strcat(OutFileName, ".bin");
			}

			// add the filename to the list
			fprintf(TxtFile, "%s\n", OutFileName);
			
			// go to the chunk pos
			fseek(InFile, Chunk[i].FilePos, SEEK_SET);

			// copy the chunk into the buffer
			FileBuf = malloc(Chunk[i].FullSize);
			fread(FileBuf, Chunk[i].FullSize, 1, InFile);

			// open the output file & write the buffer out
			OutFile = fopen(OutFileName, "wb");
			fwrite(FileBuf, Chunk[i].FullSize, 1, OutFile);

			// dump out everything and continue
			free(FileBuf);
			fclose(OutFile);

			FileCounter++;
		}
	}

	fclose(TxtFile);
	fclose(InFile);

	return 0;
}

// Similar to file concatination, except there is padding/alignment involved...
// giant WIP here... models get corrupted!
int CombineChunks(const char* FileListTxtName, const char* OutFilename)
{
	FILE* TxtFile = fopen(FileListTxtName, "r");
	FILE* OutFile;
	FILE* InFile;
	struct stat st = { 0 };

	char InFilename[255];
	void* FileBuf;
	unsigned int WriteCounter = 0;
	unsigned long int CurrentOffset = 0;
	unsigned long int CurrentAlignedOffset = 0;
	int alignment = 0;
	int alignment2 = 0;
	int PreviousChunkSize = 0;
	int CurrentChunkSize = 0;
	unsigned long int NextOffset = 0;
	unsigned int ModelZeroSize = 0;
	unsigned long int ModelChunkRelativeLoc = 0;

	if (!TxtFile)
	{
		printf("%s Can't open file %s for reading.\n", PRINTTYPEERROR, FileListTxtName);
		perror("ERROR");
		return -1;
	}

	OutFile = fopen(OutFilename, "wb");
	if (!OutFile)
	{
		printf("%s Can't open file %s for writing.\n", PRINTTYPEERROR, OutFilename);
		perror("ERROR");
		return -1;
	}

	while (!feof(TxtFile))
	{
		// we get the filename from the list
		fscanf(TxtFile, "%s\n", InFilename);

		printf("%s Writing: %s\n", PRINTTYPEINFO, InFilename);

		// open it
		InFile = fopen(InFilename, "rb");
		if (!InFile)
		{
			printf("%s Can't open file %s for reading.\n", PRINTTYPEERROR, InFilename);
			perror("ERROR");
			return -1;
		}

		// get its size
		stat(InFilename, &st);

		// copy it to the filebuffer
		FileBuf = malloc(st.st_size);
		fread(FileBuf, st.st_size, 1, InFile);

		// before anything, we check if we need to add prepadding (unless we're on the very first write) (MODELS ARE AN EXCEPTION)
		
		// ACTUALLY THIS DOESNT REALLY WORK FOR SOME STRANGE REASON, SO WE NEED TO KEEP TRACK OF THE ALIGNED OFFSET OURSELVES MANUALLY!!!
		CurrentOffset = ftell(OutFile);
		CurrentChunkSize = ((unsigned int*)FileBuf)[1];

		if (PreviousChunkSize)
			CurrentAlignedOffset += ((PreviousChunkSize + 0x87) & 0xFFFFFF80);


		while (CurrentOffset > CurrentAlignedOffset)
		{
			CurrentAlignedOffset += 0x10; // failsafe
		}
		// check if we got a model on our hands for its padding business
		if (*(unsigned int*)FileBuf == BCHUNK_SPEED_ESOLID_LIST_CHUNKS)
		{
			if (bPadding)
				alignment = CurrentAlignedOffset - CurrentOffset;

			// Model's 0x80134001 chunk ABSOLUTELY MUST BE ALIGNED!!! I have no idea why...
			// we semi-parse stuff from the model's chunk

			// at 0xC of every model chunk there is a size of the zero chunk that is used for aligning
			ModelZeroSize = ((unsigned int*)FileBuf)[3];

			// ModelZeroSize + 0x10 = relative location to the 0x80134001 chunk
			ModelChunkRelativeLoc = ModelZeroSize + 0x10;

			if (WriteCounter)
			{

				// hack
				switch (ModelZeroSize)
				{
				case 4:
					alignment += 0xC;
					break;
				case 8:
					alignment += 8;
					break;
				case 0xC:
					alignment += 4;
					break;
				default:
					break;
				}

				// hack2
				if (CurrentChunkSize == (CurrentChunkSize & 0xFFFFFFF0))
				{
					alignment2 = ((CurrentChunkSize + 0x87) & 0xFFFFFF80) - CurrentChunkSize;
					alignment += alignment2;
				}

			}
			else
			{
				alignment = ((CurrentOffset)+ModelChunkRelativeLoc) & 0xFFFFFFF0;
				alignment = 0x10 - (((CurrentOffset)+ModelChunkRelativeLoc) - alignment);
			}

			// after that we check if we are at the aligned block already
			if (alignment > 0)
			{
				// if we aren't, then check how far we are, if anything LESS than 8 then we gotta wrap around by 0x10 (chunk size = alignment size + 8)
				if (alignment < 8)
				{
					ZeroChunkWriter(OutFile, alignment + 8);
				}

				// if SAME then just a zero chunk (chunk size = 0)
				if (alignment == 8)
				{
					ZeroChunkWriter(OutFile, 0);
				}

				// if MORE then just add to the zero chunk (chunk size = alignment - 8)
				if (alignment > 8)
				{
					ZeroChunkWriter(OutFile, alignment - 8);
				}
			}

		}
		else if (bPadding)
		{
			if (WriteCounter)
			{
				alignment = CurrentAlignedOffset - CurrentOffset;



				//PrePadSize = (CurrentOffset - (CurrentOffset % 0x80)) + 0x100;

				/*NextOffset = ((CurrentOffset + 0x87) & 0xFFFFFF80);
				PrePadSize = (NextOffset - CurrentOffset) - 8;*/

				/*printf("Current: %X\nNext: %X\nPrePad: %X\n", CurrentOffset, NextOffset, PrePadSize);
				getchar();*/

				/*PrePadSize = (CurrentOffset - (CurrentOffset % 0x80)) + 0x100;
				PrePadSize -= CurrentOffset + 8;*/

				// if we aren't, then check how far we are, if anything LESS than 8 then we gotta wrap around by 0x10 (chunk size = alignment size + 8)
				if (alignment < 8)
				{
					ZeroChunkWriter(OutFile, alignment + 8);
				}

				// if SAME then just a zero chunk (chunk size = 0)
				if (alignment == 8)
				{
					ZeroChunkWriter(OutFile, 0);
				}

				// if MORE then just add to the zero chunk (chunk size = alignment - 8)
				if (alignment > 8)
				{
					ZeroChunkWriter(OutFile, alignment - 8);
				}
			}
		}

		PreviousChunkSize = ((unsigned int*)FileBuf)[1];


		// then start writing to the output file
		fwrite(FileBuf, st.st_size, 1, OutFile);

		// dump everything
		free(FileBuf);
		fclose(InFile);

		WriteCounter++;
	}

	fclose(OutFile);
	fclose(TxtFile);

	printf("%s Written %d chunks in file %s\n", PRINTTYPEINFO, WriteCounter, OutFilename);

	return 0;
}


int main(int argc, char* argv[])
{
	printf("Chunky v%d by Xanvier\n", BUILDVER);

	if (argc <= 1)
	{
		printf("%s Too few arguments passed.\nUSAGE: %s [-x/-c] InFile [OutFile]\n-x = Extract chunks\n-c = Combine chunks, [OutFile] necessary\n-cn = Combine chunks (no padding) ", PRINTTYPEERROR, argv[0]);
		return -1;
	}

	if (argv[1][0] == '-')
	{
		switch (argv[1][1])
		{
		case 'x':
			bExtract = true;
			printf("%s Extracting chunks\n", PRINTTYPEINFO);
			break;
		case 'c':
			bCombine = true;

			if (argv[1][2] == 'n')
			{
				bPadding = false;
				printf("%s Combining chunks without padding\n", PRINTTYPEINFO);
			}
			else
				printf("%s Combining chunks\n", PRINTTYPEINFO);

			break;
		default:
			break;
		}
	}

	if (bCombine)
	{
		return CombineChunks(argv[2], argv[argc - 1]);
	}

	ScannerChunkDef = ReadChunkDefFile(CHUNKDEFFILENAME);
	ScannedChunksCount = ScanChunks(argv[argc - 1]);

	if (bExtract)
	{
		ExtractChunks(argv[2], ScannedChunksCount);
	}

	return 0;
}
