#include "DynamixelAnalyzer.h"
#include "DynamixelAnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <AnalyzerHelpers.h>

DynamixelAnalyzer::DynamixelAnalyzer()
:	Analyzer2(),  
	mSettings( new DynamixelAnalyzerSettings() ),
	mSimulationInitilized( false ),
	DecodeIndex( 0 )
{
	SetAnalyzerSettings( mSettings.get() );
}

DynamixelAnalyzer::~DynamixelAnalyzer()
{
	KillThread();
}

void DynamixelAnalyzer::ComputeSampleOffsets()
{
	ClockGenerator clock_generator;
	clock_generator.Init(mSettings->mBitRate, mSampleRateHz);

	mSampleOffsets.clear();

	U32 num_bits = 8;

	mSampleOffsets.push_back(clock_generator.AdvanceByHalfPeriod(1.5));  //point to the center of the 1st bit (past the start bit)
	num_bits--;  //we just added the first bit.

	for (U32 i = 0; i<num_bits; i++)
	{
		mSampleOffsets.push_back(clock_generator.AdvanceByHalfPeriod());
	}

	//to check for framing errors, we also want to check
	//1/2 bit after the beginning of the stop bit
	mStartOfStopBitOffset = clock_generator.AdvanceByHalfPeriod(1.0);  //i.e. moving from the center of the last data bit (where we left off) to 1/2 period into the stop bit
}


void DynamixelAnalyzer::SetupResults()
{
	mResults.reset(new DynamixelAnalyzerResults(this, mSettings.get()));
	SetAnalyzerResults(mResults.get());
	mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannel);
}


