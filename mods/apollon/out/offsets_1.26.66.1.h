// ============================================================================
//  Apollon offsets -- Minecraft Bedrock 1.26.66.1 (arm64-v8a)
//  libminecraftpe.so  size 342,231,392  md5 fa377b0e07e7558b16d3684984598cc2
//
//  HOW THESE WERE DERIVED (no Ghidra/radare2 needed -- the binary is
//  symbol-rich in .data.rel.ro):
//    1. .rodata holds 7,504 mangled C++ names of entt ECS systems.
//    2. Each name is the addend of an R_AARCH64_RELATIVE relocation, so the
//       pointer slot referencing it can be located exactly.
//    3. Neighbouring relocation slots in the same metadata record are the
//       system's function pointers  ->  resolver yields 707 named systems.
//    4. Interior pointers are normalised to true entry points by requiring a
//       linear decode from the candidate entry to reach the interior address.
//    5. Constant hooks are confirmed by scanning the body for the
//       movz/movk+fmov materialisation of the hook's own constant.
//
//  CONFIDENCE
//    [A] named system + verified entry + constant match   (Gravity family)
//    [B] named system + verified entry                    (ret-patch hooks)
//    [C] resolved by string xref only, multiple candidates (see OFFSETS.md)
//    [X] class absent from every symbol source in the binary -- requires
//        runtime discovery; a plausible value MUST NOT be fabricated.
//
//  IMPORTANT
//    These are ENTRY POINTS of the named systems, not the same instruction
//    the 1.21.x table pointed at. Where a hook needs to return a value
//    (Gravity/JumpHack/Fov/BlockReach) patching the 4 bytes at the entry is
//    NOT equivalent to patching the deep interior site the 1.21.x table used.
//    Treat the [A]/[B] rows as hook sites; treat the [X] rows as unfinished.
// ============================================================================
#pragma once
#if defined(__aarch64__)

namespace Offsets_1_26_66_1 {
enum : unsigned long long {

    // ---- [A] MobMovementGravity family -------------------------------------
    // Single shared body. Contains BOTH -0.01 (water) and -0.08 (land), applied
    // via `fcsel s0, -0.01, -0.08, ne` after a travel-flag predicate -- exactly
    // the construct Apollon's Gravity/WaterDrown/LavaDrown hooks patch.
    Gravity             = 0xCD2266C,  // [A]  verified, constants -0.08 & -0.01
    WaterDrown          = 0xCD2266C,  // [A]  same body (player water branch)
    LavaDrown           = 0xCD2266C,  // [A]  same body (lava branch)

    // ---- [B] Camera systems (ret-patch targets) ---------------------------
    NoHurtCam           = 0x8A62080,  // [B]  CameraBobSystem::tick
    NoCamDistortion     = 0x11E5C3D4, // [B]  CameraPortalDistortionSystem::_tick
    NoBoatRotation      = 0x8A74BE4,  // [B]  CameraVehicleRotationLookSystem::tick
    PlaceCamera         = 0x11EBB97C, // [B]  CameraBlendSystem::_tick
    XrayCameraThird     = 0x11EB5E4C, // [B]  CameraAvoidanceSystem::tick

    // ---- [B] Movement systems --------------------------------------------
    SlowDownTriggers    = 0xC258480,  // [B]  BlockMovementSlowdownMultiplierSystemImpl::applySlowdownOnMove
    NoSlowDown          = 0xC12522C,  // [B]  ItemUseSlowdownSystemImpl::doItemUseSlowdownSystem
    WaterSpeed          = 0x88DB454,  // [B]  doWaterMoveSystem
    LavaSpeed           = 0xC133388,  // [B]  doLavaMoveSystem
    Noclip              = 0xC10FD44,  // [B]  MoveCollisionSystem
    SpeedHack           = 0xC1CA408,  // [B]  DefaultMoveSystems::doDefaultMoveSystems

