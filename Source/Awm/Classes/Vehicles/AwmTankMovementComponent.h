// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "Vehicles/WheeledVehicleMovementComponent.h"
#include "AwmTankMovementComponent.generated.h"

UENUM(BlueprintType)
enum class TankControlModel : uint8
{
    STANDARD = 0, // Left/Right thrust range [0,1]
    SPECIAL // Left/Right thrust range [-1,1]
};

USTRUCT()
struct FTankEngineData
{
    GENERATED_USTRUCT_BODY()

    /** Torque (Nm) at a given RPM*/
    UPROPERTY(EditAnywhere, Category = Setup)
    FRuntimeFloatCurve TorqueCurve;

    /** Maximum revolutions per minute of the engine */
    UPROPERTY(EditAnywhere, Category = Setup, meta = (ClampMin = "0.01", UIMin = "0.01"))
    float MaxRPM;

    /** Moment of inertia of the engine around the axis of rotation (Kgm^2). */
    UPROPERTY(EditAnywhere, Category = Setup, meta = (ClampMin = "0.01", UIMin = "0.01"))
    float MOI;

    /** Damping rate of engine when full throttle is applied (Kgm^2/s) */
    UPROPERTY(EditAnywhere, Category = Setup, AdvancedDisplay, meta = (ClampMin = "0.0", UIMin = "0.0"))
    float DampingRateFullThrottle;

    /** Damping rate of engine in at zero throttle when the clutch is engaged (Kgm^2/s)*/
    UPROPERTY(EditAnywhere, Category = Setup, AdvancedDisplay, meta = (ClampMin = "0.0", UIMin = "0.0"))
    float DampingRateZeroThrottleClutchEngaged;

    /** Damping rate of engine in at zero throttle when the clutch is disengaged (in neutral gear) (Kgm^2/s)*/
    UPROPERTY(EditAnywhere, Category = Setup, AdvancedDisplay, meta = (ClampMin = "0.0", UIMin = "0.0"))
    float DampingRateZeroThrottleClutchDisengaged;

    /** Find the peak torque produced by the TorqueCurve */
    float FindPeakTorque() const;
};


USTRUCT()
struct FTankGearData
{
    GENERATED_USTRUCT_BODY()

    /** Determines the amount of torque multiplication*/
    UPROPERTY(EditAnywhere, Category = Setup)
    float Ratio;

    /** Value of engineRevs/maxEngineRevs that is low enough to gear down*/
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"), Category = Setup)
    float DownRatio;

    /** Value of engineRevs/maxEngineRevs that is high enough to gear up*/
    UPROPERTY(EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"), Category = Setup)
    float UpRatio;
};

USTRUCT()
struct FTankTransmissionData
{
    GENERATED_USTRUCT_BODY()
    /** Whether to use automatic transmission */
    UPROPERTY(EditAnywhere, Category = VehicleSetup, meta=(DisplayName = "Automatic Transmission"))
    bool bUseGearAutoBox;

    /** Time it takes to switch gears (seconds) */
    UPROPERTY(EditAnywhere, Category = Setup, meta = (ClampMin = "0.0", UIMin = "0.0"))
    float GearSwitchTime;

    /** Minimum time it takes the automatic transmission to initiate a gear change (seconds)*/
    UPROPERTY(EditAnywhere, Category = Setup, meta = (editcondition = "bUseGearAutoBox", ClampMin = "0.0", UIMin="0.0"))
    float GearAutoBoxLatency;

    /** The final gear ratio multiplies the transmission gear ratios.*/
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Setup)
    float FinalRatio;

    /** Forward gear ratios (up to 30) */
    UPROPERTY(EditAnywhere, Category = Setup, AdvancedDisplay)
    TArray<FTankGearData> ForwardGears;

    /** Reverse gear ratio */
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Setup)
    float ReverseGearRatio;

    /** Value of engineRevs/maxEngineRevs that is high enough to increment gear*/
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Setup, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"))
    float NeutralGearUpRatio;

    /** Strength of clutch (Kgm^2/s)*/
    UPROPERTY(EditAnywhere, Category = Setup, AdvancedDisplay, meta = (ClampMin = "0.0", UIMin = "0.0"))
    float ClutchStrength;
};

UCLASS()
class ENGINE_API UAwmTankMovementComponent : public UWheeledVehicleMovementComponent {
    GENERATED_UCLASS_BODY()

public:
    
    UPROPERTY(EditAnywhere, Category = "Tank Input")
    TankControlModel ControlModel;
    
    /** Engine */
    UPROPERTY(EditAnywhere, Category = "Tank Input")
    FTankEngineData EngineSetup;

    /** Transmission data */
    UPROPERTY(EditAnywhere, Category = "Tank Input")
    FTankTransmissionData TransmissionSetup;

    UPROPERTY(EditAnywhere, Category = "Tank Input", AdvancedDisplay)
    FVehicleInputRate LeftThrustRate;
    
    UPROPERTY(EditAnywhere, Category = "Tank Input", AdvancedDisplay)
    FVehicleInputRate RightThrustRate;
    
    UPROPERTY(EditAnywhere, Category = "Tank Input", AdvancedDisplay)
    FVehicleInputRate RightBrakeRate;
    
    UPROPERTY(EditAnywhere, Category = "Tank Input", AdvancedDisplay)
    FVehicleInputRate LeftBrakeRate;

    UPROPERTY(EditAnywhere, Category = "Tank Input", AdvancedDisplay)
    float SteeringBrakeRate;
    
protected:
    virtual void SetupVehicle() override;
    virtual void UpdateSimulation(float DeltaTime) override;

public:
    virtual void PreTick(float DeltaTime) override;

protected:
    virtual float CalcThrottleInput() override;
    
};