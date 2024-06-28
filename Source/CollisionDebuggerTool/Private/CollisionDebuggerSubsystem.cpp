// Fill out your copyright notice in the Description page of Project Settings.


#include "CollisionDebuggerSubsystem.h"
#include "Engine/TextureRenderTarget2D.h"

#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Blueprint/UserWidget.h"
#include "Engine/StreamableManager.h"

#include "Engine/AssetManager.h"
#include <EditorWorldExtension.h>
#include "HAL/IConsoleManager.h"


#if WITH_EDITOR
    #include "Editor.h"
    #include "LevelEditor.h"
    #include "EditorViewportClient.h"
    #include "SLevelViewport.h"
#endif

static TAutoConsoleVariable<int32> CVarCollisionDebugEnable(
    TEXT("CollisionDebug"),
    0,
    TEXT("Creates a window to debug the collision in the game.\n")
    TEXT(" 0: off \n")
    TEXT(" 1: on  \n"),
    ECVF_Scalability | ECVF_RenderThreadSafe);


void UCollisionDebuggerSubsystem::CheckState()
{
    const int32 IntendedState = CVarCollisionDebugEnable.GetValueOnGameThread();
    bool ShouldBeOn = (IntendedState > 0);

    if (ShouldBeOn && ShouldTickOrRun())
    {
        if (!ShouldRun)
        {
            StartCollisionDebug();
        }
    }
    else 
    {
        if (ShouldRun)
        {
            StopCollisionDebug();
        }
    }
}

void UCollisionDebuggerSubsystem::Tick(float DeltaTime)
{
    CheckState();

    if (!StopHasStarted) 
    {
        if (!Task.IsValid() && ShouldRun)
        {
            UWorld* world = GetWorld();
            FTransform trans;

            if (world)
            {
                APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
                if (CameraManager)
                {
                    trans = CameraManager->GetTransform();
                }
                else
                {
#if WITH_EDITOR
                    FViewport* ViewPort = GEditor->GetActiveViewport();
                    if (ViewPort->IsPlayInEditorViewport())
                    {
                        return;
                    }
                    FEditorViewportClient* client = (FEditorViewportClient*)ViewPort->GetClient();
                    FVector CamPos = client->GetViewLocation();
                    trans = FTransform(client->GetViewRotation(), CamPos, FVector::OneVector);
#endif // WITH_EDITOR
                }
            }
            else
            {
                return;
            }

            FUpdateTextureRegion2D region = FUpdateTextureRegion2D(IndexX, IndexY, IndexX, IndexY, UpdateSize, UpdateSize);
            if (IsValid(DebugRenderTarget) && !StopHasStarted && ShouldRun)
            {
                Task = UE::Tasks::Launch(UE_SOURCE_LOCATION, [this, trans, region] {UpdateTexture(trans, region, CurrentRenderSettings); });

                IndexX += UpdateSize;
                if (IndexX >= DebugRenderTarget->SizeX)
                {
                    IndexX = 0;
                    IndexY += UpdateSize;
                }

                if (IndexY >= DebugRenderTarget->SizeY)
                {
                    IndexY = 0;
                }
            }

        }
        else if (Task.IsCompleted())
        {
            Task = {};
        }
    }
    else
    {
        if (Task.IsCompleted())
        {
            ShouldRun = false;
            Task = {};
            CleanupAndClear();
        }
    }
}

void UCollisionDebuggerSubsystem::OnPreEndPIE(const bool bIsSimulating)
{
    CleanupAndClear();
}

TStatId UCollisionDebuggerSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCollisionDebuggerSubsystem, STATGROUP_Tickables);
}

void UCollisionDebuggerSubsystem::Deinitialize()
{
    ShouldRun = false;
    StopHasStarted = true;
    CleanupAndClear();
}

bool UCollisionDebuggerSubsystem::IsTickable() const
{
    return true;
}

void UCollisionDebuggerSubsystem::SetupAssets()
{
    PIECallbackHandle = FEditorDelegates::BeginPIE.AddUObject(this, &UCollisionDebuggerSubsystem::OnPreEndPIE);
    DebugRenderTarget = Cast<UTextureRenderTarget2D>(StaticLoadObject(UTextureRenderTarget2D::StaticClass(), NULL, TEXT("/Script/Engine.TextureRenderTarget2D'/CollisionDebuggerTool/RT_CollisionDebugger_PhyCap.RT_CollisionDebugger_PhyCap'")));

    if (PixelColors.Num() != DebugRenderTarget->SizeX * DebugRenderTarget->SizeY)
    {
        PixelColors.SetNumZeroed(DebugRenderTarget->SizeX * DebugRenderTarget->SizeY);
    }
  
    SetupWidget();
}

