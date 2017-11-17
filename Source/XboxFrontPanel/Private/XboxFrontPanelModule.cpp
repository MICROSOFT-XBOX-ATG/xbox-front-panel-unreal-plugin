//*********************************************************
// Copyright (c) Microsoft. All rights reserved.
//*********************************************************

#include "XboxFrontPanelModule.h"
#include "XboxFrontPanelModulePrivate.h"

#if FRONT_PANEL_ENABLED

#include "PixelFormat.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureResource.h"
#include "Widgets/SWidget.h"
#include "Blueprint/UserWidget.h"
#include "Slate/SRetainerWidget.h"
#include "ScopeLock.h"
#include "Framework/Application/SlateApplication.h"
#include "UnrealMathDirectX.h"
#include "SceneUtils.h"

#include "XboxOneAllowPlatformTypes.h"
#include <d3d12_x.h>

DEFINE_LOG_CATEGORY_STATIC(LogXboxFrontPanel, Log, All);

class FXboxFrontPanelInputProcessor : public IInputProcessor
{
public:
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
	{
		static_cast<FXboxFrontPanelModule&>(IXboxFrontPanelModule::Get()).Tick(DeltaTime);
	}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		return static_cast<FXboxFrontPanelModule&>(IXboxFrontPanelModule::Get()).HandleKeyDownEvent(SlateApp, InKeyEvent);
	}

	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		return static_cast<FXboxFrontPanelModule&>(IXboxFrontPanelModule::Get()).HandleKeyUpEvent(SlateApp, InKeyEvent);
	}
};

const double FXboxFrontPanelModule::InitialButtonRepeatDelay = 0.2;
const double FXboxFrontPanelModule::ButtonRepeatDelay = 0.1;

FXboxFrontPanelModule::FXboxFrontPanelModule()
	: FrontScreenDataSize(0)
	, RenderTarget(nullptr)
	, CPUTextureIndex(0)
	, Width(0)
	, Height(0)
{

}

void FXboxFrontPanelModule::StartupModule()
{
	FXboxFrontPanelModuleBase::StartupModule();

	// Note: When IsXboxFrontPanelAvailable returns TRUE it also transitions the front screen to title
	// ownership.  The screen will be blank until the title provides a widget to draw (or the user holds
	// the directional button to flip back to the system display)
	if (IsXboxFrontPanelAvailable() == TRUE)
	{
		IXboxFrontPanelControl* FrontPanelRaw = nullptr;
		if (SUCCEEDED(GetDefaultXboxFrontPanel(&FrontPanelRaw)))
		{
			UE_LOG(LogXboxFrontPanel, Log, TEXT("Xbox Front Panel is present!"));
			check(FrontPanelRaw);
			FrontPanel.Attach(FrontPanelRaw);

			LastButtonStates = XBOX_FRONT_PANEL_BUTTONS_NONE;
			FMemory::Memzero(NextButtonRepeatTime);
		}
	}

	FSlateApplication::Get().RegisterInputPreProcessor(MakeShared<FXboxFrontPanelInputProcessor>());
}

bool FXboxFrontPanelModule::IsFrontPanelAvailable()
{
	return FrontPanel != nullptr;
}

