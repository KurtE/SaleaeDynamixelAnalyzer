#define _CRT_SECURE_NO_WARNINGS		// Make windows compiler shut up...
#include "DynamixelAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "DynamixelAnalyzer.h"
#include "DynamixelAnalyzerSettings.h"
#include <fstream>
#include <sstream>

//=============================================================================
// Define Global/Static data
//=============================================================================
static const U8 s_instructions[] = { DynamixelAnalyzer::NONE, DynamixelAnalyzer::APING, DynamixelAnalyzer::READ, DynamixelAnalyzer::WRITE, DynamixelAnalyzer::REG_WRITE, 
	DynamixelAnalyzer::ACTION, DynamixelAnalyzer::RESET, DynamixelAnalyzer::STATUS , DynamixelAnalyzer::SYNC_WRITE,  DynamixelAnalyzer::SYNC_WRITE_SERVO_DATA };

static const char * s_instruction_names[] = {
	"REPLY", "PING", "READ", "WRITE", "REG_WRITE", 
	"ACTION", "RESET", "REPLY", "SYNC_WRITE", "SW_DATA", "REPLY_STAT" };


// AX Servos
static const char * s_ax_register_names[] = {
	"MODEL", "MODEL_H", "VER","ID","BAUD","DELAY","CWL","CWL_H",
	"CCWL","CCWL_H","DATA2","LTEMP","LVOLTD","LVOLTU","MTORQUE", "MTORQUE_H",
	"RLEVEL","ALED","ASHUT","OMODE","DCAL","DCAL_H","UCAL","UCAL_H",
	/** RAM AREA **/
	"TENABLE","LED","CWMAR","CCWMAR","CWSLOPE","CCWSLOPE","GOAL","GOAL_H",
	"GSPEED","GSPEED_H","TLIMIT","TLIMIT_H","PPOS","PPOS_H","PSPEED","PSPEED_H",
	"PLOAD","PLOAD_H","PVOLT","PTEMP","RINST","PTIME","MOVING","LOCK",
	"PUNCH","PUNCH_H"
};

static const U8 s_is_ax_register_multi_byte[] = {
	2, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 2, 0,
	0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 2, 0,
	2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0,
	2, 0 
};

// MX Servos
static const char * s_mx_register_names[] = {
	"MODEL", "MODEL_H", "VER","ID","BAUD","DELAY","CWL","CWL_H",
	"CCWL","CCWL_H","?","LTEMP","LVOLTD","LVOLTU","MTORQUE", "MTORQUE_H",
	"RLEVEL","ALED","ASHUT","?","MTOFSET","MTOFSET_L","RESD","?",
	/** RAM AREA **/
	"TENABLE","LED","DGAIN","IGAIN","PGAIN","?","GOAL","GOAL_H",
	"GSPEED","GSPEED_H","TLIMIT","TLIMIT_H","PPOS","PPOS_H","PSPEED","PSPEED_H",
	"PLOAD","PLOAD_H","PVOLT","PTEMP","RINST","?","MOVING","LOCK",
	//0x30
	"PUNCH","PUNCH_H","?","?","?","?","?","?",
	"?","?","?","?","?","?","?","?",
	//0x40
	"?","?","?","?","CURR", "CURR_H", "TCME", "GTORQ",
	"QTORQ_H","GACCEL"
};

static const U8 s_is_mx_register_multi_byte[] = {
	2, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 2, 0,
	0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,
	2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0,
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 2, 0, 0, 2, 0, 0
};

// XL Servos
static const char * s_xl_register_names[] = {
	"MODEL", "MODEL_H", "VER","ID","BAUD","DELAY","CWL","CWL_H",
	"CCWL","CCWL_H","?","CMODE", "LTEMP","LVOLTD","LVOLTU",	"MTORQUE", 
	"MTORQUE_H", "RLEVEL", "ASHUT","?","?","?","?","?",
	/** RAM AREA **/
	"TENABLE","LED","DGAIN","IGAIN","PGAIN","?","GOAL","GOAL_H",
	"GSPEED","GSPEED_H","?", "TLIMIT","TLIMIT_H","PPOS","PPOS_H","PSPEED",
	"PSPEED_H", "PLOAD","PLOAD_H","?", "?","PVOLT", "PTEMP","RINST",
	//0x30
	"?","MOVING","HSTAT", "PUNCH","PUNCH_H"
};

static const U8 s_is_xl_register_multi_byte[] = {
	2, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 2,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0,
	2, 0, 2, 0, 0, 2, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0,
	0, 0, 0, 2, 
};

static const char * s_x_register_names[] = {
	"MODE#", "MODE#_H", "MODI", "MODI2","MODI3", "MODI4", "VER","ID",	//0
	"BAUD","DELAY","DMODE","OMODE","S-ID","PROT","?", "?",				//8
	"?","?","?","?","HOFF","HOF2","HOF3","HOF4",						//16
	"MOVT","MOVT2","MOVT3","MOVT4","?","?","?","TLIMIT",				//24
	"VMAX","VMAX2","VMIN","VMIN2","PWML","PWML2","?","?",				//32
	"ACCLL","ACCL2","ACCL3","ACCL4","VLMT","VLMT2","VLMT3", "VLMT4",	//40
	"MXPOS","MXPOS2","MXPOS3","MXPOS4","MNPOS","MNPOS2", "MNPOS3","MNPOS4", //48
	"?","?","?","?","?","?","?","SHUTDN",								//56
	/** RAM AREA  Starts at 64**/
	"TENABLE","LED","?","?","RETL", "RINST","HERR","?",	//64
	"?","?","?","?","VIGAIN","VIG2","VPGAIN","VPG2",	// 72
	"POSDG","PDG2","POSIG","PIG2","POSPG","PPG2","?","?",	// 80
	"FF2G","FF2G2","FF1G","FF1G2","?","?","?","?",		// 88
	"?","?","BWATCH","?","GPWM","GPWM2","?","?",		// 96
	"GVEL","GVEL2","GVEL3","GVEL4","GACCL","GACL2","GACL3", "GACL4", //104
	"PVEL","PVEL2","PVEL3","PVEL4","GOAL","GOAL2","GOAL3","GOAL4",	// 112
	"RTICK","RTICK2","MOVING","MSTATUS","PPWM","PPWM2","PLOAD", "PLOAD2", //120
	"PVEL","PVEL2","PVEL3","PVEL4","PPOS","PPOS2","PPOS3","PPOS4",		//128
	"VELT","VELT2","VELT3","VELT4","POST","POST2","POST3","POST4",		//136
	"PVOLT","PVOLT2", "PTEMP"								//144
};
static const U8 s_is_x_register_multi_byte[] = {
	2, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0,	//16
	2, 0, 2, 0, 2, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0,	//32
	4, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //48

	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, //64
	2, 0, 2, 0, 2, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0, 0, //80
	0, 0, 0, 0, 2, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, //96
	4, 0, 0, 0, 4, 0, 0, 0, 2, 0, 0, 0, 2, 0, 2, 0, //112
	4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, 4, 0, 0, 0, //128
	2, 0, 0 //144
};

