#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "EnemyEmergenceComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "Tests/RunTestListener.h"

namespace EmergenceTest
{
    // The engine sphere is 100 cm across, so its lowest point is at -50 in its own space.
    constexpr float SphereBottomLocalZ = -50.0f;

    // Where the bottom of a mesh is, relative to its parent, for the mesh's current scale and location.
    float BottomRelativeZ(const USceneComponent* Mesh)
    {
        return static_cast<float>(Mesh->GetRelativeLocation().Z + Mesh->GetRelativeScale3D().Z * SphereBottomLocalZ);
    }

    // A test world, an actor with a 60 cm sphere standing on its parent's origin, and an emergence component.
    struct FScene
    {
        FTestWorldWrapper WorldWrapper;
        AActor* Actor = nullptr;
        UStaticMeshComponent* Mesh = nullptr;
        UEnemyEmergenceComponent* Emergence = nullptr;
        URunTestListener* Listener = nullptr;

        bool Create(FAutomationTestBase& Test)
        {
            if (!WorldWrapper.CreateTestWorld(EWorldType::Game) || !WorldWrapper.BeginPlayInTestWorld())
            {
                WorldWrapper.ForwardErrorMessages(&Test);
                return false;
            }

            Actor = WorldWrapper.GetTestWorld()->SpawnActor<AActor>();
            UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
            if (!Test.TestNotNull(TEXT("Actor spawned"), Actor) || !Test.TestNotNull(TEXT("Sphere mesh loads"), Sphere))
            {
                return false;
            }

            USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
            Actor->SetRootComponent(Root);
            Root->RegisterComponent();

            Mesh = NewObject<UStaticMeshComponent>(Actor, TEXT("Mesh"));
            Mesh->SetStaticMesh(Sphere);
            Mesh->SetupAttachment(Root);
            Mesh->SetRelativeScale3D(FVector(0.6));
            Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Mesh->RegisterComponent();

            Emergence = NewObject<UEnemyEmergenceComponent>(Actor, TEXT("Emergence"));
            Emergence->Duration = 1.0f;
            Emergence->RegisterComponent();

            Listener = NewObject<URunTestListener>();
            Emergence->OnFinished.AddDynamic(Listener, &URunTestListener::OnEmergenceFinished);
            return true;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEmergencePoseTest,
    "GrantMoney.Run.Emergence.PoseMath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmergencePoseTest::RunTest(const FString& Parameters)
{
    const FVector Rest(0.6);
    const FVector Flat(1.25, 1.25, 0.1);
    const float Bottom = EmergenceTest::SphereBottomLocalZ;

    FVector Scale;
    FVector Location;

    UEnemyEmergenceComponent::ComputePose(Rest, FVector::ZeroVector, Bottom, Flat, 0.0f, Scale, Location);
    TestTrue(TEXT("Starts as a wide, flat shape"), Scale.Equals(FVector(0.75, 0.75, 0.06), 0.0001));
    TestEqual(TEXT("The bottom has not moved"), static_cast<float>(Location.Z + Scale.Z * Bottom), static_cast<float>(Rest.Z * Bottom), 0.001f);

    UEnemyEmergenceComponent::ComputePose(Rest, FVector::ZeroVector, Bottom, Flat, 1.0f, Scale, Location);
    TestTrue(TEXT("Ends at the normal scale"), Scale.Equals(Rest, 0.0001));
    TestTrue(TEXT("Ends at the normal location"), Location.Equals(FVector::ZeroVector, 0.0001));

    // The bottom stays put for every progress value, and the height only ever grows.
    double PreviousHeight = 0.0;
    for (int32 Step = 0; Step <= 20; ++Step)
    {
        UEnemyEmergenceComponent::ComputePose(Rest, FVector(5.0, 6.0, 7.0), Bottom, Flat, Step / 20.0f, Scale, Location);
        TestEqual(*FString::Printf(TEXT("Bottom fixed at step %d"), Step), static_cast<float>(Location.Z + Scale.Z * Bottom), static_cast<float>(7.0 + Rest.Z * Bottom), 0.001f);
        TestEqual(TEXT("Sideways position unchanged"), static_cast<float>(Location.X), 5.0f);
        TestTrue(TEXT("Height never shrinks"), Scale.Z >= PreviousHeight - UE_KINDA_SMALL_NUMBER);
        PreviousHeight = Scale.Z;
    }

    // Out-of-range progress is clamped.
    UEnemyEmergenceComponent::ComputePose(Rest, FVector::ZeroVector, Bottom, Flat, 7.0f, Scale, Location);
    TestTrue(TEXT("Progress above 1 gives the normal shape"), Scale.Equals(Rest, 0.0001));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEmergenceAnimatesVisualTest,
    "GrantMoney.Run.Emergence.AnimatesVisual",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmergenceAnimatesVisualTest::RunTest(const FString& Parameters)
{
    EmergenceTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    const float RestBottom = EmergenceTest::BottomRelativeZ(Scene.Mesh);
    TestEqual(TEXT("A 60 cm sphere's bottom is 30 cm below its centre"), RestBottom, -30.0f, 0.001f);

    Scene.Emergence->Begin(Scene.Mesh);
    TestTrue(TEXT("Emerging"), Scene.Emergence->IsEmerging());
    TestTrue(TEXT("Shown flat on the very first frame"), Scene.Mesh->GetRelativeScale3D().Z < 0.1);
    TestTrue(TEXT("Wider than normal at the start"), Scene.Mesh->GetRelativeScale3D().X > 0.6);
    TestEqual(TEXT("Bottom held at the start"), EmergenceTest::BottomRelativeZ(Scene.Mesh), RestBottom, 0.001f);

    double PreviousHeight = Scene.Mesh->GetRelativeScale3D().Z;
    for (int32 Step = 0; Step < 9; ++Step)
    {
        Scene.Emergence->AdvanceEmergence(0.1f);
        TestEqual(TEXT("Bottom held mid-animation"), EmergenceTest::BottomRelativeZ(Scene.Mesh), RestBottom, 0.001f);
        TestTrue(TEXT("Still emerging before the duration"), Scene.Emergence->IsEmerging());
        TestTrue(TEXT("Getting taller"), Scene.Mesh->GetRelativeScale3D().Z > PreviousHeight);
        PreviousHeight = Scene.Mesh->GetRelativeScale3D().Z;
    }

    TestEqual(TEXT("Progress after 0.9 s of 1 s"), Scene.Emergence->GetProgress(), 0.9f, 0.001f);

    Scene.Emergence->AdvanceEmergence(0.2f);

    TestFalse(TEXT("Finished"), Scene.Emergence->IsEmerging());
    TestEqual(TEXT("Finished event fired once"), Scene.Listener->EmergenceFinishedCount, 1);
    TestTrue(TEXT("Back to the normal scale"), Scene.Mesh->GetRelativeScale3D().Equals(FVector(0.6), 0.0001));
    TestTrue(TEXT("Back to the normal location"), Scene.Mesh->GetRelativeLocation().Equals(FVector::ZeroVector, 0.0001));

    Scene.Emergence->AdvanceEmergence(1.0f);
    TestEqual(TEXT("No second finished event"), Scene.Listener->EmergenceFinishedCount, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEmergenceEdgeCasesTest,
    "GrantMoney.Run.Emergence.EdgeCases",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FEmergenceEdgeCasesTest::RunTest(const FString& Parameters)
{
    EmergenceTest::FScene Scene;
    if (!Scene.Create(*this))
    {
        return false;
    }

    int32& FinishedCount = Scene.Listener->EmergenceFinishedCount;

    // Zero duration: finished immediately, nothing is flattened.
    Scene.Emergence->Duration = 0.0f;
    Scene.Emergence->Begin(Scene.Mesh);
    TestFalse(TEXT("Zero duration is not emerging"), Scene.Emergence->IsEmerging());
    TestEqual(TEXT("Zero duration finishes at once"), FinishedCount, 1);
    TestTrue(TEXT("Visual untouched"), Scene.Mesh->GetRelativeScale3D().Equals(FVector(0.6), 0.0001));

    // No visual: finishes at once instead of waiting forever.
    Scene.Emergence->Duration = 1.0f;
    Scene.Emergence->Begin(nullptr);
    TestFalse(TEXT("No visual is not emerging"), Scene.Emergence->IsEmerging());
    TestEqual(TEXT("No visual finishes at once"), FinishedCount, 2);

    // Cancel restores the visual and fires nothing.
    Scene.Emergence->Begin(Scene.Mesh);
    Scene.Emergence->AdvanceEmergence(0.2f);
    TestTrue(TEXT("Mid-animation the visual is flat"), Scene.Mesh->GetRelativeScale3D().Z < 0.5);
    Scene.Emergence->Cancel();
    TestFalse(TEXT("Cancelled"), Scene.Emergence->IsEmerging());
    TestEqual(TEXT("Cancel fires no event"), FinishedCount, 2);
    TestTrue(TEXT("Cancel restores the normal scale"), Scene.Mesh->GetRelativeScale3D().Equals(FVector(0.6), 0.0001));
    TestTrue(TEXT("Cancel restores the normal location"), Scene.Mesh->GetRelativeLocation().Equals(FVector::ZeroVector, 0.0001));

    // Begin while already emerging does not restart or re-capture the (now flat) pose.
    Scene.Emergence->Begin(Scene.Mesh);
    Scene.Emergence->AdvanceEmergence(0.5f);
    const FVector Midway = Scene.Mesh->GetRelativeScale3D();
    Scene.Emergence->Begin(Scene.Mesh);
    TestTrue(TEXT("A second Begin changes nothing"), Scene.Mesh->GetRelativeScale3D().Equals(Midway, 0.0001));
    Scene.Emergence->AdvanceEmergence(1.0f);
    TestTrue(TEXT("It still ends at the true normal scale"), Scene.Mesh->GetRelativeScale3D().Equals(FVector(0.6), 0.0001));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
