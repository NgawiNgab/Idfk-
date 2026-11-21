// ============================================================================
//  Apollon offsets -- Minecraft Bedrock 1.26.66.1 (arm64-v8a)  ** BODY LAYER **
//  libminecraftpe.so 342,231,392 B  md5 fa377b0e07e7558b16d3684984598cc2
//
//  CORRECTION: an earlier revision of this table claimed
//      Gravity = WaterDrown = LavaDrown = <the old pseudo-address>  '[A] verified,
//      constants -0.08 & -0.01'
//  That is FALSE for this binary and has been retracted:
//    * that address ends `and w0,w8,#1; ret` -> returns a BOOLEAN predicate,
//      it is not a gravity/float site.
//    * -0.08 and -0.01 have ZERO byte occurrences in the whole 1.26 image
//      (they DO exist in 1.21.x at 0x3172EC0 / 0x2646208). A hook that
//      patches constants that are not present cannot be correct.
//    * The 1.21 table's own Gravity site (0x9D88C20) is also a bool
//      predicate (has-component tests on ids 0x45/0x46/0x47).
//
//  LAYER: these are REAL METHOD BODIES (entt record slot[6]), not the generic
//  dispatch thunks the 1.21 table pointed at. 13 of the 77 shipped 1.21
//  offsets were the identical thunk body (`ldr x8,[x0]; ldr x8,[x8,#0x30];
//  blr x8` + canary), so thunk identity cannot identify a hook.
//
//  Every non-zero entry below is named by an entt registry adapter string
//  and was verified to disassemble as a real body.
// ============================================================================
#pragma once
#if defined(__aarch64__)
namespace Offsets_1_26_66_1 {
enum : unsigned long long {
    Gravity              = 0X0CD4C40C,  // details16CandidateAdapterIXadL_ZN18MobMovementGravity::tic
    WaterDrown           = 0X0CD4C40C,  // details16CandidateAdapterIXadL_ZN18MobMovementGravity::tic
    LavaDrown            = 0X0CD4CC98,  // SingleViewSystemWithBlockSourceIXadL_ZN::MobMovementGravit
    NoBoatRotation       = 0X08A76564,  // CameraVehicleRotationLookSystem::tick
    PlaceCamera          = 0X11EBC844,  // CameraBlendSystem::_tick
    XrayCameraThird      = 0X11EB835C,  // CameraAvoidanceSystem::tickILb1
    SlowDownTriggers     = 0X0C2593B0,  // BlockMovementSlowdownMultiplierSystemImpl::applySlowdownOn
    NoSlowDown           = 0X0C127A8C,  // details16CandidateAdapterIXadL_ZN25ItemUseSlowdownSystemIm
    WaterSpeed           = 0X0CDC2F68,  // details16CandidateAdapterIXadL_ZL17doWaterMoveSystemN4entt
    LavaSpeed            = 0X0C1353EC,  // SingleViewSystemWithBlockSourceIXadL_ZL::doLavaMoveSystemR
    SpeedHack            = 0X0C1CC8B0,  // SingleViewSystemWithBlockSourceIXadL_ZN::DefaultMoveSystem
    NoHurtCam            = 0,  // no registry-named body; needs runtime/vtable anchor
    NoCamDistortion      = 0,  // no registry-named body; needs runtime/vtable anchor
    Font                 = 0,  // no registry-named body; needs runtime/vtable anchor
    Fov                  = 0,  // no registry-named body; needs runtime/vtable anchor
    JumpHack             = 0,  // no registry-named body; needs runtime/vtable anchor
    BlockReach           = 0,  // no registry-named body; needs runtime/vtable anchor
};
}
#endif
