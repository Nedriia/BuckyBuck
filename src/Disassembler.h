//
// Created by arthu on 09/07/2026.
//
#ifndef BUCKYBUCK_DISASSEMBLER_H
#define BUCKYBUCK_DISASSEMBLER_H
#include <cstdint>
#include <cstring>
#include <vector>

#include <nlohmann/json.hpp>

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
	static void DecryptIORange( CBORWriter& oData );
	void Init();

private:
	static uint8_t _WriteInstruction( CBORWriter& oData, uint16_t _iAdress );
	struct DisassembledLine
	{
		uint16_t				m_iAdress = 0;
		uint8_t					m_iDuration = 0;
		char					m_sMnemonic[ 32 ] = { '\0' };
		char					m_sAditionalInfo[ 32 ] = { '\0' };
		char					m_aData[ 32 ] = { '\0' };
		char					m_aDataBis[ 32 ] = { '\0' };
		char					m_aLabel[ 32 ] = { '\0' };
	};

	static std::vector< DisassembledLine >	m_aDisassembly;
	static CPU*								m_pCPU;

public:
	static const std::vector< DisassembledLine >& GetDisassemblyInstructions() { return m_aDisassembly; }
	static void ToHex( const uint16_t iSrcAdress, char* Dst, int len, bool bConcat = false );

	friend class SAXParser;
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
	void SizePlaceHolder()
	{
		m_iPos = buffer.size();
		buffer.push_back( 0x1A );
		for( int i = 0; i < 4; ++i )
			buffer.push_back( 0 );
	}
	const size_t&	GetPosFixedSize() const { return m_iPos; }
	const size_t&	GetSize() const { return m_iSize; }
	void			IncrementInstructSize() { m_iSize++; }
private :
	size_t m_iPos = 0;
	size_t m_iSize = 0; //TODO : Check if opening two files 
};

static void WriteBlock( CBORWriter& oCborWriter,const char* sLabelAdress,uint16_t iAdress,const char* sHex, const char* sHexBis, const char* sMnemo,const std::string& sComment,uint8_t iDuration )
{
	uint8_t iBlockCount = ( sLabelAdress[ 0 ] != 0 ) + 1 + ( sHex[ 0 ] != 0 ) + ( sHexBis[ 0 ] != 0 ) + ( sMnemo[ 0 ] != 0 ) + !sComment.empty() + ( iDuration != 0 );
	oCborWriter.Map( iBlockCount );

	if( sLabelAdress[ 0 ] ) {		oCborWriter.Key( 'k' );			oCborWriter.Text( sLabelAdress,strlen( sLabelAdress ) ); }
									oCborWriter.Key( 'a' );			oCborWriter.UInt( iAdress );

	if( sMnemo[ 0 ] ){				oCborWriter.Key( 'm' );			oCborWriter.Text( sMnemo,strlen( sMnemo ) );}
	if( sComment[ 0 ] ){			oCborWriter.Key( 'c' );			oCborWriter.Text( sComment.data(),sComment.size() );}
	if( sHex[ 0 ] ){				oCborWriter.Key( 'b' );			oCborWriter.Text( sHex,strlen( sHex ) ); }
	if( iDuration ){				oCborWriter.Key( 'd' );			oCborWriter.UInt( iDuration );}
	if( sHexBis[ 0 ] ) {			oCborWriter.Key( 'z' );			oCborWriter.Text( sHexBis,strlen( sHexBis ) ); }

	//Patch size
	for( int i = 0; i < 4; ++i )
		oCborWriter.buffer[ oCborWriter.GetPosFixedSize() + 1 + i] = static_cast< uint8_t >( oCborWriter.GetSize() >> ( 24 - 8 * i ) );

	oCborWriter.IncrementInstructSize();
}

template<size_t N>
static void CopyTrunc( char( &sDst )[ N ],const char* sSrc )
{
	size_t iLen = std::min( strlen( sSrc ),N - 1 );
	memcpy( sDst,sSrc,iLen );
	sDst[ iLen ] = '\0';
}

struct SAXParser : public nlohmann::json_sax<nlohmann::json>
{
	// called when null is parsed
	bool null();

	// called when a boolean is parsed; value is passed
	bool boolean( bool val );

	// called when a signed or unsigned integer number is parsed; value is passed
	bool number_integer( number_integer_t val );
	bool number_unsigned( number_unsigned_t val );

	// called when a floating-point number is parsed; value and original string is passed
	bool number_float( number_float_t val,const string_t& s );

	// called when a string is parsed; value is passed and can be safely moved away
	bool string( string_t& val );
	// called when a binary value is parsed; value is passed and can be safely moved away
	bool binary( binary_t& val );

	// called when an object or array begins or ends, resp. The number of elements is passed (or -1 if not known)
	bool start_object( std::size_t elements );
	bool end_object();
	bool start_array( std::size_t elements );
	bool end_array();
	// called when an object key is parsed; value is passed and can be safely moved away
	bool key( string_t& val );

	// called when a parse error occurs; byte position, the last token, and an exception is passed
	bool parse_error( std::size_t position,const std::string& last_token,const nlohmann::json::exception& ex );


private:
	static uint8_t m_iElementFlag;
	static uint8_t m_iDepth;
	Disassembler::DisassembledLine* m_pLine = nullptr;
};

#endif //BUCKYBUCK_DISASSEMBLER_H