void FXboxFrontPanelModule::DrawScreen_RenderThread(FRHICommandListImmediate& RHICmdList, FTextureRenderTargetResource* GpuProducedScreenTexture)
{
	SCOPED_NAMED_EVENT(FXboxFrontPanelModule_DrawScreen_RenderThread, FColor::Turquoise);
	check(FrontPanel != nullptr);
	check(GpuProducedScreenTexture != nullptr);
	if (CPUTexture[CPUTextureIndex])
	{
		BYTE* ResultsBuffer = nullptr;
		int32 MappedWidth = 0;
		int32 MappedHeight = 0;

		// Note: not calling via RHICmdList because we don't want the ImmediateFlush
		GDynamicRHI->RHIMapStagingSurface(CPUTexture[CPUTextureIndex], *(void**)&ResultsBuffer, MappedWidth, MappedHeight);
		if (ResultsBuffer != nullptr)
		{
			SCOPED_NAMED_EVENT(FrontPanel_ComputeLuminance, FColor::Turquoise);

			check(MappedWidth == Width);
			check(MappedHeight == Height);

			int MappedWidthBytes = MappedWidth * 4;
			int32 MappedPitch = Align(MappedWidthBytes, D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT);
			check(MappedWidthBytes % 64 == 0);

			const VectorRegister LuminanceFactor = VectorSetFloat3(0.11f, 0.59f, 0.3f);
			const __m128i ZeroVector = _mm_setzero_si128();

			BYTE* Dest = FrontScreenData.Get();
			const BYTE* SrcSurfaceBegin = ResultsBuffer;
			const BYTE* SrcSurfaceEnd = SrcSurfaceBegin + MappedHeight * MappedPitch;

			for (const BYTE* Line = SrcSurfaceBegin; Line < SrcSurfaceEnd; Line += MappedPitch)
			{
				const BYTE* LineEnd = Line + MappedPitch;
				for (const BYTE* SrcPixelBlock = Line; SrcPixelBlock < LineEnd; SrcPixelBlock += 64)
				{
					// Operate on 16 pixels at a time in order to produce a single __m128's worth
					// of 8bpp output.  Divided into two blocks of 8 for performance.

					// First set of 8 pixels
					// Profiling indicates that this version is faster than a simpler version using 16 x VectorLoadByte4
					VectorRegisterInt Src0123 = _mm_load_si128(reinterpret_cast<const VectorRegisterInt*>(SrcPixelBlock + 0));
					VectorRegisterInt Src4567 = _mm_load_si128(reinterpret_cast<const VectorRegisterInt*>(SrcPixelBlock + 16));

					// Unpack so each channel is 32bit and convert to float.
					VectorRegister Src0 = _mm_cvtepi32_ps(_mm_unpacklo_epi8(_mm_unpacklo_epi8(Src0123, ZeroVector), ZeroVector));
					VectorRegister Src1 = _mm_cvtepi32_ps(_mm_unpackhi_epi8(_mm_unpacklo_epi8(Src0123, ZeroVector), ZeroVector));
					VectorRegister Src2 = _mm_cvtepi32_ps(_mm_unpacklo_epi8(_mm_unpackhi_epi8(Src0123, ZeroVector), ZeroVector));
					VectorRegister Src3 = _mm_cvtepi32_ps(_mm_unpackhi_epi8(_mm_unpackhi_epi8(Src0123, ZeroVector), ZeroVector));
					VectorRegister Src4 = _mm_cvtepi32_ps(_mm_unpacklo_epi8(_mm_unpacklo_epi8(Src4567, ZeroVector), ZeroVector));
					VectorRegister Src5 = _mm_cvtepi32_ps(_mm_unpackhi_epi8(_mm_unpacklo_epi8(Src4567, ZeroVector), ZeroVector));
					VectorRegister Src6 = _mm_cvtepi32_ps(_mm_unpacklo_epi8(_mm_unpackhi_epi8(Src4567, ZeroVector), ZeroVector));
					VectorRegister Src7 = _mm_cvtepi32_ps(_mm_unpackhi_epi8(_mm_unpackhi_epi8(Src4567, ZeroVector), ZeroVector));

					// Luminance computation
					VectorRegister Lum0 = VectorDot3(Src0, LuminanceFactor);
					VectorRegister Lum1 = VectorDot3(Src1, LuminanceFactor);
					VectorRegister Lum2 = VectorDot3(Src2, LuminanceFactor);
					VectorRegister Lum3 = VectorDot3(Src3, LuminanceFactor);
					VectorRegister Lum4 = VectorDot3(Src4, LuminanceFactor);
					VectorRegister Lum5 = VectorDot3(Src5, LuminanceFactor);
					VectorRegister Lum6 = VectorDot3(Src6, LuminanceFactor);
					VectorRegister Lum7 = VectorDot3(Src7, LuminanceFactor);

					// Pack back down again to 16bit integer luminance per pixel
					VectorRegister Lum0011 = VectorShuffle(Lum0, Lum1, 0, 0, 0, 0);
					VectorRegister Lum2233 = VectorShuffle(Lum2, Lum3, 0, 0, 0, 0);
					VectorRegister Lum0123 = VectorShuffle(Lum0011, Lum2233, 0, 2, 0, 2);
					VectorRegisterInt Lum0123Int = _mm_cvtps_epi32(Lum0123);

					VectorRegister Lum4455 = VectorShuffle(Lum4, Lum5, 0, 0, 0, 0);
					VectorRegister Lum6677 = VectorShuffle(Lum6, Lum7, 0, 0, 0, 0);
					VectorRegister Lum4567 = VectorShuffle(Lum4455, Lum6677, 0, 2, 0, 2);
					VectorRegisterInt Lum4567Int = _mm_cvtps_epi32(Lum4567);

					VectorRegisterInt Lum01234567 = _mm_packus_epi32(Lum0123Int, Lum4567Int);

					// Now repeat the above with the next set of 8 pixels
					VectorRegisterInt Src89ab = _mm_load_si128(reinterpret_cast<const VectorRegisterInt*>(SrcPixelBlock + 32));
					VectorRegisterInt Srccdef = _mm_load_si128(reinterpret_cast<const VectorRegisterInt*>(SrcPixelBlock + 48));

					VectorRegister Src8 = _mm_cvtepi32_ps(_mm_unpacklo_epi8(_mm_unpacklo_epi8(Src89ab, ZeroVector), ZeroVector));
					VectorRegister Src9 = _mm_cvtepi32_ps(_mm_unpackhi_epi8(_mm_unpacklo_epi8(Src89ab, ZeroVector), ZeroVector));
					VectorRegister Srca = _mm_cvtepi32_ps(_mm_unpacklo_epi8(_mm_unpackhi_epi8(Src89ab, ZeroVector), ZeroVector));
					VectorRegister Srcb = _mm_cvtepi32_ps(_mm_unpackhi_epi8(_mm_unpackhi_epi8(Src89ab, ZeroVector), ZeroVector));
					VectorRegister Srcc = _mm_cvtepi32_ps(_mm_unpacklo_epi8(_mm_unpacklo_epi8(Srccdef, ZeroVector), ZeroVector));
					VectorRegister Srcd = _mm_cvtepi32_ps(_mm_unpackhi_epi8(_mm_unpacklo_epi8(Srccdef, ZeroVector), ZeroVector));
					VectorRegister Srce = _mm_cvtepi32_ps(_mm_unpacklo_epi8(_mm_unpackhi_epi8(Srccdef, ZeroVector), ZeroVector));
					VectorRegister Srcf = _mm_cvtepi32_ps(_mm_unpackhi_epi8(_mm_unpackhi_epi8(Srccdef, ZeroVector), ZeroVector));

					VectorRegister Lum8 = VectorDot3(Src8, LuminanceFactor);
					VectorRegister Lum9 = VectorDot3(Src9, LuminanceFactor);
					VectorRegister Luma = VectorDot3(Srca, LuminanceFactor);
					VectorRegister Lumb = VectorDot3(Srcb, LuminanceFactor);
					VectorRegister Lumc = VectorDot3(Srcc, LuminanceFactor);
					VectorRegister Lumd = VectorDot3(Srcd, LuminanceFactor);
					VectorRegister Lume = VectorDot3(Srce, LuminanceFactor);
					VectorRegister Lumf = VectorDot3(Srcf, LuminanceFactor);

					VectorRegister Lum8899 = VectorShuffle(Lum8, Lum9, 0, 0, 0, 0);
					VectorRegister Lumaabb = VectorShuffle(Luma, Lumb, 0, 0, 0, 0);
					VectorRegister Lum89ab = VectorShuffle(Lum8899, Lumaabb, 0, 2, 0, 2);
					VectorRegisterInt Lum89abInt = _mm_cvttps_epi32(Lum89ab);

					VectorRegister Lumccdd = VectorShuffle(Lumc, Lumd, 0, 0, 0, 0);
					VectorRegister Lumeeff = VectorShuffle(Lume, Lumf, 0, 0, 0, 0);
					VectorRegister Lumcdef = VectorShuffle(Lumccdd, Lumeeff, 0, 2, 0, 2);
					VectorRegisterInt LumcdefInt = _mm_cvttps_epi32(Lumcdef);

					VectorRegisterInt Lum89abcdef = _mm_packus_epi32(Lum89abInt, LumcdefInt);

					// Finally, pack the 2 sets of 16bit luminance values to 8bpp and store.
					_mm_store_si128(reinterpret_cast<VectorRegisterInt*>(Dest), _mm_packus_epi16(Lum01234567, Lum89abcdef));

					Dest += 16;
				}
			}

			VectorResetFloatRegisters();
		}

		// Note: not calling via RHICmdList because we don't want the ImmediateFlush
		GDynamicRHI->RHIUnmapStagingSurface(CPUTexture[CPUTextureIndex]);

		{
			SCOPED_NAMED_EVENT(FrontPanel_PresentBuffer, FColor::Turquoise);
			FrontPanel->PresentBuffer(FrontScreenDataSize, FrontScreenData.Get());
		}
	}
	else
	{
		FRHIResourceCreateInfo CreateInfo;
		CPUTexture[CPUTextureIndex] = RHICreateTexture2D(Width, Height, PF_B8G8R8A8, 1, 1, TexCreate_CPUReadback, CreateInfo);
	}
	auto TextureRHI = GpuProducedScreenTexture->GetTextureRenderTarget2DResource()->GetTextureRHI();
	RHICmdList.CopyToResolveTarget(TextureRHI, CPUTexture[CPUTextureIndex], false, FResolveParams());

	CPUTextureIndex = CPUTextureIndex == 0 ? 1 : 0;
}

