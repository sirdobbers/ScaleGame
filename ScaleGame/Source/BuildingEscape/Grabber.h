// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Grabber.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGrabberEvent);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BUILDINGESCAPE_API UGrabber : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGrabber();

	void Grab();
	void Release();
	void ScrollUp();
	void ScrollDown();

	UPROPERTY(BlueprintAssignable) FGrabberEvent Event1;
	UPROPERTY(BlueprintAssignable) FGrabberEvent Event2;

	UFUNCTION(BlueprintCallable, BlueprintPure) UStaticMeshComponent*  GetGrabbedStaticMesh();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "My Variables") AActor* GrabbedObject = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "My Variables") bool IsGrabbing = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "My Variables") FRotator InitialRotation;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


private:
	FVector PlayerLocation;
	FRotator PlayerRotation;

	UInputComponent* InputComponent = nullptr;

	void FindComponents();

	bool GetBodyRayCast(FHitResult& HitResult);  //searches for physics bodys
	bool GetWorldRayCast(FHitResult& HitResult); //searches for world

	FVector GetGrabLocation();
	void GetPlayerTranslations();

	void TeleportAndScale(AActor* Object, FVector OriginLocation, FVector TeleportLocation,float& ScaleRatio);
	void SetFinalDestination();

	float GrabDistance = 0;
	float TeleGrabDistance = 20.f;
	float CurrMass;

	FVector CurrGrabOffset;

	float ManualCount = 0;
};
