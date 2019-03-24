#include "ScriptLoader.hpp"
#include "CActorParameters.hpp"
#include "CAnimationParameters.hpp"
#include "CDamageInfo.hpp"
#include "CFluidUVMotion.hpp"
#include "CGrappleParameters.hpp"
#include "CPatternedInfo.hpp"
#include "CRepulsor.hpp"
#include "CScriptActor.hpp"
#include "CScriptActorKeyframe.hpp"
#include "CScriptActorRotate.hpp"
#include "CScriptAiJumpPoint.hpp"
#include "CScriptAreaAttributes.hpp"
#include "MP1/World/CSeedling.hpp"
#include "MP1/World/CBurrower.hpp"
#include "CScriptBeam.hpp"
#include "CScriptCameraBlurKeyframe.hpp"
#include "CScriptCameraFilterKeyframe.hpp"
#include "CScriptCameraHint.hpp"
#include "CScriptCameraHintTrigger.hpp"
#include "CAmbientAI.hpp"
#include "MP1/World/CPuffer.hpp"
#include "MP1/World/CTryclops.hpp"
#include "MP1/World/CRidley.hpp"
#include "CScriptCameraPitchVolume.hpp"
#include "MP1/World/CMagdolite.hpp"
#include "CTeamAiMgr.hpp"
#include "CSnakeWeedSwarm.hpp"
#include "CScriptCameraShaker.hpp"
#include "CScriptCameraWaypoint.hpp"
#include "CScriptColorModulate.hpp"
#include "MP1/World/CJellyZap.hpp"
#include "CScriptControllerAction.hpp"
#include "CScriptCounter.hpp"
#include "MP1/World/CElitePirate.hpp"
#include "MP1/World/CMetroid.hpp"
#include "MP1/World/CMetroidBeta.hpp"
#include "MP1/World/CChozoGhost.hpp"
#include "CScriptCoverPoint.hpp"
#include "CScriptSpiderBallWaypoint.hpp"
#include "CScriptDamageableTrigger.hpp"
#include "MP1/World/CMetroid.hpp"
#include "CScriptDebris.hpp"
#include "CScriptDistanceFog.hpp"
#include "CScriptDock.hpp"
#include "MP1/World/CFireFlea.hpp"
#include "MP1/World/CMetaree.hpp"
#include "CScriptDockAreaChange.hpp"
#include "CScriptDoor.hpp"
#include "CScriptEffect.hpp"
#include "CScriptGenerator.hpp"
#include "CScriptGrapplePoint.hpp"
#include "MP1/World/CPuddleSpore.hpp"
#include "CScriptHUDMemo.hpp"
#include "CScriptMazeNode.hpp"
#include "CScriptMemoryRelay.hpp"
#include "CScriptMidi.hpp"
#include "CScriptPickup.hpp"
#include "CScriptPickupGenerator.hpp"
#include "CScriptPlatform.hpp"
#include "CScriptEMPulse.hpp"
#include "CScriptPlayerActor.hpp"
#include "CFishCloud.hpp"
#include "CFishCloudModifier.hpp"
#include "CScriptPlayerHint.hpp"
#include "CScriptPlayerStateChange.hpp"
#include "CScriptPointOfInterest.hpp"
#include "CScriptRandomRelay.hpp"
#include "CScriptRelay.hpp"
#include "CScriptRoomAcoustics.hpp"
#include "CScriptShadowProjector.hpp"
#include "CScriptSound.hpp"
#include "CScriptSpawnPoint.hpp"
#include "CScriptSpecialFunction.hpp"
#include "MP1/World/CSpankWeed.hpp"
#include "MP1/World/CBabygoth.hpp"
#include "MP1/World/CEyeball.hpp"
#include "CScriptSteam.hpp"
#include "CScriptRipple.hpp"
#include "CScriptBallTrigger.hpp"
#include "CScriptTargetingPoint.hpp"
#include "CScriptStreamedMusic.hpp"
#include "CScriptGunTurret.hpp"
#include "CScriptSwitch.hpp"
#include "CScriptTimer.hpp"
#include "CScriptVisorFlare.hpp"
#include "CScriptVisorGoo.hpp"
#include "CScriptWater.hpp"
#include "CScriptWaypoint.hpp"
#include "CScriptWorldTeleporter.hpp"
#include "CScriptDebugCameraWaypoint.hpp"
#include "CScriptSpiderBallAttractionSurface.hpp"
#include "MP1/World/CPuddleToadGamma.hpp"
#include "CScriptSpindleCamera.hpp"
#include "MP1/World/CAtomicAlpha.hpp"
#include "MP1/World/CAtomicBeta.hpp"
#include "CSimplePool.hpp"
#include "CStateManager.hpp"
#include "CWallCrawlerSwarm.hpp"
#include "CWorld.hpp"
#include "Camera/CCinematicCamera.hpp"
#include "Collision/CCollidableOBBTreeGroup.hpp"
#include "Editor/ProjectResourceFactoryMP1.hpp"
#include "GameGlobalObjects.hpp"
#include "MP1/World/CActorContraption.hpp"
#include "MP1/World/CBeetle.hpp"
#include "MP1/World/CMetroidPrimeRelay.hpp"
#include "MP1/World/CNewIntroBoss.hpp"
#include "MP1/World/CSpacePirate.hpp"
#include "MP1/World/CFlyingPirate.hpp"
#include "MP1/World/CWarWasp.hpp"
#include "MP1/World/CParasite.hpp"
#include "Particle/CWeaponDescription.hpp"
#include "MP1/World/CBloodFlower.hpp"
#include "MP1/World/CFlickerBat.hpp"
#include "Camera/CPathCamera.hpp"

namespace urde {
static logvisor::Module Log("urde::ScriptLoader");

static SObjectTag MorphballDoorANCS = {};

static const SObjectTag& GetMorphballDoorACS() {
  if (!MorphballDoorANCS) {
    MorphballDoorANCS.type = FOURCC('ANCS');
    MorphballDoorANCS.id = g_ResFactory->TranslateOriginalToNew(0x1F9DA858);
  }
  return MorphballDoorANCS;
}

static bool EnsurePropertyCount(int count, int expected, const char* structName) {
  if (count < expected) {
    Log.report(logvisor::Warning, "Insufficient number of props (%d/%d) for %s entity", count, expected, structName);
    return false;
  }
  return true;
}

struct SActorHead {
  std::string x0_name;
  zeus::CTransform x10_transform;
};

struct SScaledActorHead : SActorHead {
  zeus::CVector3f x40_scale;

  SScaledActorHead(SActorHead&& head) : SActorHead(std::move(head)) {}
};

static zeus::CTransform LoadEditorTransform(CInputStream& in) {
  zeus::CVector3f position;
  position.readBig(in);
  zeus::CVector3f orientation;
  orientation.readBig(in);
  return ScriptLoader::ConvertEditorEulerToTransform4f(orientation, position);
}

static zeus::CTransform LoadEditorTransformPivotOnly(CInputStream& in) {
  zeus::CVector3f position;
  position.readBig(in);
  zeus::CVector3f orientation;
  orientation.readBig(in);
  orientation.x() = 0.f;
  orientation.y() = 0.f;
  return ScriptLoader::ConvertEditorEulerToTransform4f(orientation, position);
}

static SActorHead LoadActorHead(CInputStream& in, CStateManager& stateMgr) {
  SActorHead ret;
  ret.x0_name = stateMgr.HashInstanceName(in);
  ret.x10_transform = LoadEditorTransform(in);
  return ret;
}

static SScaledActorHead LoadScaledActorHead(CInputStream& in, CStateManager& stateMgr) {
  SScaledActorHead ret = LoadActorHead(in, stateMgr);
  ret.x40_scale.readBig(in);
  return ret;
}

static zeus::CAABox GetCollisionBox(CStateManager& stateMgr, TAreaId id, const zeus::CVector3f& extent,
                                    const zeus::CVector3f& offset) {
  zeus::CAABox box(-extent * 0.5f + offset, extent * 0.5f + offset);
  const zeus::CTransform& rot = stateMgr.WorldNC()->GetGameAreas()[id]->GetTransform().getRotation();
  return box.getTransformedAABox(rot);
}

u32 ScriptLoader::LoadParameterFlags(CInputStream& in) {
  u32 count = in.readUint32Big();
  u32 ret = 0;
  for (u32 i = 0; i < count; ++i)
    if (in.readBool())
      ret |= 1 << i;
  return ret;
}

CGrappleParameters ScriptLoader::LoadGrappleParameters(CInputStream& in) {
  float a = in.readFloatBig();
  float b = in.readFloatBig();
  float c = in.readFloatBig();
  float d = in.readFloatBig();
  float e = in.readFloatBig();
  float f = in.readFloatBig();
  float g = in.readFloatBig();
  float h = in.readFloatBig();
  float i = in.readFloatBig();
  float j = in.readFloatBig();
  float k = in.readFloatBig();
  bool l = in.readBool();
  return CGrappleParameters(a, b, c, d, e, f, g, h, i, j, k, l);
}

CActorParameters ScriptLoader::LoadActorParameters(CInputStream& in) {
  u32 propCount = in.readUint32Big();
  if (propCount >= 5 && propCount <= 0xe) {
    CLightParameters lParms = ScriptLoader::LoadLightParameters(in);

    CScannableParameters sParams;
    if (propCount > 5)
      sParams = LoadScannableParameters(in);

    CAssetId xrayModel = in.readUint32Big();
    CAssetId xraySkin = in.readUint32Big();
    CAssetId infraModel = in.readUint32Big();
    CAssetId infraSkin = in.readUint32Big();

    bool globalTimeProvider = true;
    if (propCount > 7)
      globalTimeProvider = in.readBool();

    float fadeInTime = 1.f;
    if (propCount > 8)
      fadeInTime = in.readFloatBig();

    float fadeOutTime = 1.f;
    if (propCount > 9)
      fadeOutTime = in.readFloatBig();

    CVisorParameters vParms;
    if (propCount > 6)
      vParms = LoadVisorParameters(in);

    bool thermalHeat = false;
    if (propCount > 10)
      thermalHeat = in.readBool();

    bool renderUnsorted = false;
    if (propCount > 11)
      renderUnsorted = in.readBool();

    bool noSortThermal = false;
    if (propCount > 12)
      noSortThermal = in.readBool();

    float thermalMag = 1.f;
    if (propCount > 13)
      thermalMag = in.readFloatBig();

    std::pair<CAssetId, CAssetId> xray = {};
    if (g_ResFactory->GetResourceTypeById(xrayModel))
      xray = {xrayModel, xraySkin};

    std::pair<CAssetId, CAssetId> infra = {};
    if (g_ResFactory->GetResourceTypeById(infraModel))
      infra = {infraModel, infraSkin};

    return CActorParameters(lParms, sParams, xray, infra, vParms, globalTimeProvider, thermalHeat, renderUnsorted,
                            noSortThermal, fadeInTime, fadeOutTime, thermalMag);
  }
  return CActorParameters::None();
}

CVisorParameters ScriptLoader::LoadVisorParameters(CInputStream& in) {
  u32 propCount = in.readUint32Big();
  if (propCount >= 1 && propCount <= 3) {
    bool b1 = in.readBool();
    bool scanPassthrough = false;
    u8 mask = 0xf;
    if (propCount > 2)
      scanPassthrough = in.readBool();
    if (propCount >= 2)
      mask = u8(in.readUint32Big());
    return CVisorParameters(mask, b1, scanPassthrough);
  }
  return CVisorParameters();
}

CScannableParameters ScriptLoader::LoadScannableParameters(CInputStream& in) {
  u32 propCount = in.readUint32Big();
  if (propCount == 1)
    return CScannableParameters(in.readUint32Big());
  return CScannableParameters();
}

CLightParameters ScriptLoader::LoadLightParameters(CInputStream& in) {
  u32 propCount = in.readUint32Big();
  if (propCount == 14) {
    bool a = in.readBool();
    float b = in.readFloatBig();
    CLightParameters::EShadowTesselation shadowTess = CLightParameters::EShadowTesselation(in.readUint32Big());
    float d = in.readFloatBig();
    float e = in.readFloatBig();

    zeus::CColor noLightsAmbient;
    noLightsAmbient.readRGBABig(in);

    bool makeLights = in.readBool();
    CLightParameters::EWorldLightingOptions lightOpts = CLightParameters::EWorldLightingOptions(in.readUint32Big());
    CLightParameters::ELightRecalculationOptions recalcOpts =
        CLightParameters::ELightRecalculationOptions(in.readUint32Big());

    zeus::CVector3f actorPosBias;
    actorPosBias.readBig(in);

    s32 maxDynamicLights = -1;
    s32 maxAreaLights = -1;
    if (propCount >= 12) {
      maxDynamicLights = in.readUint32Big();
      maxAreaLights = in.readUint32Big();
    }

    bool ambientChannelOverflow = false;
    if (propCount >= 13)
      ambientChannelOverflow = in.readBool();

    s32 layerIdx = 0;
    if (propCount >= 14)
      layerIdx = in.readUint32Big();

    return CLightParameters(a, b, shadowTess, d, e, noLightsAmbient, makeLights, lightOpts, recalcOpts, actorPosBias,
                            maxDynamicLights, maxAreaLights, ambientChannelOverflow, layerIdx);
  }
  return CLightParameters::None();
}

CAnimationParameters ScriptLoader::LoadAnimationParameters(CInputStream& in) {
  CAssetId ancs = in.readUint32Big();
  s32 charIdx = in.readUint32Big();
  u32 defaultAnim = in.readUint32Big();
  return CAnimationParameters(ancs, charIdx, defaultAnim);
}

CFluidUVMotion ScriptLoader::LoadFluidUVMotion(CInputStream& in) {
  auto motion = CFluidUVMotion::EFluidUVMotion(in.readUint32Big());
  float a = in.readFloatBig();
  float b = in.readFloatBig();
  b = zeus::degToRad(b) - M_PIF;
  float c = in.readFloatBig();
  float d = in.readFloatBig();
  CFluidUVMotion::SFluidLayerMotion pattern1Layer(motion, a, b, c, d);

  motion = CFluidUVMotion::EFluidUVMotion(in.readUint32Big());
  a = in.readFloatBig();
  b = in.readFloatBig();
  b = zeus::degToRad(b) - M_PIF;
  c = in.readFloatBig();
  d = in.readFloatBig();
  CFluidUVMotion::SFluidLayerMotion pattern2Layer(motion, a, b, c, d);

  motion = CFluidUVMotion::EFluidUVMotion(in.readUint32Big());
  a = in.readFloatBig();
  b = in.readFloatBig();
  b = zeus::degToRad(b) - M_PIF;
  c = in.readFloatBig();
  d = in.readFloatBig();
  CFluidUVMotion::SFluidLayerMotion colorLayer(motion, a, b, c, d);

  a = in.readFloatBig();
  b = in.readFloatBig();

  b = zeus::degToRad(b) - M_PIF;

  return CFluidUVMotion(a, b, colorLayer, pattern1Layer, pattern2Layer);
}

zeus::CTransform ScriptLoader::ConvertEditorEulerToTransform4f(const zeus::CVector3f& orientation,
                                                               const zeus::CVector3f& position) {
  zeus::simd_floats f(orientation.mSimd);
  return zeus::CTransform::RotateZ(zeus::degToRad(f[2])) * zeus::CTransform::RotateY(zeus::degToRad(f[1])) *
             zeus::CTransform::RotateX(zeus::degToRad(f[0])) +
         position;
}

CEntity* ScriptLoader::LoadActor(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 24, "Actor"))
    return nullptr;

  SScaledActorHead head = LoadScaledActorHead(in, mgr);

  zeus::CVector3f collisionExtent;
  collisionExtent.readBig(in);

  zeus::CVector3f centroid;
  centroid.readBig(in);

  float mass = in.readFloatBig();
  float zMomentum = in.readFloatBig();

  CHealthInfo hInfo(in);

  CDamageVulnerability dVuln(in);

  CAssetId staticId = in.readUint32Big();
  CAnimationParameters aParms = LoadAnimationParameters(in);

  CActorParameters actParms = LoadActorParameters(in);

  bool looping = in.readBool();
  bool immovable = in.readBool();
  bool solid = in.readBool();
  bool cameraPassthrough = in.readBool();
  bool active = in.readBool();
  u32 shaderIdx = in.readUint32Big();
  float xrayAlpha = in.readFloatBig();
  bool noThermalHotZ = in.readBool();
  bool castsShadow = in.readBool();
  bool scaleAdvancementDelta = in.readBool();
  bool materialFlag54 = in.readBool();

  FourCC animType = g_ResFactory->GetResourceTypeById(aParms.GetACSFile());
  if (!g_ResFactory->GetResourceTypeById(staticId) && !animType)
    return nullptr;

  zeus::CAABox aabb = GetCollisionBox(mgr, info.GetAreaId(), collisionExtent, centroid);

  CMaterialList list;
  if (immovable) // Bool 2
    list.Add(EMaterialTypes::Immovable);

  if (solid) // Bool 3
    list.Add(EMaterialTypes::Solid);

  if (cameraPassthrough) // Bool 4
    list.Add(EMaterialTypes::CameraPassthrough);

  CModelData data;
  if (animType == SBIG('ANCS'))
    data = CAnimRes(aParms.GetACSFile(), aParms.GetCharacter(), head.x40_scale, aParms.GetInitialAnimation(), false);
  else
    data = CStaticRes(staticId, head.x40_scale);

  if ((collisionExtent.x() < 0.f || collisionExtent.y() < 0.f || collisionExtent.z() < 0.f) || collisionExtent.isZero())
    aabb = data.GetBounds(head.x10_transform.getRotation());

  return new CScriptActor(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, std::move(data), aabb, mass,
                          zMomentum, list, hInfo, dVuln, actParms, looping, active, shaderIdx, xrayAlpha, noThermalHotZ,
                          castsShadow, scaleAdvancementDelta, materialFlag54);
}

