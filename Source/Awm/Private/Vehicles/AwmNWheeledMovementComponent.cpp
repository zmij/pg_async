// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

#include "PhysicsPublic.h"
#include "PhysXPublic.h"
#include "Runtime/Engine/Private/Vehicles/PhysXVehicleManager.h"

UVehicleNoDrivedWheel::UVehicleNoDrivedWheel(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    
}


UAwmNWheeledMovementComponent::UAwmNWheeledMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    ensurePhysXFoundationSetup();
    
    // grab default values from physx
    PxVehicleEngineData DefEngineData;
    EngineSetup.MOI = DefEngineData.mMOI;
    EngineSetup.MaxRPM = OmegaToRPM(DefEngineData.mMaxOmega);
    EngineSetup.DampingRateFullThrottle = DefEngineData.mDampingRateFullThrottle;
    EngineSetup.DampingRateZeroThrottleClutchEngaged = DefEngineData.mDampingRateZeroThrottleClutchEngaged;
    EngineSetup.DampingRateZeroThrottleClutchDisengaged = DefEngineData.mDampingRateZeroThrottleClutchDisengaged;
    
    // Convert from PhysX curve to ours
    FRichCurve* TorqueCurveData = EngineSetup.TorqueCurve.GetRichCurve();
    for (PxU32 KeyIdx = 0; KeyIdx<DefEngineData.mTorqueCurve.getNbDataPairs(); KeyIdx++)
    {
        float Input = DefEngineData.mTorqueCurve.getX(KeyIdx) * EngineSetup.MaxRPM;
        float Output = DefEngineData.mTorqueCurve.getY(KeyIdx) * DefEngineData.mPeakTorque;
        TorqueCurveData->AddKey(Input, Output);
    }
    
    PxVehicleClutchData DefClutchData;
    TransmissionSetup.ClutchStrength = DefClutchData.mStrength;
    
    PxVehicleGearsData DefGearSetup;
    TransmissionSetup.GearSwitchTime = DefGearSetup.mSwitchTime;
    TransmissionSetup.ReverseGearRatio = DefGearSetup.mRatios[PxVehicleGearsData::eREVERSE];
    TransmissionSetup.FinalRatio = DefGearSetup.mFinalRatio;
    
    PxVehicleAutoBoxData DefAutoBoxSetup;
    TransmissionSetup.NeutralGearUpRatio = DefAutoBoxSetup.mUpRatios[PxVehicleGearsData::eNEUTRAL];
    TransmissionSetup.GearAutoBoxLatency = DefAutoBoxSetup.getLatency();
    TransmissionSetup.bUseGearAutoBox = true;
    
    for (uint32 i = PxVehicleGearsData::eFIRST; i < DefGearSetup.mNbRatios; i++)
    {
        FVehicleGearDataNW GearData;
        GearData.DownRatio = DefAutoBoxSetup.mDownRatios[i];
        GearData.UpRatio = DefAutoBoxSetup.mUpRatios[i];
        GearData.Ratio = DefGearSetup.mRatios[i];
        TransmissionSetup.ForwardGears.Add(GearData);
    }
    
    // Init steering speed curve
    FRichCurve* SteeringCurveData = SteeringCurve.GetRichCurve();
    SteeringCurveData->AddKey(0.f, 1.f);
    SteeringCurveData->AddKey(20.f, 0.9f);
    SteeringCurveData->AddKey(60.f, 0.8f);
    SteeringCurveData->AddKey(120.f, 0.7f);
    
    // Initialize WheelSetups array with 6 wheels
    WheelSetups.SetNum(6);
    
}

#if WITH_EDITOR
void UAwmNWheeledMovementComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
    
    if (PropertyName == TEXT("DownRatio"))
    {
        for (int32 GearIdx = 0; GearIdx < TransmissionSetup.ForwardGears.Num(); ++GearIdx)
        {
            FVehicleGearDataNW & GearData = TransmissionSetup.ForwardGears[GearIdx];
            GearData.DownRatio = FMath::Min(GearData.DownRatio, GearData.UpRatio);
        }
    }
    else if (PropertyName == TEXT("UpRatio"))
    {
        for (int32 GearIdx = 0; GearIdx < TransmissionSetup.ForwardGears.Num(); ++GearIdx)
        {
            FVehicleGearDataNW & GearData = TransmissionSetup.ForwardGears[GearIdx];
            GearData.UpRatio = FMath::Max(GearData.DownRatio, GearData.UpRatio);
        }
    }
    else if (PropertyName == TEXT("SteeringCurve"))
    {
        //make sure values are capped between 0 and 1
        TArray<FRichCurveKey> SteerKeys = SteeringCurve.GetRichCurve()->GetCopyOfKeys();
        for (int32 KeyIdx = 0; KeyIdx < SteerKeys.Num(); ++KeyIdx)
        {
            float NewValue = FMath::Clamp(SteerKeys[KeyIdx].Value, 0.f, 1.f);
            SteeringCurve.GetRichCurve()->UpdateOrAddKey(SteerKeys[KeyIdx].Time, NewValue);
        }
    }
}
#endif