//USB2AX - probably not needed as does not forward messages on normal AX BUSS

//CM730(ish) controllers. 
static const char * s_cm730_register_names[] = {
	"MODEL", "MODEL_H", "VER","ID","BAUD","DELAY","","",
	"","","","","LVOLTD","LVOLTU","", "",
	"RLEVEL","","","","","","","",
	/** RAM AREA **/
	"POWER", "LPANNEL", "LHEAD", "LHEAD_H", "LEYE", "LEYE_H", "BUTTON", "",
	"D1", "D2", "D3","D4","D5","D6", "GYROZ", "GYROZ_H",
	"GYROY","GYROY_H","GYROX","GYROX_H", "ACCX", "ACCX_H","ACCY", "ACCY_H",
	"ACCZ","ACCZ_H", "ADC0","ADC1","ADC1_H", "ADC2","ADC2_H", "ADC3",
	"ADC3_H","ADC4","ADC4_H","ADC5","ADC5_H","ADC6","ADC6_H", "ADC7",
	"ADC7_H","ADC8","ADC8_H","ADC9","ADC9_H","ADC10","ADC10_H", "ADC11",
	"ADC11_H","ADC12","ADC12_H","ADC13","ADC13_H","ADC14","ADC14_H", "ADC15",
	"ADC15_H"
};

static const U8 s_is_cm730_register_multi_byte[] = {
	2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0,
	2, 0, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2,
	0, 2, 0, 2, 0, 2, 0, 2, 0
};



//=============================================================================
// class DynamixelAnalyzerResults 
//=============================================================================