void FXboxFrontPanelModule::DrawScreen_GameThread(float DeltaTime)
{
	check(Window.IsValid());
	check(HitTestGrid.IsValid());
	check(RenderTarget != nullptr);

	// CPU cost here will depend on the complexity of the UI hosted on the front panel.
	// We do the best we can by allocating the window and hit test grid externally, plus avoiding the prepass and hit test clear when possible.
	WidgetRenderer.DrawWindow(RenderTarget, HitTestGrid.ToSharedRef(), Window.ToSharedRef(), 1.0f, FVector2D(Width, Height), DeltaTime);

	// Should only ever need a pre-pass once per widget
	WidgetRenderer.SetIsPrepassNeeded(false);

	FTextureRenderTargetResource* GpuProducedScreenTexture = RenderTarget->GameThread_GetRenderTargetResource();
	ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(FXboxFrontPanelModule_Tick,
		FXboxFrontPanelModule*, FrontPanelModule, this,
		FTextureRenderTargetResource*, GpuProducedScreenTexture, GpuProducedScreenTexture,
		{
			SCOPED_DRAW_EVENT(RHICmdList, FrontPanelReadback);
			FrontPanelModule->DrawScreen_RenderThread(RHICmdList, GpuProducedScreenTexture);
		});
}

void FXboxFrontPanelModule::Tick(float DeltaTime)
{
	if (Window.IsValid())
	{
		DrawScreen_GameThread(DeltaTime);
	}

	if (FrontPanel != nullptr)
	{
		GenerateButtonEvents();
	}
}

