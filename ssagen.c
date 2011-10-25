#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int64_t s_data[1024 * 1024];
static uint8_t* s_ptr = (uint8_t*)s_data;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ALIGNOF(t) __alignof__(t)
#define alloc(type) (type*)allocAligned(sizeof(type), ALIGNOF(type))
#define allocZero(type) (type*)allocZeroAligned(sizeof(type), ALIGNOF(type))
#define allocArray(type, count) (type*)allocAligned(sizeof(type) * count, ALIGNOF(type))
#define allocZeroArray(type, count) (type*)allocZeroAligned(sizeof(type) * count, ALIGNOF(type))
#define sizeof_array(array) (int)(sizeof(array) / sizeof(array[0]))

void* allocAligned(uint32_t size, uint32_t alignment)
{
	// Align the pointer to the current 

	intptr_t ptr = (intptr_t)s_ptr;	
	uint32_t bitMask = (alignment - 1);
	uint32_t lowBits = ptr & bitMask;
	uint32_t adjust = ((alignment - lowBits) & bitMask);

	// adjust the pointer to correct alignment
	s_ptr += adjust;

	void* retData = s_ptr;
	s_ptr += size;
	return retData;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* allocPtr()
{
	return s_ptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* allocZeroAligned(uint32_t size, uint32_t alignment)
{
	void* data = allocAligned(size, alignment);
	memset(data, 0, size);
	return data;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct Instruction
{
	const char* opCode;
	const char* regName0;
	const char* regName1;
	const char* regName2;
	bool leader;
	bool branch;
	bool condBranch;

} Instruction;

typedef struct BasicBlock
{
	uint64_t id;

	Instruction* instructions;
	struct BasicBlock* edges[16];		// temporary fixed size but should be fine for testing

	int instructionCount;
	int edgesCount;

	int instructionStart;
	int instructionEnd;

} BasicBlock;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test code for generating SSA From
// Assume simple assembly where all opts are 3 arguments except branches (all instructions are of same size)
// There is one branch instruction that jump 32 bits forward/backwards (b -1 (jump 1 instruction back)


static Instruction testCode[] =
{
	{ "bc", "6", "B" }, // goto B
	{ "inst", "1" },
	{ "inst", "2" },
	{ "bc", "6", "C" },
	{ "inst", "3" },
	{ "inst", "4" },
	{ "bc", "-6", "A" },
	{ "inst", "5" },
	{ "b", "1", "C" },
	{ "inst", "6" },
	{ "inst", "7" },
	{ "inst", "8" },
};

/*
A: if x goto B
inst 1
inst 2
jump C
inst 3
inst 4
B: if y goto A
inst 5
jump C
C: inst 6
inst 7
inst 8
*/


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// First build a Control Flow Graph (CFG) of the code

BasicBlock* buildCFG(Instruction* instructions, const int instructionCount)
{
	// First pass we just mark instructions if they are leaders or not 

	instructions[0].leader = true; // Rule 1: First instruction is leader

	for (int i = 0; i < instructionCount; ++i)
	{
		bool branch = !strcmp("b", instructions[i].opCode); 
		bool condBranch = !strcmp("bc", instructions[i].opCode); 

		if (branch || condBranch)
		{
			int offset = atoi(instructions[i].regName0);
			instructions[i + offset].leader = true;			// Rule 2: Target for branch is leader

			if (i != instructionCount-1)
				instructions[i + 1].leader = true;			// Rule 3: instruction after branch is leader 
		}

		// cache branch status

		instructions[i].branch = branch;
		instructions[i].condBranch = condBranch;
	}

	// Second pass build basic blocks from leaders
	
	int basicBlockCount = 0;

	int count = 0;
	BasicBlock* block = 0;
	BasicBlock* blocks = allocPtr();

	for (int i = 0; i < instructionCount; ++i)
	{
		if (instructions[i].leader)
		{
			if (block)
			{
				block->instructionCount = count;
				block->instructionStart = i - count;
				block->instructionEnd = i;
				count = 0;
			}

			block = allocZero(BasicBlock);
			block->instructions = &instructions[i];

			block->id = basicBlockCount;
			basicBlockCount++;
		}

		count++;
	}

	// finish last block if we got one

	if (block)
	{
		block->instructionCount = count;
		block->instructionStart = instructionCount - count;
		block->instructionEnd = instructionCount;
	}

	// Pass 3. Link the BasicBlocks togheter

	for (int b = 0; b < basicBlockCount-1; ++b)
	{
		BasicBlock* block = &blocks[b];
		
		for (int i = 0, instCount = block->instructionCount; i < instCount; ++i)
		{
			if (block->instructions[i].branch || block->instructions[i].condBranch)
			{
				int offset = atoi(block->instructions[i].regName0);
				int instructionOffset = block->instructionStart + i + offset;

				// find the block to link with
				
				for (int f = 0; f < basicBlockCount; ++f)
				{
					BasicBlock* tempBlock = &blocks[f];

					// Check if inside range and link if that is the case

					if (instructionOffset >= tempBlock->instructionStart &&
						instructionOffset < tempBlock->instructionEnd)
					{
						block->edges[block->edgesCount++] = tempBlock;
						break;
					}
				}
			}
		}

		// if last instruction isn't a unconditonal branch link the block with the next one

		if (!block->instructions[block->instructionCount - 1].branch)
			block->edges[block->edgesCount++] = &blocks[b + 1];
	}
	/*
	for (int i = 0; i < basicBlockCount; ++i)
	{
		BasicBlock* block = &blocks[i];

		printf("==================================\n");
		//printf("id %d\n", (int)blocks[i].id);
		
		printf("block	                %p\n", block);
		printf("block->edgesCount       %d\n", block->edgesCount);
		printf("block->firstEdge        %p\n", block->edges[0]);
		printf("block->instructions     %p\n", block->instructions);
		printf("block->instructionCount %d\n", block->instructionCount);
		printf("block->instructionStart %d\n", block->instructionStart);
		printf("block->instructionEnd   %d\n", block->instructionEnd);

		for (int count = 0; count < blocks[i].instructionCount; ++count)
		{
			printf("%s ", blocks[i].instructions[count].opCode);

			if (blocks[i].instructions[count].regName0)
				printf("%s ", blocks[i].instructions[count].regName0);
			if (blocks[i].instructions[count].regName1)
				printf("%s ", blocks[i].instructions[count].regName1);
			if (blocks[i].instructions[count].regName2)
				printf("%s ", blocks[i].instructions[count].regName2);

			printf("\n");
		}

		printf("\n");
	}
	*/

	// dump out a graphviz file

	FILE* file = fopen("/Users/daniel/test.gv", "wt");

	fprintf(file, "digraph G {\ngraph [\nrankdir = \"LR\"\n];\nnode [\nfontsize = \"16\"\n shape = \"ellipse\"\n];\n edge [\n];\n");

	for (int i = 0; i < basicBlockCount; ++i)
	{
		BasicBlock* block = &blocks[i];

		fprintf(file, "\"node%d\" [\nlabel = \"", (int)block->id);

		for (int count = 0; count < block->instructionCount; ++count)
		{
			fprintf(file, " %s ", blocks[i].instructions[count].opCode);

			if (blocks[i].instructions[count].regName0)
				fprintf(file, "%s ", blocks[i].instructions[count].regName0);
			if (blocks[i].instructions[count].regName1)
				fprintf(file, "%s ", blocks[i].instructions[count].regName1);
			if (blocks[i].instructions[count].regName2)
				fprintf(file, "%s ", blocks[i].instructions[count].regName2);

			fprintf(file, "\\n");
		}

		fprintf(file, "\"\nshape = \"ellipse\"\n];\n");

		//for (int c = 0; c < block->edgesCount; ++c)
		//	fprintf(file, "b%d -> b%d;\n", (int)block->id, (int)block->edges[c]->id);
	}

	for (int i = 0; i < basicBlockCount; ++i)
	{
		BasicBlock* block = &blocks[i];

		for (int c = 0; c < block->edgesCount; ++c)
			fprintf(file, "\"node%d\":f0 -> \"node%d\":f0 [\nid = %d\n];\n", (int)block->id, (int)block->edges[c]->id, (int)block->id);
	}

	fprintf(file, "}\n");
	fclose(file);

	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
	buildCFG(testCode, sizeof_array(testCode));
}