CEntity* ScriptLoader::LoadWaypoint(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 13, "Waypoint"))
    return nullptr;

  SActorHead head = LoadActorHead(in, mgr);

  bool active = in.readBool();
  float speed = in.readFloatBig();
  float pause = in.readFloatBig();
  u32 patternTranslate = in.readUint32Big();
  u32 patternOrient = in.readUint32Big();
  u32 patternFit = in.readUint32Big();
  u32 behaviour = in.readUint32Big();
  u32 behaviourOrient = in.readUint32Big();
  u32 behaviourModifiers = in.readUint32Big();
  u32 animation = in.readUint32Big();

  return new CScriptWaypoint(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, active, speed, pause,
                             patternTranslate, patternOrient, patternFit, behaviour, behaviourOrient,
                             behaviourModifiers, animation);
}

CEntity* ScriptLoader::LoadDoor(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 13, "Door") || propCount > 14)
    return nullptr;

  SScaledActorHead head = LoadScaledActorHead(in, mgr);
  CAnimationParameters aParms = LoadAnimationParameters(in);
  CActorParameters actParms = LoadActorParameters(in);

  zeus::CVector3f orbitPos;
  orbitPos.readBig(in);
  zeus::CVector3f collisionExtent;
  collisionExtent.readBig(in);
  zeus::CVector3f offset;
  offset.readBig(in);

  bool active = in.readBool();
  bool open = in.readBool();
  bool projectilesCollide = in.readBool();
  float animationLength = in.readFloatBig();

  zeus::CAABox aabb = GetCollisionBox(mgr, info.GetAreaId(), collisionExtent, offset);

  if (!g_ResFactory->GetResourceTypeById(aParms.GetACSFile()))
    return nullptr;

  CModelData mData = CAnimRes(aParms.GetACSFile(), aParms.GetCharacter(), head.x40_scale, 0, false);
  if (collisionExtent.isZero())
    aabb = mData.GetBounds(head.x10_transform.getRotation());

  bool isMorphballDoor = false;
  if (propCount == 13) {
    if (aParms.GetACSFile() == GetMorphballDoorACS().id)
      isMorphballDoor = true;
  } else if (propCount == 14)
    isMorphballDoor = in.readBool();

  return new CScriptDoor(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, std::move(mData), actParms,
                         orbitPos, aabb, active, open, projectilesCollide, animationLength, isMorphballDoor);
}

CEntity* ScriptLoader::LoadTrigger(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 9, "Trigger"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);

  zeus::CVector3f position = zeus::CVector3f::ReadBig(in);

  zeus::CVector3f extent = zeus::CVector3f::ReadBig(in);

  CDamageInfo dInfo(in);

  zeus::CVector3f forceVec = zeus::CVector3f::ReadBig(in);

  ETriggerFlags flags = ETriggerFlags(in.readUint32Big());
  bool active = in.readBool();
  bool b2 = in.readBool();
  bool b3 = in.readBool();

  zeus::CAABox box(-extent * 0.5f, extent * 0.5f);

  const zeus::CTransform& areaXf = mgr.WorldNC()->GetGameAreas()[info.GetAreaId()]->GetTransform();
  zeus::CVector3f orientedForce = areaXf.basis * forceVec;

  return new CScriptTrigger(mgr.AllocateUniqueId(), name, info, position, box, dInfo, orientedForce, flags, active, b2,
                            b3);
}

CEntity* ScriptLoader::LoadTimer(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 6, "Timer"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);

  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  bool b1 = in.readBool();
  bool b2 = in.readBool();
  bool b3 = in.readBool();

  return new CScriptTimer(mgr.AllocateUniqueId(), name, info, f1, f2, b1, b2, b3);
}

CEntity* ScriptLoader::LoadCounter(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 5, "Counter"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);

  u32 w1 = in.readUint32Big();
  u32 w2 = in.readUint32Big();
  bool b1 = in.readBool();
  bool b2 = in.readBool();

  return new CScriptCounter(mgr.AllocateUniqueId(), name, info, w1, w2, b1, b2);
}

CEntity* ScriptLoader::LoadEffect(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 24, "Effect"))
    return nullptr;

  SScaledActorHead head = LoadScaledActorHead(in, mgr);

  CAssetId partId(in);
  CAssetId elscId(in);
  bool hotInThermal = in.readBool();
  bool noTimerUnlessAreaOccluded = in.readBool();
  bool rebuildSystemsOnActivate = in.readBool();
  bool active = in.readBool();

  if (!partId.IsValid() && !elscId.IsValid())
    return nullptr;

  if (!g_ResFactory->GetResourceTypeById(partId) && !g_ResFactory->GetResourceTypeById(elscId))
    return nullptr;

  bool useRateInverseCamDist = in.readBool();
  float rateInverseCamDist = in.readFloatBig();
  float rateInverseCamDistRate = in.readFloatBig();
  float duration = in.readFloatBig();
  float durationResetWhileVisible = in.readFloatBig();
  bool useRateCamDistRange = in.readBool();
  float rateCamDistRangeMin = in.readFloatBig();
  float rateCamDistRangeMax = in.readFloatBig();
  float rateCamDistRangeFarRate = in.readFloatBig();
  bool combatVisorVisible = in.readBool();
  bool thermalVisorVisible = in.readBool();
  bool xrayVisorVisible = in.readBool();
  bool dieWhenSystemsDone = in.readBool();

  CLightParameters lParms = LoadLightParameters(in);

  return new CScriptEffect(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, head.x40_scale, partId,
                           elscId, hotInThermal, noTimerUnlessAreaOccluded, rebuildSystemsOnActivate, active,
                           useRateInverseCamDist, rateInverseCamDist, rateInverseCamDistRate, duration,
                           durationResetWhileVisible, useRateCamDistRange, rateCamDistRangeMin, rateCamDistRangeMax,
                           rateCamDistRangeFarRate, combatVisorVisible, thermalVisorVisible, xrayVisorVisible, lParms,
                           dieWhenSystemsDone);
}

CEntity* ScriptLoader::LoadPlatform(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 19, "Platform"))
    return nullptr;

  SScaledActorHead head = LoadScaledActorHead(in, mgr);

  zeus::CVector3f extent;
  extent.readBig(in);

  zeus::CVector3f centroid;
  centroid.readBig(in);

  CAssetId staticId = in.readUint32Big();
  CAnimationParameters aParms = LoadAnimationParameters(in);

  CActorParameters actParms = LoadActorParameters(in);

  float speed = in.readFloatBig();
  bool active = in.readBool();
  CAssetId dclnId = in.readUint32Big();

  CHealthInfo hInfo(in);

  CDamageVulnerability dInfo(in);

  bool detectCollision = in.readBool();
  float xrayAlpha = in.readFloatBig();
  bool rainSplashes = in.readBool();
  u32 maxRainSplashes = in.readUint32Big();
  u32 rainGenRate = in.readUint32Big();

  FourCC animType = g_ResFactory->GetResourceTypeById(aParms.GetACSFile());
  if (!g_ResFactory->GetResourceTypeById(staticId) && !animType)
    return nullptr;

  zeus::CAABox aabb = GetCollisionBox(mgr, info.GetAreaId(), extent, centroid);

  FourCC dclnType = g_ResFactory->GetResourceTypeById(dclnId);
  rstl::optional<TLockedToken<CCollidableOBBTreeGroupContainer>> dclnToken;
  if (dclnType) {
    dclnToken.emplace(g_SimplePool->GetObj({SBIG('DCLN'), dclnId}));
    dclnToken->GetObj();
  }

  CModelData data;
  if (animType == SBIG('ANCS'))
    data = CAnimRes(aParms.GetACSFile(), aParms.GetCharacter(), head.x40_scale, aParms.GetInitialAnimation(), true);
  else
    data = CStaticRes(staticId, head.x40_scale);

  if (extent.isZero())
    aabb = data.GetBounds(head.x10_transform.getRotation());

  return new CScriptPlatform(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, std::move(data), actParms,
                             aabb, speed, detectCollision, xrayAlpha, active, hInfo, dInfo, dclnToken, rainSplashes,
                             maxRainSplashes, rainGenRate);
}

CEntity* ScriptLoader::LoadSound(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 20, "Sound"))
    return nullptr;

  SActorHead head = LoadActorHead(in, mgr);
  s32 soundId = in.readInt32Big();
  bool active = in.readBool();
  float maxDist = in.readFloatBig();
  float distComp = in.readFloatBig();
  float startDelay = in.readFloatBig();
  u32 minVol = in.readUint32Big();
  u32 vol = in.readUint32Big();
  u32 prio = in.readUint32Big();
  u32 pan = in.readUint32Big();
  bool loop = in.readBool();
  bool nonEmitter = in.readBool();
  bool autoStart = in.readBool();
  bool occlusionTest = in.readBool();
  bool acoustics = in.readBool();
  bool worldSfx = in.readBool();
  bool allowDuplicates = in.readBool();
  s32 pitch = in.readInt32Big();

  if (soundId < 0)
    return nullptr;

  return new CScriptSound(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, u16(soundId), active, maxDist,
                          distComp, startDelay, minVol, vol, 0, prio, pan, 0, loop, nonEmitter, autoStart,
                          occlusionTest, acoustics, worldSfx, allowDuplicates, pitch);
}

CEntity* ScriptLoader::LoadGenerator(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 8, "Generator"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);

  u32 spawnCount = in.readUint32Big();
  bool noReuseFollowers = in.readBool();
  bool noInheritXf = in.readBool();

  zeus::CVector3f offset = zeus::CVector3f::ReadBig(in);

  bool active = in.readBool();
  float minScale = in.readFloatBig();
  float maxScale = in.readFloatBig();

  return new CScriptGenerator(mgr.AllocateUniqueId(), name, info, spawnCount, noReuseFollowers, offset, noInheritXf,
                              active, minScale, maxScale);
}

CEntity* ScriptLoader::LoadDock(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 7, "Dock"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  bool active = in.readBool();
  zeus::CVector3f position;
  position.readBig(in);
  zeus::CVector3f scale;
  scale.readBig(in);
  u32 dock = in.readUint32Big();
  TAreaId area = in.readUint32Big();
  bool loadConnected = in.readBool();
  return new CScriptDock(mgr.AllocateUniqueId(), name, info, position, scale, dock, area, active, 0, loadConnected);
}

CEntity* ScriptLoader::LoadCamera(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 14, "Camera"))
    return nullptr;

  SActorHead head = LoadActorHead(in, mgr);

  bool active = in.readBool();
  float shotDuration = in.readFloatBig();
  bool lookAtPlayer = in.readBool();
  bool outOfPlayerEye = in.readBool();
  bool intoPlayerEye = in.readBool();
  bool drawPlayer = in.readBool();
  bool disableInput = in.readBool();
  bool b7 = in.readBool();
  bool finishCineSkip = in.readBool();
  float fov = in.readFloatBig();
  bool checkFailsafe = in.readBool();

  bool disableOutOfInto = false;
  if (propCount > 14)
    disableOutOfInto = in.readBool();

  s32 flags = s32(lookAtPlayer) | s32(outOfPlayerEye) << 1 | s32(intoPlayerEye) << 2 | s32(b7) << 3 |
              s32(finishCineSkip) << 4 | s32(disableInput) << 5 | s32(drawPlayer) << 6 | s32(checkFailsafe) << 7 |
              s32(disableOutOfInto) << 9;

  return new CCinematicCamera(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, active, shotDuration,
                              fov / CCameraManager::Aspect(), CCameraManager::NearPlane(), CCameraManager::FarPlane(),
                              CCameraManager::Aspect(), flags);
}

CEntity* ScriptLoader::LoadCameraWaypoint(CStateManager& mgr, CInputStream& in, int propCount,
                                          const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 6, "CameraWaypoint"))
    return nullptr;

  SActorHead head = LoadActorHead(in, mgr);

  bool active = in.readBool();
  float f1 = in.readFloatBig();
  u32 w1 = in.readUint32Big();

  return new CScriptCameraWaypoint(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, active, f1, w1);
}

CEntity* ScriptLoader::LoadNewIntroBoss(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 13, "NewIntroBoss"))
    return nullptr;

  SScaledActorHead head = LoadScaledActorHead(in, mgr);

  std::pair<bool, u32> pcount = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pcount.first)
    return nullptr;

  CPatternedInfo pInfo(in, pcount.second);

  CActorParameters actParms = LoadActorParameters(in);

  float minTurnAngle = in.readFloatBig();
  CAssetId projectile(in);

  CDamageInfo dInfo(in);

  CAssetId beamContactFxId(in);
  CAssetId beamPulseFxId(in);
  CAssetId beamTextureId(in);
  CAssetId beamGlowTextureId(in);

  const CAnimationParameters& aParms = pInfo.GetAnimationParameters();
  FourCC animType = g_ResFactory->GetResourceTypeById(aParms.GetACSFile());
  if (animType != SBIG('ANCS'))
    return nullptr;

  CAnimRes res(aParms.GetACSFile(), aParms.GetCharacter(), head.x40_scale, aParms.GetInitialAnimation(), true);

  return new MP1::CNewIntroBoss(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, res, pInfo, actParms,
                                minTurnAngle, projectile, dInfo, beamContactFxId, beamPulseFxId, beamTextureId,
                                beamGlowTextureId);
}

CEntity* ScriptLoader::LoadSpawnPoint(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 35, "SpawnPoint"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);

  zeus::CVector3f position;
  position.readBig(in);

  zeus::CVector3f rotation;
  rotation.readBig(in);

  rstl::reserved_vector<u32, int(CPlayerState::EItemType::Max)> itemCounts;
  itemCounts.resize(size_t(CPlayerState::EItemType::Max));
  for (int i = 0; i < propCount - 6; ++i)
    itemCounts[i] = in.readUint32Big();

  bool defaultSpawn = in.readBool();
  bool active = in.readBool();
  bool morphed = false;
  if (propCount > 34)
    morphed = in.readBool();

  return new CScriptSpawnPoint(mgr.AllocateUniqueId(), name, info, ConvertEditorEulerToTransform4f(rotation, position),
                               itemCounts, defaultSpawn, active, morphed);
}