DynamixelAnalyzerResults::DynamixelAnalyzerResults( DynamixelAnalyzer* analyzer, DynamixelAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

DynamixelAnalyzerResults::~DynamixelAnalyzerResults()
{
}

void DynamixelAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base)
{
	ClearResultStrings();
	Frame frame = GetFrame(frame_index);
	bool frame_handled = false;
	std::ostringstream ss;
	const char *pregister_name;

	// Note: the mData1 and mData2 are encoded with as much of the data as we can fit.
	// frame.mType has our = packet type
	// mData1 bytes low to high ID, Checksum, CHK2, length, data0-data3
	// mData2  data4-11

	U8 packet_type = frame.mType;
	U8 servo_id = frame.mData1 & 0xff;
	U16 CRC = (frame.mData1 >> (1 * 8)) & 0xffff;
	bool protocol_2 = (CRC & 0xff00) != 0;

	U8 Packet_length = (frame.mData1 >> (3 * 8)) & 0xff;

	char id_str[20];
	AnalyzerHelpers::GetNumberString(servo_id, display_base, 8, id_str, sizeof(id_str));

	char Packet_length_string[20];
	AnalyzerHelpers::GetNumberString(Packet_length, display_base, 8, Packet_length_string, sizeof(Packet_length_string));

	// Several packet types use the register start and Register length, so move that out here
	char reg_start_str[20];
	char reg_count_str[20];

	U16 reg_start;
	U16 reg_count;

	U8 data_count = protocol_2 ? (Packet_length - 3) : (Packet_length - 2);

	U8 Data[12];		// Quick and dirty simply extract all up to 11 data paramters back into an array.
	U64 DataX = frame.mData1 >> 32;
	for (auto i = 0; i < 4; i++) {
		Data[i] = DataX & 0xff;
		DataX >>= 8;
	}
	DataX = frame.mData2;
	for (auto i = 4; i < 12; i++) {
		Data[i] = DataX & 0xff;
		DataX >>= 8;
	}
	
	if (protocol_2)
	{
		reg_start = Data[0] + (Data[1] << 8);
		reg_count = Data[2] + (Data[3] << 8);
	}
	else
	{
		reg_start = Data[0];
		reg_count = Data[1];
	}
	AnalyzerHelpers::GetNumberString(reg_start, display_base, 8, reg_start_str, sizeof(reg_start_str));
	AnalyzerHelpers::GetNumberString(reg_count, display_base, 8, reg_count_str, sizeof(reg_count_str));

	if (frame.mFlags & DISPLAY_AS_ERROR_FLAG)
	{
		//char packet_checksum[20];
		// See if packet 1 or 2... 
		//if (CRC & 0xff00) 
		//U8 checksum = ~((frame.mData1 >> (1 * 8)) & 0xff) & 0xff;
		//AnalyzerHelpers::GetNumberString(checksum, display_base, 8, packet_checksum, sizeof(packet_checksum));

		if (protocol_2) 
		{
			char checksum_str[10];
			char calc_checksum_str[10];
			U16 CRCPacket = Data[4] + (Data[5] << 8);
			AnalyzerHelpers::GetNumberString(CRC, display_base, 8, calc_checksum_str, sizeof(calc_checksum_str));
			AnalyzerHelpers::GetNumberString(CRCPacket, display_base, 8, checksum_str, sizeof(checksum_str));
			AddResultString("*");
			AddResultString("Checksum");
			AddResultString("CHK: ", checksum_str, "!=", calc_checksum_str);
		}
		else 
		{
			U8 checksum = ~(frame.mData1 >> (1 * 8)) & 0xff;
			char checksum_str[10];
			char calc_checksum_str[10];
			AnalyzerHelpers::GetNumberString(checksum, display_base, 8, calc_checksum_str, sizeof(calc_checksum_str));
			AnalyzerHelpers::GetNumberString(frame.mData2 & 0xff, display_base, 8, checksum_str, sizeof(checksum_str));
			AddResultString("*");
			AddResultString("Checksum");
			AddResultString("CHK: ", checksum_str, "!=", calc_checksum_str);
		}
		frame_handled = true;
	}
	else if ((packet_type == DynamixelAnalyzer::APING) && (data_count == 0))
	{
		AddResultString("P");
		AddResultString("PING");
		AddResultString("PING ID(", id_str, ")");
		frame_handled = true;
	}
	else if (packet_type == DynamixelAnalyzer::READ)
	{
		// P1: 0xff 0xff 0x1 0x4 0x2 0x24 0x2 0xd2  // Servo 1 register 0x24 read 2 bytes
		// p2: 0xff 0xff 0xfd 0x0 0x2 0x7 0x0 0x2 0x84 0x0 0x4 0x0 0x17 0x25 // Servo 2 register 84 read 4 bytes
		if ((protocol_2 && (data_count == 4)) || (!protocol_2 && (data_count == 2))) {
			AddResultString("R");
			AddResultString("READ");
			AddResultString("RD(", id_str, ")");
			ss << "RD(" << id_str << ") REG: " << reg_start_str;
			AddResultString(ss.str().c_str());

			if ((pregister_name = GetServoRegisterName(servo_id, reg_start, protocol_2)))
			{
				ss << "(" << pregister_name << ")";
				AddResultString(ss.str().c_str());
			}
			ss << " LEN:" << reg_count_str;
			AddResultString(ss.str().c_str());
			frame_handled = true;
		}
	}
	else if ( ((packet_type == DynamixelAnalyzer::WRITE) || (packet_type == DynamixelAnalyzer::REG_WRITE)) && (data_count > 1))
	{
		// P1: 0xff 0xff 0x15 0x3 0x1e 0xf4 0x1 0xcf : Write Servo 0x15 register 1e len 2: 0xf4 0x1
		// P2: 0xff 0xff 0xfd 0x0 0x2 0x6 0x0 0x3 0x40 0x00 0x1 0xeb 0x65: Write servo 2 register 0x40 1 byte = 0x1
		// Assume packet must have at least two data bytes: starting register and at least one byte.
		U8 count_data_bytes;
		U8 index_data_byte;
		if (protocol_2)
		{
			// Packet length includes: <cmd><Reg_l><Reg_H>{DATA]<CRC_L><CRC_H>  so for write 1 byte 6 bytes we have 5 overhead.
			count_data_bytes = Packet_length - 5;
			index_data_byte = 2;
		}
		else {
			// Packet length includes: <cmd><Reg>{DATA]<Checksum>  so for write 1 cnt 6 bytes we have 5 overhead.
			count_data_bytes = Packet_length - 3;
			index_data_byte = 1;
		}

		// Need to redo as length is calculated. 
		AnalyzerHelpers::GetNumberString(count_data_bytes, display_base, 8, reg_count_str, sizeof(reg_count_str));

		char short_command_str[10];
		char long_command_str[12];
			if (packet_type == DynamixelAnalyzer::WRITE)
		{
			AddResultString("W");
			AddResultString("WRITE");
			AddResultString("WR(", id_str, ")");
			strcpy(short_command_str, "WR");
			strcpy(long_command_str, "WRITE");
		}
		else
		{
			AddResultString("RW");
			AddResultString("REG_WRITE");
			AddResultString("RW(", id_str, ")");
			strcpy(short_command_str, "RW");
			strcpy(long_command_str, "REG WRITE");
		}

		ss << "(" << id_str << ") REG:" << reg_start_str;

		AddResultString( short_command_str, ss.str().c_str());
		ss << " LEN: " << reg_count_str;
		AddResultString(short_command_str, ss.str().c_str());

		// Try to build string showing bytes
		U16 register_number = reg_start;
		if (count_data_bytes > 0)
		{
			ss << " D: ";
			U8 loop_count = 0;
			char w_str[20];
			U32 wval;
			U8 last_data_index = index_data_byte + count_data_bytes;
			if (last_data_index > 12) last_data_index = 12;
			while (index_data_byte < last_data_index)
			{
				if (loop_count == 2)
					AddResultString(short_command_str, ss.str().c_str());
				if (loop_count) {
					ss << ", ";
				}
				loop_count++;

				// now see if register is associated with a pair of registers. 
				U8 data_size;
				data_size = NextDataValue(servo_id, reg_start, index_data_byte, Data, last_data_index, protocol_2, wval);
				AnalyzerHelpers::GetNumberString(wval, display_base, 16, w_str, sizeof(w_str));	// reuse string; 
				if (data_size > 1)
				{
					if (data_size == 4)
						ss << "#";
					else
					{
						ss << "$";
					}
				}
				ss << w_str;
				reg_start += data_size;
				index_data_byte += data_size;
			}
			AddResultString(short_command_str, ss.str().c_str());
			AddResultString(long_command_str, ss.str().c_str());
			
			if ((pregister_name = GetServoRegisterName(servo_id, register_number, protocol_2)))
			{
				AddResultString(short_command_str, "(", pregister_name, ")", ss.str().c_str());
				AddResultString(long_command_str, "(", pregister_name, ")", ss.str().c_str());
			}

		}
		else
			AddResultString(long_command_str, id_str, ") REG:", reg_start_str, " LEN:", reg_count_str);

		frame_handled = true;
	}
	else if ( (packet_type == DynamixelAnalyzer::ACTION ) && (data_count == 0) )
	{
		AddResultString( "A" );
		AddResultString( "ACTION" );
		AddResultString( "ACTION ID(", id_str , ")" );		
		frame_handled = true;
	}
	else if ( (packet_type == DynamixelAnalyzer::RESET ) && (data_count == 0) )
	{
		AddResultString( "RS" );
		AddResultString( "RESET" );
		AddResultString( "RESET ID(", id_str , ")" );
		AddResultString( "RESET ID(", id_str , ") LEN(", Packet_length_string, ")" );
		frame_handled = true;
	}
	else if (( packet_type == DynamixelAnalyzer::SYNC_WRITE ) && (data_count >= 2))
	{
//		char cmd_header_str[50];

		AddResultString("SW");
		AddResultString("SYNC_WRITE");
		// if ID == 0xfe no need to say it... 
		if ((frame.mData1 & 0xff) == 0xfe)
			ss << " ";
		else
		{
			ss << "(" << id_str << ") ";
			AddResultString(ss.str().c_str());
		}

		ss << "REG:" << reg_start_str;
		AddResultString("SW", ss.str().c_str());

	//	ss << " LEN:" << reg_count_str;
		AddResultString("SW", ss.str().c_str(), "LEN:", reg_count_str);

		if ((pregister_name = GetServoRegisterName(servo_id, reg_start, protocol_2)))
		{
			ss << "(" << pregister_name
				<< ") LEN: " << reg_count_str;
			AddResultString("SW", ss.str().c_str());
			AddResultString("SYNC_WRITE", ss.str().c_str());

		}

		frame_handled = true;
	}
	else if (packet_type == DynamixelAnalyzer::SYNC_WRITE_SERVO_DATA)
	{
		AddResultString("D");	
		AddResultString(id_str);	// Show Servo number.
		char w_str[20];

		// for more bytes lets try concatenating strings... 
		ss << ":";

		U8 loop_count = 0;

		U32 wval;
		U8 index_data_byte = 4;	// start on 2nd word...
		U8 last_data_index = index_data_byte + reg_count;
		if (last_data_index > 12) last_data_index = 12;
		while (index_data_byte < last_data_index)
		{
			if (loop_count == 1)
				AddResultString(id_str, ss.str().c_str());
			if (loop_count) {
				ss << ", ";
			}
			loop_count++;

			// now see if register is associated with a pair of registers. 
			U8 data_size;
			data_size = NextDataValue(servo_id, reg_start, index_data_byte, Data, last_data_index, protocol_2, wval);
			AnalyzerHelpers::GetNumberString(wval, display_base, 16, w_str, sizeof(w_str));	// reuse string; 
			if (data_size > 1)
			{
				if (data_size == 4)
					ss << "#";
				else
				{
					ss << "$";
				}
			}
			ss << w_str;
			reg_start += data_size;
			index_data_byte += data_size;
		}
		AddResultString(id_str, ss.str().c_str());
		frame_handled = true;
	}

	// If the package was not handled, and packet type 0 then probably normal response, else maybe respone with error status
	if (!frame_handled)
	{
		U8 count_data_bytes;
		U8 index_data_byte;
		U8 error_code = 0;
		if (protocol_2)
		{
			
			error_code = (packet_type == DynamixelAnalyzer::STATUS)? Data[0] : packet_type;	// first byte is error status
			count_data_bytes = Packet_length - 4;
			index_data_byte = 1;
		}
		else 
		{
			count_data_bytes = Packet_length - 2;
			index_data_byte = 0;
			if (packet_type != DynamixelAnalyzer::NONE)
				error_code = packet_type;
		}

		AnalyzerHelpers::GetNumberString(count_data_bytes, display_base, 8, reg_count_str, sizeof(reg_count_str));

		// Replay for read current position
		//P2: ff ff fd 00 02 08 00 55 00 1a 03 00 00 25 7a - Status Servo 2 <status 0> 4 bytes position 1a 3 0 0 -> 794
		//P1 :ff ff 15 04 00 f3 01 f2 -  Status servo 15 status 0 pos: f3 01 -> 499
		if (error_code == 0)
		{
			AddResultString("RP");
			AddResultString("REPLY");
			AddResultString("RP(", id_str, ")");
			reg_start = Data[11];	// extract the start register we stashed earlier
			ss << "RP(" << id_str << ") LEN: " << reg_count_str;
		}
		else
		{
			char status_str[20];
			AnalyzerHelpers::GetNumberString(error_code, display_base, 8, status_str, sizeof(status_str));
			AddResultString("SR");
			AddResultString("STATUS");
			AddResultString("SR:", status_str, "(", id_str, ")");
			ss << "SR:" << status_str << "(" << id_str << ") LEN: " << reg_count_str;
			reg_start = 0xff;		// make sure 
		}
		
		AddResultString(ss.str().c_str());

		if (count_data_bytes)
		{
			ss << " D: ";
			bool first_data_item = true;
			char w_str[20];
			U32 wval;
			U8 last_data_index = index_data_byte + count_data_bytes;
			if (last_data_index > 12) last_data_index = 12;
			while (index_data_byte < last_data_index)
			{
				if (!first_data_item != 0) {
					ss << ", ";
				}
				first_data_item = false;

				// now see if register is associated with a pair of registers. 
				U8 data_size;
				data_size = NextDataValue(servo_id, reg_start, index_data_byte, Data, last_data_index, protocol_2, wval);
				AnalyzerHelpers::GetNumberString(wval, display_base, 16, w_str, sizeof(w_str));	// reuse string; 
				if (data_size > 1)
				{
					if (data_size == 4)
						ss << "#";
					else
					{
						ss << "$";
					}
				}
				ss << w_str;
				AddResultString(ss.str().c_str());  // Add up to this point...  

				reg_start += data_size;
				index_data_byte += data_size;
			}
		}
	}
}

