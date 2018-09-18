//*********************************************************
// Copyright (c) Microsoft. All rights reserved.
//*********************************************************

#include "XboxFrontPanelBlueprintLibrary.h"

#include "Engine/EngineTypes.h"
#include "RHI.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

void UXboxFrontPanelBlueprintLibrary::SetScreenWidget(UUserWidget* Widget)
{
	IXboxFrontPanelModule::Get().SetScreenWidget(Widget);
}

void UXboxFrontPanelBlueprintLibrary::SetButtonLightState(EXboxFrontPanelButtonLight Light, bool OnOff)
{
	IXboxFrontPanelModule::Get().SetButtonLightState(Light, OnOff);
}

bool UXboxFrontPanelBlueprintLibrary::GetButtonLightState(EXboxFrontPanelButtonLight Light)
{
	return IXboxFrontPanelModule::Get().GetButtonLightState(Light);

}

bool UXboxFrontPanelBlueprintLibrary::IsFrontPanelAvailable()
{
	return IXboxFrontPanelModule::Get().IsFrontPanelAvailable();
}