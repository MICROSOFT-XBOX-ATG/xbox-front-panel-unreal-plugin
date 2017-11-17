//*********************************************************
// Copyright (c) Microsoft. All rights reserved.
//*********************************************************
#include "XboxFrontPanelModulePrivate.h"

#include "InputCore.h"

#define LOCTEXT_NAMESPACE "XboxFrontPanel"

IMPLEMENT_MODULE(FXboxFrontPanelModule, XboxFrontPanel);

namespace XboxFrontPanelKeyNames
{
	const FGamepadKeyNames::Type Button1("Xbox_Front_Panel_Button_1");
	const FGamepadKeyNames::Type Button2("Xbox_Front_Panel_Button_2");
	const FGamepadKeyNames::Type Button3("Xbox_Front_Panel_Button_3");
	const FGamepadKeyNames::Type Button4("Xbox_Front_Panel_Button_4");
	const FGamepadKeyNames::Type Button5("Xbox_Front_Panel_Button_5");

	const FGamepadKeyNames::Type DPadUp("Xbox_Front_Panel_DPad_Up");
	const FGamepadKeyNames::Type DPadDown("Xbox_Front_Panel_DPad_Down");
	const FGamepadKeyNames::Type DPadLeft("Xbox_Front_Panel_DPad_Left");
	const FGamepadKeyNames::Type DPadRight("Xbox_Front_Panel_DPad_Right");

	const FGamepadKeyNames::Type DPadPress("Xbox_Front_Panel_DPad_Press");
}

void FXboxFrontPanelModuleBase::StartupModule()
{
	// Deliberate typo (it's really Xbox, not XBox) in order to align with existing menu category.
	// Would be nice to put these in their own Xbox One|Front Panel sub-category, but the UI doesn't seem to do nesting here.
	EKeys::AddKey(FKeyDetails(XboxFrontPanelKeyNames::Button1, LOCTEXT("Xbox_Front_Panel_Button_1", "Front Panel Button 1"), FKeyDetails::NoFlags, "XBoxOne"));
	EKeys::AddKey(FKeyDetails(XboxFrontPanelKeyNames::Button2, LOCTEXT("Xbox_Front_Panel_Button_2", "Front Panel Button 2"), FKeyDetails::NoFlags, "XBoxOne"));
	EKeys::AddKey(FKeyDetails(XboxFrontPanelKeyNames::Button3, LOCTEXT("Xbox_Front_Panel_Button_3", "Front Panel Button 3"), FKeyDetails::NoFlags, "XBoxOne"));
	EKeys::AddKey(FKeyDetails(XboxFrontPanelKeyNames::Button4, LOCTEXT("Xbox_Front_Panel_Button_4", "Front Panel Button 4"), FKeyDetails::NoFlags, "XBoxOne"));
	EKeys::AddKey(FKeyDetails(XboxFrontPanelKeyNames::Button5, LOCTEXT("Xbox_Front_Panel_Button_5", "Front Panel Button 5"), FKeyDetails::NoFlags, "XBoxOne"));

	EKeys::AddKey(FKeyDetails(XboxFrontPanelKeyNames::DPadUp, LOCTEXT("Xbox_Front_Panel_DPad_Up", "Front Panel DPad Up"), FKeyDetails::NoFlags, "XBoxOne"));
	EKeys::AddKey(FKeyDetails(XboxFrontPanelKeyNames::DPadDown, LOCTEXT("Xbox_Front_Panel_DPad_Down", "Front Panel DPad Down"), FKeyDetails::NoFlags, "XBoxOne"));
	EKeys::AddKey(FKeyDetails(XboxFrontPanelKeyNames::DPadLeft, LOCTEXT("Xbox_Front_Panel_DPad_Left", "Front Panel DPad Left"), FKeyDetails::NoFlags, "XBoxOne"));
	EKeys::AddKey(FKeyDetails(XboxFrontPanelKeyNames::DPadRight, LOCTEXT("Xbox_Front_Panel_DPad_Right", "Front Panel DPad Right"), FKeyDetails::NoFlags, "XBoxOne"));

	EKeys::AddKey(FKeyDetails(XboxFrontPanelKeyNames::DPadPress, LOCTEXT("Xbox_Front_Panel_DPad_Press", "Front Panel DPad Press"), FKeyDetails::NoFlags, "XBoxOne"));
}

#undef LOCTEXT_NAMESPACE