CEntity* ScriptLoader::LoadCameraHint(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (propCount > 25) {
    Log.report(logvisor::Warning, "Too many props (%d > 25) for CameraHint entity", propCount);
    return nullptr;
  }

  SActorHead head = LoadActorHead(in, mgr);

  bool active = in.readBool();
  u32 prio = in.readUint32Big();
  auto behaviour = CBallCamera::EBallCameraBehaviour(in.readUint32Big());
  u32 overrideFlags = LoadParameterFlags(in);
  overrideFlags |= in.readBool() << 22;
  float minDist = in.readFloatBig();
  overrideFlags |= in.readBool() << 23;
  float maxDist = in.readFloatBig();
  overrideFlags |= in.readBool() << 24;
  float backwardsDist = in.readFloatBig();
  overrideFlags |= in.readBool() << 25;
  zeus::CVector3f lookAtOffset;
  lookAtOffset.readBig(in);
  overrideFlags |= in.readBool() << 26;
  zeus::CVector3f chaseLookAtOffset;
  chaseLookAtOffset.readBig(in);
  zeus::CVector3f ballToCam;
  ballToCam.readBig(in);
  overrideFlags |= in.readBool() << 27;
  float fov = in.readFloatBig();
  overrideFlags |= in.readBool() << 28;
  float attitudeRange = zeus::degToRad(in.readFloatBig());
  overrideFlags |= in.readBool() << 29;
  float azimuthRange = zeus::degToRad(in.readFloatBig());
  overrideFlags |= in.readBool() << 30;
  float anglePerSecond = zeus::degToRad(in.readFloatBig());
  float clampVelRange = in.readFloatBig();
  float clampRotRange = zeus::degToRad(in.readFloatBig());
  overrideFlags |= in.readBool() << 31;
  float elevation = in.readFloatBig();
  float interpolateTime = in.readFloatBig();
  float clampVelTime = in.readFloatBig();
  float controlInterpDur = in.readFloatBig();

  return new CScriptCameraHint(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, active, prio, behaviour,
                               overrideFlags, minDist, maxDist, backwardsDist, lookAtOffset, chaseLookAtOffset,
                               ballToCam, fov, attitudeRange, azimuthRange, anglePerSecond, clampVelRange,
                               clampRotRange, elevation, interpolateTime, clampVelTime, controlInterpDur);
}

CEntity* ScriptLoader::LoadPickup(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 18, "Pickup"))
    return nullptr;

  SScaledActorHead head = LoadScaledActorHead(in, mgr);
  zeus::CVector3f extent = zeus::CVector3f::ReadBig(in);
  zeus::CVector3f offset = zeus::CVector3f::ReadBig(in);
  CPlayerState::EItemType itemType = CPlayerState::EItemType(in.readUint32Big());
  u32 capacity = in.readUint32Big();
  u32 amount = in.readUint32Big();
  float possibility = in.readFloatBig();
  float lifeTime = in.readFloatBig();
  float fadeInTime = in.readFloatBig();
  CAssetId staticModel = in.readUint32Big();
  CAnimationParameters animParms = LoadAnimationParameters(in);
  CActorParameters actorParms = LoadActorParameters(in);
  bool active = in.readBool();
  float startDelay = in.readFloatBig();
  CAssetId pickupEffect(in);

  FourCC acsType = g_ResFactory->GetResourceTypeById(animParms.GetACSFile());
  if (g_ResFactory->GetResourceTypeById(staticModel) == 0 && acsType == 0)
    return nullptr;

  zeus::CAABox aabb = GetCollisionBox(mgr, info.GetAreaId(), extent, offset);

  CModelData data;

  if (acsType == SBIG('ANCS'))
    data = CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), head.x40_scale, animParms.GetInitialAnimation(),
                    true);
  else
    data = CStaticRes(staticModel, head.x40_scale);

  if (extent.isZero())
    aabb = data.GetBounds(head.x10_transform.getRotation());

  return new CScriptPickup(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, std::move(data), actorParms,
                           aabb, itemType, amount, capacity, pickupEffect, possibility, lifeTime, fadeInTime,
                           startDelay, active);
}

CEntity* ScriptLoader::LoadMemoryRelay(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 3, "MemoryRelay") || propCount > 4)
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  bool b1 = in.readBool();
  bool b2 = in.readBool();
  bool b3 = false;
  if (propCount > 3)
    b3 = in.readBool();

  return new CScriptMemoryRelay(mgr.AllocateUniqueId(), name, info, b1, b2, b3);
}

CEntity* ScriptLoader::LoadRandomRelay(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 5, "RandomRelay"))
    return nullptr;
  std::string name = mgr.HashInstanceName(in);
  u32 sendSetSize = in.readUint32Big();
  u32 sendSetVariance = in.readUint32Big();
  bool percentSize = in.readBool();
  bool active = in.readBool();

  return new CScriptRandomRelay(mgr.AllocateUniqueId(), name, info, sendSetSize, sendSetVariance, percentSize, active);
}

CEntity* ScriptLoader::LoadRelay(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 2, "Relay") || propCount > 3)
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  if (propCount >= 3)
    in.readUint32Big();
  bool b1 = in.readBool();

  return new CScriptRelay(mgr.AllocateUniqueId(), name, info, b1);
}

CEntity* ScriptLoader::LoadBeetle(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 16, "Beetle"))
    return nullptr;
  std::string name = mgr.HashInstanceName(in);
  CPatterned::EFlavorType flavor = CPatterned::EFlavorType(in.readUint32Big());
  zeus::CTransform xfrm = LoadEditorTransform(in);
  zeus::CVector3f scale = zeus::CVector3f::ReadBig(in);
  std::pair<bool, u32> pcount = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pcount.first)
    return nullptr;

  CPatternedInfo pInfo(in, pcount.second);
  CActorParameters aParams = LoadActorParameters(in);
  CDamageInfo touchDamage(in);
  zeus::CVector3f tailAimReference = zeus::CVector3f::ReadBig(in);
  float unused = in.readFloatBig();
  CDamageVulnerability tailVuln(in);
  CDamageVulnerability platingVuln(in);
  CAssetId tailModel = in.readUint32Big();
  MP1::CBeetle::EEntranceType entranceType = MP1::CBeetle::EEntranceType(in.readUint32Big());
  float initialAttackDelay = in.readFloatBig();
  float retreatTime = in.readFloatBig();

  FourCC animType = g_ResFactory->GetResourceTypeById(pInfo.GetAnimationParameters().GetACSFile());
  if (animType != SBIG('ANCS'))
    return nullptr;

  rstl::optional<CStaticRes> tailRes;
  if (flavor == CPatterned::EFlavorType::One)
    tailRes.emplace(CStaticRes(tailModel, scale));

  const CAnimationParameters& animParams = pInfo.GetAnimationParameters();
  CAnimRes animRes(animParams.GetACSFile(), animParams.GetCharacter(), scale, animParams.GetInitialAnimation(), true);

  return new MP1::CBeetle(mgr.AllocateUniqueId(), name, info, xfrm, animRes, pInfo, flavor, entranceType, touchDamage,
                          platingVuln, tailAimReference, initialAttackDelay, retreatTime, unused, tailVuln, aParams,
                          tailRes);
}

CEntity* ScriptLoader::LoadHUDMemo(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (propCount != 5 && !EnsurePropertyCount(propCount, 6, "HUDMemo"))
    return 0;
  std::string name = mgr.HashInstanceName(in);
  CHUDMemoParms hParms(in);
  CScriptHUDMemo::EDisplayType displayType = CScriptHUDMemo::EDisplayType::MessageBox;
  if (propCount == 6)
    displayType = CScriptHUDMemo::EDisplayType(in.readUint32Big());
  CAssetId message = in.readUint32Big();
  bool active = in.readBool();

  return new CScriptHUDMemo(mgr.AllocateUniqueId(), name, info, hParms, displayType, message, active);
}

CEntity* ScriptLoader::LoadCameraFilterKeyframe(CStateManager& mgr, CInputStream& in, int propCount,
                                                const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 10, "CameraFilterKeyframe"))
    return nullptr;
  std::string name = mgr.HashInstanceName(in);
  bool active = in.readBool();
  EFilterType ftype = EFilterType(in.readUint32Big());
  EFilterShape shape = EFilterShape(in.readUint32Big());
  u32 filterIdx = in.readUint32Big();
  u32 unk = in.readUint32Big();
  zeus::CColor color;
  color.readRGBABig(in);
  float timeIn = in.readFloatBig();
  float timeOut = in.readFloatBig();
  CAssetId txtr = in.readUint32Big();

  return new CScriptCameraFilterKeyframe(mgr.AllocateUniqueId(), name, info, ftype, shape, filterIdx, unk, color,
                                         timeIn, timeOut, txtr, active);
}

CEntity* ScriptLoader::LoadCameraBlurKeyframe(CStateManager& mgr, CInputStream& in, int propCount,
                                              const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 7, "CameraBlurKeyframe"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  bool active = in.readBool();
  EBlurType type = EBlurType(in.readUint32Big());
  float amount = in.readFloatBig();
  u32 unk = in.readUint32Big();
  float timeIn = in.readFloatBig();
  float timeOut = in.readFloatBig();

  return new CScriptCameraBlurKeyframe(mgr.AllocateUniqueId(), name, info, type, amount, unk, timeIn, timeOut, active);
}

u32 ClassifyVector(const zeus::CVector3f& dir) {
  zeus::CVector3f absDir(std::fabs(dir.x()), std::fabs(dir.y()), std::fabs(dir.z()));
  u32 max = (absDir.x() > absDir.y() ? 0 : 1);
  max = (absDir[max] > absDir.z() ? max : 2);

  bool positive = (absDir[max] == dir[max]);
  if (max == 0)
    return (positive ? 0x08 : 0x04);
  else if (max == 1)
    return (positive ? 0x01 : 0x02);
  else if (max == 2)
    return (positive ? 0x10 : 0x20);

  return 0;
}

u32 TransformDamagableTriggerFlags(CStateManager& mgr, TAreaId aId, u32 flags) {
  CGameArea* area = mgr.WorldNC()->GetGameAreas().at(u32(aId)).get();
  zeus::CTransform rotation = area->GetTransform().getRotation();

  u32 ret = 0;
  if (flags & 0x01)
    ret |= ClassifyVector(rotation * zeus::skForward);
  if (flags & 0x02)
    ret |= ClassifyVector(rotation * zeus::skBack);
  if (flags & 0x04)
    ret |= ClassifyVector(rotation * zeus::skLeft);
  if (flags & 0x08)
    ret |= ClassifyVector(rotation * zeus::skRight);
  if (flags & 0x10)
    ret |= ClassifyVector(rotation * zeus::skUp);
  if (flags & 0x20)
    ret |= ClassifyVector(rotation * zeus::skDown);
  return ret;
}

CEntity* ScriptLoader::LoadDamageableTrigger(CStateManager& mgr, CInputStream& in, int propCount,
                                             const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 12, "DamageableTrigger"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f position(zeus::CVector3f::ReadBig(in));
  zeus::CVector3f volume(zeus::CVector3f::ReadBig(in));

  CHealthInfo hInfo(in);
  CDamageVulnerability dVuln(in);
  u32 triggerFlags = in.readUint32Big();
  triggerFlags = TransformDamagableTriggerFlags(mgr, info.GetAreaId(), triggerFlags);
  CAssetId patternTex1 = in.readUint32Big();
  CAssetId patternTex2 = in.readUint32Big();
  CAssetId colorTex = in.readUint32Big();
  CScriptDamageableTrigger::ECanOrbit canOrbit = CScriptDamageableTrigger::ECanOrbit(in.readBool());
  bool active = in.readBool();
  CVisorParameters vParms = LoadVisorParameters(in);
  return new CScriptDamageableTrigger(mgr.AllocateUniqueId(), name, info, position, volume, hInfo, dVuln, triggerFlags,
                                      patternTex1, patternTex2, colorTex, canOrbit, active, vParms);
}

CEntity* ScriptLoader::LoadDebris(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 18, "Debris"))
    return nullptr;

  SScaledActorHead head = LoadScaledActorHead(in, mgr);
  float zImpulse = in.readFloatBig();
  zeus::CVector3f velocity = zeus::CVector3f::ReadBig(in);
  zeus::CColor endsColor;
  endsColor.readRGBABig(in);
  float mass = in.readFloatBig();
  float restitution = in.readFloatBig();
  float duration = in.readFloatBig();
  CScriptDebris::EScaleType scaleType = CScriptDebris::EScaleType(in.readUint32Big());
  bool randomAngImpulse = in.readBool();
  CAssetId model = in.readUint32Big();
  CActorParameters aParams = LoadActorParameters(in);
  CAssetId particleId = in.readUint32Big();
  zeus::CVector3f particleScale = zeus::CVector3f::ReadBig(in);
  bool b1 = in.readBool();
  bool active = in.readBool();

  if (!g_ResFactory->GetResourceTypeById(model))
    return nullptr;
  return new CScriptDebris(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform,
                           CStaticRes(model, head.x40_scale), aParams, particleId, particleScale, zImpulse, velocity,
                           endsColor, mass, restitution, duration, scaleType, b1, randomAngImpulse, active);
}

CEntity* ScriptLoader::LoadCameraShaker(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 9, "CameraShaker"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  CCameraShakeData shakeData = CCameraShakeData::LoadCameraShakeData(in);
  bool active = in.readBool();

  return new CScriptCameraShaker(mgr.AllocateUniqueId(), name, info, active, shakeData);
}

CEntity* ScriptLoader::LoadActorKeyframe(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 7, "ActorKeyframe"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  s32 animId = in.readInt32Big();
  bool looping = in.readBool();
  float lifetime = in.readFloatBig();
  bool active = in.readBool();
  u32 fadeOut = in.readUint32Big();
  float totalPlayback = in.readFloatBig();

  if (animId == -1)
    return nullptr;

  return new CScriptActorKeyframe(mgr.AllocateUniqueId(), name, info, animId, looping, lifetime, false, fadeOut, active,
                                  totalPlayback);
}

CEntity* ScriptLoader::LoadWater(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 63, "Water"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f position;
  position.readBig(in);
  zeus::CVector3f extent;
  extent.readBig(in);
  CDamageInfo dInfo(in);
  zeus::CVector3f orientedForce;
  orientedForce.readBig(in);
  ETriggerFlags triggerFlags = ETriggerFlags(in.readUint32Big()) | ETriggerFlags::DetectProjectiles1 |
                               ETriggerFlags::DetectProjectiles2 | ETriggerFlags::DetectProjectiles3 |
                               ETriggerFlags::DetectProjectiles4 | ETriggerFlags::DetectBombs |
                               ETriggerFlags::DetectPowerBombs | ETriggerFlags::DetectProjectiles5 |
                               ETriggerFlags::DetectProjectiles6 | ETriggerFlags::DetectProjectiles7;
  bool thermalCold = in.readBool();
  bool displaySurface = in.readBool();
  CAssetId patternMap1 = in.readUint32Big();
  CAssetId patternMap2 = in.readUint32Big();
  CAssetId colorMap = in.readUint32Big();
  CAssetId bumpMap = in.readUint32Big();
  CAssetId _envMap = in.readUint32Big();
  CAssetId _envBumpMap = in.readUint32Big();
  zeus::CVector3f _bumpLightDir;
  _bumpLightDir.readBig(in);

  zeus::CVector3f bumpLightDir = _bumpLightDir;
  if (!bumpLightDir.canBeNormalized())
    bumpLightDir.assign(0.f, 0.f, -1.f);

  float bumpScale = 1.f / in.readFloatBig();
  float morphInTime = in.readFloatBig();
  float morphOutTime = in.readFloatBig();
  bool active = in.readBool();
  auto fluidType = EFluidType(in.readUint32Big());
  bool b4 = in.readBool();
  float alpha = in.readFloatBig();
  CFluidUVMotion uvMotion = LoadFluidUVMotion(in);

  float turbSpeed = in.readFloatBig();
  float turbDistance = in.readFloatBig();
  float turbFreqMax = in.readFloatBig();
  float turbFreqMin = in.readFloatBig();
  float turbPhaseMax = zeus::degToRad(in.readFloatBig());
  float turbPhaseMin = zeus::degToRad(in.readFloatBig());
  float turbAmplitudeMax = in.readFloatBig();
  float turbAmplitudeMin = in.readFloatBig();
  zeus::CColor splashColor;
  splashColor.readRGBABig(in);
  zeus::CColor insideFogColor;
  insideFogColor.readRGBABig(in);
  CAssetId splashParticle1 = in.readUint32Big();
  CAssetId splashParticle2 = in.readUint32Big();
  CAssetId splashParticle3 = in.readUint32Big();
  CAssetId visorRunoffParticle = in.readUint32Big();
  CAssetId unmorphVisorRunoffParticle = in.readUint32Big();
  u32 visorRunoffSfx = in.readUint32Big();
  u32 unmorphVisorRunoffSfx = in.readUint32Big();
  u32 splashSfx1 = in.readUint32Big();
  u32 splashSfx2 = in.readUint32Big();
  u32 splashSfx3 = in.readUint32Big();
  float tileSize = in.readFloatBig();
  u32 tileSubdivisions = in.readUint32Big();
  float specularMin = in.readFloatBig();
  float specularMax = in.readFloatBig();
  float reflectionSize = in.readFloatBig();
  float rippleIntensity = in.readFloatBig();
  float reflectionBlend = in.readFloatBig();
  float fogBias = in.readFloatBig();
  float fogMagnitude = in.readFloatBig();
  float fogSpeed = in.readFloatBig();
  zeus::CColor fogColor;
  fogColor.readRGBABig(in);
  CAssetId lightmap = in.readUint32Big();
  float unitsPerLightmapTexel = in.readFloatBig();
  float alphaInTime = in.readFloatBig();
  float alphaOutTime = in.readFloatBig();
  u32 w21 = in.readUint32Big();
  u32 w22 = in.readUint32Big();
  bool b5 = in.readBool();

  std::unique_ptr<u32[]> bitset;
  u32 bitVal0 = 0;
  u32 bitVal1 = 0;

  if (b5) {
    bitVal0 = in.readUint16Big();
    bitVal1 = in.readUint16Big();
    u32 len = ((bitVal0 * bitVal1) + 31) / 31;
    bitset.reset(new u32[len]);
    in.readBytesToBuf(bitset.get(), len * 4);
  }

  zeus::CAABox box(-extent * 0.5f, extent * 0.5f);

  CAssetId envBumpMap;
  if (!bumpMap.IsValid())
    envBumpMap = _envBumpMap;

  CAssetId envMap;
  if (!bumpMap.IsValid())
    envMap = _envMap;

  return new CScriptWater(mgr, mgr.AllocateUniqueId(), name, info, position, box, dInfo, orientedForce, triggerFlags,
                          thermalCold, displaySurface, patternMap1, patternMap2, colorMap, bumpMap, envMap, envBumpMap,
                          {}, bumpLightDir, bumpScale, morphInTime, morphOutTime, active, fluidType, b4, alpha,
                          uvMotion, turbSpeed, turbDistance, turbFreqMax, turbFreqMin, turbPhaseMax, turbPhaseMin,
                          turbAmplitudeMax, turbAmplitudeMin, splashColor, insideFogColor, splashParticle1,
                          splashParticle2, splashParticle3, visorRunoffParticle, unmorphVisorRunoffParticle,
                          visorRunoffSfx, unmorphVisorRunoffSfx, splashSfx1, splashSfx2, splashSfx3, tileSize,
                          tileSubdivisions, specularMin, specularMax, reflectionSize, rippleIntensity, reflectionBlend,
                          fogBias, fogMagnitude, fogSpeed, fogColor, lightmap, unitsPerLightmapTexel, alphaInTime,
                          alphaOutTime, w21, w22, b5, bitVal0, bitVal1, std::move(bitset));
}

