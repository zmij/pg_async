// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "Vehicles/WheeledVehicleMovementComponent.h"
#include "Vehicles/VehicleWheel.h"
#include "Curves/CurveFloat.h"
#include "AwmNWheeledMovementComponent.generated.h"

USTRUCT()
struct FVehicleEngineDataNW
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
struct FVehicleGearDataNW
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
struct FVehicleTransmissionDataNW
{
    GENERATED_USTRUCT_BODY()
    
    /** Whether to use automatic transmission */
    UPROPERTY(EditAnywhere, Category = VehicleSetup, meta = (DisplayName = "Automatic Transmission"))
    bool bUseGearAutoBox;
    
    /** Time it takes to switch gears (seconds) */
    UPROPERTY(EditAnywhere, Category = Setup, meta = (ClampMin = "0.0", UIMin = "0.0"))
    float GearSwitchTime;
    
    /** Minimum time it takes the automatic transmission to initiate a gear change (seconds)*/
    UPROPERTY(EditAnywhere, Category = Setup, meta = (editcondition = "bUseGearAutoBox", ClampMin = "0.0", UIMin = "0.0"))
    float GearAutoBoxLatency;
    
    /** The final gear ratio multiplies the transmission gear ratios.*/
    UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Setup)
    float FinalRatio;
    
    /** Forward gear ratios (up to 30) */
    UPROPERTY(EditAnywhere, Category = Setup, AdvancedDisplay)
    TArray<FVehicleGearDataNW> ForwardGears;
    
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

UCLASS(BlueprintType)
class UVehicleNoDrivedWheel : public UVehicleWheel
{
    GENERATED_UCLASS_BODY()
    
};

UCLASS()
class UAwmNWheeledMovementComponent : public UWheeledVehicleMovementComponent
{
    GENERATED_UCLASS_BODY()
    
    /** Engine */
    UPROPERTY(EditAnywhere, Category = MechanicalSetup)
    FVehicleEngineDataNW EngineSetup;
    
    /** Transmission data */
    UPROPERTY(EditAnywhere, Category = MechanicalSetup)
    FVehicleTransmissionDataNW TransmissionSetup;
    
    /** Maximum steering versus forward speed (km/h) */
    UPROPERTY(EditAnywhere, Category = SteeringSetup)
    FRuntimeFloatCurve SteeringCurve;
    
#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
    
    virtual void DestroyPhysicsState() override;
    
protected:
    virtual void SetupVehicle() override;
    virtual void UpdateSimulation(float DeltaTime) override;
    

};