static void GetVehicleDifferentialNWSetup(const TArray<FWheelSetup>& WheelsSetup, PxVehicleDifferentialNWData& PxSetup)
{
    for (int32 WheelIdx = 0; WheelIdx < WheelsSetup.Num(); ++WheelIdx)
    {
        const FWheelSetup& WheelSetup = WheelsSetup[WheelIdx];
        PxSetup.setDrivenWheel(WheelIdx, !WheelSetup.WheelClass->IsChildOf<UVehicleNoDrivedWheel>());
    }
}

float FVehicleEngineDataNW::FindPeakTorque() const
{
    // Find max torque
    float PeakTorque = 0.f;
    TArray<FRichCurveKey> TorqueKeys = TorqueCurve.GetRichCurveConst()->GetCopyOfKeys();
    for (int32 KeyIdx = 0; KeyIdx < TorqueKeys.Num(); KeyIdx++)
    {
        FRichCurveKey& Key = TorqueKeys[KeyIdx];
        PeakTorque = FMath::Max(PeakTorque, Key.Value);
    }
    return PeakTorque;
}

static void GetVehicleEngineSetup(const FVehicleEngineDataNW& Setup, PxVehicleEngineData& PxSetup)
{
    PxSetup.mMOI = M2ToCm2(Setup.MOI);
    PxSetup.mMaxOmega = RPMToOmega(Setup.MaxRPM);
    PxSetup.mDampingRateFullThrottle = M2ToCm2(Setup.DampingRateFullThrottle);
    PxSetup.mDampingRateZeroThrottleClutchEngaged = M2ToCm2(Setup.DampingRateZeroThrottleClutchEngaged);
    PxSetup.mDampingRateZeroThrottleClutchDisengaged = M2ToCm2(Setup.DampingRateZeroThrottleClutchDisengaged);
    
    float PeakTorque = Setup.FindPeakTorque(); // In Nm
    PxSetup.mPeakTorque = M2ToCm2(PeakTorque);	// convert Nm to (kg cm^2/s^2)
    
    // Convert from our curve to PhysX
    PxSetup.mTorqueCurve.clear();
    TArray<FRichCurveKey> TorqueKeys = Setup.TorqueCurve.GetRichCurveConst()->GetCopyOfKeys();
    int32 NumTorqueCurveKeys = FMath::Min<int32>(TorqueKeys.Num(), PxVehicleEngineData::eMAX_NB_ENGINE_TORQUE_CURVE_ENTRIES);
    for (int32 KeyIdx = 0; KeyIdx < NumTorqueCurveKeys; KeyIdx++)
    {
        FRichCurveKey& Key = TorqueKeys[KeyIdx];
        PxSetup.mTorqueCurve.addPair(FMath::Clamp(Key.Time / Setup.MaxRPM, 0.f, 1.f), Key.Value / PeakTorque); // Normalize torque to 0-1 range
    }
}

static void GetVehicleGearSetup(const FVehicleTransmissionDataNW& Setup, PxVehicleGearsData& PxSetup)
{
    PxSetup.mSwitchTime = Setup.GearSwitchTime;
    PxSetup.mRatios[PxVehicleGearsData::eREVERSE] = Setup.ReverseGearRatio;
    for (int32 i = 0; i < Setup.ForwardGears.Num(); i++)
    {
        PxSetup.mRatios[i + PxVehicleGearsData::eFIRST] = Setup.ForwardGears[i].Ratio;
    }
    PxSetup.mFinalRatio = Setup.FinalRatio;
    PxSetup.mNbRatios = Setup.ForwardGears.Num() + PxVehicleGearsData::eFIRST;
}

static void GetVehicleAutoBoxSetup(const FVehicleTransmissionDataNW& Setup, PxVehicleAutoBoxData& PxSetup)
{
    for (int32 i = 0; i < Setup.ForwardGears.Num(); i++)
    {
        const FVehicleGearDataNW& GearData = Setup.ForwardGears[i];
        PxSetup.mUpRatios[i] = GearData.UpRatio;
        PxSetup.mDownRatios[i] = GearData.DownRatio;
    }
    PxSetup.mUpRatios[PxVehicleGearsData::eNEUTRAL] = Setup.NeutralGearUpRatio;
    PxSetup.setLatency(Setup.GearAutoBoxLatency);
}

void SetupDriveHelper(const UAwmNWheeledMovementComponent* VehicleData, const PxVehicleWheelsSimData* PWheelsSimData, PxVehicleDriveSimDataNW& DriveData)
{
    PxVehicleDifferentialNWData DifferentialSetup;
    GetVehicleDifferentialNWSetup(VehicleData->WheelSetups, DifferentialSetup);
    
    DriveData.setDiffData(DifferentialSetup);
    
    PxVehicleEngineData EngineSetup;
    GetVehicleEngineSetup(VehicleData->EngineSetup, EngineSetup);
    DriveData.setEngineData(EngineSetup);
    
    PxVehicleClutchData ClutchSetup;
    ClutchSetup.mStrength = M2ToCm2(VehicleData->TransmissionSetup.ClutchStrength);
    DriveData.setClutchData(ClutchSetup);
    
    PxVehicleGearsData GearSetup;
    GetVehicleGearSetup(VehicleData->TransmissionSetup, GearSetup);
    DriveData.setGearsData(GearSetup);
    
    PxVehicleAutoBoxData AutoBoxSetup;
    GetVehicleAutoBoxSetup(VehicleData->TransmissionSetup, AutoBoxSetup);
    DriveData.setAutoBoxData(AutoBoxSetup);
}

