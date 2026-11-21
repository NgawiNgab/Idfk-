#if defined(__aarch64__)
namespace Offsets64 {
	enum Offsets64 {
		// ===== 1.26.66.1 BODY-LAYER table =====
		// Non-zero = real entt system method body, resolved by name from the
		// 1.26.66.1 registry (highest valid record slot = slot[6] = body) and
		// machine-validated: inside .text, decodes to a function prologue, and
		// contains real work before the first ret.
		// 0 = unrecovered and therefore inert.
		//
		// WaterDrown and Gravity are the SAME body in this build
		// (MobMovementGravity::tickPlayerWaterGravity); main.cpp lets the Gravity
		// inline hook own it and skips the WaterDrown hex patch.
		//
		Font = 0,
		NoHurtCam = 0,
		NoCamDistortion = 0,
		NoBoatRotation = 0X8A76564,// RECOVERED: CameraVehicleRotationLookSystem::tick
		NoCamSleep = 0,
		PlaceCamera = 0X11EBC7BC,// RECOVERED: CameraBlendSystem::_tick
		WaterTrigger1 = 0,
		WaterTrigger2 = 0,
		WaterTrigger3 = 0,
		SlowDownTriggers = 0XC259410,// RECOVERED: BlockMovementSlowdownMultiplierSystemImpl::applySlowdownOnMove
		NoSlowDown = 0XC12825C,// RECOVERED: details16CandidateAdapterIXadL_ZN25ItemUseSlowdownSystemImpl::tickItem
		WaterSpeed = 0XCDC2F68,// RECOVERED: details16CandidateAdapterIXadL_ZL17doWaterMoveSystemN4entt9type_listIJ
		LavaSpeed = 0XC1353EC,// RECOVERED: SingleViewSystemWithBlockSourceIXadL_ZL::doLavaMoveSystemRK19Strict
		Noclip = 0,
		XrayCameraThird = 0X11EB835C,// RECOVERED: CameraAvoidanceSystem::tickILb1
		LavaDrown = 0XCD4CCE0,// RECOVERED: SingleViewSystemWithBlockSourceIXadL_ZN::MobMovementGravity15tickLavaG
		WaterDrown = 0XCD4C40C,// RECOVERED: details16CandidateAdapterIXadL_ZN18MobMovementGravity::tickPlayerWater
		Fov = 0,
		SpeedHack = 0XC1CC8B0,// RECOVERED: SingleViewSystemWithBlockSourceIXadL_ZN::DefaultMoveSystems20doDefault
		JumpHack = 0,
		Gravity = 0XCD4C40C,// RECOVERED: details16CandidateAdapterIXadL_ZN18MobMovementGravity::tickPlayerWater
		AutoClickMine = 0,
		FastBridge = 0,
		BlockReach = 0,
		PlayerAuthInput = 0,
		NativeKeyHandler = 0,
		InMainMenu = 0,
		InLanGame1 = 0,
		InLanGame2 = 0,
		FastDrop = 0,
		UnlockMarket = 0,
		GetMaxPlayers = 0,
		FogState = 0,
		SetupFog = 0,
		FullBright = 0,
		ForEachActivePlayer = 0,
		GetRuntimeActor = 0,
		IsAttackable = 0,
		Attack = 0,
		LerpMotion = 0,
		NormalTick = 0,
		Swing = 0,
		SetSprinting = 0,
		GetEntityTypeId = 0,
		GetHealth = 0,
		SetClimbing = 0,
		SetYHeadRot = 0,
		SetYBodyRot = 0,
		SetInvisible = 0,
		Intersects = 0,
		GetYHeadRot = 0,
		InitMaxAutoStep = 0,
		GetAABBDim = 0,
		IsEmoting = 0,
		NoBlur1 = 0,
		NoBlur2 = 0,
		ShowPaperdoll = 0,
		Subtitle = 0,
		GetAbilities = 0,
		GetLayer = 0,
		SetAbility = 0,
		ClientInstanceOnTick = 0,
		HandleDestroyOrAttack = 0,
		PlayUI = 0,
		SetNoJumpDelay = 0,
		SetOnGround = 0,
		MobIsJumping = 0,
		DisplayMessage = 0,
		Block_getRenderLayer = 0,
		RebuildAllChunkGeometry = 0,
		VignetteRenderer = 0,
		NoNametags = 0,
		ScreenName = 0,
		GetNameTag = 0,
		GetDimensionBlockSource = 0,
		GetBlock = 0,
		IsAir = 0,
	};
}
#endif