void UCollisionDebuggerSubsystem::CleanupAndClear()
{
    StopHasStarted = true;
    ShouldRun = false;
    RemoveEditorWidget();

    if (Task.IsValid())
    {
        Task.Wait();
    }

    FEditorDelegates::BeginPIE.Remove(PIECallbackHandle);
    DebugRenderTarget = nullptr;
    PixelColors.Empty();
}

ETickableTickType UCollisionDebuggerSubsystem::GetTickableTickType() const
{
    return ETickableTickType::Always;
}

bool UCollisionDebuggerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void UCollisionDebuggerSubsystem::RemoveEditorWidget()
{
#if WITH_EDITOR
    if (CreatedSlateWidget)
    {
        FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
        TWeakPtr<ILevelEditor> editor = LevelEditor.GetLevelEditorInstance();
        TSharedPtr<ILevelEditor> PinnedEditor(editor.Pin());

        if (PinnedEditor.IsValid())
        {
            TSharedPtr<SLevelViewport> LevelViewport = PinnedEditor->GetActiveViewportInterface();
            if (CreatedSlateWidget)
            {
                LevelViewport->RemoveOverlayWidget(CreatedSlateWidget.ToSharedRef());
            }
        }

        CreatedSlateWidget.Reset();
        CreatedSlateWidget = nullptr;
    }
#endif// WITH_EDITOR

    if (CollisionDebugMainWidget)
    {
        CollisionDebugMainWidget->RemoveFromParent();
        CollisionDebugMainWidget->MarkAsGarbage();
        CollisionDebugMainWidget = nullptr;
    }

    CollisionDebugMainWidgetClass = nullptr;
}

void UCollisionDebuggerSubsystem::SetupWidget()
{
    FStreamableManager& AssetLoader = UAssetManager::GetStreamableManager();
    CollisionDebugMainWidgetClass = AssetLoader.LoadSynchronous<UClass>(FSoftObjectPath("/CollisionDebuggerTool/UI_CollisionDebugView.UI_CollisionDebugView_C"));

    if (CollisionDebugMainWidgetClass)
    {
        UWorld* World = GetWorld();
        CollisionDebugMainWidget = CreateWidget<UUserWidget>(World, CollisionDebugMainWidgetClass);

        if (World->IsGameWorld())
        {
            CollisionDebugMainWidget->AddToViewport();
        }
        else
        {
#if WITH_EDITOR

            FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
            TWeakPtr<ILevelEditor> editor = LevelEditor.GetLevelEditorInstance();
            TSharedPtr<ILevelEditor> PinnedEditor(editor.Pin());

            if (PinnedEditor.IsValid())
            {
                TSharedPtr<SLevelViewport> slevelviewport = PinnedEditor->GetActiveViewportInterface();
                FChildren* childrenSlot = slevelviewport->GetChildren();
                if (childrenSlot)
                {
                    TSharedRef<SWidget> InternalCreatedSlateWidget = CollisionDebugMainWidget->TakeWidget();
                    CreatedSlateWidget = InternalCreatedSlateWidget.ToSharedPtr();
                    slevelviewport->AddOverlayWidget(CreatedSlateWidget.ToSharedRef());
                }
            }
#endif // WITH_EDITOR
        }
    }
}


void UCollisionDebuggerSubsystem::UpdateTextureRegion(
    FTexture2DRHIRef TextureRHI,
    int32 MipIndex,
    uint32 NumRegions,
    FUpdateTextureRegion2D Region,
    uint32 SrcPitch,
    uint32 SrcBpp,
    uint8* SrcData,
    TFunction<void(uint8* SrcData)> DataCleanupFunc
)
{
    FTaskTagScope Scope(ETaskTag::EParallelRenderingThread);
    ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)(
        [=](FRHICommandListImmediate& RHICmdList)
        {
            if (!TextureRHI.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Invalid Texture2DRHIRef"));
                return;
            }

    RHIUpdateTexture2D(
        TextureRHI,
        MipIndex,
        Region,
        SrcPitch,
        SrcData
        + Region.SrcY * SrcPitch
        + Region.SrcX * SrcBpp
    );

    DataCleanupFunc(SrcData);
        });
}