void UAwmNWheeledMovementComponent::SetupVehicle()
{
    if (!UpdatedPrimitive)
    {
        return;
    }
    
    for (int32 WheelIdx = 0; WheelIdx < WheelSetups.Num(); ++WheelIdx)
    {
        const FWheelSetup& WheelSetup = WheelSetups[WheelIdx];
        if (WheelSetup.BoneName == NAME_None)
        {
            return;
        }
    }
    
    // Setup the chassis and wheel shapes
    SetupVehicleShapes();
    
    // Setup mass properties
    SetupVehicleMass();
    
    // Setup the wheels
    PxVehicleWheelsSimData* PWheelsSimData = PxVehicleWheelsSimData::allocate(WheelSetups.Num());
    SetupWheels(PWheelsSimData);
    
    // Setup drive data
    PxVehicleDriveSimDataNW DriveData;
    SetupDriveHelper(this, PWheelsSimData, DriveData);
    
    // Create the vehicle
    PxVehicleDriveNW* PVehicleDriveNW = PxVehicleDriveNW::allocate(WheelSetups.Num());
    check(PVehicleDriveNW);
    
    PVehicleDriveNW->setup(GPhysXSDK, UpdatedPrimitive->GetBodyInstance()->GetPxRigidDynamic_AssumesLocked(), *PWheelsSimData, DriveData, WheelSetups.Num());
    
    PVehicleDriveNW->setToRestState();
    
    PVehicleDriveNW->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    
    PWheelsSimData->free();
    
    // cache values
    PVehicle = PVehicleDriveNW;
    PVehicleDrive = PVehicleDriveNW;
    
    SetUseAutoGears(TransmissionSetup.bUseGearAutoBox);
    
}

/** FPhysXVehicleManager can't free PxVehicleTypes::eDRIVENW type, this is hack */
void UAwmNWheeledMovementComponent::DestroyPhysicsState()
{
    //skip super invoke
    //Super::DestroyPhysicsState();
    
    if ( PVehicle )
    {
        DestroyWheels();
        
        // hack
        try
        {
            //World->GetPhysicsScene()->GetVehicleManager()->RemoveVehicle( this );
        }
        catch(...)
        {
            ((PxVehicleDriveNW*)PVehicle)->free();
        }
        
        PVehicle = NULL;
        
        if ( UpdatedComponent )
        {
            UpdatedComponent->RecreatePhysicsState();
        }
    }
}

void UAwmNWheeledMovementComponent::UpdateSimulation(float DeltaTime)
{
    if (PVehicleDrive == NULL)
        return;
    
    PxVehicleDriveNWRawInputData VehicleInputData;
    VehicleInputData.setAnalogAccel(ThrottleInput);
    VehicleInputData.setAnalogSteer(SteeringInput);
    VehicleInputData.setAnalogBrake(BrakeInput);
    VehicleInputData.setAnalogHandbrake(HandbrakeInput);
    
    if (!PVehicleDrive->mDriveDynData.getUseAutoGears())
    {
        VehicleInputData.setGearUp(bRawGearUpInput);
        VehicleInputData.setGearDown(bRawGearDownInput);
    }
    
    // TODO: move to SetupVehicle
    PxFixedSizeLookupTable<8> SpeedSteerLookup;
    TArray<FRichCurveKey> SteerKeys = SteeringCurve.GetRichCurve()->GetCopyOfKeys();
    const int32 MaxSteeringSamples = FMath::Min(8, SteerKeys.Num());
    for (int32 KeyIdx = 0; KeyIdx < MaxSteeringSamples; KeyIdx++)
    {
        FRichCurveKey& Key = SteerKeys[KeyIdx];
        SpeedSteerLookup.addPair(KmHToCmS(Key.Time), FMath::Clamp(Key.Value, 0.f, 1.f));
    }
    //end TODO
    
    PxVehiclePadSmoothingData SmoothData = {
        { ThrottleInputRate.RiseRate, BrakeInputRate.RiseRate, HandbrakeInputRate.RiseRate, SteeringInputRate.RiseRate, SteeringInputRate.RiseRate },
        { ThrottleInputRate.FallRate, BrakeInputRate.FallRate, HandbrakeInputRate.FallRate, SteeringInputRate.FallRate, SteeringInputRate.FallRate }
    };
    
    PxVehicleDriveNW* PVehicleDriveNW = (PxVehicleDriveNW*)PVehicleDrive;
    PxVehicleDriveNWSmoothAnalogRawInputsAndSetAnalogInputs(SmoothData, SpeedSteerLookup, VehicleInputData, DeltaTime, false, *PVehicleDriveNW);
    
}


