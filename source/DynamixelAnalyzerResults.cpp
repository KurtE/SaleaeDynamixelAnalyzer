#include "DynamixelAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "DynamixelAnalyzer.h"
#include "DynamixelAnalyzerSettings.h"
#include <iostream>
#include <fstream>

DynamixelAnalyzerResults::DynamixelAnalyzerResults( DynamixelAnalyzer* analyzer, DynamixelAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

DynamixelAnalyzerResults::~DynamixelAnalyzerResults()
{
}

void DynamixelAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base )
{
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	// Note: the mData1 and mData2 are encoded with as much of the data as we can fit.
	// frame.mType has our = packet type
	// mData1 bytes low to high ID, Checksum, length, data0-data4
	// mData2  data5-12

	char id_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1 & 0xff, display_base, 8, id_str, 128 );
	char packet_checksum[8];
	AnalyzerHelpers::GetNumberString( ( frame.mData1 >> ( 1 * 8 ) ) & 0xff, display_base, 8, packet_checksum, 8 );

	char packet_length[8];
	U8 packet_count_bytes = (frame.mData1 >> (2 * 8)) & 0xff;
	AnalyzerHelpers::GetNumberString( packet_count_bytes, display_base, 8, packet_length, 8 );

	char packet_type = frame.mType;
	//char checksum = ( frame.mData2 >> ( 1 * 8 ) ) & 0xff;

	if ( packet_type == DynamixelAnalyzer::NONE )
	{
		// BUGBUG: This is just one case of response (no errors)
		// BUGBUG:: Almost direct cut and paste of write... Should cleanup use function...
		char reg_count[8];
		U8 count_data_bytes = packet_count_bytes - 2;
		AnalyzerHelpers::GetNumberString(count_data_bytes, display_base, 8, reg_count, 8);

		AddResultString("RP");
		AddResultString("REPLY");
		AddResultString("RP(", id_str, ")");
		AddResultString("RP(", id_str, ") LEN:", reg_count);
		// Try to build string showing bytes
		if (count_data_bytes)
		{
			// BUGBUG:: for now only handle 1 or 2 bytes...
			char w1_str[8];
			char params_str[40];
			U64 shift_data = frame.mData1 >> (3 * 8);
			AnalyzerHelpers::GetNumberString(shift_data & 0xff, display_base, 8, w1_str, 8);
			sprintf_s(params_str, sizeof(params_str), ") LEN: %s D: %s", reg_count, w1_str);
			AddResultString("RP(", id_str, params_str);

			if (count_data_bytes > 1)
			{
				char w2_str[8];
				shift_data >>= 8;	
				AnalyzerHelpers::GetNumberString(shift_data & 0xff, display_base, 8, w2_str, 8);
				sprintf_s(params_str, sizeof(params_str), ") LEN: %s D: %s, %s", reg_count, w1_str, w2_str);
				AddResultString("RP(", id_str,  params_str);

				if (count_data_bytes > 2)
				{
					// for more bytes lets try concatenating strings... 
					// we can reuse some of the above strings, as params_str has our first two parameters. 
					char remaining_data[100] = { 0 };
					if (count_data_bytes > 13)
						count_data_bytes = 13;	// Max bytes we can store
					for (U8 i = 2; i < count_data_bytes; i++)
					{
						if (i == 5)
						{
							AddResultString("RP(", id_str, params_str, remaining_data);  // Add partial so depends on how large space
							shift_data = frame.mData2;
						}
						else
							shift_data >>= 8;
						AnalyzerHelpers::GetNumberString(shift_data & 0xff, display_base, 8, w2_str, 8);	// reuse string; 
						strcat_s(remaining_data, sizeof(remaining_data), ", ");
						strcat_s(remaining_data, sizeof(remaining_data), w2_str);
					}
					AddResultString("RP(", id_str, params_str, remaining_data);  // Add full one. 
					AddResultString("REPLY(", id_str, params_str, remaining_data);  // Add full one. 
				}
			}
		}
	}
	else if ( packet_type == DynamixelAnalyzer::APING )
	{
		AddResultString( "P" );
		AddResultString( "PING" );
		AddResultString( "PING ID(", id_str , ")" );
	}
	else if ( packet_type == DynamixelAnalyzer::READ )
	{
		// bugbug: assuming packet length of 4 <reg> <len>
		char reg_start[8];
		AnalyzerHelpers::GetNumberString((frame.mData1 >> (3 * 8)) & 0xff, display_base, 8, reg_start, 8);
		char reg_count[8];
		AnalyzerHelpers::GetNumberString((frame.mData1 >> (4 * 8)) & 0xff, display_base, 8, reg_count, 8);

		AddResultString( "R" );
		AddResultString( "READ" );
		AddResultString( "RD(", id_str , ")" );
		AddResultString( "RD(", id_str , ") REG:", reg_start );
		AddResultString( "RD(", id_str , ") REG:", reg_start, " LEN:", reg_count);
		AddResultString( "READ(", id_str, ") REG:", reg_start, " LEN:", reg_count);
		//		AddResultString( "READ(", id_str , ") REG:", reg_start, " LEN:", reg_count, " CHKSUM: ", packet_checksum );
	}
	else if ( packet_type == DynamixelAnalyzer::WRITE )
	{
		char reg_start[8];
		char reg_count[8];
		U8 count_data_bytes = packet_count_bytes - 2;
		AnalyzerHelpers::GetNumberString((frame.mData1 >> (3 * 8)) & 0xff, display_base, 8, reg_start, 8);
		AnalyzerHelpers::GetNumberString(count_data_bytes, display_base, 8, reg_count, 8);

		AddResultString( "W" );
		AddResultString( "WRITE" );
		AddResultString( "WR(", id_str , ")" );
		AddResultString( "WR(", id_str, ") R:", reg_start);
		AddResultString( "WR(", id_str, ") R:", reg_start, " LEN:", reg_count);
		// Try to build string showing bytes
		if (count_data_bytes )
		{
			char w_str[8];
			U64 shift_data = frame.mData1 >> (3 * 8);
			// for more bytes lets try concatenating strings... 
			char remaining_data[120];
			sprintf_s(remaining_data, sizeof(remaining_data), ") LEN: %s D: ", reg_count);

			if (count_data_bytes > 13)
				count_data_bytes = 13;	// Max bytes we can store
			for (U8 i = 0; i < count_data_bytes; i++)
			{
				if (i == 2)
					AddResultString("WR(", id_str, ") R:", reg_start, remaining_data);
				if (i == 5)
				{
					AddResultString("WR(", id_str, ") R:", reg_start, remaining_data);
					shift_data = frame.mData2;
				}
				else
					shift_data >>= 8;

				AnalyzerHelpers::GetNumberString(shift_data & 0xff, display_base, 8, w_str, 8);	// reuse string; 
				if (i != 0)
					strcat_s(remaining_data, sizeof(remaining_data), ", ");
				strcat_s(remaining_data, sizeof(remaining_data), w_str);
			}
			AddResultString("WR(", id_str, ") R:", reg_start, remaining_data);
			AddResultString("WRITE(", id_str, ") R:", reg_start, remaining_data);
		}
		else
			AddResultString("WRITE(", id_str, ") R:", reg_start, " LEN:", reg_count);

	}
	else if ( packet_type == DynamixelAnalyzer::REG_WRITE )
	{
		AddResultString( "RW" );
		AddResultString( "REG_WRITE" );
		AddResultString( "REG_WRITE ID(", id_str , ")" );
		AddResultString( "REG_WRITE ID(", id_str , ") LEN(", packet_length, ")" );
		AddResultString( "REG_WRITE ID(", id_str , ") LEN(", packet_length, ") CHKSUM: ", packet_checksum );
	}
	else if ( packet_type == DynamixelAnalyzer::ACTION )
	{
		AddResultString( "A" );
		AddResultString( "ACTION" );
		AddResultString( "ACTION ID(", id_str , ")" );		
	}
	else if ( packet_type == DynamixelAnalyzer::RESET )
	{
		AddResultString( "RS" );
		AddResultString( "RESET" );
		AddResultString( "RESET ID(", id_str , ")" );
		AddResultString( "RESET ID(", id_str , ") LEN(", packet_length, ")" );
	}
	else if ( packet_type == DynamixelAnalyzer::SYNC_WRITE )
	{
		AddResultString( "S" );
		AddResultString( "SYNC_WRITE" );
	}
	else
	{
		AddResultString( "?" );
		AddResultString( "unknown" );
		AddResultString( "unknown packet type" );
	}
}

void DynamixelAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s],Value" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );
		
		char time_str[128];
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128 );

		char id_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, id_str, 128 );

		file_stream << time_str << "," << id_str << std::endl;

		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			file_stream.close();
			return;
		}
	}

	file_stream.close();
}

void DynamixelAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	Frame frame = GetFrame( frame_index );
	ClearResultStrings();

	char id_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, id_str, 128 );
	AddResultString( id_str );
}

void DynamixelAnalyzerResults::GeneratePacketTabularText( U64 packet_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void DynamixelAnalyzerResults::GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base )
{
	ClearResultStrings();
	AddResultString( "not supported" );
}