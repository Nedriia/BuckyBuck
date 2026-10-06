//
// Created by arthu on 09/07/2026.
//
#ifndef BUCKYBUCK_DISASSEMBLER_H
#define BUCKYBUCK_DISASSEMBLER_H
#include <cstdint>
#include <cstring>
#include <vector>

#include "nlohmann/json_fwd.hpp"

struct CBORWriter;
class CPU;
class Disassembler
{
public:
	Disassembler();
	~Disassembler();

	static void Disassemble_ROM( const char* sRomPath );
	static std::string Format( const char* sFormat, ... );
	static void DecryptCartridge( CBORWriter& oData );
	static void DecryptIORange();
	void Init();

private:
	static void _WriteInstruction( CBORWriter& oData, uint16_t _iAdress, uint8_t* iLengthIncrease = nullptr );
	struct DisassembledLine
	{
		uint16_t				m_iAdress = 0;
		uint8_t					m_iDuration = 0;
		char					m_sMnemonic[ 32 ] = { '\0' };
		char					m_sAditionalInfo[ 32 ] = { '\0' };
		char					m_aData[ 32 ] = { '\0' };
	};

	static std::vector< DisassembledLine > m_aDisassembly;
	static CPU* m_pCPU;

public:
	static const std::vector< DisassembledLine >& GetDisassemblyInstructions() { return m_aDisassembly; }
	static void ToHex( const uint8_t* Src, char* Dst, int len );
};

struct CBORWriter
{
	std::vector<uint8_t> buffer;

	void Head( const uint8_t mt, const uint64_t iData )
	{
		const uint8_t major = static_cast< uint8_t >( mt << 5 );

		if( iData < 24 )
			buffer.push_back( major | static_cast<uint8_t>( iData ) );
		else if( iData <= 0xFF )
		{	buffer.push_back( major | 24 ); buffer.push_back( static_cast<uint8_t>( iData ) ); }
		else if( iData <= 0xFFFF )
		{	buffer.push_back( major | 25 ); buffer.push_back( static_cast< uint8_t >( iData >> 8 ) ); buffer.push_back( static_cast< uint8_t >( iData ) );}
		else {
			buffer.push_back( major | 26 );
			for( int s = 24; s >= 0; s -= 8 ) buffer.push_back( static_cast< uint8_t >( iData >> s ) );
		}
	}

	void UInt( uint64_t iData ){ Head( 0, iData ); }
	void Text( const char* sText, size_t n )
	{
		Head( 3, n );
		buffer.insert( buffer.end(), sText, sText + n );
	}
	void Key( const char key )
	{
		buffer.push_back( 0x61 ); // 3 << 5 | 1 ( limit to 1 char key )
		buffer.push_back( static_cast<uint8_t>( key ) );
	}
	void Map( size_t n ){ Head( 5, n ); }
	void Array_Begin()	{ buffer.push_back( 0x9F ); }
	void Break()		{ buffer.push_back( 0xFF ); }
};

static void WriteBlock( CBORWriter& oCborWriter,uint16_t iAdress,const char* sHex, const char* sMnemo,const std::string& sComment,uint8_t iDuration )
{
	oCborWriter.Map( 1 + ( sHex[ 0 ] != 0 ) + ( sMnemo[ 0 ] != 0 ) + !sComment.empty() + ( iDuration != 0 ) );
									oCborWriter.Key( 'a' );			oCborWriter.UInt( iAdress );

	if( sMnemo[ 0 ] ){				oCborWriter.Key( 'm' );			oCborWriter.Text( sMnemo,strlen( sMnemo ) );}
	if( sComment[ 0 ] ){			oCborWriter.Key( 'c' );			oCborWriter.Text( sComment.data(),sComment.size() );}
	if( sHex[ 0 ] ){				oCborWriter.Key( 'b' );			oCborWriter.Text( sHex,strlen( sHex ) ); }
	if( iDuration ){				oCborWriter.Key( 'd' );			oCborWriter.UInt( iDuration );}
}

template<size_t N>
static void CopyTrunc( char (&sDst)[N], const char* sSrc )
{
	size_t iLen = std::min( strlen(sSrc), N - 1 );
	memcpy( sDst,sSrc,iLen );
	sDst[iLen] = '\0';
}

#endif //BUCKYBUCK_DISASSEMBLER_H