//-----------------------------------------------------------------------------

U8 DynamixelAnalyzerResults::NextDataValue(U8 servo_id, U16 register_index, U8 data_index, U8 *Data, U8 data_length, bool protocol_2, U32 &w_val)
{
	// now see if register is associated with a pair of registers. 

	U8 multibyte_register = IsRegisterIndexStartOfMultipleByteValue(servo_id, register_index, protocol_2);
	if (multibyte_register && (multibyte_register <= (data_length - data_index)))
	{
		w_val = Data[data_index] + (Data[data_index + 1] << 8);
		if (multibyte_register == 4)
		{
			w_val += (Data[data_index + 2] << 16) + (Data[data_index + 3] << 24);
			return 4;
		}
		return 2;
	}
	else
	{
		w_val = Data[data_index];
		return 1;
	}
	return multibyte_register;
}

//-----------------------------------------------------------------------------

void DynamixelAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id )
{
	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();
	char w_str[20];
	const char *pregister_name;

	std::ofstream file_stream(file, std::ios::out);

	file_stream << "Time [s],Instruction, Instructions(S), ID, Errors, Reg start, Reg(s), count, data" << std::endl;

	U64 num_frames = GetNumFrames();
	for( U32 frame_index=0; frame_index < num_frames; frame_index++ )
	{
		char id_str[20];
		char reg_start_str[20];
		const char *reg_start_name_str_ptr;
		char reg_count_str[20];
		char time_str[16];
		char status_str[16];
		const char *instruct_str_ptr;

		Frame frame = GetFrame( frame_index );
		
		AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, sizeof(time_str) );

		//-----------------------------------------------------------------------------
		bool frame_handled = false;

		// Note: the mData1 and mData2 are encoded with as much of the data as we can fit.
		// frame.mType has our = packet type
		// mData1 bytes low to high ID, Checksum, <CRC2>, length, data0-3
		// mData2  data4-11

		U8 packet_type = frame.mType;
		U8 servo_id = frame.mData1 & 0xff;
		U8 Packet_length = (frame.mData1 >> (3 * 8)) & 0xff;
		U16 CRC = (frame.mData1 >> (1 * 8)) & 0xffff;
		bool protocol_2 = (CRC & 0xff00) != 0;

		AnalyzerHelpers::GetNumberString(packet_type, display_base, 8, w_str, sizeof(w_str));
		AnalyzerHelpers::GetNumberString(servo_id, display_base, 8, id_str, sizeof(id_str));

		U8 data_count = protocol_2 ? (Packet_length - 3) : (Packet_length - 2);

		U8 Data[12];		// Quick and dirty simply extract all up to 11 data paramters back into an array.
		U64 DataX = frame.mData1 >> 32;
		for (auto i = 0; i < 4; i++) {
			Data[i] = DataX & 0xff;
			DataX >>= 8;
		}
		DataX = frame.mData2;
		for (auto i = 4; i < 12; i++) {
			Data[i] = DataX & 0xff;
			DataX >>= 8;
		}

		U16 reg_start;
		U16 reg_count;

		if (protocol_2)
		{
			reg_start = Data[0] + (Data[1] << 8);
			reg_count = Data[2] + (Data[3] << 8);
		}
		else
		{
			reg_start = Data[0];
			reg_count = Data[1];
		}
		AnalyzerHelpers::GetNumberString(reg_start, display_base, 8, reg_start_str, sizeof(reg_start_str));

		if ((pregister_name = GetServoRegisterName(servo_id, reg_start, protocol_2)))
			reg_start_name_str_ptr = pregister_name;
		else
			reg_start_name_str_ptr = "";

		AnalyzerHelpers::GetNumberString(reg_count, display_base, 8, reg_count_str, sizeof(reg_count_str));

		// Setup initial 
		if (frame.mFlags & DISPLAY_AS_ERROR_FLAG)
			strncpy(status_str, "CHECKSUM", sizeof(status_str));
		else
			status_str[0] = 0;

		// Figure out our Instruction string
		U8 packet_type_index;
		instruct_str_ptr = "";
		for (packet_type_index = 0; packet_type_index < sizeof(s_instructions); packet_type_index++)
		{
			if (packet_type == s_instructions[packet_type_index])
			{
				instruct_str_ptr = s_instruction_names[packet_type_index];
				break;
			}
		}
		
		if (packet_type == DynamixelAnalyzer::SYNC_WRITE_SERVO_DATA)
		{
			w_str[0] = 0;	// lets not output our coded 0xff
		}

		file_stream << time_str << "," << w_str << "," << instruct_str_ptr << "," << id_str;

		// Now lets handle the different packet types
		if ((data_count == 0) && (
				(packet_type == DynamixelAnalyzer::ACTION) ||
				(packet_type == DynamixelAnalyzer::RESET) ||
				(packet_type == DynamixelAnalyzer::APING)))
		{
			if (strlen(status_str))
				file_stream << "," << status_str;
			frame_handled = true;
		}
		else if (packet_type == DynamixelAnalyzer::READ)
		{
			file_stream << "," << status_str << "," << reg_start_str << "," << reg_start_name_str_ptr << "," << reg_count_str;
			frame_handled = true;
		}

		else if (((packet_type == DynamixelAnalyzer::WRITE) || (packet_type == DynamixelAnalyzer::REG_WRITE)) && (Packet_length > 3))
		{
			U8 count_data_bytes;
			U8 index_data_byte;
			if (protocol_2)
			{
				// Packet length includes: <cmd><Reg_l><Reg_H>{DATA]<CRC_L><CRC_H>  so for write 1 byte 6 bytes we have 5 overhead.
				count_data_bytes = Packet_length - 5;
				index_data_byte = 2;
			}
			else {
				// Packet length includes: <cmd><Reg>{DATA]<Checksum>  so for write 1 cnt 6 bytes we have 5 overhead.
				count_data_bytes = Packet_length - 3;
				index_data_byte = 1;
			}

			// output first part... Warning: Write register count is not stored but derived...
			AnalyzerHelpers::GetNumberString(count_data_bytes, display_base, 8, reg_count_str, sizeof(reg_count_str));
			file_stream << "," << status_str << "," << reg_start_str << "," << reg_start_name_str_ptr << "," << reg_count_str;

			// Try to build string showing bytes
			// Try to build string showing bytes
			if (count_data_bytes > 0)
			{
				U8 loop_count = 0;
				char w_str[20];
				U32 wval;
				U8 last_data_index = index_data_byte + count_data_bytes;
				if (last_data_index > 12) last_data_index = 12;
				while (index_data_byte < last_data_index)
				{
					file_stream << ", ";
					// now see if register is associated with a pair of registers. 
					U8 data_size;
					data_size = NextDataValue(servo_id, reg_start, index_data_byte, Data, last_data_index, protocol_2, wval);
					AnalyzerHelpers::GetNumberString(wval, display_base, 16, w_str, sizeof(w_str));	// reuse string; 
					if (data_size > 1)
					{
						if (data_size == 4)
							file_stream << "#";
						else
						{
							file_stream << "$";
						}
					}
					file_stream << w_str;
					reg_start += data_size;
					index_data_byte += data_size;
				}
			}
			frame_handled = true;
		}
		else if ((packet_type == DynamixelAnalyzer::SYNC_WRITE) && (Packet_length >= 4))
		{
			file_stream << "," << status_str << "," << reg_start_str << "," << reg_start_name_str_ptr << "," << reg_count_str;
			frame_handled = true;
		}
		else if (packet_type == DynamixelAnalyzer::SYNC_WRITE_SERVO_DATA)
		{
			file_stream << "," << status_str << "," << reg_start_str << "," << reg_start_name_str_ptr << "," << reg_count_str;

			char w_str[20];

			U32 wval;
			U8 index_data_byte = 4;	// start on 2nd word...
			U8 last_data_index = index_data_byte + reg_count;
			if (last_data_index > 12) last_data_index = 12;
			while (index_data_byte < last_data_index)
			{
				file_stream << ", ";

				// now see if register is associated with a pair of registers. 
				U8 data_size;
				data_size = NextDataValue(servo_id, reg_start, index_data_byte, Data, last_data_index, protocol_2, wval);
				AnalyzerHelpers::GetNumberString(wval, display_base, 16, w_str, sizeof(w_str));	// reuse string; 
				if (data_size > 1)
				{
					if (data_size == 4)
						file_stream << "#";
					else
					{
						file_stream << "$";
					}
				}
				file_stream << w_str;
				reg_start += data_size;
				index_data_byte += data_size;
			}

			frame_handled = true;
		}

		// If the package was not handled, and packet type 0 then probably normal response, else maybe respone with error status
		if (!frame_handled)
		{
			U8 count_data_bytes;
			U8 index_data_byte;
			U8 error_code = 0;
			if (protocol_2)
			{
				error_code = (packet_type == DynamixelAnalyzer::STATUS) ? Data[0] : packet_type;	// first byte is error status
				count_data_bytes = Packet_length - 4;
				index_data_byte = 1;
			}
			else
			{
				count_data_bytes = Packet_length - 2;
				index_data_byte = 0;
				if (packet_type != DynamixelAnalyzer::NONE)
					error_code = packet_type;
			}

			AnalyzerHelpers::GetNumberString(count_data_bytes, display_base, 8, reg_count_str, sizeof(reg_count_str));

			// Replay for read current position
			//P2: ff ff fd 00 02 08 00 55 00 1a 03 00 00 25 7a - Status Servo 2 <status 0> 4 bytes position 1a 3 0 0 -> 794
			//P1 :ff ff 15 04 00 f3 01 f2 -  Status servo 15 status 0 pos: f3 01 -> 499
			if (error_code == 0)
			{
				reg_start = Data[11];	// extract the start register we stashed earlier
				if (reg_start != 0xff)
				{
					AnalyzerHelpers::GetNumberString(reg_start, display_base, 8, reg_start_str, sizeof(reg_start_str));
					if ((pregister_name = GetServoRegisterName(servo_id, reg_start, protocol_2)))
						reg_start_name_str_ptr = pregister_name;
					else
						reg_start_name_str_ptr = "";

					file_stream << "," << status_str << "," << reg_start_str << "," << reg_start_name_str_ptr << "," << reg_count_str;
				}
				else
					file_stream << "," << status_str << ",,," << reg_count_str;
			}
			else
			{
				AnalyzerHelpers::GetNumberString(packet_type, display_base, 8, status_str, sizeof(status_str));
				file_stream << "," << status_str << ",,," << reg_count_str;
			}

			if (count_data_bytes)
			{
				char w_str[20];
				U32 wval;
				U8 last_data_index = index_data_byte + count_data_bytes;
				if (last_data_index > 12) last_data_index = 12;
				while (index_data_byte < last_data_index)
				{
					// now see if register is associated with a pair of registers. 
					U8 data_size;
					data_size = NextDataValue(servo_id, reg_start, index_data_byte, Data, last_data_index, protocol_2, wval);
					AnalyzerHelpers::GetNumberString(wval, display_base, 16, w_str, sizeof(w_str));	// reuse string; 
					file_stream << ", ";

					if (data_size > 1)
					{
						if (data_size == 4)
							file_stream << "#";
						else
						{
							file_stream << "$";
						}
					}
					file_stream << w_str;

					reg_start += data_size;
					index_data_byte += data_size;
				}
			}
		}

		file_stream <<  std::endl;

		if (UpdateExportProgressAndCheckForCancel(frame_index, num_frames) == true)
			break;
	}

	UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
	file_stream.close();

}

void DynamixelAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
	ClearTabularText();
	std::ostringstream ss;
	Frame frame = GetFrame(frame_index);
	bool frame_handled = false;
	const char *pregister_name;

	char id_str[16];
	char w_str[20];
	char reg_count_str[20];
	char reg_start_str[16];

	// Note: the mData1 and mData2 are encoded with as much of the data as we can fit.
	// frame.mType has our = packet type
	// mData1 bytes low to high ID, Checksum, CRC2, length, data0-data3
	// mData2  data4-11

	U8 packet_type = frame.mType;
	U8 servo_id = frame.mData1 & 0xff;
	U8 Packet_length = (frame.mData1 >> (3 * 8)) & 0xff;
	U16 CRC = (frame.mData1 >> (1 * 8)) & 0xffff;
	bool protocol_2 = (CRC & 0xff00) != 0;


	U8 data_count = protocol_2 ? (Packet_length - 3) : (Packet_length - 2);

	U8 Data[12];		// Quick and dirty simply extract all up to 11 data paramters back into an array.
	U64 DataX = frame.mData1 >> 32;
	for (auto i = 0; i < 4; i++) {
		Data[i] = DataX & 0xff;
		DataX >>= 8;
	}
	DataX = frame.mData2;
	for (auto i = 4; i < 12; i++) {
		Data[i] = DataX & 0xff;
		DataX >>= 8;
	}

	U16 reg_start;
	U16 reg_count;

	if (protocol_2)
	{
		reg_start = Data[0] + (Data[1] << 8);
		reg_count = Data[2] + (Data[3] << 8);
	}
	else
	{
		reg_start = Data[0];
		reg_count = Data[1];
	}

	AnalyzerHelpers::GetNumberString(reg_start, display_base, 8, reg_start_str, sizeof(reg_start_str));
	AnalyzerHelpers::GetNumberString(packet_type, display_base, 8, w_str, sizeof(w_str));
	AnalyzerHelpers::GetNumberString(servo_id, display_base, 8, id_str, sizeof(id_str));


	AnalyzerHelpers::GetNumberString(servo_id, display_base, 8, id_str, sizeof(id_str));


	if (frame.mFlags & DISPLAY_AS_ERROR_FLAG)
	{
		U8 checksum = ~(frame.mData1 >> (1 * 8)) & 0xff;
		char checksum_str[10];
		char calc_checksum_str[10];
		AnalyzerHelpers::GetNumberString(checksum, display_base, 8, calc_checksum_str, sizeof(calc_checksum_str));
		AnalyzerHelpers::GetNumberString(frame.mData2 & 0xff, display_base, 8, checksum_str, sizeof(checksum_str));
		AddResultString("*");
		AddResultString("Checksum");

		ss << "CHK " << checksum_str << " != " << calc_checksum_str;
		AddResultString(ss.str().c_str());
		frame_handled = true;
	}
	else if ((packet_type == DynamixelAnalyzer::APING) && (data_count == 0))
	{
		ss << "PG " << id_str;
		frame_handled = true;
	}
	else if ((packet_type == DynamixelAnalyzer::READ))
	{
		AnalyzerHelpers::GetNumberString(reg_start, display_base, 8, reg_start_str, sizeof(reg_start_str));
		AnalyzerHelpers::GetNumberString(reg_count, display_base, 8, reg_count_str, sizeof(reg_count_str));

		ss << "RD " << id_str << ": R:" << reg_start_str << " L:" << reg_count_str;
		if ((pregister_name = GetServoRegisterName(servo_id, reg_start)))
		{
			ss <<" - ";
			ss << pregister_name;
		}
		frame_handled = true;
	}

	else if (((packet_type == DynamixelAnalyzer::WRITE) || (packet_type == DynamixelAnalyzer::REG_WRITE)) && (data_count > 0))
	{
		// Assume packet must have at least two data bytes: starting register and at least one byte.
		U8 count_data_bytes;
		U8 index_data_byte;
		if (protocol_2)
		{
			// Packet length includes: <cmd><Reg_l><Reg_H>{DATA]<CRC_L><CRC_H>  so for write 1 byte 6 bytes we have 5 overhead.
			count_data_bytes = Packet_length - 5;
			index_data_byte = 2;
		}
		else {
			// Packet length includes: <cmd><Reg>{DATA]<Checksum>  so for write 1 cnt 6 bytes we have 5 overhead.
			count_data_bytes = Packet_length - 3;
			index_data_byte = 1;
		}

		AnalyzerHelpers::GetNumberString(reg_start, display_base, 8, reg_start_str, sizeof(reg_start_str));
		AnalyzerHelpers::GetNumberString(reg_count, display_base, 8, reg_count_str, sizeof(reg_count_str));

		if (packet_type == DynamixelAnalyzer::WRITE)
		{
			ss << "WR " << id_str << ": R:" << reg_start_str << " L:" << reg_count_str;
		}
		else
		{
			ss << "RW " << id_str << ": R:" << reg_start_str << " L:" << reg_count_str;
		}

		if (count_data_bytes > 0)
		{
			ss << " D: ";
			U8 loop_count = 0;
			char w_str[20];
			U32 wval;
			U8 last_data_index = index_data_byte + count_data_bytes;
			if (last_data_index > 12) last_data_index = 12;
			while (index_data_byte < last_data_index)
			{
				if (loop_count) {
					ss << ", ";
				}
				loop_count++;

				// now see if register is associated with a pair of registers. 
				U8 data_size;
				data_size = NextDataValue(servo_id, reg_start, index_data_byte, Data, last_data_index, protocol_2, wval);
				AnalyzerHelpers::GetNumberString(wval, display_base, 16, w_str, sizeof(w_str));	// reuse string; 
				if (data_size > 1)
				{
					if (data_size == 4)
						ss << "#";
					else
					{
						ss << "$";
					}
				}
				ss << w_str;
				reg_start += data_size;
				index_data_byte += data_size;
			}

			if ((pregister_name = GetServoRegisterName(servo_id, reg_start, protocol_2)))
			{
				ss << " - " << pregister_name;
			}

		}

		frame_handled = true;
	}
	else if ((packet_type == DynamixelAnalyzer::ACTION) && (data_count == 0))
	{
		ss << "AC " << id_str;
		frame_handled = true;
	}
	else if ((packet_type == DynamixelAnalyzer::RESET) && (data_count == 0))
	{
		ss << "RS " << id_str;
		frame_handled = true;
	}
	else if ((packet_type == DynamixelAnalyzer::SYNC_WRITE) && (data_count >= 2))
	{
		ss <<"SW " << id_str << ": R:" << reg_start_str << " L:" << reg_count;
		if ((pregister_name = GetServoRegisterName(servo_id, reg_start, protocol_2)))
		{
			ss << " - " << pregister_name;
		}
		frame_handled = true;
	}
	else if (packet_type == DynamixelAnalyzer::SYNC_WRITE_SERVO_DATA)
	{
		char w_str[20];
		ss << "SD " << id_str << " ";

		U32 wval;
		U8 index_data_byte = 4;	// start on 2nd word...
		U8 last_data_index = index_data_byte + reg_count;
		if (last_data_index > 12) last_data_index = 12;
		bool first_item = true; 
		while (index_data_byte < last_data_index)
		{
			// now see if register is associated with a pair of registers. 
			U8 data_size;
			data_size = NextDataValue(servo_id, reg_start, index_data_byte, Data, last_data_index, protocol_2, wval);
			AnalyzerHelpers::GetNumberString(wval, display_base, 16, w_str, sizeof(w_str));	// reuse string; 
			if (!first_item) ss << ", ";
			first_item = false; 
			if (data_size > 1)
			{
				if (data_size == 4)
					ss << "#";
				else
				{
					ss << "$";
				}
			}
			ss << w_str;
			reg_start += data_size;
			index_data_byte += data_size;
		}
		frame_handled = true;
	}

	if (!frame_handled)
	{
		U8 count_data_bytes;
		U8 index_data_byte;
		U8 error_code = 0;
		if (protocol_2)
		{

			error_code = (packet_type == DynamixelAnalyzer::STATUS) ? Data[0] : packet_type;	// first byte is error status
			count_data_bytes = Packet_length - 4;
			index_data_byte = 1;
		}
		else
		{
			count_data_bytes = Packet_length - 2;
			index_data_byte = 0;
			if (packet_type != DynamixelAnalyzer::NONE)
				error_code = packet_type;
		}

		AnalyzerHelpers::GetNumberString(count_data_bytes, display_base, 8, reg_count_str, sizeof(reg_count_str));

		// Replay for read current position
		//P2: ff ff fd 00 02 08 00 55 00 1a 03 00 00 25 7a - Status Servo 2 <status 0> 4 bytes position 1a 3 0 0 -> 794
		//P1 :ff ff 15 04 00 f3 01 f2 -  Status servo 15 status 0 pos: f3 01 -> 499
		if (error_code == 0)
		{
			ss << "RP " << id_str;
			reg_start = Data[11];	// extract the start register we stashed earlier
		}
		else
		{
			char status_str[20];
			AnalyzerHelpers::GetNumberString(error_code, display_base, 8, status_str, sizeof(status_str));
			ss << "SR " << id_str << " " << status_str;
			reg_start = 0xff;		// make sure 
		}

		AddResultString(ss.str().c_str());

		if (count_data_bytes)
		{
			ss << "L:" << reg_count << "D:";
			bool first_data_item = true;
			char w_str[20];
			U32 wval;
			U8 last_data_index = index_data_byte + count_data_bytes;
			if (last_data_index > 12) last_data_index = 12;
			while (index_data_byte < last_data_index)
			{
				if (!first_data_item != 0) {
					ss << ", ";
				}
				first_data_item = false;

				// now see if register is associated with a pair of registers. 
				U8 data_size;
				data_size = NextDataValue(servo_id, reg_start, index_data_byte, Data, last_data_index, protocol_2, wval);
				AnalyzerHelpers::GetNumberString(wval, display_base, 16, w_str, sizeof(w_str));	// reuse string; 
				if (data_size > 1)
				{
					if (data_size == 4)
						ss << "#";
					else
					{
						ss << "$";
					}
				}
				ss << w_str;
				reg_start += data_size;
				index_data_byte += data_size;
			}
		}
	}
	AddTabularText(ss.str().c_str());
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