CEntity* ScriptLoader::LoadWarWasp(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 13, "WarWasp"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  CPatterned::EFlavorType flavor = CPatterned::EFlavorType(in.readUint32Big());
  zeus::CTransform xf = LoadEditorTransformPivotOnly(in);
  zeus::CVector3f scale;
  scale.readBig(in);

  std::pair<bool, u32> verifyPair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!verifyPair.first)
    return nullptr;

  CPatternedInfo pInfo(in, verifyPair.second);
  CActorParameters actorParms = LoadActorParameters(in);
  CPatterned::EColliderType collider = CPatterned::EColliderType(in.readBool());
  CDamageInfo damageInfo1(in);
  CAssetId projectileWeapon = in.readUint32Big();
  CDamageInfo projectileDamage(in);
  CAssetId projectileVisorParticle = in.readUint32Big();
  u32 projectileVisorSfx = in.readUint32Big();

  const CAnimationParameters& aParms = pInfo.GetAnimationParameters();
  FourCC animType = g_ResFactory->GetResourceTypeById(aParms.GetACSFile());
  if (animType != SBIG('ANCS'))
    return nullptr;

  CAnimRes res(aParms.GetACSFile(), aParms.GetCharacter(), scale, aParms.GetInitialAnimation(), true);
  CModelData mData(res);
  return new MP1::CWarWasp(mgr.AllocateUniqueId(), name, info, xf, std::move(mData), pInfo, flavor, collider,
                           damageInfo1, actorParms, projectileWeapon, projectileDamage, projectileVisorParticle,
                           projectileVisorSfx);
}

CEntity* ScriptLoader::LoadSpacePirate(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 34, "SpacePirate"))
    return nullptr;
  SScaledActorHead head = LoadScaledActorHead(in, mgr);
  std::pair<bool, u32> verifyPair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!verifyPair.first)
    return nullptr;

  CPatternedInfo pInfo(in, verifyPair.second);
  CActorParameters aParams = LoadActorParameters(in);
  CAnimationParameters& animParms = pInfo.GetAnimationParameters();

  if (g_ResFactory->GetResourceTypeById(animParms.GetACSFile()) != SBIG('ANCS'))
    return nullptr;

  if (animParms.GetCharacter() == 0) {
    Log.report(logvisor::Warning, "SpacePirate <%s> has AnimationInformation property with invalid character selected",
               head.x0_name.c_str());
    animParms.SetCharacter(2);
  }

  CModelData mData(CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), head.x40_scale,
                            animParms.GetInitialAnimation(), true));

  return new MP1::CSpacePirate(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, std::move(mData),
                               aParams, pInfo, in, propCount);
}

CEntity* ScriptLoader::LoadFlyingPirate(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 35, "FlyingPirate"))
    return nullptr;

  SScaledActorHead actHead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);

  if (g_ResFactory->GetResourceTypeById(pInfo.GetAnimationParameters().GetACSFile()) != SBIG('ANCS'))
    return nullptr;

  const CAnimationParameters& animParms = pInfo.GetAnimationParameters();
  CModelData mData(CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), actHead.x40_scale,
                            animParms.GetInitialAnimation(), true));

  return new MP1::CFlyingPirate(mgr.AllocateUniqueId(), actHead.x0_name, info, actHead.x10_transform, std::move(mData),
                                actParms, pInfo, in, propCount);
}

CEntity* ScriptLoader::LoadElitePirate(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 41, "ElitePirate"))
    return nullptr;

  SScaledActorHead actHead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  MP1::CElitePirateData elitePirateData(in, propCount);

  if (!pInfo.GetAnimationParameters().GetACSFile().IsValid())
    return nullptr;

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            actHead.x40_scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));

  return new MP1::CElitePirate(mgr.AllocateUniqueId(), actHead.x0_name, info, actHead.x10_transform, std::move(mData),
                               pInfo, actParms, elitePirateData);
}

CEntity* ScriptLoader::LoadMetroidBeta(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 23, "MetroidBeta"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CTransform xf = LoadEditorTransform(in);
  zeus::CVector3f scale = zeus::CVector3f::ReadBig(in);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  MP1::CMetroidBetaData metData(in);
  if (!pInfo.GetAnimationParameters().GetACSFile().IsValid())
    return nullptr;

  const CAnimationParameters& animParms = pInfo.GetAnimationParameters();
  CModelData mData(
      CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), scale, animParms.GetInitialAnimation(), true));

  return new MP1::CMetroidBeta(mgr.AllocateUniqueId(), name, info, xf, std::move(mData), pInfo, actParms, metData);
}

CEntity* ScriptLoader::LoadChozoGhost(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 32, "ChozoGhost"))
    return nullptr;

  SScaledActorHead actorHead = LoadScaledActorHead(in, mgr);

  auto pair = CPatternedInfo::HasCorrectParameterCount(in);

  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();
  CAssetId wpsc1(in);
  CDamageInfo dInfo1(in);
  CAssetId wpsc2(in);
  CDamageInfo dInfo2(in);
  MP1::CChozoGhost::CBehaveChance behaveChance1(in);
  MP1::CChozoGhost::CBehaveChance behaveChance2(in);
  MP1::CChozoGhost::CBehaveChance behaveChance3(in);
  s16 sId1 = CSfxManager::TranslateSFXID(in.readUint32Big());
  float f5 = in.readFloatBig();
  s16 sId2 = CSfxManager::TranslateSFXID(in.readUint32Big());
  s16 sId3 = CSfxManager::TranslateSFXID(in.readUint32Big());
  u32 w1 = in.readUint32Big();
  float f6 = in.readFloatBig();
  u32 w2 = in.readUint32Big();
  float f7 = in.readFloatBig();
  CAssetId partId(in);
  s16 sId4 = CSfxManager::TranslateSFXID(in.readUint32Big());
  float f8 = in.readFloatBig();
  float f9 = in.readFloatBig();
  u32 w3 = in.readUint32Big();
  u32 w4 = in.readUint32Big();

  const CAnimationParameters& animParms = pInfo.GetAnimationParameters();
  if (g_ResFactory->GetResourceTypeById(animParms.GetACSFile()) != SBIG('ANCS'))
    return nullptr;

  CModelData mData(CAnimRes(animParms.GetACSFile(), 0, actorHead.x40_scale, animParms.GetInitialAnimation(), true));

  return new MP1::CChozoGhost(mgr.AllocateUniqueId(), actorHead.x0_name, info, actorHead.x10_transform,
                              std::move(mData), actParms, pInfo, f1, f2, f3, f4, wpsc1, dInfo1, wpsc2, dInfo2,
                              behaveChance1, behaveChance2, behaveChance3, sId1, f5, sId2, sId3, w1, f6, w2, f7, partId,
                              sId4, f8, f9, w3, w4);
}

CEntity* ScriptLoader::LoadCoverPoint(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 9, "CoverPoint"))
    return nullptr;

  SActorHead head = LoadActorHead(in, mgr);
  bool active = in.readBool();
  u32 flags = in.readUint32Big();
  bool crouch = in.readBool();
  float horizontalAngle = in.readFloatBig();
  float verticalAngle = in.readFloatBig();
  float coverTime = in.readFloatBig();

  return new CScriptCoverPoint(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, active, flags, crouch,
                               horizontalAngle, verticalAngle, coverTime);
}

CEntity* ScriptLoader::LoadSpiderBallWaypoint(CStateManager& mgr, CInputStream& in, int propCount,
                                              const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 5, "SpiderBallWaypoint"))
    return nullptr;

  SActorHead actHead = LoadActorHead(in, mgr);
  bool active = in.readBool();
  u32 w1 = in.readUint32Big();

  return new CScriptSpiderBallWaypoint(mgr.AllocateUniqueId(), actHead.x0_name, info, actHead.x10_transform, active,
                                       w1);
}

CEntity* ScriptLoader::LoadBloodFlower(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 18, "BloodFlower"))
    return nullptr;

  SScaledActorHead actHead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  CAssetId partId1(in);
  CAssetId wpsc1(in);
  CAssetId wpsc2(in);
  CDamageInfo dInfo1(in);
  CDamageInfo dInfo2(in);
  CDamageInfo dInfo3(in);
  CAssetId partId2(in);
  CAssetId partId3(in);
  CAssetId partId4(in);
  float f1 = in.readFloatBig();
  CAssetId partId5(in);
  u32 soundId = in.readUint32Big();

  if (g_ResFactory->GetResourceTypeById(pInfo.GetAnimationParameters().GetACSFile()) != SBIG('ANCS'))
    return nullptr;

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            actHead.x40_scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));

  return new MP1::CBloodFlower(mgr.AllocateUniqueId(), actHead.x0_name, info, actHead.x10_transform, std::move(mData),
                               pInfo, partId1, wpsc1, actParms, wpsc2, dInfo1, dInfo2, dInfo3, partId2, partId3,
                               partId4, f1, partId5, soundId);
}

CEntity* ScriptLoader::LoadFlickerBat(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 10, "FlickerBat"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  CPatterned::EFlavorType flavor = CPatterned::EFlavorType(in.readUint32Big());
  zeus::CTransform xf = LoadEditorTransform(in);
  zeus::CVector3f scale = zeus::CVector3f::ReadBig(in);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  bool b1 = in.readBool();
  bool b2 = in.readBool();
  bool b3 = in.readBool();

  if (g_ResFactory->GetResourceTypeById(pInfo.GetAnimationParameters().GetACSFile()) != SBIG('ANCS'))
    return nullptr;

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));
  return new MP1::CFlickerBat(mgr.AllocateUniqueId(), name, flavor, info, xf, std::move(mData), pInfo,
                              CPatterned::EColliderType(b1), b2, actParms, b3);
}

CEntity* ScriptLoader::LoadPathCamera(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (propCount > 15) {
    Log.report(logvisor::Warning, "Too many props (%d > 15) for PathCamera entity", propCount);
    return nullptr;
  }

  SActorHead aHead = LoadActorHead(in, mgr);
  bool active = in.readBool();
  u32 flags = LoadParameterFlags(in);
  float lengthExtent = in.readFloatBig();
  float filterMag = in.readFloatBig();
  float filterProportion = in.readFloatBig();
  CPathCamera::EInitialSplinePosition initPos = CPathCamera::EInitialSplinePosition(in.readUint32Big());
  float minEaseDist = in.readFloatBig();
  float maxEaseDist = in.readFloatBig();

  return new CPathCamera(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, active, lengthExtent,
                         filterMag, filterProportion, minEaseDist, maxEaseDist, flags, initPos);
}

CEntity* ScriptLoader::LoadGrapplePoint(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 5, "GrapplePoint"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CTransform grappleXf = LoadEditorTransform(in);
  bool active = in.readBool();
  CGrappleParameters parameters = LoadGrappleParameters(in);
  return new CScriptGrapplePoint(mgr.AllocateUniqueId(), name, info, grappleXf, active, parameters);
}

CEntity* ScriptLoader::LoadPuddleSpore(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 16, "PuddleSpore"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  CPatterned::EFlavorType flavor = CPatterned::EFlavorType(in.readUint32Big());
  zeus::CTransform xf = LoadEditorTransform(in);
  zeus::CVector3f scale = zeus::CVector3f::ReadBig(in);

  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  bool b1 = in.readBool();
  CAssetId w1(in);
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();
  float f5 = in.readFloatBig();
  CAssetId w2(in);
  CDamageInfo dInfo(in);

  const CAnimationParameters& animParms = pInfo.GetAnimationParameters();
  if (g_ResFactory->GetResourceTypeById(animParms.GetACSFile()) != SBIG('ANCS'))
    return nullptr;

  CModelData mData(
    CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), scale, animParms.GetInitialAnimation(), true));
  return new MP1::CPuddleSpore(mgr.AllocateUniqueId(), name, flavor, info, xf, std::move(mData), pInfo,
                               CPatterned::EColliderType(b1), w1, f1, f2, f3, f4, f5, actParms, w2, dInfo);
}

CEntity* ScriptLoader::LoadDebugCameraWaypoint(CStateManager& mgr, CInputStream& in, int propCount,
                                               const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 4, "DebugCameraWaypoint"))
    return nullptr;

  SActorHead actHead = LoadActorHead(in, mgr);
  u32 w1 = in.readUint32Big();
  return new CScriptDebugCameraWaypoint(mgr.AllocateUniqueId(), actHead.x0_name, info, actHead.x10_transform, w1);
}

CEntity* ScriptLoader::LoadSpiderBallAttractionSurface(CStateManager& mgr, CInputStream& in, int propCount,
                                                       const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 5, "SpiderBallAttractionSurface"))
    return nullptr;
  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);
  bool active = in.readBool();
  return new CScriptSpiderBallAttractionSurface(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform,
                                                aHead.x40_scale, active);
}

CEntity* ScriptLoader::LoadPuddleToadGamma(CStateManager& mgr, CInputStream& in, int propCount,
                                           const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 17, "PuddleToadGamma"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  CPatterned::EFlavorType flavor = CPatterned::EFlavorType(in.readUint32Big());
  zeus::CTransform xf = LoadEditorTransform(in);
  zeus::CVector3f scale = zeus::CVector3f::ReadBig(in);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  zeus::CVector3f vec = zeus::CVector3f::ReadBig(in);
  float f4 = in.readFloatBig();
  float f5 = in.readFloatBig();
  float f6 = in.readFloatBig();
  CDamageInfo dInfo1(in);
  CDamageInfo dInfo2(in);
  CAssetId collisionData(in);
  const CAnimationParameters& animParms = pInfo.GetAnimationParameters();
  CModelData mData(
      CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), scale, animParms.GetInitialAnimation(), true));
  return new MP1::CPuddleToadGamma(mgr.AllocateUniqueId(), name, flavor, info, xf, std::move(mData), pInfo, actParms,
                                   f1, f2, f3, vec, f4, f5, f6, dInfo1, dInfo2, collisionData);
}