void FXboxFrontPanelModule::GenerateSingleButtonEvent(int32 NewState, int32 LastState, FGamepadKeyNames::Type KeyName, double CurrentTime, double& RepeatAt)
{
	if (NewState != 0)
	{
		if (LastState == 0)
		{
			FSlateApplication::Get().OnControllerButtonPressed(KeyName, 0, false);
			RepeatAt = CurrentTime + InitialButtonRepeatDelay;
		}
		else if (CurrentTime >= RepeatAt)
		{
			FSlateApplication::Get().OnControllerButtonPressed(KeyName, 0, true);
			RepeatAt = CurrentTime + ButtonRepeatDelay;
		}
	}
	else if (LastState != 0)
	{
		FSlateApplication::Get().OnControllerButtonReleased(KeyName, 0, false);
	}
}

void FXboxFrontPanelModule::GenerateButtonEvents()
{
	XBOX_FRONT_PANEL_BUTTONS NewButtonStates;
	HRESULT ButtonStateResult = FrontPanel->GetButtonStates(&NewButtonStates);
	if (FAILED(ButtonStateResult))
	{
		UE_LOG(LogXboxFrontPanel, Warning, TEXT("Failed to read Xbox Front Panel button states: %08X"), ButtonStateResult);
		return;
	}

	const double CurrentTime = FPlatformTime::Seconds();
	GenerateSingleButtonEvent(NewButtonStates & XBOX_FRONT_PANEL_BUTTONS_BUTTON1, LastButtonStates & XBOX_FRONT_PANEL_BUTTONS_BUTTON1, XboxFrontPanelKeyNames::Button1, CurrentTime, NextButtonRepeatTime[0]);
	GenerateSingleButtonEvent(NewButtonStates & XBOX_FRONT_PANEL_BUTTONS_BUTTON2, LastButtonStates & XBOX_FRONT_PANEL_BUTTONS_BUTTON2, XboxFrontPanelKeyNames::Button2, CurrentTime, NextButtonRepeatTime[1]);
	GenerateSingleButtonEvent(NewButtonStates & XBOX_FRONT_PANEL_BUTTONS_BUTTON3, LastButtonStates & XBOX_FRONT_PANEL_BUTTONS_BUTTON3, XboxFrontPanelKeyNames::Button3, CurrentTime, NextButtonRepeatTime[2]);
	GenerateSingleButtonEvent(NewButtonStates & XBOX_FRONT_PANEL_BUTTONS_BUTTON4, LastButtonStates & XBOX_FRONT_PANEL_BUTTONS_BUTTON4, XboxFrontPanelKeyNames::Button4, CurrentTime, NextButtonRepeatTime[3]);
	GenerateSingleButtonEvent(NewButtonStates & XBOX_FRONT_PANEL_BUTTONS_BUTTON5, LastButtonStates & XBOX_FRONT_PANEL_BUTTONS_BUTTON5, XboxFrontPanelKeyNames::Button5, CurrentTime, NextButtonRepeatTime[4]);

	GenerateSingleButtonEvent(NewButtonStates & XBOX_FRONT_PANEL_BUTTONS_LEFT, LastButtonStates & XBOX_FRONT_PANEL_BUTTONS_LEFT, XboxFrontPanelKeyNames::DPadLeft, CurrentTime, NextButtonRepeatTime[5]);
	GenerateSingleButtonEvent(NewButtonStates & XBOX_FRONT_PANEL_BUTTONS_RIGHT, LastButtonStates & XBOX_FRONT_PANEL_BUTTONS_RIGHT, XboxFrontPanelKeyNames::DPadRight, CurrentTime, NextButtonRepeatTime[6]);
	GenerateSingleButtonEvent(NewButtonStates & XBOX_FRONT_PANEL_BUTTONS_UP, LastButtonStates & XBOX_FRONT_PANEL_BUTTONS_UP, XboxFrontPanelKeyNames::DPadUp, CurrentTime, NextButtonRepeatTime[7]);
	GenerateSingleButtonEvent(NewButtonStates & XBOX_FRONT_PANEL_BUTTONS_DOWN, LastButtonStates & XBOX_FRONT_PANEL_BUTTONS_DOWN, XboxFrontPanelKeyNames::DPadDown, CurrentTime, NextButtonRepeatTime[8]);

	GenerateSingleButtonEvent(NewButtonStates & XBOX_FRONT_PANEL_BUTTONS_SELECT, LastButtonStates & XBOX_FRONT_PANEL_BUTTONS_SELECT, XboxFrontPanelKeyNames::DPadPress, CurrentTime, NextButtonRepeatTime[9]);

	LastButtonStates = NewButtonStates;
}

