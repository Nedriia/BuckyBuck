//
// Created by arthu on 09/07/2026.
//
#ifndef BUCKYBUCK_DISASSEMBLER_H
#define BUCKYBUCK_DISASSEMBLER_H
#include <cstdint>

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

	static CPU* m_pCPU;
};


#endif //BUCKYBUCK_DISASSEMBLER_H
