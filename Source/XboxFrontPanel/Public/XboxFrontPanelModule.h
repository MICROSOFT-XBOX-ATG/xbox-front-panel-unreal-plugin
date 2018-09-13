//*********************************************************
// Copyright (c) Microsoft. All rights reserved.
//*********************************************************
#pragma once

#include "Modules/ModuleManager.h"
#include "UObject/ObjectMacros.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"

class SWidget;
class UUserWidget;

UENUM()
enum class EXboxFrontPanelButtonLight : uint8
{
	Button1,
	Button2,
	Button3,
	Button4,
	Button5
};

namespace XboxFrontPanelKeyNames
{
	extern XBOXFRONTPANEL_API const FGamepadKeyNames::Type Button1;
	extern XBOXFRONTPANEL_API const FGamepadKeyNames::Type Button2;
	extern XBOXFRONTPANEL_API const FGamepadKeyNames::Type Button3;
	extern XBOXFRONTPANEL_API const FGamepadKeyNames::Type Button4;
	extern XBOXFRONTPANEL_API const FGamepadKeyNames::Type Button5;

	extern XBOXFRONTPANEL_API const FGamepadKeyNames::Type DPadUp;
	extern XBOXFRONTPANEL_API const FGamepadKeyNames::Type DPadDown;
	extern XBOXFRONTPANEL_API const FGamepadKeyNames::Type DPadLeft;
	extern XBOXFRONTPANEL_API const FGamepadKeyNames::Type DPadRight;

	extern XBOXFRONTPANEL_API const FGamepadKeyNames::Type DPadPress;
}

/**
* Interface for Xbox One X front panel features.
*/
class IXboxFrontPanelModule : 
	public IModuleInterface
{
public:

	static inline IXboxFrontPanelModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IXboxFrontPanelModule>("XboxFrontPanel");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("XboxFrontPanel");
	}

public:

	/**
	* Checks to see whether or not front panel features are available in the current runtime environment.
	* When not available, all other methods on this interface are no-ops with get-style methods returning
	* default values.
	*
	* @return		True if the front panel is available.
	*/
	virtual bool IsFrontPanelAvailable() = 0;

	/**
	* Switch the light associated with a front panel button on or off.
	*
	* @param Light	Enumerated value indicating which light to affect, in terms of the button it is associated with.
	* @param OnOff	New state for the light - true for on, false for off.
	*/
	virtual void SetButtonLightState(EXboxFrontPanelButtonLight Light, bool OnOff) = 0;

	/**
	* Query whether a light associated with a front panel button is on or off.
	*
	* @param Light	Enumerated value indicating which light to query, in terms of the button it is associated with.
	*
	* @return		True if the light is on.  False for off, or if the front panel is not currently available.
	*/
	virtual bool GetButtonLightState(EXboxFrontPanelButtonLight Light) = 0;

	/**
	* Provide a Slate Widget for display on the front panel screen.  The front panel module will take ownership
	* of the widget, updating and rendering it until the current widget is changed.  The widget will be asked to
	* draw within a 256x64 window and will be rendered in grayscale.  
	* 
	* Note: calling this method will automatically transition the front panel screen to title-provided UI.
	*
	* @param Light	Slate widget to display on the front screen.  Null to clear the front panel screen.
	*/
	virtual void SetScreenWidget(TSharedPtr<SWidget> Widget) = 0;

	/**
	* Provide a UMG Widget for display on the front panel screen.  The front panel module will take ownership
	* of the widget, updating and rendering it until the current widget is changed.  The widget will be asked to
	* draw within a 256x64 window and will be rendered in grayscale.
	*
	* Note: calling this method will automatically transition the front panel screen to title-provided UI.
	*
	* @param Light	Slate widget to display on the front screen.  Null to clear the front panel screen.
	*/
	virtual void SetScreenWidget(UUserWidget* Widget) = 0;
};