//*********************************************************
// Copyright (c) Microsoft. All rights reserved.
//*********************************************************
#pragma once

#include "XboxFrontPanelModule.h"

#include "InputCore.h"
#include "Templates/SharedPointer.h"

class FXboxFrontPanelModuleBase :
	public IXboxFrontPanelModule
{
public:
	virtual void StartupModule() override;
};

#if PLATFORM_XBOXONE && !UE_BUILD_SHIPPING
#include "XboxOneAllowPlatformTypes.h"
#include <xdk.h>
#include "XboxOneHidePlatformTypes.h"

#define FRONT_PANEL_ENABLED _XDK_EDITION >= 170600
#else
#define FRONT_PANEL_ENABLED 0
#endif

#if FRONT_PANEL_ENABLED
#include "Ticker.h"
#include "Windows/ComPointer.h"
#include "RenderUtils.h"
#include "Slate/WidgetRenderer.h"
#include "Framework/Application/IInputProcessor.h"
#include "GenericApplicationMessageHandler.h"

#include "XboxOneAllowPlatformTypes.h"
#include <d3d11_x.h>
#define DXGI_FORMAT_DEFINED
#include <XboxFrontPanel.h>

class UTextureRenderTarget2D;
class SRetainerWidget;

class FXboxFrontPanelModule :
	public FXboxFrontPanelModuleBase,
	public FGCObject,
	public TSharedFromThis<FXboxFrontPanelModule>
{
public:
	FXboxFrontPanelModule();

public:
	virtual void StartupModule() override;

public:
	virtual bool IsFrontPanelAvailable();
	virtual void SetButtonLightState(EXboxFrontPanelButtonLight Light, bool OnOff);
	virtual bool GetButtonLightState(EXboxFrontPanelButtonLight Light);

	virtual void SetScreenWidget(TSharedPtr<SWidget> Widget);
	virtual void SetScreenWidget(UUserWidget* Widget);

public:
	bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent);
	bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent);

public:
	void Tick(float DeltaTime);

public:
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	void DrawScreen_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRenderTargetResource* GpuProducedScreenTexture);

private:

	void DrawScreen_GameThread(float DeltaTime);

	void GenerateButtonEvents();
	void GenerateSingleButtonEvent(int32 NewState, int32 LastState, FGamepadKeyNames::Type KeyName, double CurrentTime, double& RepeatAt);

	bool InitScreenResources();
	void DeinitScreenResources();

public:
	TComPtr<IXboxFrontPanelControl> FrontPanel;

	TUniquePtr<BYTE[]> FrontScreenData;
	uint64 FrontScreenDataSize;

	TSharedPtr<FHittestGrid> HitTestGrid;
	TSharedPtr<SVirtualWindow> Window;
	TSharedPtr<SWidget> FrontScreenWidget;
	FWidgetRenderer WidgetRenderer;

	UTextureRenderTarget2D* RenderTarget;
	FTexture2DRHIRef CPUTexture[2];
	int32 CPUTextureIndex;

	UINT32 Width;
	UINT32 Height;

	XBOX_FRONT_PANEL_BUTTONS LastButtonStates;
	double NextButtonRepeatTime[10];

	static const double InitialButtonRepeatDelay;
	static const double ButtonRepeatDelay;
};

#include "XboxOneHidePlatformTypes.h"

#else

class FXboxFrontPanelModule :
	public FXboxFrontPanelModuleBase
{
	virtual bool IsFrontPanelAvailable() { return false; }
	virtual void SetButtonLightState(EXboxFrontPanelButtonLight Light, bool OnOff) {}
	virtual bool GetButtonLightState(EXboxFrontPanelButtonLight Light) { return false; }

	virtual void SetScreenWidget(TSharedPtr<SWidget> Widget) {}
	virtual void SetScreenWidget(UUserWidget* Widget) {}
};

#endif