CEntity* ScriptLoader::LoadDistanceFog(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 8, "DistanceFog"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  u32 mode = in.readUint32Big();
  zeus::CColor color;
  color.readRGBABig(in);
  zeus::CVector2f range;
  range.readBig(in);
  float colorDelta = in.readFloatBig();
  zeus::CVector2f rangeDelta;
  rangeDelta.readBig(in);
  bool expl = in.readBool();
  bool active = in.readBool();
  ERglFogMode realMode;

  if (mode == 0)
    realMode = ERglFogMode::None;
  else if (mode == 1)
    realMode = ERglFogMode::PerspLin;
  else if (mode == 2)
    realMode = ERglFogMode::PerspExp;
  else if (mode == 3)
    realMode = ERglFogMode::PerspExp2;
  else if (mode == 4)
    realMode = ERglFogMode::PerspRevExp;
  else if (mode == 5)
    realMode = ERglFogMode::PerspRevExp2;

  return new CScriptDistanceFog(mgr.AllocateUniqueId(), name, info, realMode, color, range, colorDelta, rangeDelta,
                                expl, active, 0.f, 0.f, 0.f, 0.f);
}

CEntity* ScriptLoader::LoadFireFlea(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 9, "FireFlea"))
    return nullptr;

  SScaledActorHead acthead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  in.readBool();
  in.readBool();
  float f1 = in.readFloatBig();

  if (!pInfo.GetAnimationParameters().GetACSFile().IsValid())
    return nullptr;

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            acthead.x40_scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));

  return new MP1::CFireFlea(mgr.AllocateUniqueId(), acthead.x0_name, info, acthead.x10_transform, std::move(mData),
                            actParms, pInfo, f1);
}

CEntity* ScriptLoader::LoadMetaree(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 12, "Metaree"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CTransform xf = LoadEditorTransform(in);
  zeus::CVector3f scale = zeus::CVector3f::ReadBig(in);

  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters aParms = LoadActorParameters(in);
  CDamageInfo dInfo(in);
  float f1 = in.readFloatBig();
  zeus::CVector3f vec = zeus::CVector3f::ReadBig(in);
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();

  if (g_ResFactory->GetResourceTypeById(pInfo.GetAnimationParameters().GetACSFile()) != SBIG('ANCS'))
    return nullptr;

  float f4 = in.readFloatBig();

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));
  return new MP1::CMetaree(mgr.AllocateUniqueId(), name, CPatterned::EFlavorType::Zero, info, xf, std::move(mData),
                           pInfo, dInfo, f1, vec, f2, EBodyType::Invalid, f3, f4, aParms);
}

CEntity* ScriptLoader::LoadDockAreaChange(CStateManager& mgr, CInputStream& in, int propCount,
                                          const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 3, "DockAreaChange"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  s32 w1 = in.readInt32Big();
  bool active = in.readBool();

  return new CScriptDockAreaChange(mgr.AllocateUniqueId(), name, info, w1, active);
}

CEntity* ScriptLoader::LoadActorRotate(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 6, "ActorRotate"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f rotation = zeus::CVector3f::ReadBig(in);
  float scale = in.readFloatBig();
  bool updateActors = in.readBool();
  bool updateOnCreation = in.readBool();
  bool active = in.readBool();

  return new CScriptActorRotate(mgr.AllocateUniqueId(), name, info, rotation, scale, updateActors, updateOnCreation,
                                active);
}

CEntity* ScriptLoader::LoadSpecialFunction(CStateManager& mgr, CInputStream& in, int propCount,
                                           const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 15, "SpecialFunction"))
    return nullptr;

  SActorHead head = LoadActorHead(in, mgr);
  CScriptSpecialFunction::ESpecialFunction specialFunction =
      CScriptSpecialFunction::ESpecialFunction(in.readUint32Big());
  std::string str = in.readString();
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  u32 w2 = in.readUint32Big();
  u32 w3 = in.readUint32Big();
  CPlayerState::EItemType w4 = CPlayerState::EItemType(in.readUint32Big());
  bool active1 = in.readBool();
  float f4 = in.readFloatBig();
  s16 w5 = in.readUint32Big() & 0xFFFF;
  s16 w6 = in.readUint32Big() & 0xFFFF;
  s16 w7 = in.readUint32Big() & 0xFFFF;
  if (specialFunction == CScriptSpecialFunction::ESpecialFunction::FogVolume ||
      specialFunction == CScriptSpecialFunction::ESpecialFunction::RadialDamage)
    return nullptr;

  return new CScriptSpecialFunction(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, specialFunction,
                                    str, f1, f2, f3, f4, zeus::skZero3f, zeus::skBlack, active1,
                                    CDamageInfo(), w2, w3, w4, w5, w6, w7);
}

CEntity* ScriptLoader::LoadSpankWeed(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 11, "SpankWeed"))
    return nullptr;
  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  in.readBool();
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();

  const CAnimationParameters& animParms = pInfo.GetAnimationParameters();
  if (!animParms.GetACSFile().IsValid())
    return nullptr;

  CModelData mData(CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), aHead.x40_scale,
                            animParms.GetInitialAnimation(), true));
  return new MP1::CSpankWeed(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, std::move(mData),
                             actParms, pInfo, f1, f2, f3, f4);
}

CEntity* ScriptLoader::LoadParasite(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 25, "Parasite"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  CPatterned::EFlavorType flavor = CPatterned::EFlavorType(in.readUint32Big());
  zeus::CTransform xf = LoadEditorTransform(in);
  zeus::CVector3f scale = zeus::CVector3f::ReadBig(in);

  std::pair<bool, u32> pcount = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pcount.first)
    return nullptr;

  CPatternedInfo pInfo(in, pcount.second);
  CActorParameters aParms = LoadActorParameters(in);
  float maxTelegraphReactDist = in.readFloatBig();
  float advanceWpRadius = in.readFloatBig();
  float f3 = in.readFloatBig();
  float alignAngVel = in.readFloatBig();
  float f5 = in.readFloatBig();
  float stuckTimeThreshold = in.readFloatBig();
  float collisionCloseMargin = in.readFloatBig();
  float parasiteSearchRadius = in.readFloatBig();
  float parasiteSeparationDist = in.readFloatBig();
  float parasiteSeparationWeight = in.readFloatBig();
  float parasiteAlignmentWeight = in.readFloatBig();
  float parasiteCohesionWeight = in.readFloatBig();
  float destinationSeekWeight = in.readFloatBig();
  float forwardMoveWeight = in.readFloatBig();
  float playerSeparationDist = in.readFloatBig();
  float playerSeparationWeight = in.readFloatBig();
  float playerObstructionMinDist = in.readFloatBig();
  bool disableMove = in.readBool();

  if (g_ResFactory->GetResourceTypeById(pInfo.GetAnimationParameters().GetACSFile()) != SBIG('ANCS'))
    return nullptr;
  const CAnimationParameters& animParms = pInfo.GetAnimationParameters();
  CModelData mData(
      CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), scale, animParms.GetInitialAnimation(), true));
  return new MP1::CParasite(mgr.AllocateUniqueId(), name, flavor, info, xf, std::move(mData), pInfo,
                            EBodyType::WallWalker, maxTelegraphReactDist, advanceWpRadius, f3, alignAngVel, f5,
                            stuckTimeThreshold, collisionCloseMargin, parasiteSearchRadius, parasiteSeparationDist,
                            parasiteSeparationWeight, parasiteAlignmentWeight, parasiteCohesionWeight,
                            destinationSeekWeight, forwardMoveWeight, playerSeparationDist, playerSeparationWeight,
                            playerObstructionMinDist, 0.f, disableMove, CWallWalker::EWalkerType::Parasite,
                            CDamageVulnerability::NormalVulnerabilty(), CDamageInfo(), -1, -1, -1, {}, {}, 0.f, aParms);
}

CEntity* ScriptLoader::LoadPlayerHint(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 6, "PlayerHint"))
    return nullptr;

  SActorHead aHead = LoadActorHead(in, mgr);
  bool active = in.readBool();
  u32 w2 = LoadParameterFlags(in);
  u32 w1 = in.readUint32Big();

  return new CScriptPlayerHint(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, active, w1, w2);
}

CEntity* ScriptLoader::LoadRipper(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  return nullptr;
}

CEntity* ScriptLoader::LoadPickupGenerator(CStateManager& mgr, CInputStream& in, int propCount,
                                           const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 4, "PickupGenerator"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f pos = zeus::CVector3f::ReadBig(in);
  bool active = in.readBool();
  float f1 = in.readFloatBig();
  return new CScriptPickupGenerator(mgr.AllocateUniqueId(), name, info, pos, f1, active);
}

CEntity* ScriptLoader::LoadAIKeyframe(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  return nullptr;
}

CEntity* ScriptLoader::LoadPointOfInterest(CStateManager& mgr, CInputStream& in, int propCount,
                                           const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 6, "PointOfInterest"))
    return nullptr;

  SActorHead aHead = LoadActorHead(in, mgr);
  bool active = in.readBool();
  CScannableParameters sParms = LoadScannableParameters(in);
  float f1 = in.readFloatBig();

  return new CScriptPointOfInterest(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, active, sParms,
                                    f1);
}

CEntity* ScriptLoader::LoadDrone(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  return nullptr;
}

CEntity* ScriptLoader::LoadMetroid(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, MP1::CMetroidData::GetNumProperties(), "Metroid"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  CPatterned::EFlavorType flavor = CPatterned::EFlavorType(in.readUint32Big());
  zeus::CTransform xf = LoadEditorTransform(in);
  zeus::CVector3f scale = zeus::CVector3f::ReadBig(in);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  MP1::CMetroidData metData(in);
  const CAnimationParameters& animParms = pInfo.GetAnimationParameters();
  if (!animParms.GetACSFile().IsValid() || flavor == CPatterned::EFlavorType::One)
    return nullptr;

  CModelData mData(
    CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), scale, animParms.GetInitialAnimation(), true));
  return new MP1::CMetroid(mgr.AllocateUniqueId(), name, flavor, info, xf, std::move(mData), pInfo, actParms, metData,
                           kInvalidUniqueId);
}

CEntity* ScriptLoader::LoadDebrisExtended(CStateManager& mgr, CInputStream& in, int propCount,
                                          const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 39, "DebrisExtended"))
    return nullptr;

  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);

  float linConeAngle = in.readFloatBig();
  float linMinMag = in.readFloatBig();
  float linMaxMag = in.readFloatBig();
  float angMinMag = in.readFloatBig();
  float angMaxMag = in.readFloatBig();
  float minDuration = in.readFloatBig();
  float maxDuration = in.readFloatBig();
  float colorInT = in.readFloatBig();
  float colorOutT = in.readFloatBig();

  zeus::CColor color = zeus::CColor::ReadRGBABig(in);
  zeus::CColor endsColor = zeus::CColor::ReadRGBABig(in);

  float scaleOutT = in.readFloatBig();

  zeus::CVector3f endScale = zeus::CVector3f::ReadBig(in);

  float restitution = in.readFloatBig();
  float downwardSpeed = in.readFloatBig();

  zeus::CVector3f localOffset = zeus::CVector3f::ReadBig(in);

  CAssetId model = in.readUint32Big();

  CActorParameters aParam = LoadActorParameters(in);

  CAssetId particle1 = in.readUint32Big();
  zeus::CVector3f particle1Scale = zeus::CVector3f::ReadBig(in);
  bool particle1GlobalTranslation = in.readBool();
  bool deferDeleteTillParticle1Done = in.readBool();
  CScriptDebris::EOrientationType particle1Or = CScriptDebris::EOrientationType(in.readUint32Big());

  CAssetId particle2 = in.readUint32Big();
  zeus::CVector3f particle2Scale = zeus::CVector3f::ReadBig(in);
  bool particle2GlobalTranslation = in.readBool();
  bool deferDeleteTillParticle2Done = in.readBool();
  CScriptDebris::EOrientationType particle2Or = CScriptDebris::EOrientationType(in.readUint32Big());

  CAssetId particle3 = in.readUint32Big();
  zeus::CVector3f particle3Scale = zeus::CVector3f::ReadBig(in); /* Not actually used, go figure */
  CScriptDebris::EOrientationType particle3Or = CScriptDebris::EOrientationType(in.readUint32Big());

  bool solid = in.readBool();
  bool dieOnProjectile = in.readBool();
  bool noBounce = in.readBool();
  bool active = in.readBool();

  CModelData modelData;
  if (g_ResFactory->GetResourceTypeById(model))
    modelData = CModelData(CStaticRes(model, aHead.x40_scale));

  return new CScriptDebris(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, std::move(modelData),
                           aParam, linConeAngle, linMinMag, linMaxMag, angMinMag, angMaxMag, minDuration, maxDuration,
                           colorInT, colorOutT, color, endsColor, scaleOutT, aHead.x40_scale, endScale, restitution,
                           downwardSpeed, localOffset, particle1, particle1Scale, particle1GlobalTranslation,
                           deferDeleteTillParticle1Done, particle1Or, particle2, particle2Scale,
                           particle2GlobalTranslation, deferDeleteTillParticle2Done, particle2Or, particle3,
                           particle2Scale, particle3Or, solid, dieOnProjectile, noBounce, active);
}

CEntity* ScriptLoader::LoadSteam(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 11, "Steam"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);

  zeus::CVector3f v1 = zeus::CVector3f::ReadBig(in);
  zeus::CVector3f v2 = zeus::CVector3f::ReadBig(in);

  CDamageInfo dInfo(in);

  zeus::CVector3f v3 = zeus::CVector3f::ReadBig(in);

  ETriggerFlags w1 = ETriggerFlags(in.readUint32Big());
  bool b1 = in.readBool();
  u32 w2 = in.readUint32Big();
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();
  bool b2 = in.readBool();

  zeus::CAABox aabb(-v2 * 0.5f, v2 * 0.5f);

  return new CScriptSteam(mgr.AllocateUniqueId(), name, info, v1, aabb, dInfo, v3, w1, b1, w2, f1, f2, f3, f4, b2);
}

CEntity* ScriptLoader::LoadRipple(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 4, "Ripple"))
    return nullptr;
  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f center = zeus::CVector3f::ReadBig(in);
  bool active = in.readBool();
  float mag = in.readFloatBig();
  return new CScriptRipple(mgr.AllocateUniqueId(), name, info, center, active, mag);
}

CEntity* ScriptLoader::LoadBallTrigger(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 9, "BallTrigger"))
    return nullptr;

  std::string name = in.readString();
  zeus::CVector3f pos = zeus::CVector3f::ReadBig(in);
  zeus::CVector3f scale = zeus::CVector3f::ReadBig(in);

  bool b1 = in.readBool();
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  zeus::CVector3f vec = zeus::CVector3f::ReadBig(in);
  bool b2 = in.readBool();
  return new CScriptBallTrigger(mgr.AllocateUniqueId(), name, info, pos, scale, b1, f1, f2, f3, vec, b2);
}

CEntity* ScriptLoader::LoadTargetingPoint(CStateManager& mgr, CInputStream& in, int propCount,
                                          const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 4, "TargetingPoint"))
    return nullptr;

  SActorHead aHead = LoadActorHead(in, mgr);
  bool active = in.readBool();

  return new CScriptTargetingPoint(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, active);
}

CEntity* ScriptLoader::LoadEMPulse(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 12, "EMPulse"))
    return nullptr;

  SActorHead actorHead = LoadActorHead(in, mgr);
  bool active = in.readBool();
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();
  float f5 = in.readFloatBig();
  float f6 = in.readFloatBig();
  float f7 = in.readFloatBig();
  CAssetId particleId(in);

  return new CScriptEMPulse(mgr.AllocateUniqueId(), actorHead.x0_name, info, actorHead.x10_transform, active, f1, f2,
                            f3, f4, f5, f6, f7, particleId);
}

CEntity* ScriptLoader::LoadIceSheegoth(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  return nullptr;
}

CEntity* ScriptLoader::LoadPlayerActor(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 19, "PlayerActor"))
    return nullptr;

  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);
  zeus::CVector3f extents = zeus::CVector3f::ReadBig(in);
  zeus::CVector3f offset = zeus::CVector3f::ReadBig(in);
  float mass = in.readFloatBig();
  float zMomentum = in.readFloatBig();
  CHealthInfo hInfo(in);
  CDamageVulnerability dVuln(in);
  in.readUint32Big();
  CAnimationParameters animParms = LoadAnimationParameters(in);
  CActorParameters actParms = LoadActorParameters(in);
  bool loop = in.readBool();
  bool snow = in.readBool();
  bool solid = in.readBool();
  bool active = in.readBool();
  u32 flags = LoadParameterFlags(in);
  CPlayerState::EBeamId beam = CPlayerState::EBeamId(in.readUint32Big() - 1);

  FourCC fcc = g_ResFactory->GetResourceTypeById(animParms.GetACSFile());
  if (!fcc || fcc != SBIG('ANCS'))
    return nullptr;

  zeus::CAABox aabox = GetCollisionBox(mgr, info.GetAreaId(), extents, offset);
  CMaterialList list;
  if (snow)
    list.Add(EMaterialTypes::Snow);

  if (solid)
    list.Add(EMaterialTypes::Solid);

  if ((extents.x() < 0.f || extents.y() < 0.f || extents.z() < 0.f) || extents.isZero())
    aabox = zeus::CAABox(-.5f, 0.5f);

  return new CScriptPlayerActor(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform,
                                CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), aHead.x40_scale,
                                         animParms.GetInitialAnimation(), loop),
                                CModelData::CModelDataNull(), aabox, true, list, mass, zMomentum, hInfo, dVuln,
                                actParms, loop, active, flags, beam);
}