    // ---- [X] NOT RECOVERABLE statically ----------------------------------
    // These classes appear in NO symbol source: not in .dynsym (only
    // cohtml/v8/webrtc/Xbox + JNI are exported), not as entt names, not as
    // RTTI strings. Reaching any of them needs a vtable slot index, which is
    // only obtainable at runtime. Any fixed address here would be a guess.
    JumpHack            = 0,  // [X] ApplyJumpModifierSystem exists by name, but
                              //     the 0.42 constant is not in any of its slots
    Font                = 0,  // [X] BaseOptions/GuiData field
    Fov                 = 0,  // [X] CameraAPI return
    BlockReach          = 0,  // [X] GameMode/HitResult
    PlayerAuthInput     = 0,  // [X] packet handler
    NativeKeyHandler    = 0,  // [X] ClientInputCallbacks
    InMainMenu          = 0,  // [X] screen class
    InLanGame1          = 0,  // [X] NetherNet
    InLanGame2          = 0,  // [X] NetherNet
    FastDrop            = 0,  // [X] GuiData
    UnlockMarket        = 0,  // [X] Entitlement
    GetMaxPlayers       = 0,  // [X] AppPlatform
    FogState            = 0,  // [X] LevelRendererCamera
    SetupFog            = 0,  // [X] LevelRendererPlayer
    FullBright          = 0,  // [X] BaseOptions::getGamma
    ForEachActivePlayer = 0,  // [X] GameplayUserManager
    GetRuntimeActor     = 0,  // [X] Level
    IsAttackable        = 0,  // [X] Actor
    Attack              = 0,  // [X] GameMode::_attack
    LerpMotion          = 0,  // [X] Actor
    NormalTick          = 0,  // [X] LocalPlayer
    Swing               = 0,  // [X] LocalPlayer
    SetSprinting        = 0,  // [X] LocalPlayer
    GetEntityTypeId     = 0,  // [X] Actor
    GetHealth           = 0,  // [X] Actor
    SetClimbing         = 0,  // [X] Actor
    SetYHeadRot         = 0,  // [X] Actor
    SetYBodyRot         = 0,  // [X] Mob
    SetInvisible        = 0,  // [X] Actor
    Intersects          = 0,  // [X] Actor
    GetYHeadRot         = 0,  // [X] Actor
    InitMaxAutoStep     = 0,  // [X] Actor
    GetAABBDim          = 0,  // [X] Actor
    IsEmoting           = 0,  // [X] Player
    NoBlur1             = 0,  // [X] FullScreenEffectRenderer
    NoBlur2             = 0,  // [X] FullScreenEffectRenderer
    ShowPaperdoll       = 0,  // [X] HudPlayerRenderer
    Subtitle            = 0,  // [X] TileMessage
    GetAbilities        = 0,  // [X] Player
    GetLayer            = 0,  // [X] LayeredAbilities
    SetAbility          = 0,  // [X] Abilities
    ClientInstanceOnTick= 0,  // [X] ClientInstance
    HandleDestroyOrAttack = 0,// [X] ClientInputCallbacks
    PlayUI              = 0,  // [X] ClientInstance
    SetNoJumpDelay      = 0,  // [X] MobJump
    SetOnGround         = 0,  // [X] ActorEntityAccess
    MobIsJumping        = 0,  // [X] ActorEntityAccess
    DisplayMessage      = 0,  // [X] LocalPlayer
    Block_getRenderLayer= 0,  // [X] Block
    RebuildAllChunkGeometry = 0, // [X] LevelRenderer
    VignetteRenderer    = 0,  // [X] HudVignetteRenderer
    NoNametags          = 0,  // [X] BaseActorRenderer
    ScreenName          = 0xBA308C8, // [C] string-xref, 2 candidates
    GetNameTag          = 0,  // [X] Actor
    GetDimensionBlockSource = 0, // [X] Actor
    GetBlock            = 0,  // [X] BlockSource
    IsAir               = 0,  // [X] Block
};
}

namespace Fields_1_26_66_1 {
// [X] All field offsets require live memory inspection of 1.26.66.1.
// The 1.21.x values (GetGamemode 0x9C0, GetLevel 0x1D0, GetPosition 0x208,
// GetCameraPos 0x65C, FovX 0xF28, FovY 0xF3C, ...) are NOT valid here.
enum : unsigned long long {
    GetGamemode = 0, GetLevel = 0, GetPosition = 0, ActorRotation = 0,
    GetName = 0, GetHurtTime = 0, GetCameraPos = 0, GetLevelRendererPlayer = 0,
    GetLevelRenderer = 0, BlockTypeClass = 0, GetPlatform = 0,
    GetGameplayUserManager = 0, ViewMatrix = 0, FovX = 0, FovY = 0,
};
}
#endif
