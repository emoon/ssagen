#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int64_t s_data[1024 * 1024];
static uint8_t* s_ptr = (uint8_t*)s_data;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ALIGNOF(t) __alignof__(t)
#define alloc(type) (type*)allocAligned(sizeof(type), ALIGNOF(type))
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

} Instruction;

typedef struct BasicBlock
{
	uint64_t id;

	Instruction* instructions;
	struct BasicBlock** blocks;

	int instructionCount;
	int basicBlocks;

	int instructionStart;
	int instructionEnd;

} BasicBlock;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test code for generating SSA From
// Assume simple assembly where all opts are 3 arguments except branches (all instructions are of same size)
// There is one branch instruction that jump 32 bits forward/backwards (b -1 (jump 1 instruction back)


static Instruction testCode[] =
{
	{ "add", "c", "c", "temp" },

	{ "add", "c", "c", "c" },
	{ "b", "-1", },
	{ "mov", "out", "c" },
	{ "ret", },
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int sortOffsets(const void* a, const void* b)
{
	return (*(int*)a - *(int*)b);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// First build a Control Flow Graph (CFG) of the code

BasicBlock* buildCFG(Instruction* instructions, const int instructionCount)
{
	int* offsets = allocArray(int, instructionCount);
	int offsetCount = 1;

	memset(offsets, 0x0f, sizeof(int) * instructionCount); // clear to big value
	offsets[0] = 0; // the first block is always a basic block

	// First pass we just parse the instruction stream to find labels 

	for (int i = 0; i < instructionCount; ++i)
	{
		if (!strcmp("b", instructions[i].opCode))
		{
			int offset = atoi(instructions[i].regName0);
			offsets[offsetCount + 0] = i + offset; // branch target
			offsets[offsetCount + 1] = i + 1; // instruction after branch makes up the code as a basic block
			offsetCount += 2;
			i++;
		}
	}

	// end of the instruction stream is a offset as well
	offsets[offsetCount++] = instructionCount - 1;

	// sort the offsets

	printf("offsets pre-sort\n");

	for (int i = 0; i < offsetCount; ++i)
		printf("%d\n", offsets[i]);

	qsort(offsets, offsetCount, sizeof(int), sortOffsets);

	printf("offsets sort\n");

	for (int i = 0; i < offsetCount; ++i)
		printf("%d\n", offsets[i]);

	// Create the code blocks and assign the instructions

	int blockCount = offsetCount - 1;

	BasicBlock* blocks = allocArray(BasicBlock, blockCount);

	for (int i = 0; i < blockCount; ++i)
	{
		BasicBlock* block = &blocks[i];
		block->id = i;
		block->instructions = &instructions[offsets[i]];
		block->instructionCount = offsets[i + 1] - offsets[i]; 
		block->instructionStart = offsets[i];
		block->instructionEnd = offsets[i + 1];

		printf("block->id %d\n", (int)block->id);
		printf("block->instructions     %p\n", block->instructions);
		printf("block->instructionCount %d\n", block->instructionCount);
		printf("block->instructionStart %d\n", block->instructionStart);
		printf("block->instructionEnd   %d\n", block->instructionEnd);
	}

	// dump out the blocks
	printf("blockCount %d\n", blockCount);

	for (int i = 0; i < blockCount; ++i)
	{
		printf("id %d\n", (int)blocks[i].id);

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

	return blocks;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main()
{
	buildCFG(testCode, sizeof_array(testCode));
}