CEntity* ScriptLoader::LoadFlaahgra(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  return nullptr;
}

CEntity* ScriptLoader::LoadAreaAttributes(CStateManager& mgr, CInputStream& in, int propCount,
                                          const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 9, "AreaAttributes"))
    return nullptr;

  bool load = in.readUint32Big() != 0;
  if (!load)
    return nullptr;

  bool showSkybox = in.readBool();
  EEnvFxType fxType = EEnvFxType(in.readUint32Big());
  float envFxDensity = in.readFloatBig();
  float thermalHeat = in.readFloatBig();
  float xrayFogDistance = in.readFloatBig();
  float worldLightingLevel = in.readFloatBig();
  CAssetId skybox = in.readUint32Big();
  EPhazonType phazonType = EPhazonType(in.readUint32Big());

  return new CScriptAreaAttributes(mgr.AllocateUniqueId(), info, showSkybox, fxType, envFxDensity, thermalHeat,
                                   xrayFogDistance, worldLightingLevel, skybox, phazonType);
}

CEntity* ScriptLoader::LoadFishCloud(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 36, "FishCloud"))
    return nullptr;
  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);
  bool b1 = in.readBool();
  CAssetId w1(in);
  CAnimationParameters animParms(in);
  u32 w5 = u32(in.readFloatBig());
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();
  float f5 = in.readFloatBig();
  float f6 = in.readFloatBig();
  float f7 = in.readFloatBig();
  float f8 = in.readFloatBig();
  float f9 = in.readFloatBig();
  float f10 = in.readFloatBig();
  float f11 = in.readFloatBig();
  float f12 = in.readFloatBig();
  float f13 = in.readFloatBig();
  u32 w6 = in.readUint32Big();

  if (!g_ResFactory->GetResourceTypeById(w1))
    return nullptr;

  zeus::CColor col = zeus::CColor::ReadRGBABig(in);
  bool b2 = in.readBool();
  float f14 = in.readFloatBig();
  CAssetId w7 = in.readUint32Big();
  u32 w8 = in.readUint32Big();
  CAssetId w9 = in.readUint32Big();
  u32 w10 = in.readUint32Big();
  CAssetId w11 = in.readUint32Big();
  u32 w12 = in.readUint32Big();
  CAssetId w13 = in.readUint32Big();
  u32 w14 = in.readUint32Big();
  u32 w15 = in.readUint32Big();
  bool b3 = in.readBool();
  bool b4 = in.readBool();

  CModelData mData(CStaticRes(w1, zeus::skOne3f));
  CAnimRes animRes(animParms.GetACSFile(), animParms.GetCharacter(), zeus::skOne3f,
                   animParms.GetInitialAnimation(), true);
  return new CFishCloud(mgr.AllocateUniqueId(), b1, aHead.x0_name, info, aHead.x40_scale, aHead.x10_transform,
                        std::move(mData), animRes, w5, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, w6, col,
                        b2, f14, w7, w8, w9, w10, w11, w12, w13, w14, w15, b3, b4);
}

CEntity* ScriptLoader::LoadFishCloudModifier(CStateManager& mgr, CInputStream& in, int propCount,
                                             const CEntityInfo& info) {
  if (propCount < 6 || !EnsurePropertyCount(propCount, 7, "FishCloudModifier"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f vec = zeus::CVector3f::ReadBig(in);
  bool b1 = in.readBool();
  bool b2 = in.readBool();
  bool b3 = propCount > 6 ? in.readBool() : false;
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  return new CFishCloudModifier(mgr.AllocateUniqueId(), b1, name, info, vec, b2, b3, f1, f2);
}

CEntity* ScriptLoader::LoadVisorFlare(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 14, "VisorFlare"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f pos = zeus::CVector3f::ReadBig(in);
  bool b1 = in.readBool();
  CVisorFlare::EBlendMode w1 = CVisorFlare::EBlendMode(in.readUint32Big());
  bool b2 = in.readBool();
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  u32 w2 = in.readUint32Big();
  std::vector<CVisorFlare::CFlareDef> flares;
  flares.reserve(5);
  for (int i = 0; i < 5; ++i)
    if (auto flare = CVisorFlare::LoadFlareDef(in))
      flares.push_back(*flare);

  return new CScriptVisorFlare(mgr.AllocateUniqueId(), name, info, b1, pos, w1, b2, f1, f2, f3, 2, w2, flares);
}

CEntity* ScriptLoader::LoadWorldTeleporter(CStateManager& mgr, CInputStream& in, int propCount,
                                           const CEntityInfo& info) {
  if (propCount < 4 || propCount > 21) {
    Log.report(logvisor::Warning, "Incorrect number of props for WorldTeleporter");
    return nullptr;
  }

  std::string name = mgr.HashInstanceName(in);
  bool active = in.readBool();
  CAssetId worldId = in.readUint32Big();
  CAssetId areaId = in.readUint32Big();

  if (propCount == 4)
    return new CScriptWorldTeleporter(mgr.AllocateUniqueId(), name, info, active, worldId, areaId);

  CAnimationParameters animParms = LoadAnimationParameters(in);
  zeus::CVector3f playerScale = zeus::CVector3f::ReadBig(in);
  CAssetId platformModel = in.readUint32Big();
  zeus::CVector3f platformScale = zeus::CVector3f::ReadBig(in);
  CAssetId backgroundModel = in.readUint32Big();
  zeus::CVector3f backgroundScale = zeus::CVector3f::ReadBig(in);
  bool upElevator = in.readBool();

  s16 elevatorSound = (propCount < 12 ? s16(-1) : s16(in.readUint32Big()));
  u8 volume = (propCount < 13 ? u8(127) : u8(in.readUint32Big()));
  u8 panning = (propCount < 14 ? u8(64) : u8(in.readUint32Big()));
  bool showText = (propCount < 15 ? false : in.readBool());
  CAssetId fontId = (propCount < 16 ? CAssetId() : in.readUint32Big());
  CAssetId stringId = (propCount < 17 ? CAssetId() : in.readUint32Big());
  bool fadeWhite = (propCount < 18 ? false : in.readBool());
  float charFadeInTime = (propCount < 19 ? 0.1f : in.readFloatBig());
  float charsPerSecond = (propCount < 20 ? 16.f : in.readFloatBig());
  float showDelay = (propCount < 21 ? 0.f : in.readFloatBig());

  if (showText)
    return new CScriptWorldTeleporter(mgr.AllocateUniqueId(), name, info, active, worldId, areaId, elevatorSound,
                                      volume, panning, fontId, stringId, fadeWhite, charFadeInTime, charsPerSecond,
                                      showDelay);

  return new CScriptWorldTeleporter(mgr.AllocateUniqueId(), name, info, active, worldId, areaId, animParms.GetACSFile(),
                                    animParms.GetCharacter(), animParms.GetInitialAnimation(), playerScale,
                                    platformModel, platformScale, backgroundModel, backgroundScale, upElevator,
                                    elevatorSound, volume, panning);
}

CEntity* ScriptLoader::LoadVisorGoo(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 11, "VisorGoo"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f position;
  position.readBig(in);
  zeus::CTransform xf = zeus::CTransform::Translate(position);
  CAssetId particle(in);
  CAssetId electric(in);
  float minDist = in.readFloatBig();
  float maxDist = in.readFloatBig();
  float nearProb = in.readFloatBig();
  float farProb = in.readFloatBig();
  zeus::CColor color;
  color.readRGBABig(in);
  s32 sfx = in.readInt32Big();
  bool forceShow = in.readBool();

  if (!particle.IsValid() && !electric.IsValid())
    return nullptr;
  return new CScriptVisorGoo(mgr.AllocateUniqueId(), name, info, xf, particle, electric, minDist, maxDist, nearProb,
                             farProb, color, sfx, forceShow, false);
}

CEntity* ScriptLoader::LoadJellyZap(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 20, "JellyZap"))
    return nullptr;

  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  CDamageInfo dInfo(in);
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();
  float f5 = in.readFloatBig();
  float f6 = in.readFloatBig();
  float f7 = in.readFloatBig();
  float f8 = in.readFloatBig();
  float f9 = in.readFloatBig();
  float f10 = in.readFloatBig();
  float f11 = in.readFloatBig();
  float f12 = in.readFloatBig();
  bool b1 = in.readBool();

  const CAnimationParameters& animParms = pInfo.GetAnimationParameters();
  if (g_ResFactory->GetResourceTypeById(animParms.GetACSFile()) != SBIG('ANCS'))
    return nullptr;

  CModelData mData(CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), aHead.x40_scale,
                            animParms.GetInitialAnimation(), true));
  return new MP1::CJellyZap(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, std::move(mData), dInfo,
                            b1, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, pInfo, actParms);
}

CEntity* ScriptLoader::LoadControllerAction(CStateManager& mgr, CInputStream& in, int propCount,
                                            const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 4, "ControllerAction") || propCount > 6)
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  bool active = in.readBool();
  ControlMapper::ECommands w1 = ControlMapper::ECommands(in.readUint32Big());
  bool b1 = false;
  u32 w2 = 0;
  if (propCount == 6) {
    b1 = in.readBool();
    w2 = in.readUint32Big();
  }
  bool b2 = in.readBool();

  return new CScriptControllerAction(mgr.AllocateUniqueId(), name, info, active, w1, b1, w2, b2);
}

CEntity* ScriptLoader::LoadSwitch(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 4, "Switch"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  bool active = in.readBool();
  bool b2 = in.readBool();
  bool b3 = in.readBool();

  return new CScriptSwitch(mgr.AllocateUniqueId(), name, info, active, b2, b3);
}

CEntity* ScriptLoader::LoadPlayerStateChange(CStateManager& mgr, CInputStream& in, int propCount,
                                             const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 7, "PlayerStateChange"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  bool active = in.readBool();
  s32 itemType = in.readUint32Big();
  s32 itemCount = in.readInt32Big();
  s32 itemCapacity = in.readInt32Big();
  CScriptPlayerStateChange::EControl ctrl = CScriptPlayerStateChange::EControl(in.readUint32Big());
  CScriptPlayerStateChange::EControlCommandOption ctrlCmdOpt =
      CScriptPlayerStateChange::EControlCommandOption(in.readUint32Big());
  return new CScriptPlayerStateChange(mgr.AllocateUniqueId(), name, info, active, itemType, itemCount, itemCapacity,
                                      ctrl, ctrlCmdOpt);
}

CEntity* ScriptLoader::LoadThardus(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  return nullptr;
}

CEntity* ScriptLoader::LoadWallCrawlerSwarm(CStateManager& mgr, CInputStream& in, int propCount,
                                            const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 39, "WallCrawlerSwarm"))
    return nullptr;

  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);
  bool active = in.readBool();
  CActorParameters aParams = LoadActorParameters(in);
  u32 w1 = in.readUint32Big();
  u32 w2 = in.readUint32Big();
  u32 w3 = in.readUint32Big();
  u32 w4 = in.readUint32Big();
  u32 w5 = in.readUint32Big();
  u32 w6 = in.readUint32Big();
  u32 w7 = in.readUint32Big();
  u32 w8 = in.readUint32Big();
  u32 w9 = in.readUint32Big();
  u32 w10 = in.readUint32Big();
  CDamageInfo dInfo1(in);
  float f1 = in.readFloatBig();
  CDamageInfo dInfo2(in);
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();
  float f5 = in.readFloatBig();
  u32 w11 = in.readUint32Big();
  u32 w12 = in.readUint32Big();
  float f6 = in.readFloatBig();
  float f7 = in.readFloatBig();
  float f8 = in.readFloatBig();
  float f9 = in.readFloatBig();
  float f10 = in.readFloatBig();
  float f11 = in.readFloatBig();
  float f12 = in.readFloatBig();
  float f13 = in.readFloatBig();
  u32 w13 = in.readUint32Big();
  float f14 = in.readFloatBig();
  float f15 = in.readFloatBig();
  float f16 = in.readFloatBig();
  CHealthInfo hInfo(in);
  CDamageVulnerability dVulns(in);
  u32 w14 = in.readUint32Big();
  u32 w15 = in.readUint32Big();

  return new CWallCrawlerSwarm(mgr.AllocateUniqueId(), active, aHead.x0_name, info, aHead.x40_scale,
                               aHead.x10_transform, w1, CAnimRes(w2, w3, zeus::CVector3f(1.5f), w4, true), w5, w6, w7,
                               w8, w9, w10, dInfo1, dInfo2, f1, f2, f3, f4, w11, w12, f5, f6, f7, f8, f9, f10, f11, f12,
                               f13, w13, f14, f15, f16, hInfo, dVulns, w14, w15, aParams);
}

CEntity* ScriptLoader::LoadAiJumpPoint(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 5, "AiJumpPoint"))
    return nullptr;

  SActorHead aHead = LoadActorHead(in, mgr);
  bool active = in.readBool();
  float f1 = in.readFloat();

  return new CScriptAiJumpPoint(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, active, f1);
}

CEntity* ScriptLoader::LoadFlaahgraTentacle(CStateManager& mgr, CInputStream& in, int propCount,
                                            const CEntityInfo& info) {
  return nullptr;
}

CEntity* ScriptLoader::LoadRoomAcoustics(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 32, "RoomAcoustics"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  bool a = in.readBool();
  u32 b = in.readUint32Big();
  bool c = in.readBool();
  bool d = in.readBool();
  float e = in.readFloatBig();
  float f = in.readFloatBig();
  float g = in.readFloatBig();
  float h = in.readFloatBig();
  float i = in.readFloatBig();
  float j = in.readFloatBig();
  bool k = in.readBool();
  float l = in.readFloatBig();
  float m = in.readFloatBig();
  float n = in.readFloatBig();
  bool o = in.readBool();
  bool p = in.readBool();
  float q = in.readFloatBig();
  float r = in.readFloatBig();
  float s = in.readFloatBig();
  float t = in.readFloatBig();
  float u = in.readFloatBig();
  bool v = in.readBool();
  u32 w = in.readUint32Big();
  u32 x = in.readUint32Big();
  u32 y = in.readUint32Big();
  u32 z = in.readUint32Big();
  u32 _a = in.readUint32Big();
  u32 _b = in.readUint32Big();
  u32 _c = in.readUint32Big();
  u32 _d = in.readUint32Big();
  u32 _e = in.readUint32Big();

  return new CScriptRoomAcoustics(mgr.AllocateUniqueId(), name, info, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q,
                                  r, s, t, u, v, w, x, y, z, _a, _b, _c, _d, _e);
}

CEntity* ScriptLoader::LoadColorModulate(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 12, "ColorModulate"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CColor colorA;
  colorA.readRGBABig(in);
  zeus::CColor colorB;
  colorB.readRGBABig(in);
  CScriptColorModulate::EBlendMode blendMode = CScriptColorModulate::EBlendMode(in.readUint32Big());
  float timeA2B = in.readFloatBig();
  float timeB2A = in.readFloatBig();
  bool doReverse = in.readBool();
  bool resetTargetWhenDone = in.readBool();
  bool depthCompare = in.readBool();
  bool depthUpdate = in.readBool();
  bool depthBackwards = in.readBool();
  bool active = in.readBool();
  return new CScriptColorModulate(mgr.AllocateUniqueId(), name, info, colorA, colorB, blendMode, timeA2B, timeB2A,
                                  doReverse, resetTargetWhenDone, depthCompare, depthUpdate, depthBackwards, active);
}

CEntity* ScriptLoader::LoadThardusRockProjectile(CStateManager& mgr, CInputStream& in, int propCount,
                                                 const CEntityInfo& info) {
  return nullptr;
}

CEntity* ScriptLoader::LoadMidi(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 6, "Midi"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  bool active = in.readBool();
  u32 csng = in.readUint32Big();
  float fadeIn = in.readFloatBig();
  float fadeOut = in.readFloatBig();
  u32 vol = in.readUint32Big();
  return new CScriptMidi(mgr.AllocateUniqueId(), info, name, active, csng, fadeIn, fadeOut, vol);
}

