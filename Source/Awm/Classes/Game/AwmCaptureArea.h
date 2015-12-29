// Copyright 2015 Mail.Ru Group. All Rights Reserved.

#pragma once

#include "AwmCaptureArea.generated.h"

UENUM()
enum class CaptureAreaLostSuperiorityType : uint8
{
    //
    STANDART,
    // Clear all capture points if team lost superiority
    RESET,
    // If area have more one team, stop distributing capture points
    STOP
};

USTRUCT()
struct FCaptureAreaIncomeData
{
    GENERATED_USTRUCT_BODY();
    
    /** Time step */
    UPROPERTY(EditAnywhere, Category = "Setup")
    float Time;
    
    /** Capture points income */
    UPROPERTY(EditAnywhere, Category = "Setup")
    float Value;
    
    FCaptureAreaIncomeData();
    
    FCaptureAreaIncomeData(float InTime, float InValue);
    
};

UCLASS(BlueprintType)
class AAwmCaptureArea : public AActor
{
	GENERATED_UCLASS_BODY()
	
public:
    
    /** Called when the game starts or when spawned */
	virtual void BeginPlay() override;
    
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
    /** Called every CheckTime seconds */
    virtual void DefaultTimer();
    
    /** Called every BonusPointsIncome.Time, if bBonusPointsBroadcast == true */
    virtual void BonusTimer();
    
    DECLARE_EVENT_OneParam( AAwmCaptureArea, FBonusEvent, AAwmCaptureArea* )
    FBonusEvent BonusEvent;
    
    /** Enable bonus broadcast  */
    bool bBonusPointsBroadcast;
    
    /** Bonus income data */
    UPROPERTY(EditDefaultsOnly, Category = "Setup")
    FCaptureAreaIncomeData BonusPointsIncome;
    
    /** Area radius */
    UPROPERTY(EditDefaultsOnly, Category = "Setup")
    float Radius;
    
    /** Total points for the capture */
    UPROPERTY(EditDefaultsOnly, Category = "Setup")
    int32 TotalCapturePoints;
    
    /** Curve data points for the capture. It changes the time step and capture points for each occupant */
    UPROPERTY(EditDefaultsOnly, Category = "Setup")
    TArray<FCaptureAreaIncomeData> CapturePointsCurve;
    
    /** How many points percent remove, if occupant damaged */
    UPROPERTY(EditDefaultsOnly, Category = "Setup")
    float DamageCapturePercent;
    
    /** Behaivor if team lost superiority */
    UPROPERTY(EditDefaultsOnly, Category = "Setup")
    CaptureAreaLostSuperiorityType LostSuperiority;
    
    /** Get current capture points with estimate */
    UFUNCTION(BlueprintCallable, Category = "Awm|CaptureArea")
    float GetCurrentCapturePoints() const;
    
    /** Current capture progress in percent */
    UFUNCTION(BlueprintCallable, Category = "Awm|CaptureArea")
    float GetCaptureProgress() const;
    
    /** Has occupant */
    UFUNCTION(BlueprintCallable, Category = "Awm|CaptureArea")
    bool HasOccupant() const;
    
    /** Get occupant team num */
    UFUNCTION(BlueprintCallable, Category = "Awm|CaptureArea")
    int32 GetOccupantTeam() const;
    
    /** Has owner */
    UFUNCTION(BlueprintCallable, Category = "Awm|CaptureArea")
    bool HasOwner() const;
    
    /** Get owner team num */
    UFUNCTION(BlueprintCallable, Category = "Awm|CaptureArea")
    int32 GetOwnerTeam() const;
    
    /** Get radius */
    UFUNCTION(BlueprintCallable, Category = "Awm|CaptureArea")
    float GetRadius() const;

protected:
    
    /** Handle for efficient management of DefaultTimer timer */
    FTimerHandle TimerHandle_DefaultTimer;
    
    /** Handle for efficient management of BonusTimer timer */
    FTimerHandle TimerHandle_BonusTimer;
    
    /** Current points capture for each occupant <occupant controller, points for the capture> */
    UPROPERTY(Transient)
    TMap<AController*, float> CurrentCapturePointsMap;
    
    /** Penetrations map for DamageCapture <occupant controller, last penetrations num> */
    UPROPERTY(Transient)
    TMap<AController*, int32> Penetrations;
    
    /** Current occupant team */
    UPROPERTY(Transient, Replicated)
    int32 OccupantTeam;
    
    /** Current owner team */
    UPROPERTY(EditDefaultsOnly, Replicated)
    int32 OwnerTeam;
    
    /** Last calculate capture points */
    UPROPERTY(Transient, Replicated)
    float LastCalculateCapturePoints;
    
    /** Last calculate time */
    UPROPERTY(Transient, Replicated)
    float LastCalculateTime;
    
    /** Estimated time */
    UPROPERTY(Transient, Replicated)
    float EstimatedTime;
    
    /** Estimated points */
    UPROPERTY(Transient, Replicated)
    int32 EstimatedCapturePoints;
    
    /** Vehicle filter & calculate capture points */
    FORCEINLINE void Calculate(float CurrentTime, float DeltaTime);
    
    /** Get lost vehicle */
    FORCEINLINE TArray<AController*> GetLostControllers(TArray<AController*>& ControllersOfOccupants);
    
    /** Add points to occupants */
    FORCEINLINE void CalculateCapturePoints(TMap<int32,TArray<AController*>>& Teams, TArray<AController*>& Newbies, float DeltaTime);
    
    /** Remove excess points */
    FORCEINLINE void CalculateExcessPoints(TMap<int32,TArray<AController*>>& Teams);
    
    /** Remove points by LostSuperiority rule */
    FORCEINLINE void CalculateLostSuperiority(int32 MaxTeam);
    
    /** Remove "damaged" points */
    FORCEINLINE void CalculateDamageCapture(TMap<AController*,int32>& DeltaPenetrations);
    
    /** Clear lost vehicle points & all information */
    FORCEINLINE void ClearLostControllers(TArray<AController*>& Lost);
    
    /** Calculate owner */
    FORCEINLINE void CalculateOwner(TMap<int32,TArray<AController*>>& Teams);
    
    /** Calculate estimate */
    FORCEINLINE void CalculateEstimate(TMap<int32,TArray<AController*>>& Teams, float CurrentTime);
    
    /** Clear all capture points */
    FORCEINLINE void ClearCapturePoints();
    
    /** Stop bonus timer */
    FORCEINLINE void StopBonusTimer();
    
private:
    
    /** Calculate total points for each team */
    FORCEINLINE TMap<int32,float> GetTeamsPoints(TMap<int32,TArray<AController*>>& Teams);
    
    /** Get current time step & income points by num occupants */
    FORCEINLINE FCaptureAreaIncomeData& GetCapturePointsCurve(int32 Num);
    
    /** Last check time for delta time in DefaultTimer() */
    float LastCheckTick;
    
    /** Check time step */
    float CheckTime;
    
};
