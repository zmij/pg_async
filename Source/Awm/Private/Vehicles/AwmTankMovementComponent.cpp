// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#include "Awm.h"

#include "PhysicsPublic.h"
#include "PhysXPublic.h"


UAwmTankMovementComponent::UAwmTankMovementComponent(const FObjectInitializer &ObjectInitializer)
        : Super(ObjectInitializer)
{

    ensurePhysXFoundationSetup();
            
    StopThreshold = 10.0f;
    IdleBrakeInput = 1.0f;

    LeftThrustRate.FallRate = 5.f;
    LeftThrustRate.RiseRate = 2.5f;
    
    LeftBrakeRate.RiseRate = 6.f;
    LeftBrakeRate.FallRate = 10.f;
    
    RightThrustRate.FallRate = 5.f;
    RightThrustRate.RiseRate = 2.5f;
    
    RightBrakeRate.RiseRate = 6.f;
    RightBrakeRate.FallRate = 10.f;

    SteeringBrakeRate = 0.3f;
            
    ControlModel = TankControlModel::SPECIAL;

    PxVehicleEngineData DefEngineData;
    EngineSetup.MOI = DefEngineData.mMOI;
    EngineSetup.MaxRPM = OmegaToRPM(DefEngineData.mMaxOmega);
    EngineSetup.DampingRateFullThrottle = DefEngineData.mDampingRateFullThrottle;
    EngineSetup.DampingRateZeroThrottleClutchEngaged = DefEngineData.mDampingRateZeroThrottleClutchEngaged;
    EngineSetup.DampingRateZeroThrottleClutchDisengaged = DefEngineData.mDampingRateZeroThrottleClutchDisengaged;

    // Convert from PhysX curve to ours
    FRichCurve* TorqueCurveData = EngineSetup.TorqueCurve.GetRichCurve();
    for (PxU32 KeyIdx = 0; KeyIdx < DefEngineData.mTorqueCurve.getNbDataPairs(); KeyIdx++)
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

    for (uint32 i = PxVehicleGearsData::eFIRST; i < DefGearSetup.mNbRatios; i++) {
        FTankGearData GearData;
        GearData.DownRatio = DefAutoBoxSetup.mDownRatios[i];
        GearData.UpRatio = DefAutoBoxSetup.mUpRatios[i];
        GearData.Ratio = DefGearSetup.mRatios[i];
        TransmissionSetup.ForwardGears.Add(GearData);
    }

    WheelSetups.SetNum(4);
}