const char * DynamixelAnalyzerResults::GetServoRegisterName(U8 servo_id, U16 register_number, bool protocol_2)
{
	// See if this is our controller servo ID
	if (servo_id == mSettings->mServoControllerID)
	{
		// Now only processing CM730...  special...
		if (servo_id == CONTROLLER_CM730ISH)
		{
			if (register_number < (sizeof(s_cm730_register_names) / sizeof(s_cm730_register_names[0])))
				return s_cm730_register_names[register_number];
		}
		return NULL;
	}
	U32 servo_type = protocol_2 ? mSettings->mServoType_p2 : mSettings->mServoType;
	// Else process like normal servos
	switch (servo_type)
	{
	case SERVO_TYPE_AX:
		if (register_number < (sizeof(s_ax_register_names) / sizeof(s_ax_register_names[0])))
			return s_ax_register_names[register_number];
		break;

	case SERVO_TYPE_MX:
		if (register_number < (sizeof(s_mx_register_names) / sizeof(s_mx_register_names[0])))
			return s_mx_register_names[register_number];
		break;

	case SERVO_TYPE_XL:
		if (register_number < (sizeof(s_xl_register_names) / sizeof(s_xl_register_names[0])))
			return s_xl_register_names[register_number];
		break;

	case SERVO_TYPE_X:
		if (register_number < (sizeof(s_x_register_names) / sizeof(s_x_register_names[0])))
			return s_x_register_names[register_number];
		break;
	}
	return NULL;
}

