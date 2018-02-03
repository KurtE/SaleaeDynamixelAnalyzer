#ifndef DYNAMIXEL_ANALYZER_RESULTS
#define DYNAMIXEL_ANALYZER_RESULTS

#include <AnalyzerResults.h>

class DynamixelAnalyzer;
class DynamixelAnalyzerSettings;

class DynamixelAnalyzerResults : public AnalyzerResults
{
public:
	DynamixelAnalyzerResults( DynamixelAnalyzer* analyzer, DynamixelAnalyzerSettings* settings );
	virtual ~DynamixelAnalyzerResults();

	virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
	virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base );
	virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
	virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

protected: //functions
	const char * GetServoRegisterName(U8 servo_id, U16 register_number, bool protocol_2 = false);
	U8 IsRegisterIndexStartOfMultipleByteValue(U8 servo_id, U16 register_number, bool protocol_2 = false);
	U8 NextDataValue(U8 servo_id, U16 register_index, U8 data_index, U8 * Data, U8 data_length, bool protocol_2, U32 & w_val);

protected:  //vars
	DynamixelAnalyzerSettings* mSettings;
	DynamixelAnalyzer* mAnalyzer;
};

#endif //DYNAMIXEL_ANALYZER_RESULTS
