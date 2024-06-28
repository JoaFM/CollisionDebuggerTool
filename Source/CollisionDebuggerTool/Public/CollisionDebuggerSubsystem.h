// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RHI.h"
#include "RHIResources.h"
#include "RHICommandList.h"
#include "TextureResource.h"
#include "RenderingThread.h"
#include "Tasks/Task.h"

// Slate
#include "Widgets/SWidget.h"
#include "Templates/SharedPointer.h"

// Reflection 
#include "CollisionDebuggerSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FInputRenderSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision Debugger Subsystem")
	bool IsChannelTest = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision Debugger Subsystem")
	bool TraceComplex = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision Debugger Subsystem")
	FString ChannelName = "";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision Debugger Subsystem")
	FString ProfileName = "";
};

USTRUCT()
struct FInputRenderSettingsInternal
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	bool bIsChannelTest = true;

	UPROPERTY(Transient)
	TEnumAsByte<ECollisionChannel> ChannelToTest = ECollisionChannel::ECC_WorldStatic;

	UPROPERTY(Transient)
	FName ProfileNameToTest = TEXT("");
	
	UPROPERTY(Transient)
	bool TraceComplex = false;
};

/**
 * TODO:
 * Make sure it compiles in development
 * Allow for selecting collision profiles
 * Exclude from shipping builds 
 * Add to button to toolbar
 * Make debug & camera FOV similar
 */
UCLASS()
class COLLISIONDEBUGGERTOOL_API UCollisionDebuggerSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	
public:
	//------- UTickableWorldSubsystem--------
	void Tick(float DeltaTime) override;
	TStatId GetStatId() const override;
	void Deinitialize() override;
	bool IsTickableWhenPaused() const override { return false; };;
	bool IsTickableInEditor() const override { return true; };
	bool IsTickable() const override;
	ETickableTickType GetTickableTickType() const override;
	bool ShouldCreateSubsystem(UObject* Outer) const override;
	//------- UTickableWorldSubsystem--------

	// Tools
	
	UFUNCTION(BlueprintCallable)
	TArray<FString> GetCollisionProfiles() ;

	UFUNCTION(BlueprintCallable)
	TArray<FString> GetCollisionChannels();

	UFUNCTION(BlueprintCallable)
	void SetRenderSettings(FInputRenderSettings NewSettings);

private:
	// ------------ Running --------------
	UPROPERTY(Transient)
	TObjectPtr<class UTextureRenderTarget2D> DebugRenderTarget = nullptr;

	UPROPERTY(Transient)
	TArray<FLinearColor> PixelColors;

	UPROPERTY(Transient)
	UClass* CollisionDebugMainWidgetClass = nullptr;

	UPROPERTY(Transient)
	class UUserWidget* CollisionDebugMainWidget = nullptr;

	UPROPERTY(Transient)
	bool ShouldRun = false;

	UPROPERTY(Transient)
	bool StopHasStarted = false;

	UPROPERTY(Transient)
	int32 IndexX = 0;

	UPROPERTY(Transient)
	int32 IndexY = 0;


	TSharedPtr<SWidget> CreatedSlateWidget = nullptr;
	const int32 UpdateSize = 256;
	UE::Tasks::FTask Task;
	FDelegateHandle PIECallbackHandle;

// ------------ Rendering --------------

	FInputRenderSettingsInternal CurrentRenderSettings;

private:
	 void UpdateTextureRegion(
		 FTexture2DRHIRef TextureRHI,
		 int32 MipIndex,
		 uint32 NumRegions,
		 FUpdateTextureRegion2D Region,
		 uint32 SrcPitch,
		 uint32 SrcBpp,
		 uint8* SrcData,
		 TFunction<void(uint8* SrcData)> DataCleanupFunc = [](uint8*) {}
	 );

	 void SetupAssets();
	 void CleanupAndClear();
	 void CheckState();
	 void SetupWidget();
	 bool ShouldTickOrRun();
	 void RemoveEditorWidget();
	 void StartCollisionDebug();
	 void StopCollisionDebug();

	 //GPU
	 void UpdateTexture(FTransform trans, FUpdateTextureRegion2D region, FInputRenderSettingsInternal TileRenderSettings);

	 //Callback
	 void OnPreEndPIE(const bool bIsSimulating);

};