//=============================================================================

U8 DynamixelAnalyzerResults::IsRegisterIndexStartOfMultipleByteValue(U8 servo_id, U16 register_number, bool protocol_2)
{
	if (!mSettings->mShowWords) return false; // we are not processing multi byte values

	// See if this is our controller servo ID
	if (servo_id == mSettings->mServoControllerID)
	{
		// Now only processing CM730...  special...
		if (servo_id == CONTROLLER_CM730ISH)
		{
			return (mSettings->mShowWords && (register_number < sizeof(s_is_cm730_register_multi_byte)) && s_is_cm730_register_multi_byte[register_number]);
		}
		return 0;
	}

	U32 servo_type = protocol_2 ? mSettings->mServoType_p2 : mSettings->mServoType;
	// Else process like normal servos
	switch (servo_type)
	{
	case SERVO_TYPE_AX:
		return  (register_number < sizeof(s_is_ax_register_multi_byte)) ? s_is_ax_register_multi_byte[register_number] : 0;

	case SERVO_TYPE_MX:
		return  (register_number < sizeof(s_is_mx_register_multi_byte)) ? s_is_mx_register_multi_byte[register_number] : 0;

	case SERVO_TYPE_XL:
		return  (register_number < sizeof(s_is_xl_register_multi_byte)) ? s_is_xl_register_multi_byte[register_number] : 0;
		break;

	case SERVO_TYPE_X:
		return  (register_number < sizeof(s_is_x_register_multi_byte)) ? s_is_x_register_multi_byte[register_number] : 0;
		break;
	}
	return false;
}