CEntity* ScriptLoader::LoadStreamedAudio(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 9, "StreamedAudio"))
    return nullptr;

  const std::string name = mgr.HashInstanceName(in);
  bool active = in.readBool();
  std::string fileName = in.readString();
  bool noStopOnDeactivate = in.readBool();
  float fadeIn = in.readFloatBig();
  float fadeOut = in.readFloatBig();
  u32 volume = in.readUint32Big();
  u32 oneShot = in.readUint32Big();
  bool music = in.readBool();

  return new CScriptStreamedMusic(mgr.AllocateUniqueId(), info, name, active, fileName, noStopOnDeactivate, fadeIn,
                                  fadeOut, volume, !oneShot, music);
}

CEntity* ScriptLoader::LoadRepulsor(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 4, "Repulsor"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f center = in.readVec3fBig();
  bool active = in.readBool();
  float radius = in.readFloatBig();

  return new CRepulsor(mgr.AllocateUniqueId(), active, name, info, center, radius);
}

CEntity* ScriptLoader::LoadGunTurret(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, CScriptGunTurretData::GetMinProperties(), "GunTurret"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  CScriptGunTurret::ETurretComponent component = CScriptGunTurret::ETurretComponent(in.readUint32Big());
  zeus::CTransform xf = LoadEditorTransform(in);
  zeus::CVector3f scale = zeus::CVector3f::ReadBig(in);
  zeus::CVector3f collisionExtent = zeus::CVector3f::ReadBig(in);
  zeus::CVector3f collisionOffset = zeus::CVector3f::ReadBig(in);
  CAnimationParameters animParms = LoadAnimationParameters(in);
  CActorParameters actParms = LoadActorParameters(in);
  CHealthInfo hInfo(in);
  CDamageVulnerability dVuln(in);
  CScriptGunTurretData turretData(in, propCount);

  if (!g_ResFactory->GetResourceTypeById(animParms.GetACSFile()))
    return nullptr;

  CModelData mData(
      CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), scale, animParms.GetInitialAnimation(), true));
  zeus::CAABox aabb = GetCollisionBox(mgr, info.GetAreaId(), collisionExtent, collisionOffset);

  if ((collisionExtent.x() < 0.f || collisionExtent.y() < 0.f || collisionExtent.z() < 0.f) || collisionExtent.isZero())
    aabb = mData.GetBounds(xf.getRotation());

  return new CScriptGunTurret(mgr.AllocateUniqueId(), name, component, info, xf, std::move(mData), aabb, hInfo, dVuln,
                              actParms, turretData);
}

CEntity* ScriptLoader::LoadFogVolume(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 7, "FogVolume"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f center = zeus::CVector3f::ReadBig(in);
  zeus::CVector3f volume = zeus::CVector3f::ReadBig(in);
  float flickerSpeed = in.readFloatBig();
  float f2 = in.readFloatBig();
  zeus::CColor fogColor = zeus::CColor::ReadRGBABig(in);
  bool active = in.readBool();

  volume.x() = std::fabs(volume.x());
  volume.y() = std::fabs(volume.y());
  volume.z() = std::fabs(volume.z());

  return new CScriptSpecialFunction(mgr.AllocateUniqueId(), name, info, ConvertEditorEulerToTransform4f(center, {}),
                                    CScriptSpecialFunction::ESpecialFunction::FogVolume, "", flickerSpeed, f2, 0.f, 0.f,
                                    volume, fogColor, active, CDamageInfo(), -1, -1, CPlayerState::EItemType::Invalid,
                                    -1, -1, -1);
}

CEntity* ScriptLoader::LoadBabygoth(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 33, "Babygoth"))
    return nullptr;

  SScaledActorHead actHead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  MP1::CBabygothData babyData(in);

  if (!pInfo.GetAnimationParameters().GetACSFile().IsValid())
    return nullptr;

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            actHead.x40_scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));
  return new MP1::CBabygoth(mgr.AllocateUniqueId(), actHead.x0_name, info, actHead.x10_transform, std::move(mData),
                            pInfo, actParms, babyData);
}

CEntity* ScriptLoader::LoadEyeball(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 21, "Eyeball"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  CPatterned::EFlavorType flavor = CPatterned::EFlavorType(in.readUint32Big());
  zeus::CTransform xf = LoadEditorTransform(in);
  zeus::CVector3f scale = zeus::CVector3f::ReadBig(in);

  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  float attackDelay = in.readFloatBig();
  float attackStartTime = in.readFloatBig();
  CAssetId wpsc(in);
  CDamageInfo dInfo(in);
  CAssetId beamContactFxId(in);
  CAssetId beamPulseFxId(in);
  CAssetId beamTextureId(in);
  CAssetId beamGlowTextureId(in);
  u32 anim0 = in.readUint32Big();
  u32 anim1 = in.readUint32Big();
  u32 anim2 = in.readUint32Big();
  u32 anim3 = in.readUint32Big();
  u32 beamSfx = in.readUint32Big();

  if (g_ResFactory->GetResourceTypeById(pInfo.GetAnimationParameters().GetACSFile()) != SBIG('ANCS'))
    return nullptr;

  bool attackDisabled = in.readBool();

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));

  return new MP1::CEyeball(mgr.AllocateUniqueId(), name, flavor, info, xf, std::move(mData), pInfo, attackDelay,
                           attackStartTime, wpsc, dInfo, beamContactFxId, beamPulseFxId, beamTextureId,
                           beamGlowTextureId, anim0, anim1, anim2, anim3, beamSfx, attackDisabled, actParms);
}

CEntity* ScriptLoader::LoadRadialDamage(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 5, "RadialDamage"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f center = zeus::CVector3f::ReadBig(in);
  bool active = in.readBool();
  CDamageInfo dInfo(in);
  float radius = in.readFloatBig();
  zeus::CTransform xf = ConvertEditorEulerToTransform4f(zeus::skZero3f, center);

  return new CScriptSpecialFunction(mgr.AllocateUniqueId(), name, info, xf,
                                    CScriptSpecialFunction::ESpecialFunction::RadialDamage, "", radius, 0.f, 0.f, 0.f,
                                    zeus::skZero3f, zeus::skBlack, active, dInfo, -1, -1,
                                    CPlayerState::EItemType::Invalid, -1, -1, -1);
}

CEntity* ScriptLoader::LoadCameraPitchVolume(CStateManager& mgr, CInputStream& in, int propCount,
                                             const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 8, "CameraPitchVolume"))
    return nullptr;

  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);
  bool active = in.readBool();
  zeus::CRelAngle f1 = zeus::CRelAngle::FromDegrees(in.readFloatBig());
  zeus::CRelAngle f2 = zeus::CRelAngle::FromDegrees(in.readFloatBig());
  float f3 = in.readFloatBig();

  return new CScriptCameraPitchVolume(mgr.AllocateUniqueId(), active, aHead.x0_name, info, aHead.x40_scale,
                                      aHead.x10_transform, f1, f2, f3);
}

CEntity* ScriptLoader::LoadEnvFxDensityController(CStateManager& mgr, CInputStream& in, int propCount,
                                                  const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 4, "EnvFxDensityController"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  bool active = in.readBool();
  float density = in.readFloatBig();
  u32 w1 = in.readUint32Big();

  return new CScriptSpecialFunction(mgr.AllocateUniqueId(), name, info, zeus::CTransform(),
                                    CScriptSpecialFunction::ESpecialFunction::EnvFxDensityController, "", density, w1,
                                    0.f, 0.f, zeus::skZero3f, zeus::skBlack, active, CDamageInfo(), -1,
                                    -1, CPlayerState::EItemType::Invalid, -1, -1, -1);
}

CEntity* ScriptLoader::LoadMagdolite(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 22, "Magdolite"))
    return nullptr;

  SScaledActorHead actorHead = LoadScaledActorHead(in, mgr);

  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actorParameters = LoadActorParameters(in);

  if (!pInfo.GetAnimationParameters().GetACSFile().IsValid())
    return nullptr;

  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  CDamageInfo damageInfo1(in);
  CDamageInfo damageInfo2(in);
  CDamageVulnerability damageVulnerability1(in);
  CDamageVulnerability damageVulnerability2(in);
  CAssetId modelId(in);
  CAssetId skinId(in);
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();
  float f5 = in.readFloatBig();
  float f6 = in.readFloatBig();
  MP1::CMagdolite::CMagdoliteData magData(in);
  float f7 = in.readFloatBig();
  float f8 = in.readFloatBig();
  float f9 = in.readFloatBig();

  CModelData modelData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(),
                                pInfo.GetAnimationParameters().GetCharacter(), actorHead.x40_scale,
                                pInfo.GetAnimationParameters().GetInitialAnimation(), true));

  return new MP1::CMagdolite(mgr.AllocateUniqueId(), actorHead.x0_name, info, actorHead.x10_transform,
                             std::move(modelData), pInfo, actorParameters, f1, f2, damageInfo1, damageInfo2,
                             damageVulnerability1, damageVulnerability2, modelId, skinId, f3, f4, f5, f6, magData, f7,
                             f8, f9);
}

CEntity* ScriptLoader::LoadTeamAIMgr(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 8, "TeamAiMgr"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  CTeamAiData data(in, propCount);
  return new CTeamAiMgr(mgr.AllocateUniqueId(), name, info, data);
}

CEntity* ScriptLoader::LoadSnakeWeedSwarm(CStateManager& mgr, CInputStream& in, int propCount,
                                          const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 25, "SnakeWeedSwarm") || propCount > 29)
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f pos = zeus::CVector3f::ReadBig(in);
  zeus::CVector3f scale = zeus::CVector3f::ReadBig(in);
  bool active = in.readBool();
  CAnimationParameters animParms = LoadAnimationParameters(in);
  CActorParameters actParms = LoadActorParameters(in);
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();
  float f5 = in.readFloatBig();
  float f6 = in.readFloatBig();
  float f7 = in.readFloatBig();
  float f8 = in.readFloatBig();
  float f9 = in.readFloatBig();
  float f10 = in.readFloatBig();
  float f11 = in.readFloatBig();
  float f12 = in.readFloatBig();
  float f13 = in.readFloatBig();
  float f14 = in.readFloatBig();
  CDamageInfo dInfo(in);
  float f15 = in.readFloatBig();
  u32 w4 = in.readUint32Big();
  u32 w5 = in.readUint32Big();
  u32 w6 = in.readUint32Big();
  u32 w7 = (propCount < 29 ? -1 : in.readUint32Big());
  u32 w8 = (propCount < 29 ? -1 : in.readUint32Big());
  u32 w9 = (propCount < 29 ? -1 : in.readUint32Big());
  u32 f16 = (propCount < 29 ? 0.f : in.readFloatBig());

  return new CSnakeWeedSwarm(mgr.AllocateUniqueId(), active, name, info, pos, scale, animParms, actParms, f1, f2, f3,
                             f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, dInfo, f15, w4, w5, w6, w7, w8, w9, f16);
}

CEntity* ScriptLoader::LoadActorContraption(CStateManager& mgr, CInputStream& in, int propCount,
                                            const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 15, "ActorContraption"))
    return nullptr;

  SScaledActorHead head = LoadScaledActorHead(in, mgr);
  zeus::CVector3f collisionExtent = zeus::CVector3f::ReadBig(in);
  zeus::CVector3f collisionOrigin = zeus::CVector3f::ReadBig(in);
  float mass = in.readFloatBig();
  float zMomentum = in.readFloatBig();
  CHealthInfo hInfo(in);
  CDamageVulnerability dVuln(in);
  CAnimationParameters animParams(in);
  CActorParameters actParams = LoadActorParameters(in);
  CAssetId flameFxId = in.readUint32Big();
  CDamageInfo dInfo(in);
  bool active = in.readBool();

  if (!g_ResFactory->GetResourceTypeById(animParams.GetACSFile()))
    return nullptr;

  zeus::CAABox aabb = GetCollisionBox(mgr, info.GetAreaId(), collisionExtent, collisionOrigin);
  CMaterialList list;
  list.Add(EMaterialTypes::Immovable);
  list.Add(EMaterialTypes::Solid);

  CModelData data(CAnimRes(animParams.GetACSFile(), animParams.GetCharacter(), head.x40_scale,
                           animParams.GetInitialAnimation(), true));

  if ((collisionExtent.x() < 0.f || collisionExtent.y() < 0.f || collisionExtent.z() < 0.f) || collisionExtent.isZero())
    aabb = data.GetBounds(head.x10_transform.getRotation());

  return new MP1::CActorContraption(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, std::move(data),
                                    aabb, list, mass, zMomentum, hInfo, dVuln, actParams, flameFxId, dInfo, active);
}

CEntity* ScriptLoader::LoadOculus(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  return nullptr;
}

CEntity* ScriptLoader::LoadGeemer(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 16, "Geemer"))
    return nullptr;
  SScaledActorHead actHead = LoadScaledActorHead(in, mgr);

  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);

  if (pInfo.GetAnimationParameters().GetACSFile() == CAssetId())
    return nullptr;

  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();
  float f5 = in.readFloatBig();
  float f6 = in.readFloatBig();
  float f7 = in.readFloatBig();
  u16 sId1 = in.readUint32Big() & 0xFFFF;
  u16 sId2 = in.readUint32Big() & 0xFFFF;
  u16 sId3 = in.readUint32Big() & 0xFFFF;

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            actHead.x40_scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));

  return new MP1::CParasite(mgr.AllocateUniqueId(), actHead.x0_name, CPatterned::EFlavorType::Zero, info,
                            actHead.x10_transform, std::move(mData), pInfo, EBodyType::WallWalker, 0.f, f1, f2, f3, f4,
                            0.2f, 0.4f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, f7, 0.f, 0.f, f5, f6, false,
                            CWallWalker::EWalkerType::Geemer, CDamageVulnerability::NormalVulnerabilty(), CDamageInfo(),
                            sId1, sId2, sId3, -1, -1, 0.f, actParms);
}

CEntity* ScriptLoader::LoadSpindleCamera(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 24, "SpindleCamera"))
    return nullptr;

  SActorHead aHead = LoadActorHead(in, mgr);
  bool active = in.readBool();
  u32 flags = LoadParameterFlags(in);
  float hintToCamDistMin = in.readFloatBig();
  float hintToCamDistMax = in.readFloatBig();
  float hintToCamVOffMin = in.readFloatBig();
  float hintToCamVOffMax = in.readFloatBig();

  SSpindleProperty targetHintToCamDeltaAngleVel(in);
  targetHintToCamDeltaAngleVel.FixupAngles();
  SSpindleProperty deltaAngleScaleWithCamDist(in);
  SSpindleProperty hintToCamDist(in);
  SSpindleProperty distOffsetFromBallDist(in);
  SSpindleProperty hintBallToCamAzimuth(in);
  hintBallToCamAzimuth.FixupAngles();
  SSpindleProperty unused(in);
  unused.FixupAngles();
  SSpindleProperty maxHintBallToCamAzimuth(in);
  maxHintBallToCamAzimuth.FixupAngles();
  SSpindleProperty camLookRelAzimuth(in);
  camLookRelAzimuth.FixupAngles();
  SSpindleProperty lookPosZOffset(in);
  SSpindleProperty camPosZOffset(in);
  SSpindleProperty clampedAzimuthFromHintDir(in);
  clampedAzimuthFromHintDir.FixupAngles();
  SSpindleProperty dampingAzimuthSpeed(in);
  dampingAzimuthSpeed.FixupAngles();
  SSpindleProperty targetHintToCamDeltaAngleVelRange(in);
  targetHintToCamDeltaAngleVelRange.FixupAngles();
  SSpindleProperty deleteHintBallDist(in);
  SSpindleProperty recoverClampedAzimuthFromHintDir(in);
  recoverClampedAzimuthFromHintDir.FixupAngles();

  return new CScriptSpindleCamera(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, active, flags,
                                  hintToCamDistMin, hintToCamDistMax, hintToCamVOffMin, hintToCamVOffMax,
                                  targetHintToCamDeltaAngleVel, deltaAngleScaleWithCamDist, hintToCamDist,
                                  distOffsetFromBallDist, hintBallToCamAzimuth, unused, maxHintBallToCamAzimuth,
                                  camLookRelAzimuth, lookPosZOffset, camPosZOffset, clampedAzimuthFromHintDir,
                                  dampingAzimuthSpeed, targetHintToCamDeltaAngleVelRange, deleteHintBallDist,
                                  recoverClampedAzimuthFromHintDir);
}