float FTankEngineData::FindPeakTorque() const {
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

static void GetTankVehicleEngineSetup(const FTankEngineData &Setup, PxVehicleEngineData &PxSetup) {
    PxSetup.mMOI = M2ToCm2(Setup.MOI);
    PxSetup.mMaxOmega = RPMToOmega(Setup.MaxRPM);
    PxSetup.mDampingRateFullThrottle = M2ToCm2(Setup.DampingRateFullThrottle);
    PxSetup.mDampingRateZeroThrottleClutchEngaged = M2ToCm2(Setup.DampingRateZeroThrottleClutchEngaged);
    PxSetup.mDampingRateZeroThrottleClutchDisengaged = M2ToCm2(Setup.DampingRateZeroThrottleClutchDisengaged);

    float PeakTorque = Setup.FindPeakTorque(); // In Nm
    PxSetup.mPeakTorque = M2ToCm2(PeakTorque);    // convert Nm to (kg cm^2/s^2)

    // Convert from our curve to PhysX
    PxSetup.mTorqueCurve.clear();
    TArray<FRichCurveKey> TorqueKeys = Setup.TorqueCurve.GetRichCurveConst()->GetCopyOfKeys();
    int32 NumTorqueCurveKeys = FMath::Min<int32>(TorqueKeys.Num(), PxVehicleEngineData::eMAX_NB_ENGINE_TORQUE_CURVE_ENTRIES);
    for (int32 KeyIdx = 0; KeyIdx < NumTorqueCurveKeys; KeyIdx++)
    {
        FRichCurveKey& Key = TorqueKeys[KeyIdx];
        PxSetup.mTorqueCurve.addPair( FMath::Clamp(Key.Time / Setup.MaxRPM, 0.f, 1.f), Key.Value / PeakTorque ); // Normalize torque to 0-1 range
    }
}

static void GetTankVehicleGearSetup(const FTankTransmissionData &Setup, PxVehicleGearsData &PxSetup) {
    PxSetup.mSwitchTime = Setup.GearSwitchTime;
    PxSetup.mRatios[PxVehicleGearsData::eREVERSE] = Setup.ReverseGearRatio;
    for (int32 i = 0; i < Setup.ForwardGears.Num(); i++) {
        PxSetup.mRatios[i + PxVehicleGearsData::eFIRST] = Setup.ForwardGears[i].Ratio;
    }
    PxSetup.mFinalRatio = Setup.FinalRatio;
    PxSetup.mNbRatios = Setup.ForwardGears.Num() + PxVehicleGearsData::eFIRST;
}

static void GetTankVehicleAutoBoxSetup(const FTankTransmissionData &Setup, PxVehicleAutoBoxData &PxSetup) {
    for (int32 i = 0; i < Setup.ForwardGears.Num(); i++) {
        const FTankGearData &GearData = Setup.ForwardGears[i];
        PxSetup.mUpRatios[i] = GearData.UpRatio;
        PxSetup.mDownRatios[i] = GearData.DownRatio;
    }
    PxSetup.mUpRatios[PxVehicleGearsData::eNEUTRAL] = Setup.NeutralGearUpRatio;
    PxSetup.setLatency(Setup.GearAutoBoxLatency);
}

void SetupTankDriveHelper(const UAwmTankMovementComponent *VehicleData,
                          PxVehicleDriveSimData &DriveData) {


    PxVehicleEngineData EngineSetup;
    GetTankVehicleEngineSetup(VehicleData->EngineSetup, EngineSetup);
    DriveData.setEngineData(EngineSetup);

    PxVehicleClutchData ClutchSetup;
    ClutchSetup.mStrength = M2ToCm2(VehicleData->TransmissionSetup.ClutchStrength);
    DriveData.setClutchData(ClutchSetup);

    PxVehicleGearsData GearSetup;
    GetTankVehicleGearSetup(VehicleData->TransmissionSetup, GearSetup);
    DriveData.setGearsData(GearSetup);

    PxVehicleAutoBoxData AutoBoxSetup;
    GetTankVehicleAutoBoxSetup(VehicleData->TransmissionSetup, AutoBoxSetup);
    DriveData.setAutoBoxData(AutoBoxSetup);

}

void UAwmTankMovementComponent::SetupVehicle() {
    if (!UpdatedPrimitive) {
        return;
    }

    if (WheelSetups.Num() % 2 != 0) {
        PVehicle = NULL;
        PVehicleDrive = NULL;
        return;
    }

    for (int32 WheelIdx = 0; WheelIdx < WheelSetups.Num(); ++WheelIdx) {
        const FWheelSetup &WheelSetup = WheelSetups[WheelIdx];
        if (WheelSetup.BoneName == NAME_None) {
            return;
        }
    }

    // Setup the chassis and wheel shapes
    SetupVehicleShapes();

    // Setup mass properties
    SetupVehicleMass();

    // Setup the wheels
    PxVehicleWheelsSimData *PWheelsSimData = PxVehicleWheelsSimData::allocate(WheelSetups.Num());
    SetupWheels(PWheelsSimData);

    // Setup drive data
    PxVehicleDriveSimData DriveData;
    SetupTankDriveHelper(this, DriveData);

    // Create the vehicle
    PxVehicleDriveTank *PVehicleDriveTank = PxVehicleDriveTank::allocate(WheelSetups.Num());
    check(PVehicleDriveTank);
    
    PVehicleDriveTank->setDriveModel( (PxVehicleDriveTankControlModel::Enum) ControlModel );

    PVehicleDriveTank->setup(GPhysXSDK, UpdatedPrimitive->GetBodyInstance()->GetPxRigidDynamic_AssumesLocked(),
                             *PWheelsSimData,
                             DriveData, WheelSetups.Num());

    PVehicleDriveTank->setToRestState();
    PVehicleDriveTank->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);

    // cleanup
    PWheelsSimData->free();

    // cache values
    PVehicle = PVehicleDriveTank;
    PVehicleDrive = PVehicleDriveTank;

    SetUseAutoGears(TransmissionSetup.bUseGearAutoBox);
    
}


void UAwmTankMovementComponent::PreTick(float DeltaTime) {
    UWheeledVehicleMovementComponent::PreTick(DeltaTime);
}