void UCollisionDebuggerSubsystem::UpdateTexture(FTransform trans, FUpdateTextureRegion2D region, FInputRenderSettingsInternal TileRenderSettings)
{
    UWorld* world = GetWorld();
    if (!IsValid(world)) { return; }

    FlushPersistentDebugLines(world);
    FVector CamPos = trans.GetLocation();

    for (int32 y = region.SrcY; y < region.SrcY + UpdateSize; y++)
    {
        for (int32 x = region.SrcX; x < region.SrcX + UpdateSize; x++)
        {
            float xOff = ((x / (float)(DebugRenderTarget->SizeX)) - .5) * 2;
            float yOff = ((y / (float)(DebugRenderTarget->SizeY)) - .5) * -2;
            float FovHack = .7;
            FVector dir = trans.TransformVector(FVector(1, xOff * FovHack, yOff * FovHack)).GetSafeNormal();

            FHitResult RV_Hit;

            FCollisionQueryParams RV_TraceParams;
            RV_TraceParams.bTraceComplex = TileRenderSettings.TraceComplex;
            RV_TraceParams.bReturnPhysicalMaterial = false;

            bool DidTrace = false;
            if (TileRenderSettings.bIsChannelTest)
            {
                DidTrace = world->LineTraceSingleByChannel(RV_Hit, CamPos, CamPos + (dir * 100000), TileRenderSettings.ChannelToTest, RV_TraceParams);
            }
            else
            {
                DidTrace = world->LineTraceSingleByProfile(RV_Hit, CamPos, CamPos + (dir * 100000), TileRenderSettings.ProfileNameToTest, RV_TraceParams);
            }
            

            const int32 index = x + (y * DebugRenderTarget->SizeX);
            if (!PixelColors.IsValidIndex(index)) { return; }
            if (DidTrace)
            {
                PixelColors[index] = FLinearColor(RV_Hit.Normal.X, RV_Hit.Normal.Y, RV_Hit.Normal.Z, RV_Hit.Time);
            }
            else
            {
                PixelColors[index] = FLinearColor(-1, -1, -1, -1);

            }

        }
    }



    FTaskTagScope scope(ETaskTag::EParallelRenderingThread);
    UpdateTextureRegion(DebugRenderTarget->GetResource()->GetTexture2DRHI(), 0, 1, region, DebugRenderTarget->SizeX * 16, 16, reinterpret_cast<uint8*>(PixelColors.GetData()));

}


void UCollisionDebuggerSubsystem::StartCollisionDebug()
{
    SetupAssets();
    StopHasStarted = false;
    ShouldRun = true;
}

void UCollisionDebuggerSubsystem::StopCollisionDebug()
{
    StopHasStarted = true;
}

bool UCollisionDebuggerSubsystem::ShouldTickOrRun()
{
    bool IsRuntimeOrPIE = true;
#if WITH_EDITOR
    IsRuntimeOrPIE = (GEditor && GEditor->PlayWorld) || GIsPlayInEditorWorld || IsRunningGame();
#endif // WITH_EDITOR
    
    UWorld* World = GetWorld();

    if (IsRuntimeOrPIE && World->IsGameWorld())
    {
        return true;
    }

    if (!IsRuntimeOrPIE && World->IsEditorWorld())
    {
        return true;
    }

    return false;

}

TArray<FString> UCollisionDebuggerSubsystem::GetCollisionProfiles()
{
    TArray<TSharedPtr<FName>> OutNameList;
    UCollisionProfile::Get()->GetProfileNames(OutNameList);
    TArray<FString> result;
    for (TSharedPtr<FName> Name : OutNameList)
    {
        result.Add(Name.Get()->ToString());
    }
    return result;
}

TArray<FString> UCollisionDebuggerSubsystem::GetCollisionChannels()
{
    UEnum* Enum = StaticEnum<ECollisionChannel>();
    int32 NumEnum = Enum->NumEnums();

    TArray<FString> result;

    for (int32 i = 0; i < NumEnum; i++)
    {
        result.Add(UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(i).ToString());
    }
    return result;

}

void UCollisionDebuggerSubsystem::SetRenderSettings(FInputRenderSettings NewSettings)
{
    // Test type
    CurrentRenderSettings.bIsChannelTest = NewSettings.IsChannelTest;
    
    {// channels
        TArray<FString> Channels = GetCollisionChannels();
        bool Channelfound = false;
        for (int32 i = 0; i < Channels.Num() && !Channelfound; i++)
        {
            if (Channels[i] == NewSettings.ChannelName)
            {
                CurrentRenderSettings.ChannelToTest = (ECollisionChannel)i;
                break;
            }
        }
        if (!Channelfound) 
        {
            CurrentRenderSettings.ChannelToTest = ECollisionChannel::ECC_WorldStatic;
        }
    }

    { // Profile

        TArray<FString> Profiles = GetCollisionProfiles();

        bool Profilefound = false;
        FName TestProfile = FName(NewSettings.ProfileName);
        for (int32 i = 0; i < Profiles.Num() && !Profilefound; i++)
        {
            if (Profiles[i] == TestProfile)
            {
                Profilefound = true;
                break;
            }
        }
        if (!Profilefound)
        {
            CurrentRenderSettings.ProfileNameToTest = FName(Profiles[0]);
        }
        else
        {
            CurrentRenderSettings.ProfileNameToTest = TestProfile;
        }
    }


    CurrentRenderSettings.TraceComplex = NewSettings.TraceComplex;

}