CEntity* ScriptLoader::LoadAtomicAlpha(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 14, "AtomicAlpha"))
    return nullptr;

  SScaledActorHead actHead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);

  CAssetId wpsc(in);
  CAssetId model(in);
  CDamageInfo dInfo(in);
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  bool b1 = in.readBool();
  bool b2 = in.readBool();

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            actHead.x40_scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));

  return new MP1::CAtomicAlpha(mgr.AllocateUniqueId(), actHead.x0_name, info, actHead.x10_transform, std::move(mData),
                               actParms, pInfo, wpsc, dInfo, f1, f2, f3, model, b1, b2);
}

CEntity* ScriptLoader::LoadCameraHintTrigger(CStateManager& mgr, CInputStream& in, int propCount,
                                             const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 7, "CameraHintTrigger"))
    return nullptr;

  SActorHead aHead = LoadActorHead(in, mgr);
  zeus::CVector3f scale = 0.5f * zeus::CVector3f::ReadBig(in);
  bool active = in.readBool();
  bool b2 = in.readBool();
  bool b3 = in.readBool();

  zeus::CTransform xfRot = aHead.x10_transform.getRotation();
  if (xfRot == zeus::CTransform())
    return new CScriptTrigger(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform.origin,
                              zeus::CAABox(-scale, scale), CDamageInfo(), zeus::skZero3f,
                              ETriggerFlags::DetectPlayer, active, b2, b3);

  return new CScriptCameraHintTrigger(mgr.AllocateUniqueId(), active, aHead.x0_name, info, scale, aHead.x10_transform,
                                      b2, b3);
}

CEntity* ScriptLoader::LoadRumbleEffect(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 6, "RumbleEffect"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f position;
  position.readBig(in);
  bool active = in.readBool();
  float f1 = in.readFloatBig();
  u32 w1 = in.readUint32Big();
  u32 pFlags = LoadParameterFlags(in);

  return new CScriptSpecialFunction(
      mgr.AllocateUniqueId(), name, info, ConvertEditorEulerToTransform4f(zeus::skZero3f, position),
      CScriptSpecialFunction::ESpecialFunction::RumbleEffect, "", f1, w1, pFlags, 0.f, zeus::skZero3f,
      zeus::skBlack, active, {}, {}, {}, CPlayerState::EItemType::Invalid, -1, -1, -1);
}

CEntity* ScriptLoader::LoadAmbientAI(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 16, "AmbientAI"))
    return nullptr;

  SScaledActorHead head = LoadScaledActorHead(in, mgr);
  zeus::CVector3f collisionExtent = zeus::CVector3f::ReadBig(in);
  zeus::CVector3f collisionOffset = zeus::CVector3f::ReadBig(in);
  float mass = in.readFloatBig();
  CHealthInfo hInfo(in);
  CDamageVulnerability dVuln(in);
  CAnimationParameters animParms = LoadAnimationParameters(in);
  CActorParameters actParms = LoadActorParameters(in);
  float alertRange = in.readFloatBig();
  float impactRange = in.readFloatBig();
  s32 alertAnim = in.readInt32Big();
  s32 impactAnim = in.readInt32Big();
  bool active = in.readBool();

  if (!g_ResFactory->GetResourceTypeById(animParms.GetACSFile()))
    return nullptr;

  zeus::CAABox aabox = GetCollisionBox(mgr, info.GetAreaId(), collisionExtent, collisionOffset);

  CMaterialList matList(EMaterialTypes::Immovable, EMaterialTypes::NonSolidDamageable);

  CModelData mData(CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), head.x40_scale,
                            animParms.GetInitialAnimation(), true));
  if ((collisionExtent.x() < 0.f || collisionExtent.y() < 0.f || collisionExtent.z() < 0.f) || collisionExtent.isZero())
    aabox = mData.GetBounds(head.x10_transform.getRotation());

  return new CAmbientAI(mgr.AllocateUniqueId(), head.x0_name, info, head.x10_transform, std::move(mData), aabox,
                        matList, mass, hInfo, dVuln, actParms, alertRange, impactRange, alertAnim, impactAnim, active);
}

CEntity* ScriptLoader::LoadAtomicBeta(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 21, "AtomicBeta"))
    return nullptr;
  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;
  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  CAssetId electricId(in);
  CAssetId weaponId(in);
  CDamageInfo dInfo(in);
  CAssetId particleId(in);
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  CDamageVulnerability dVuln(in);
  float f4 = in.readFloatBig();
  float f5 = in.readFloatBig();
  float f6 = in.readFloatBig();
  s16 sId1 = s16(in.readInt32Big() & 0xFFFF);
  s16 sId2 = s16(in.readInt32Big() & 0xFFFF);
  s16 sId3 = s16(in.readInt32Big() & 0xFFFF);
  float f7 = in.readFloatBig();
  const CAnimationParameters& animParms = pInfo.GetAnimationParameters();
  CModelData mData(CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), aHead.x40_scale,
                            animParms.GetInitialAnimation(), true));
  return new MP1::CAtomicBeta(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, std::move(mData),
                              actParms, pInfo, electricId, weaponId, dInfo, particleId, f1, f2, f3, dVuln, f4, f5, f6,
                              sId1, sId2, sId3, f7);
}

CEntity* ScriptLoader::LoadIceZoomer(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 16, "IceZoomer"))
    return nullptr;
  SScaledActorHead actHead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  const CAnimationParameters& animParms = pInfo.GetAnimationParameters();
  if (!animParms.GetACSFile().IsValid())
    return nullptr;

  float advanceWpRadius = in.readFloatBig();
  float f2 = in.readFloatBig();
  float alignAngleVel = in.readFloatBig();
  float f4 = in.readFloatBig();
  float playerObstructionMinDist = in.readFloatBig();
  float moveFowardWeight = in.readFloatBig();
  CAssetId modelRes(in.readUint32Big());
  CAssetId skinRes(in.readUint32Big());
  CDamageVulnerability dVuln(in);
  float iceZoomerJointHP = in.readFloatBig();

  CModelData mData(
    CAnimRes(animParms.GetACSFile(), animParms.GetCharacter(), actHead.x40_scale, animParms.GetInitialAnimation(),
             true));
  return new MP1::CParasite(mgr.AllocateUniqueId(), actHead.x0_name, CPatterned::EFlavorType::Zero, info,
                            actHead.x10_transform, std::move(mData), pInfo, EBodyType::WallWalker, 0.f, advanceWpRadius,
                            f2, alignAngleVel, f4, 0.2f, 0.4f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, moveFowardWeight, 0.f, 0.f,
                            playerObstructionMinDist, 0.f, false, CWallWalker::EWalkerType::IceZoomer, dVuln,
                            CDamageInfo(), 0xFFFF, 0xFFFF, 0xFFFF, modelRes, skinRes, iceZoomerJointHP, actParms);
}

CEntity* ScriptLoader::LoadPuffer(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 16, "Puffer"))
    return nullptr;

  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  if (!pInfo.GetAnimationParameters().GetACSFile().IsValid())
    return nullptr;

  CActorParameters actorParameters = LoadActorParameters(in);
  float hoverSpeed = in.readFloatBig();
  CAssetId cloudEffect(in);
  CDamageInfo cloudDamage(in);
  CAssetId cloudSteam(in);
  float f2 = in.readFloatBig();
  bool b1 = in.readBool();
  bool b2 = in.readBool();
  bool b3 = in.readBool();
  CDamageInfo explosionDamage(in);
  s16 sfxId = in.readUint16Big();

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            aHead.x40_scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));
  return new MP1::CPuffer(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, std::move(mData),
                          actorParameters, pInfo, hoverSpeed, cloudEffect, cloudDamage, cloudSteam, f2, b1, b2, b3,
                          explosionDamage, sfxId);
}

CEntity* ScriptLoader::LoadTryclops(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 10, "Tryclops"))
    return nullptr;

  SScaledActorHead actorHead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);

  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);

  if (!pInfo.GetAnimationParameters().GetACSFile().IsValid())
    return nullptr;

  CActorParameters actorParameters = LoadActorParameters(in);
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            actorHead.x40_scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));

  return new MP1::CTryclops(mgr.AllocateUniqueId(), actorHead.x0_name, info, actorHead.x10_transform, std::move(mData),
                            pInfo, actorParameters, f1, f2, f3, f4);
}

CEntity* ScriptLoader::LoadRidley(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 47, "Ridley"))
    return nullptr;

  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);
  CActorParameters actParms = LoadActorParameters(in);
  if (!pInfo.GetAnimationParameters().GetACSFile().IsValid())
    return nullptr;

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            aHead.x40_scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));
  return new MP1::CRidley(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, std::move(mData), pInfo,
                          actParms, in, propCount);
}

CEntity* ScriptLoader::LoadSeedling(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 14, "Seedling"))
    return nullptr;
  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);

  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);

  if (!pInfo.GetAnimationParameters().GetACSFile().IsValid())
    return nullptr;

  CActorParameters actParms = LoadActorParameters(in);
  CAssetId needleId(in);
  CAssetId weaponId(in);
  CDamageInfo dInfo1(in);
  CDamageInfo dInfo2(in);
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            aHead.x40_scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));
  return new MP1::CSeedling(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, std::move(mData), pInfo,
                            actParms, needleId, weaponId, dInfo1, dInfo2, f1, f2, f3, f4);
}

CEntity* ScriptLoader::LoadThermalHeatFader(CStateManager& mgr, CInputStream& in, int propCount,
                                            const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 4, "ThermalHeatFader"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  bool active = in.readBool();
  float fadedLevel = in.readFloatBig();
  float initialLevel = in.readFloatBig();
  return new CScriptDistanceFog(mgr.AllocateUniqueId(), name, info, ERglFogMode::None, zeus::skBlack,
                                zeus::CVector2f(), 0.f, zeus::CVector2f(), false, active, fadedLevel, initialLevel, 0.f,
                                0.f);
}

CEntity* ScriptLoader::LoadBurrower(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 13, "Burrower"))
    return nullptr;

  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);
  auto pair = CPatternedInfo::HasCorrectParameterCount(in);
  if (!pair.first)
    return nullptr;

  CPatternedInfo pInfo(in, pair.second);

  if (!pInfo.GetAnimationParameters().GetACSFile().IsValid())
    return nullptr;

  CActorParameters actParms = LoadActorParameters(in);
  CAssetId w1(in);
  CAssetId w2(in);
  CAssetId w3(in);
  CDamageInfo dInfo(in);

  CAssetId w4(in);
  u32 w5 = in.readUint32Big();
  CAssetId w6(in);

  CModelData mData(CAnimRes(pInfo.GetAnimationParameters().GetACSFile(), pInfo.GetAnimationParameters().GetCharacter(),
                            aHead.x40_scale, pInfo.GetAnimationParameters().GetInitialAnimation(), true));

  return new MP1::CBurrower(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, std::move(mData), pInfo,
                            actParms, w1, w2, w3, dInfo, w4, w5, w6);
}

CEntity* ScriptLoader::LoadBeam(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 7, "Beam"))
    return nullptr;

  SActorHead aHead = LoadActorHead(in, mgr);
  bool active = in.readBool();
  u32 weaponDescId = in.readUint32Big();
  if (!g_ResFactory->GetResourceTypeById(weaponDescId))
    return nullptr;

  CBeamInfo beamInfo(in);
  CDamageInfo dInfo(in);
  TToken<CWeaponDescription> weaponDesc = g_SimplePool->GetObj({SBIG('WPSC'), weaponDescId});

  return new CScriptBeam(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, active, weaponDesc, beamInfo,
                         dInfo);
}

CEntity* ScriptLoader::LoadWorldLightFader(CStateManager& mgr, CInputStream& in, int propCount,
                                           const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 4, "WorldLightFader"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  bool active = in.readBool();
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();

  return new CScriptDistanceFog(mgr.AllocateUniqueId(), name, info, ERglFogMode::None, zeus::skBlack,
                                zeus::skZero2f, 0.f, zeus::skZero2f, false, active, 0.f, 0.f, f1, f2);
}

CEntity* ScriptLoader::LoadMetroidPrimeStage2(CStateManager& mgr, CInputStream& in, int propCount,
                                              const CEntityInfo& info) {
  return nullptr;
}

CEntity* ScriptLoader::LoadMetroidPrimeStage1(CStateManager& mgr, CInputStream& in, int propCount,
                                              const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 22, "MetroidPrimeStage1"))
    return nullptr;
  u32 version = in.readUint32Big();
  if (version != 3)
    return nullptr;

  SScaledActorHead aHead = LoadScaledActorHead(in, mgr);
  bool active = in.readBool();
  float f1 = in.readFloatBig();
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  u32 w1 = in.readUint32Big();
  bool b1 = in.readBool();
  u32 w2 = in.readUint32Big();
  CHealthInfo hInfo1(in);
  CHealthInfo hInfo2(in);
  u32 w3 = in.readUint32Big();
  rstl::reserved_vector<MP1::SPrimeExoRoomParameters, 4> roomParms;
  for (int i = 0; i < 4; ++i)
    roomParms.emplace_back(in);
  u32 w4 = in.readUint32Big();
  u32 w5 = in.readUint32Big();
  MP1::SPrimeExoParameters primeParms(in);

  return new MP1::CMetroidPrimeRelay(mgr.AllocateUniqueId(), aHead.x0_name, info, active, aHead.x10_transform,
                                     aHead.x40_scale, std::move(primeParms), f1, f2, f3, w1, b1, w2, hInfo1, hInfo2, w3,
                                     w4, w5, std::move(roomParms));
}

CEntity* ScriptLoader::LoadMazeNode(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 10, "MazeNode"))
    return nullptr;

  SActorHead aHead = LoadActorHead(in, mgr);
  bool active = in.readBool();
  u32 w1 = in.readUint32Big();
  u32 w2 = in.readUint32Big();
  u32 w3 = in.readUint32Big();
  zeus::CVector3f vec1 = zeus::CVector3f::ReadBig(in);
  zeus::CVector3f vec2 = zeus::CVector3f::ReadBig(in);
  zeus::CVector3f vec3 = zeus::CVector3f::ReadBig(in);

  return new CScriptMazeNode(mgr.AllocateUniqueId(), aHead.x0_name, info, aHead.x10_transform, active, w1, w2, w3, vec1,
                             vec2, vec3);
}

CEntity* ScriptLoader::LoadOmegaPirate(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  return nullptr;
}

CEntity* ScriptLoader::LoadPhazonPool(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  return nullptr;
}

CEntity* ScriptLoader::LoadPhazonHealingNodule(CStateManager& mgr, CInputStream& in, int propCount,
                                               const CEntityInfo& info) {
  return nullptr;
}

CEntity* ScriptLoader::LoadNewCameraShaker(CStateManager& mgr, CInputStream& in, int propCount,
                                           const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 8, "NewCameraShaker"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f sfxPos = zeus::CVector3f::ReadBig(in);
  bool active = in.readBool();
  u32 flags = LoadParameterFlags(in);
  float duration = in.readFloatBig();
  float sfxDist = in.readFloatBig();
  CCameraShakerComponent shakerX = CCameraShakerComponent::LoadNewCameraShakerComponent(in);
  CCameraShakerComponent shakerY = CCameraShakerComponent::LoadNewCameraShakerComponent(in);
  CCameraShakerComponent shakerZ = CCameraShakerComponent::LoadNewCameraShakerComponent(in);

  CCameraShakeData shakeData(duration, sfxDist, flags, sfxPos, shakerX, shakerY, shakerZ);

  return new CScriptCameraShaker(mgr.AllocateUniqueId(), name, info, active, shakeData);
}

CEntity* ScriptLoader::LoadShadowProjector(CStateManager& mgr, CInputStream& in, int propCount,
                                           const CEntityInfo& info) {
  if (!EnsurePropertyCount(propCount, 10, "ShadowProjector"))
    return nullptr;

  std::string name = mgr.HashInstanceName(in);
  zeus::CVector3f position(zeus::CVector3f::ReadBig(in));
  bool b1 = in.readBool();
  float f1 = in.readFloatBig();
  zeus::CVector3f vec2(zeus::CVector3f::ReadBig(in));
  float f2 = in.readFloatBig();
  float f3 = in.readFloatBig();
  float f4 = in.readFloatBig();
  bool b2 = in.readBool();
  u32 w1 = in.readUint32Big();
  return new CScriptShadowProjector(mgr.AllocateUniqueId(), name, info, zeus::CTransform::Translate(position), b1, vec2,
                                    b2, f1, f2, f3, f4, w1);
}

CEntity* ScriptLoader::LoadEnergyBall(CStateManager& mgr, CInputStream& in, int propCount, const CEntityInfo& info) {
  return nullptr;
}
} // namespace urde