float UAwmTankMovementComponent::CalcThrottleInput() {
    if ( FMath::Abs(RawThrottleInput) < SMALL_NUMBER && FMath::Abs(RawSteeringInput) > SMALL_NUMBER ) {
        if ( FMath::Abs(GetForwardSpeed()) < StopThreshold ) {
            SetTargetGear(1, true);
        }
    }
    return UWheeledVehicleMovementComponent::CalcThrottleInput();;
}

void UAwmTankMovementComponent::UpdateSimulation(float DeltaTime) {

    if (PVehicleDrive == NULL)
        return;

    PxVehicleDriveTankRawInputData VehicleInputData((PxVehicleDriveTankControlModel::Enum) ControlModel);
    
    float AbsForwardSpeed = FMath::Abs(GetForwardSpeed());
    float AbsSteering = FMath::Abs(SteeringInput);
    float AbsThrottle = FMath::Abs(ThrottleInput);

    float Throttle = FMath::Max(AbsThrottle, AbsSteering);
    
    float LBrake = BrakeInput;
    float RBrake = BrakeInput;
    
    float RThrust = 1.0f;
    float LThrust = 1.0f;

    float scaledSteering = SteeringInput * 100;
    float scaledThrottle = ThrottleInput * 100;
    float invSteering = -scaledSteering;
    float v = (100 - FMath::Abs(invSteering)) * (scaledThrottle / 100) + scaledThrottle;
    float w = (100 - FMath::Abs(scaledThrottle))* (invSteering / 100) + invSteering;

    LThrust = (v - w) / 200.f;
    RThrust = (v + w) / 200.f;

    if ( AbsSteering > AbsThrottle) {
        if ( AbsForwardSpeed < WrongDirectionThreshold ) {
            if ( GetTargetGear() > 1 ) {
                SetTargetGear(1, true);
            }
            LBrake = 0.0f;
            RBrake = 0.0f;
        } else {
            if ( SteeringInput > 0 ) {  
                LBrake = 0.0f;
                RBrake = SteeringBrakeRate;
            } else {
                LBrake = SteeringBrakeRate;
                RBrake = 0.0f;
            }
        }
    }

    RBrake = FMath::Clamp(RBrake, 0.0f, 1.0f);
    LBrake = FMath::Clamp(LBrake, 0.0f, 1.0f);
    
    if ( ControlModel == TankControlModel::STANDARD )
    {
        RThrust = FMath::Clamp(RThrust, 0.0f, 1.0f);
        LThrust = FMath::Clamp(LThrust, 0.0f, 1.0f);
    }
    else 
    {
        RThrust = FMath::Clamp(RThrust, -1.0f, 1.0f);
        LThrust = FMath::Clamp(LThrust, -1.0f, 1.0f);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("LT: %f  RT: %f, TH: %f, LB: %f, RB: %f, TG: %d, S: %f"), LThrust, RThrust, Throttle, LBrake, RBrake, GetTargetGear(), GetForwardSpeed())
    
    VehicleInputData.setAnalogAccel(Throttle);
    VehicleInputData.setAnalogLeftThrust(LThrust);
    VehicleInputData.setAnalogRightThrust(RThrust);
    VehicleInputData.setAnalogLeftBrake(LBrake);
    VehicleInputData.setAnalogRightBrake(RBrake);

    if (!PVehicleDrive->mDriveDynData.getUseAutoGears()) {
        VehicleInputData.setGearUp(bRawGearUpInput);
        VehicleInputData.setGearDown(bRawGearDownInput);
    }

    PxVehiclePadSmoothingData SmoothData = {
            {ThrottleInputRate.RiseRate, LeftBrakeRate.RiseRate, RightBrakeRate.RiseRate, LeftThrustRate.RiseRate, RightThrustRate.RiseRate},
            {ThrottleInputRate.FallRate, LeftBrakeRate.FallRate, RightBrakeRate.FallRate, LeftThrustRate.FallRate, RightThrustRate.FallRate}
    };

    PxVehicleDriveTank *PVehicleDriveTank = (PxVehicleDriveTank *) PVehicleDrive;
    PxVehicleDriveTankSmoothAnalogRawInputsAndSetAnalogInputs(SmoothData, VehicleInputData, DeltaTime, *PVehicleDriveTank);

}