#include "DynamixelAnalyzerSettings.h"
#include <AnalyzerHelpers.h>


DynamixelAnalyzerSettings::DynamixelAnalyzerSettings()
:	mInputChannel( UNDEFINED_CHANNEL ),
	mBitRate( 1000000 ),
	mServoType (SERVO_TYPE_AX),
	mServoType_p2 (SERVO_TYPE_X),
	mShowWords ( true ), 
	mServoControllerID (CONTROLLER_UNKNOWN)
{
	mInputChannelInterface.reset( new AnalyzerSettingInterfaceChannel() );
	mInputChannelInterface->SetTitleAndTooltip( "Dynamixel", "Standard Dynamixel Protocol" );
	mInputChannelInterface->SetChannel( mInputChannel );

	mBitRateInterface.reset( new AnalyzerSettingInterfaceInteger() );
	mBitRateInterface->SetTitleAndTooltip( "Bit Rate (Bits/S)",  "Specify the bit rate in bits per second." );
	mBitRateInterface->SetMax( 6000000 );
	mBitRateInterface->SetMin( 1 );
	mBitRateInterface->SetInteger( mBitRate );

	mServoTypeInterface.reset(new AnalyzerSettingInterfaceNumberList());
	mServoTypeInterface->SetTitleAndTooltip("Protocol 1 Servo Type", "Type of Servos.");
	mServoTypeInterface->AddNumber(SERVO_TYPE_AX, "AX Servos (default)", "");
	mServoTypeInterface->AddNumber(SERVO_TYPE_MX, "MX Servos", "");
	mServoTypeInterface->AddNumber(SERVO_TYPE_XL, "XL320 Servos", "");
	mServoTypeInterface->AddNumber(SERVO_TYPE_X, "X Servos", "");
	mServoTypeInterface->SetNumber(mServoType);
	mServoTypeP2Interface.reset(new AnalyzerSettingInterfaceNumberList());
	mServoTypeP2Interface->AddNumber(SERVO_TYPE_X, "X Servos (default)", "");
	mServoTypeP2Interface->SetTitleAndTooltip("Protocol 2 Servo Type", "Type of Servos.");
	mServoTypeP2Interface->AddNumber(SERVO_TYPE_MX, "MX Servos", "");
	mServoTypeP2Interface->AddNumber(SERVO_TYPE_XL, "XL320 Servos", "");
	mServoTypeP2Interface->SetNumber(mServoType_p2);
	mShowWordsInterface.reset(new AnalyzerSettingInterfaceBool());
	mShowWordsInterface->SetTitleAndTooltip("", "Show two registers Low/High are shown as a word.");
	mShowWordsInterface->SetCheckBoxText("Show Register Pair values as words");
	mShowWordsInterface->SetValue(mShowWords);

	mServoControllerInterface.reset(new AnalyzerSettingInterfaceNumberList());
	mServoControllerInterface->SetTitleAndTooltip("Servo Controller", "Servo Controller (Servo ID)");
	mServoControllerInterface->AddNumber(CONTROLLER_UNKNOWN, "Unknown (default)", "");
	mServoControllerInterface->AddNumber(CONTROLLER_USB2AX, "USB2AX(0xFD)", "");
	mServoControllerInterface->AddNumber(CONTROLLER_CM730ISH, "CM730ish(0xC8)", "");
	mServoControllerInterface->SetNumber(mServoControllerID);


	AddInterface( mInputChannelInterface.get() );
	AddInterface( mBitRateInterface.get() );
	AddInterface( mServoTypeInterface.get() );
	AddInterface(mServoTypeP2Interface.get());
	AddInterface(mShowWordsInterface.get());
	AddInterface(mServoControllerInterface.get());

	AddExportOption(0, "Export as csv file");
	AddExportExtension(0, "csv", "csv");

	ClearChannels();
	AddChannel( mInputChannel, "Dynamixel", false );
}

DynamixelAnalyzerSettings::~DynamixelAnalyzerSettings()
{
}

bool DynamixelAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();
	mBitRate = mBitRateInterface->GetInteger();
	mServoType = (U32)mServoTypeInterface->GetNumber();
	mServoType_p2 = (U32)mServoTypeP2Interface->GetNumber();
	mServoControllerID = (U32)mServoControllerInterface->GetNumber();
	mShowWords = mShowWordsInterface->GetValue();

	ClearChannels();
	AddChannel( mInputChannel, "Dynamixel Protocol", true );

	return true;
}

void DynamixelAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel( mInputChannel );
	mBitRateInterface->SetInteger( mBitRate );
	mServoTypeInterface->SetNumber( mServoType );
	mServoTypeP2Interface->SetNumber(mServoType_p2);
	mServoControllerInterface->SetNumber(mServoControllerID);
	mShowWordsInterface->SetValue(mShowWords);

}

void DynamixelAnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mInputChannel;
	text_archive >> mBitRate;
	text_archive >> mServoType;
	text_archive >> mShowWords;
	text_archive >> mServoType_p2;


	ClearChannels();
	AddChannel( mInputChannel, "Dynamixel Protocol", true );

	UpdateInterfacesFromSettings();
}

const char* DynamixelAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mInputChannel;
	text_archive << mBitRate;
	text_archive << mServoType;
	text_archive << mShowWords;
	text_archive << mServoType_p2;



	return SetReturnString( text_archive.GetString() );
}