void FXboxFrontPanelModule::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(RenderTarget);
}

static const XBOX_FRONT_PANEL_LIGHTS LightsByIndex[] =
{
	XBOX_FRONT_PANEL_LIGHTS_LIGHT1,
	XBOX_FRONT_PANEL_LIGHTS_LIGHT2,
	XBOX_FRONT_PANEL_LIGHTS_LIGHT3,
	XBOX_FRONT_PANEL_LIGHTS_LIGHT4,
	XBOX_FRONT_PANEL_LIGHTS_LIGHT5
};

void FXboxFrontPanelModule::SetButtonLightState(EXboxFrontPanelButtonLight Light, bool OnOff)
{
	if (FrontPanel != nullptr)
	{
		int32 ButtonIndex = static_cast<int32>(Light);
		check(ButtonIndex < _countof(LightsByIndex));

		XBOX_FRONT_PANEL_LIGHTS CurrentLightState = XBOX_FRONT_PANEL_LIGHTS_NONE;
		HRESULT GetLightStateResult = FrontPanel->GetLightStates(&CurrentLightState);
		if (SUCCEEDED(GetLightStateResult))
		{
			if (OnOff)
			{
				CurrentLightState = static_cast<XBOX_FRONT_PANEL_LIGHTS>(static_cast<uint32>(CurrentLightState) | LightsByIndex[ButtonIndex]);
			}
			else
			{
				CurrentLightState = static_cast<XBOX_FRONT_PANEL_LIGHTS>(static_cast<uint32>(CurrentLightState) & ~LightsByIndex[ButtonIndex]);
			}
			HRESULT SetLightStateResult = FrontPanel->SetLightStates(CurrentLightState);
			if (FAILED(SetLightStateResult))
			{
				UE_LOG(LogXboxFrontPanel, Warning, TEXT("SetButtonLightState failed setting new light state: %08X"), SetLightStateResult);
			}
		}
		else
		{
			UE_LOG(LogXboxFrontPanel, Warning, TEXT("SetButtonLightState failed reading existing light state: %08X"), GetLightStateResult);
		}
	}
}

