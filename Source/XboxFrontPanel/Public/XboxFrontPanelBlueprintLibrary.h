//*********************************************************
// Copyright (c) Microsoft. All rights reserved.
//*********************************************************
#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "XboxFrontPanelModule.h"
#include "XboxFrontPanelBlueprintLibrary.generated.h"


UCLASS()
class XBOXFRONTPANEL_API UXboxFrontPanelBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	* Provide a UMG Widget for display on the front panel screen.  The front panel module will take ownership
	* of the widget, updating and rendering it until the current widget is changed.  The widget will be asked to
	* draw within a 256x64 window and will be rendered in grayscale.
	*
	* Note: calling this method will automatically transition the front panel screen to title-provided UI.
	*
	* @param Light	Slate widget to display on the front screen.  Null to clear the front panel screen.
	*/
	UFUNCTION(BlueprintCallable, Category = "Xbox Front Panel", meta=(DevelopmentOnly))
	static void SetScreenWidget(UUserWidget* Widget);

	/**
	* Switch the light associated with a front panel button on or off.
	*
	* @param Light	Enumerated value indicating which light to affect, in terms of the button it is associated with.
	* @param OnOff	New state for the light - true for on, false for off.
	*/
	UFUNCTION(BlueprintCallable, Category = "Xbox Front Panel", meta = (DevelopmentOnly))
	static void SetButtonLightState(EXboxFrontPanelButtonLight Light, bool OnOff);

	/**
	* Query whether a light associated with a front panel button is on or off.
	*
	* @param Light	Enumerated value indicating which light to query, in terms of the button it is associated with.
	*
	* @return		True if the light is on.  False for off, or if the front panel is not currently available.
	*/
	UFUNCTION(BlueprintPure , Category = "Xbox Front Panel")
	static bool GetButtonLightState(EXboxFrontPanelButtonLight Light);

	/**
	* Checks to see whether or not front panel features are available in the current runtime environment.
	* When not available, all other methods on this interface are no-ops with get-style methods returning
	* default values.
	*
	* @return		True if the front panel is available.
	*/
	UFUNCTION(BlueprintPure, Category = "Xbox Front Panel")
	static bool IsFrontPanelAvailable();
};