void DynamixelAnalyzer::WorkerThread()
{
	mSampleRateHz = GetSampleRate();
	ComputeSampleOffsets();

	mSerial = GetAnalyzerChannelData( mSettings->mInputChannel );

	if( mSerial->GetBitState() == BIT_LOW )
		mSerial->AdvanceToNextEdge();

	U32 samples_per_bit = mSampleRateHz / mSettings->mBitRate;
//	U32 samples_to_first_center_of_first_current_byte_bit = U32( 1.5 * double( mSampleRateHz ) / double( mSettings->mBitRate ) );

	U64 starting_sample = 0;
	U64 data_samples_starting[256];		// Hold starting positions for all possible data byte positions. 

	U8 previous_ID = 0xff;
	U8 previous_instruction = 0xff;
	U8 previous_reg_start = 0xff;

	Frame frame;
	bool  output_new_frame = false;
	bool protocol_2 = false; 
	for( ; ; )
	{
		U8 current_byte = 0;
		U8 mask = 1 << 0;
		

		// Lets verify we have the right state for a start bit. 
		// before we continue. and like wise don't say it is a byte unless it also has the right stop bit...
		do {

			do {
				mSerial->AdvanceToNextEdge(); //falling edge -- beginning of the start bit

			} while ((mSerial->GetBitState() == BIT_HIGH));		// start bit should be logicall low. 

			//U64 starting_sample = mSerial->GetSampleNumber();
			if (DecodeIndex == DE_HEADER1)
			{
				starting_sample = mSerial->GetSampleNumber();
			}
			else
			{
				// Try checking for packets that are taking too long. 
				U64 packet_time_ms = (mSerial->GetSampleNumber() - starting_sample) / (mSampleRateHz / 1000);
				if (packet_time_ms > PACKET_TIMEOUT_MS)
				{
					DecodeIndex = DE_HEADER1;
					starting_sample = mSerial->GetSampleNumber();
				}
				else if ((DecodeIndex == DE_DATA) || (DecodeIndex == DE_P2_DATA))
				{
                    data_samples_starting[mCount] = mSerial->GetSampleNumber();
				}
			}

//			mSerial->Advance(samples_to_first_center_of_first_current_byte_bit);

			for (U32 i = 0; i < 8; i++)
			{
				//let's put a dot exactly where we sample this bit:
				//NOTE: Dot, ErrorDot, Square, ErrorSquare, UpArrow, DownArrow, X, ErrorX, Start, Stop, One, Zero
				//mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Start, mSettings->mInputChannel );
				mSerial->Advance(mSampleOffsets[i]);
				if (mSerial->GetBitState() == BIT_HIGH)
					current_byte |= mask;

//				mSerial->Advance(samples_per_bit);
				mask = mask << 1;
			}
			mSerial->Advance(mStartOfStopBitOffset);
		} while (mSerial->GetBitState() != BIT_HIGH);		// Stop bit should be logically high

		//Process new byte
		output_new_frame = false; // assume we are not outputting a new frame here...

		switch ( DecodeIndex )
		{
			case DE_HEADER1:
				mCRC = 0;	// Make sure our CRC is zeroed out
				mCRCFirstByte = 0;	
				protocol_2 = false; // assume not 2
				if ( current_byte == 0xFF )
				{
					DecodeIndex = DE_HEADER2;
					//starting_sample = mSerial->GetSampleNumber();
					mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
				}
				else
				{
					mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mInputChannel );
				}
			break;
			case DE_HEADER2:
				if ( current_byte == 0xFF )
				{
					DecodeIndex = DE_ID;
					mChecksum = 1;
					mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
				}
				else
				{
					DecodeIndex = DE_HEADER1;
					mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::ErrorSquare, mSettings->mInputChannel );
				}
			break;
			case DE_ID:
				if ( current_byte != 0xFF )    // we are not allowed 3 0xff's in a row, ie. id != 0xff
				{   
					mID = current_byte;
					DecodeIndex = DE_LENGTH;
					mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel );
				}
				else
				{
					DecodeIndex = DE_HEADER1;
					mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::ErrorX, mSettings->mInputChannel );
				}
			break;
			case DE_LENGTH:
				if ((current_byte == 0) && (mID == 0xfd))
				{
					DecodeIndex = DE_P2_ID;	// Looks like Protocol 2 message next byte should be index.
					mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel);
				}
				else
				{
					mLength = current_byte;
					DecodeIndex = DE_INSTRUCTION;
					mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel);
				}
			break;
			case DE_INSTRUCTION:
				mInstruction = current_byte;
				mCount = 0;
				DecodeIndex = DE_DATA;
				mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::DownArrow, mSettings->mInputChannel );
				if ( mLength == 2 ) DecodeIndex = DE_CHECKSUM;
				for (U8 i = 0; i < 13; i++)
					mData[i] = 0;	// Lets clear out any data for the heck of it...
			break;
			case DE_DATA:
				mData[ mCount++ ] = current_byte;
				mResults->AddMarker( mSerial->GetSampleNumber(), AnalyzerResults::UpArrow, mSettings->mInputChannel );
				if ( mCount >= mLength - 2 )
				{
					DecodeIndex = DE_CHECKSUM;
				}
			break;
			case DE_CHECKSUM:
				//We have a new frame to save! 
				output_new_frame = true;
				frame.mFlags = 0;	// clear flags as we may set error one depending on checksum...

				DecodeIndex = DE_HEADER1;
				if ((~mChecksum & 0xff) == (current_byte & 0xff))
				{
					// Checksums match!
					mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel);
				}
				else
				{
					mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::ErrorDot, mSettings->mInputChannel);
					frame.mFlags = DISPLAY_AS_ERROR_FLAG;
				}

				break;

			// Here is where we diverged for Prototol 2 messages
			case DE_P2_ID:
				mID = current_byte;
				protocol_2 = true;	
				DecodeIndex = DE_P2_LENGTH1;
				mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::One, mSettings->mInputChannel);
				break;
			case DE_P2_LENGTH1:
				mLength = current_byte;
				DecodeIndex = DE_P2_LENGTH2;
				mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel);
				break;
			case DE_P2_LENGTH2:
				mLength += (current_byte << 8);
				DecodeIndex = DE_P2_INSTRUCTION;
				mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Zero, mSettings->mInputChannel);
				break;
			case DE_P2_INSTRUCTION:
				mInstruction = current_byte;
				mCount = 0;
				DecodeIndex = DE_P2_DATA;
				mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::DownArrow, mSettings->mInputChannel);
				if (mLength == 3) DecodeIndex = DE_P2_CHECKSUM;
				for (U8 i = 0; i < 13; i++)
					mData[i] = 0;	// Lets clear out any data for the heck of it...
				break;
			case DE_P2_DATA:
				mData[mCount++] = current_byte;
				mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::UpArrow, mSettings->mInputChannel);
				if (mCount >= mLength - 3)
				{
					DecodeIndex = DE_P2_CHECKSUM;
				}
				break;
			case DE_P2_CHECKSUM:
				mCRCFirstByte = current_byte;
				mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel);
				DecodeIndex = DE_P2_CHECKSUM2;
				break;
			case DE_P2_CHECKSUM2:
				output_new_frame = true;	// we will output a frame. 
				frame.mFlags = 0;	// clear flags as we may set error one depending on checksum...

				DecodeIndex = DE_HEADER1;
				if ((mCRCFirstByte == (mCRC & 0xff)) && (current_byte == ((mCRC >> 8) & 0xff)))
				{
					// Checksums match!
					mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel);
				}
				else
				{
					mResults->AddMarker(mSerial->GetSampleNumber(), AnalyzerResults::ErrorDot, mSettings->mInputChannel);
					frame.mFlags = DISPLAY_AS_ERROR_FLAG;
				}

			break;
		}

		// Now see if the code signalled that we should add a new frame. 
		if (output_new_frame)
		{

			//
			// Lets build our Frame. Right now all are done same way, except lets try to special case SYNC_WRITE, as we won't likely fit all 
			// of the data into one frame... So break out each servos part as their own frame 
			// For Protocol 2 we store both bytes of CRC if Protocol 1 we store just checksum
			frame.mType = mInstruction;		// Save the packet type in mType
			if (protocol_2) 
				frame.mData1 = mID | (mCRC << (1 * 8)) | (mLength << (3 * 8));
			else
				frame.mData1 = mID | (mChecksum << (1 * 8)) | (mLength << (3 * 8));
			for (U8 i = 0; i < 4; i++)
				frame.mData1 |= (((U64)(mData[i])) << ((i + 4) * 8));

			// Use mData2 to store up to 8 bytes of the packet data. 
			if (frame.mFlags == 0)
			{
				// Try hack if Same ID as previous and Instruction before was read and this is a 0 response then 
				// Maybe encode starting register as mData[12]
				if ((mInstruction == DynamixelAnalyzer::NONE) || (mInstruction == DynamixelAnalyzer::STATUS))
				{
					if (((mID == previous_ID) && (previous_instruction == DynamixelAnalyzer::READ))
						 || (previous_instruction == DynamixelAnalyzer::SYNC_READ) || (previous_instruction == DynamixelAnalyzer::STATUS))
						mData[11] = previous_reg_start;
					else
						mData[11] = 0xff;
				}

				frame.mData2 = mData[4];
				for (U8 i = 1; i < 8; i++)
					frame.mData2 |= (((U64)(mData[i + 4])) << (i * 8));
			}
			else
				frame.mData2 = (mCRCFirstByte << 8) | current_byte;	// in error frame have mData2 with the checksum byte. 

			frame.mStartingSampleInclusive = starting_sample;

			// See if we are doing a SYNC_WRITE...  But not if error!
			if ((mInstruction == SYNC_WRITE) && (mLength > 4) && (frame.mFlags == 0))
			{
				U8 count_of_servos;
				U8 data_index;
				U16 register_count;
				U16 register_start;
				if (protocol_2)
				{
					// MData: <rs low><rs high><rc low><rc high><Servo 1 data><Servo2 data>
					register_start = mData[0] + (mData[1] << 8);
					register_count = mData[2] + (mData[3] << 8);
					count_of_servos = (mLength - 7) / (register_count + 1);	// Should validate this is correct but will try this for now...
					data_index = 4;
				}
				else
				{
					// MData: <Reg start><Regcount><Servo one data><servo two data>
					register_start = mData[0];
					register_count = mData[1];
					count_of_servos = (mLength - 4) / (register_count + 1);	// Should validate this is correct but will try this for now...
					data_index = 2;

				}
				// Add Header Frame. 
				// Data byte: <start reg><reg count> 

				frame.mEndingSampleInclusive = data_samples_starting[data_index - 1] + samples_per_bit * 10;

				mResults->AddFrame(frame);
				ReportProgress(frame.mEndingSampleInclusive);

				// Now lets figure out how many frames to add plus bytes per frame
				if (register_count && (register_count <= 8) && count_of_servos /*&& ((count_of_servos *(mData[1]+1)+4) == mLength)*/)
				{
					frame.mType = SYNC_WRITE_SERVO_DATA;
					for (U8 iServo = 0; iServo < count_of_servos; iServo++)
					{
						frame.mStartingSampleInclusive = data_samples_starting[data_index];
						//frame.mStartingSampleInclusive = frame.mEndingSampleInclusive + 1;
						if ((iServo + 1) < count_of_servos)
							frame.mEndingSampleInclusive = data_samples_starting[data_index + register_count] + samples_per_bit * 10;
						else
							frame.mEndingSampleInclusive = mSerial->GetSampleNumber();
						// Now to encode the data bytes. 
						// mData1 - Maybe Servo ID, 0, 0, Starting index, count bytes < updated same as other packets, but 
						// mData2 - Up to 8 bytes per servo... Could pack more... but
						// BUGBUG Should verify that count of bytes <= 8
						if (protocol_2)  // Note: the ff0000 should let the other side know that we are Protocol2...
							frame.mData1 = mData[data_index++] | 0xff0000 | ((U64)(register_start & 0xff) << (4 * 8)) | ((U64)((register_start >> 8) & 0xff) << (5 * 8))
							| ((U64)(register_count & 0xff) << (6 * 8)) | ((U64)((register_count >> 8) & 0xff) << (7 * 8));
						else
							frame.mData1 = mData[data_index++] | ((U64)(register_start & 0xff) << (4 * 8)) | ((U64)(register_count & 0xff) << (5 * 8));

						frame.mData2 = 0;
						for (U8 i = 0; i < register_count; i++)
							frame.mData2 |= (U64)mData[data_index + i] << (i * 8);

						data_index += register_count;	// advance to start of next one...
						// Now try to report this one. 
						mResults->AddFrame(frame);
						ReportProgress(frame.mEndingSampleInclusive);
					}
				}

			}
			else
			{
				// Normal frames...
				frame.mEndingSampleInclusive = mSerial->GetSampleNumber();
				mResults->AddFrame(frame);
				ReportProgress(frame.mEndingSampleInclusive);
			}
			previous_ID = mID;
			previous_instruction = mInstruction;
			switch (previous_instruction) {
			case  DynamixelAnalyzer::READ:
			case DynamixelAnalyzer::SYNC_READ:
				previous_reg_start = mData[0];	// 0 should be reg start...
				break;
			case DynamixelAnalyzer::STATUS:
				break; // leave previous value...
			default:
				previous_reg_start = 0xff;		// Not read...
			}
		}

		// Update checksums... 
		mChecksum += current_byte;
		if (DecodeIndex != DE_P2_CHECKSUM2)
			mCRC = update_crc(mCRC, current_byte);

		mResults->CommitResults();
	}
}
//==========================================================================================
// Protocol 2 stuff. 
U16  DynamixelAnalyzer::update_crc(U16 crc_accum, U8 data_byte)
{
	static const U16 crc_table[256] = {
		0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,
		0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027, 0x0022,
		0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072,
		0x0050, 0x8055, 0x805F, 0x005A, 0x804B, 0x004E, 0x0044, 0x8041,
		0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2,
		0x00F0, 0x80F5, 0x80FF, 0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1,
		0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1,
		0x8093, 0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
		0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197, 0x0192,
		0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1,
		0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB, 0x01FE, 0x01F4, 0x81F1,
		0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2,
		0x0140, 0x8145, 0x814F, 0x014A, 0x815B, 0x015E, 0x0154, 0x8151,
		0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162,
		0x8123, 0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
		0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104, 0x8101,
		0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312,
		0x0330, 0x8335, 0x833F, 0x033A, 0x832B, 0x032E, 0x0324, 0x8321,
		0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371,
		0x8353, 0x0356, 0x035C, 0x8359, 0x0348, 0x834D, 0x8347, 0x0342,
		0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1,
		0x83F3, 0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
		0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7, 0x03B2,
		0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381,
		0x0280, 0x8285, 0x828F, 0x028A, 0x829B, 0x029E, 0x0294, 0x8291,
		0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2,
		0x82E3, 0x02E6, 0x02EC, 0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2,
		0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1,
		0x8243, 0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,
		0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264, 0x8261,
		0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231,
		0x8213, 0x0216, 0x021C, 0x8219, 0x0208, 0x820D, 0x8207, 0x0202
	};

	U16	i = ((uint16_t)(crc_accum >> 8) ^ data_byte) & 0xFF;
	return (crc_accum << 8) ^ crc_table[i];
}


bool DynamixelAnalyzer::NeedsRerun()
{
	return false;
}

U32 DynamixelAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 DynamixelAnalyzer::GetMinimumSampleRateHz()
{
	return mSettings->mBitRate * 4;
}

const char* DynamixelAnalyzer::GetAnalyzerName() const
{
	return "Dynamixel Protocol";
}

const char* GetAnalyzerName()
{
	return "Dynamixel Protocol";
}

Analyzer* CreateAnalyzer()
{
	return new DynamixelAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}