bool FXboxFrontPanelModule::GetButtonLightState(EXboxFrontPanelButtonLight Light)
{
	if (FrontPanel != nullptr)
	{
		int32 ButtonIndex = static_cast<int32>(Light);
		check(ButtonIndex < _countof(LightsByIndex));

		XBOX_FRONT_PANEL_LIGHTS CurrentLightState = XBOX_FRONT_PANEL_LIGHTS_NONE;
		HRESULT GetLightStateResult = FrontPanel->GetLightStates(&CurrentLightState);
		if (FAILED(GetLightStateResult))
		{
			UE_LOG(LogXboxFrontPanel, Warning, TEXT("GetButtonLightState failed reading existing light state: %08X"), GetLightStateResult);
		}
		return (CurrentLightState & LightsByIndex[ButtonIndex]) != 0;
	}

	return false;
}

void FXboxFrontPanelModule::SetScreenWidget(UUserWidget* Widget)
{
	if (Widget)
	{
		SetScreenWidget(Widget->TakeWidget());
	}
	else
	{
		SetScreenWidget(TSharedPtr<SWidget>());
	}
}

void FXboxFrontPanelModule::SetScreenWidget(TSharedPtr<SWidget> Widget)
{
	if (Widget.IsValid())
	{
		if (!InitScreenResources())
		{
			UE_LOG(LogXboxFrontPanel, Warning, TEXT("SetScreenWidget ignored: Xbox Front Panel screen is not supported."));
			return;
		}

		check(Window.IsValid());

		FrontScreenWidget = Widget;
		Window->SetContent(FrontScreenWidget.ToSharedRef());

		// New widget needs a new prepass
		WidgetRenderer.SetIsPrepassNeeded(true);
	}
	else if (FrontScreenWidget.IsValid())
	{
		FrontScreenWidget.Reset();

		DeinitScreenResources();
	}
}

bool FXboxFrontPanelModule::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (FrontScreenWidget.IsValid())
	{
		FReply Reply = FrontScreenWidget->OnPreviewKeyDown(FrontScreenWidget->GetCachedGeometry(), InKeyEvent);
		if (Reply.IsEventHandled())
		{
			return true;
		}

		Reply = FrontScreenWidget->OnKeyDown(FrontScreenWidget->GetCachedGeometry(), InKeyEvent);
		if (Reply.IsEventHandled())
		{
			return true;
		}
	}

	return false;
}

bool FXboxFrontPanelModule::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (FrontScreenWidget.IsValid())
	{
		FReply Reply = FrontScreenWidget->OnKeyUp(FrontScreenWidget->GetCachedGeometry(), InKeyEvent);
		if (Reply.IsEventHandled())
		{
			return true;
		}
	}

	return false;
}

