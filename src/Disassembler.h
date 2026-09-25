//
// Created by arthu on 09/07/2026.
//
#ifndef BUCKYBUCK_DISASSEMBLER_H
#define BUCKYBUCK_DISASSEMBLER_H
#include <cstdint>
#include <vector>

class CPU;
class Disassembler
{
public:
	Disassembler();
	~Disassembler();

	static void Disassemble_ROM( const char* sRomPath );
	static std::string Format( const char* sFormat, ... );
	static void DecryptCartridge(std::fstream& file);
	static void DecryptIORange(std::fstream& file);
	void Init();

private:
	static void _WriteInstruction( std::fstream& file, const uint16_t iAdress, const char* sComment, uint8_t* iLengthIncrease = nullptr );
	struct DisassembledLine
	{
		std::string m_sText;
	};

	static std::vector< DisassembledLine > m_aDisassembly;
	static CPU* m_pCPU;

public:
	static const std::vector< DisassembledLine >& GetDisassemblyInstructions() { return m_aDisassembly; }
};


#endif //BUCKYBUCK_DISASSEMBLER_H
