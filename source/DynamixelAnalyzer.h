#ifndef DYNAMIXEL_ANALYZER_H
#define DYNAMIXEL_ANALYZER_H

#include <Analyzer.h>
#include "DynamixelAnalyzerResults.h"
#include "DynamixelSimulationDataGenerator.h"

#define PACKET_TIMEOUT_MS 3		// If a packet takes this long bail 
class DynamixelAnalyzerSettings;
class ANALYZER_EXPORT DynamixelAnalyzer : public Analyzer2
{
public:
	DynamixelAnalyzer();
	virtual ~DynamixelAnalyzer();
	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();
	enum INSTRUCTION
	{
	  NONE,
	  APING,
	  READ,
	  WRITE,
	  REG_WRITE,
	  ACTION,
	  RESET,
	  STATUS=0x55,					// Protocol 2 status message
	  SYNC_READ=0x82,				// Protocol 2 only
	  SYNC_WRITE=0x83,
	  SYNC_READ_SERVOS=0xfe,			// Special case for additional parameters. 
      SYNC_WRITE_SERVO_DATA=0xff		// Special case for where we break up sync write into multiple frames. 
	};

	enum DECODE_STEP
	{
	  DE_HEADER1,
	  DE_HEADER2,
	  DE_ID,
	  DE_LENGTH,
	  DE_INSTRUCTION,
	  DE_DATA,
	  DE_CHECKSUM,
	  // Added to try to decode Protocol 2 packets
	  DE_P2_ID,
	  DE_P2_LENGTH1,
	  DE_P2_LENGTH2,
	  DE_P2_INSTRUCTION,
	  DE_P2_DATA,
	  DE_P2_CHECKSUM,
	  DE_P2_CHECKSUM2
	};

	enum SERVO_TYPE
	{
		AX_SERVOS,
		MX_SERVOS

	};

protected: //functions
	void ComputeSampleOffsets();
	U16 update_crc(U16 crc_accum, U8 data_byte);


protected: //vars
	std::auto_ptr< DynamixelAnalyzerSettings > mSettings;
	std::auto_ptr< DynamixelAnalyzerResults > mResults;
	AnalyzerChannelData* mSerial;

	DynamixelSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	std::vector<U32> mSampleOffsets;
	U32 mStartOfStopBitOffset;
	U32 mEndOfStopBitOffset;

	//Dynamixel analysis vars:
	int DecodeIndex;
    U8 mID;
    U8 mCount;
    U8 mLength;
    U8 mInstruction;
    U8 mData[ 256 ];
    U8 mChecksum;
	U8 mCRCFirstByte;
	U16 mCRC;
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //DYNAMIXEL_ANALYZER_H