bool FXboxFrontPanelModule::InitScreenResources()
{
	if (Window.IsValid())
	{
		// Already initialized
		return true;
	}

	if (!FrontPanel)
	{
		// No front panel at all, no hope of a screen
		return false;
	}

	// Validate that the front panel dimensions and format match
	// the assumptions in DrawScreen_RenderThread. 
	bool bCanUseFrontScreen = true;
	bCanUseFrontScreen &= SUCCEEDED(FrontPanel->GetScreenWidth(&Width));
	bCanUseFrontScreen &= SUCCEEDED(FrontPanel->GetScreenHeight(&Height));

	DXGI_FORMAT PlatformPixelFormat;
	bCanUseFrontScreen &= SUCCEEDED(FrontPanel->GetScreenPixelFormat(&PlatformPixelFormat));
	switch (PlatformPixelFormat)
	{
	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
		// We read and write 16 pixels at a time when calculating luminance on the CPU
		bCanUseFrontScreen &= (Width % 16 == 0);
		break;
	default:
		bCanUseFrontScreen = false;
		break;
	}

	if (bCanUseFrontScreen)
	{
		Window = SNew(SVirtualWindow).Size(FVector2D(Width, Height));
		Window->Resize(FVector2D(Width, Height));

		HitTestGrid = MakeShared<FHittestGrid>();

		RenderTarget = NewObject<UTextureRenderTarget2D>();
		RenderTarget->Filter = TF_Nearest;
		RenderTarget->ClearColor = FLinearColor::Transparent;
		RenderTarget->SRGB = false;
		RenderTarget->TargetGamma = 1;
		RenderTarget->InitCustomFormat(Width, Height, PF_B8G8R8A8, true);
		RenderTarget->UpdateResourceImmediate(true);

		WidgetRenderer.SetUseGammaCorrection(false);
		WidgetRenderer.SetClearHitTestGrid(false);

		// Schedule init for members owned by the render side
		ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(FXboxFrontPanelModule_InitScreenResources_RenderThread,
			FXboxFrontPanelModule*, FrontPanelModule, this,
			UINT32, Width, Width,
			UINT32, Height, Height,
			{
				FrontPanelModule->CPUTexture[0] = nullptr;
				FrontPanelModule->CPUTexture[1] = nullptr;
				FrontPanelModule->CPUTextureIndex = 0;

				FrontPanelModule->FrontScreenDataSize = Width * Height;
				FrontPanelModule->FrontScreenData.Reset(static_cast<BYTE*>(FMemory::Malloc(FrontPanelModule->FrontScreenDataSize, 16)));

				// No need for initial zero or present.  The previous owner (whether us, or the system) should have ensured
				// that the screen isn't presenting garbage.
			});
	}
	else
	{
		UE_LOG(LogXboxFrontPanel, Warning, TEXT("Xbox Front Panel screen (width %d, pixel format %d) is not compatible with plugin render code."), Width, PlatformPixelFormat);
	}

	return bCanUseFrontScreen;
}

void FXboxFrontPanelModule::DeinitScreenResources()
{
	if (Window.IsValid())
	{
		RenderTarget = nullptr;

		Window.Reset();
		HitTestGrid.Reset();

		// Schedule deinit for members owned by the render side
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(FXboxFrontPanelModule_DeinitScreenResources_RenderThread,
			FXboxFrontPanelModule*, FrontPanelModule, this,
			{
				// Clear the screen before cleaning up.
				FMemory::Memzero(FrontPanelModule->FrontScreenData.Get(), FrontPanelModule->FrontScreenDataSize);
				FrontPanelModule->FrontPanel->PresentBuffer(FrontPanelModule->FrontScreenDataSize, FrontPanelModule->FrontScreenData.Get());

				FrontPanelModule->FrontScreenData.Reset();

				FrontPanelModule->CPUTexture[0] = nullptr;
				FrontPanelModule->CPUTexture[1] = nullptr;
				FrontPanelModule->CPUTextureIndex = 0;
			});
	}
}

#include "XboxOneHidePlatformTypes.h"

#endif // PLATFORM_XBOXONE