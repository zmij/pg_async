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
    
    WheelSetups.SetNum(6);
    
}

float FVehicleEngineDataNW::FindPeakTorque() const
{
    float PeakTorque = 0.f;
    TArray<FRichCurveKey> TorqueKeys = TorqueCurve.GetRichCurveConst()->GetCopyOfKeys();
    for (int32 KeyIdx = 0; KeyIdx < TorqueKeys.Num(); KeyIdx++)
    {
        FRichCurveKey& Key = TorqueKeys[KeyIdx];
        PeakTorque = FMath::Max(PeakTorque, Key.Value);
    }
    return PeakTorque;
}

////////////////////////////////////////////////////////////
// Setup helpers

static void GetVehicleDifferentialNWSetup(const TArray<FWheelSetup>& WheelsSetup, PxVehicleDifferentialNWData& PxSetup)
{
    for (int32 WheelIdx = 0; WheelIdx < WheelsSetup.Num(); ++WheelIdx)
    {
        const FWheelSetup& WheelSetup = WheelsSetup[WheelIdx];
        PxSetup.setDrivenWheel(WheelIdx, !WheelSetup.WheelClass->IsChildOf<UVehicleNoDrivedWheel>());
    }
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
        PxSetup.mTorqueCurve.addPair(FMath::Clamp(Key.Time / Setup.MaxRPM, 0.f, 1.f), Key.Value / PeakTorque);
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

////////////////////////////////////////////////////////////

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
    
    SetupVehicleShapes();
    
    SetupVehicleMass();
    
    PxVehicleWheelsSimData* PWheelsSimData = PxVehicleWheelsSimData::allocate(WheelSetups.Num());
    SetupWheels(PWheelsSimData);
    
    PxVehicleDriveSimDataNW DriveData;
    SetupDriveHelper(this, PWheelsSimData, DriveData);
    
    PxVehicleDriveNW* PVehicleDriveNW = PxVehicleDriveNW::allocate(WheelSetups.Num());
    check(PVehicleDriveNW);
    
    PVehicleDriveNW->setup(GPhysXSDK, UpdatedPrimitive->GetBodyInstance()->GetPxRigidDynamic_AssumesLocked(), *PWheelsSimData, DriveData, WheelSetups.Num());
    
    PVehicleDriveNW->setToRestState();
    
    PVehicleDriveNW->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    
    PWheelsSimData->free();
    
    PVehicle = PVehicleDriveNW;
    PVehicleDrive = PVehicleDriveNW;
    
    SetUseAutoGears(TransmissionSetup.bUseGearAutoBox);
    
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
    
    PxFixedSizeLookupTable<8> SpeedSteerLookup;
    TArray<FRichCurveKey> SteerKeys = SteeringCurve.GetRichCurve()->GetCopyOfKeys();
    const int32 MaxSteeringSamples = FMath::Min(8, SteerKeys.Num());
    for (int32 KeyIdx = 0; KeyIdx < MaxSteeringSamples; KeyIdx++)
    {
        FRichCurveKey& Key = SteerKeys[KeyIdx];
        SpeedSteerLookup.addPair(KmHToCmS(Key.Time), FMath::Clamp(Key.Value, 0.f, 1.f));
    }
    
    PxVehiclePadSmoothingData SmoothData = {
        { ThrottleInputRate.RiseRate, BrakeInputRate.RiseRate, HandbrakeInputRate.RiseRate, SteeringInputRate.RiseRate, SteeringInputRate.RiseRate },
        { ThrottleInputRate.FallRate, BrakeInputRate.FallRate, HandbrakeInputRate.FallRate, SteeringInputRate.FallRate, SteeringInputRate.FallRate }
    };
    
    PxVehicleDriveNW* PVehicleDriveNW = (PxVehicleDriveNW*)PVehicleDrive;
    PxVehicleDriveNWSmoothAnalogRawInputsAndSetAnalogInputs(SmoothData, SpeedSteerLookup, VehicleInputData, DeltaTime, false, *PVehicleDriveNW);
    
}


