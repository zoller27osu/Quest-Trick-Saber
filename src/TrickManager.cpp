#include "../include/TrickManager.hpp"
#include <algorithm>
#include <optional>
#include <queue>
#include "../include/PluginConfig.hpp"
#include "../include/AllInputHandlers.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "beatsaber-hook/shared/utils/instruction-parsing.hpp"
#include "modloader/shared/modloader.hpp"

// Define static fields
constexpr Space RotateSpace = Space::Self;

// for "ApplySlowmoSmooth" / "EndSlowmoSmooth"
bool TrickManager::disableClashing = false;
static TrickState _slowmoState = Inactive;  // must also be reset in Start()
static float _slowmoTimeScale;
static float _originalTimeScale;
static float _targetTimeScale;
static Il2CppObject* _audioSource;
static function_ptr_t<void, Il2CppObject*> RigidbodySleep;
static bool _gamePaused;


static const MethodInfo* VRController_get_transform = nullptr;
static std::unordered_map<Il2CppObject*, Il2CppObject*> fakeTransforms;
static bool VRController_transform_is_hooked = false;
MAKE_HOOK_OFFSETLESS(VRController_get_transform_hook, Il2CppObject*, Il2CppObject* self) {
    auto pair = fakeTransforms.find(self);
    if ( pair == fakeTransforms.end() ) {
        return VRController_get_transform_hook(self);
    } else {
        return pair->second;
    }
}

static Il2CppObject* AudioTimeSyncController;

void ButtonMapping::Update() {
    // According to Oculus documentation, left is always Primary and Right is always secondary UNLESS referred to individually.
    // https://developer.oculus.com/reference/unity/v14/class_o_v_r_input
    Controller oculusController;
    XRNode node;
    if (left) {
        oculusController = Controller::LTouch;
        node = XRNode::LeftHand;
    } else {
        oculusController = Controller::RTouch;
        node = XRNode::RightHand;
    }

    // Method missing from libil2cpp.so
    //auto* controllerInputDevice = CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine.XR", "InputDevices", "GetDeviceAtXRNode", node));
    static auto getDeviceIdAtXRNode = (function_ptr_t<uint64_t, XRNode>)CRASH_UNLESS(
        il2cpp_functions::resolve_icall("UnityEngine.XR.InputTracking::GetDeviceIdAtXRNode"));
    logger().debug("getDeviceIdAtXRNode ptr offset: %lX", asOffset(getDeviceIdAtXRNode));
    auto deviceId = getDeviceIdAtXRNode(node);
    auto* controllerInputDevice = CRASH_UNLESS(il2cpp_utils::New("UnityEngine.XR", "InputDevice", deviceId));

    logger().debug("oculusController: %i", (int)oculusController);
    bool isOculus = CRASH_UNLESS(il2cpp_utils::RunMethod<bool>("", "OVRInput", "IsControllerConnected", oculusController));
    logger().debug("isOculus: %i", isOculus);
    auto vrSystem = isOculus ? VRSystem::Oculus : VRSystem::SteamVR;

    auto dir = PluginConfig::Instance().ThumbstickDirection;

    actionHandlers.clear();
    actionHandlers[PluginConfig::Instance().TriggerAction].insert(std::unique_ptr<InputHandler>(
        new TriggerHandler(node, PluginConfig::Instance().TriggerThreshold)
    ));
    actionHandlers[PluginConfig::Instance().GripAction].insert(std::unique_ptr<InputHandler>(
        new GripHandler(vrSystem, oculusController, controllerInputDevice, PluginConfig::Instance().GripThreshold)
    ));
    actionHandlers[PluginConfig::Instance().ThumbstickAction].insert(std::unique_ptr<InputHandler>(
        new ThumbstickHandler(node, PluginConfig::Instance().ThumbstickThreshold, dir)
    ));
    actionHandlers[PluginConfig::Instance().ButtonOneAction].insert(std::unique_ptr<InputHandler>(
        new ButtonHandler(oculusController, Button::One)
    ));
    actionHandlers[PluginConfig::Instance().ButtonTwoAction].insert(std::unique_ptr<InputHandler>(
        new ButtonHandler(oculusController, Button::Two)
    ));
    if (actionHandlers[TrickAction::Throw].empty()) {
        logger().warning("No inputs assigned to Throw! Throw will never trigger!");
    }
    if (actionHandlers[TrickAction::Spin].empty()) {
        logger().warning("No inputs assigned to Spin! Spin will never trigger!");
    }
}


void TrickManager::LogEverything() {
    logger().debug("_throwState %i", _throwState);
    logger().debug("_spinState %i", _spinState);
    logger().debug("RotationSpeed: %f", _saberSpeed);
}

float getDeltaTime() {
    return RET_0_UNLESS(il2cpp_utils::RunMethod<float>("UnityEngine", "Time", "get_deltaTime"));
}


Vector3 Vector3_Multiply(const Vector3 &vec, float scalar) {
    Vector3 result;
    result.x = vec.x * scalar;
    result.y = vec.y * scalar;
    result.z = vec.z * scalar;
    return result;
}
Vector3 Vector3_Divide(const Vector3 &vec, float scalar) {
    Vector3 result;
    result.x = vec.x / scalar;
    result.y = vec.y / scalar;
    result.z = vec.z / scalar;
    return result;
}

float Vector3_Distance(const Vector3 &a, const Vector3 &b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

float Vector3_Magnitude(const Vector3 &v) {
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector3 Vector3_Add(const Vector3 &a, const Vector3 &b) {
    Vector3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}
Vector3 Vector3_Subtract(const Vector3 &a, const Vector3 &b) {
    Vector3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

static Il2CppClass* cQuaternion;
Quaternion Quaternion_Multiply(const Quaternion &lhs, const Quaternion &rhs) {
    return Quaternion{
        lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.w * rhs.y + lhs.y * rhs.w + lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.w * rhs.z + lhs.z * rhs.w + lhs.x * rhs.y - lhs.y * rhs.x,
        lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z
    };
}

// Copied from DnSpy
Vector3 Quaternion_Multiply(const Quaternion& rotation, const Vector3& point) {
	float num = rotation.x * 2.0f;
	float num2 = rotation.y * 2.0f;
	float num3 = rotation.z * 2.0f;
	float num4 = rotation.x * num;
	float num5 = rotation.y * num2;
	float num6 = rotation.z * num3;
	float num7 = rotation.x * num2;
	float num8 = rotation.x * num3;
	float num9 = rotation.y * num3;
	float num10 = rotation.w * num;
	float num11 = rotation.w * num2;
	float num12 = rotation.w * num3;
	Vector3 result;
	result.x = (1.0f - (num5 + num6)) * point.x + (num7 - num12) * point.y + (num8 + num11) * point.z;
	result.y = (num7 + num12) * point.x + (1.0f - (num4 + num6)) * point.y + (num9 - num10) * point.z;
	result.z = (num8 - num11) * point.x + (num9 + num10) * point.y + (1.0f - (num4 + num5)) * point.z;
	return result;
}

Quaternion Quaternion_Inverse(const Quaternion &q) {
    Quaternion ret = CRASH_UNLESS(il2cpp_utils::RunMethod<Quaternion>(cQuaternion, "Inverse", q));
    return ret;
}

Il2CppObject* GetComponent(Il2CppObject *object, std::string_view componentNamespace, std::string_view componentClassName) {
    auto* sysType = RET_0_UNLESS(il2cpp_utils::GetSystemType(componentNamespace, componentClassName));
    return RET_0_UNLESS(il2cpp_utils::RunMethod(object, "GetComponent", sysType));
}

Vector3 GetAngularVelocity(const Quaternion& foreLastFrameRotation, const Quaternion& lastFrameRotation)
{
    auto foreLastInv = Quaternion_Inverse(foreLastFrameRotation);
    auto q = Quaternion_Multiply(lastFrameRotation, foreLastInv);
    if (abs(q.w) > (1023.5f / 1024.0f)) {
        return Vector3_Zero;
    }
    float gain;
    if (q.w < 0.0f) {
        auto angle = acos(-q.w);
        gain = (float) (-2.0f * angle / (sin(angle) * getDeltaTime()));
    } else {
        auto angle = acos(q.w);
        gain = (float) (2.0f * angle / (sin(angle) * getDeltaTime()));
    }

    return {q.x * gain, q.y * gain, q.z * gain};
}

void TrickManager::AddProbe(const Vector3& vel, const Vector3& ang) {
    if (_currentProbeIndex >= _velocityBuffer.size()) _currentProbeIndex = 0;
    _velocityBuffer[_currentProbeIndex] = vel;
    _angularVelocityBuffer[_currentProbeIndex] = ang;
    _currentProbeIndex++;
}

Vector3 TrickManager::GetAverageVelocity() {
    Vector3 avg = Vector3_Zero;
    for (size_t i = 0; i < _velocityBuffer.size(); i++) {
        avg = Vector3_Add(avg, _velocityBuffer[i]);
    }
    return Vector3_Divide(avg, _velocityBuffer.size());
}

Vector3 TrickManager::GetAverageAngularVelocity() {
    Vector3 avg = Vector3_Zero;
    for (size_t i = 0; i < _velocityBuffer.size(); i++) {
        avg = Vector3_Add(avg, _angularVelocityBuffer[i]);
    }
    return Vector3_Divide(avg, _velocityBuffer.size());
}

Il2CppObject* TrickManager::FindBasicSaberTransform() {
    auto* basicSaberT = CRASH_UNLESS(il2cpp_utils::RunMethod(_saberT, "Find", _basicSaberName));
    if (!basicSaberT) {
        auto* saberModelT = CRASH_UNLESS(il2cpp_utils::RunMethod(_saberT, "Find", _saberName).value_or(nullptr));
        basicSaberT = CRASH_UNLESS(il2cpp_utils::RunMethod(saberModelT, "Find", _basicSaberName));
    }
    return CRASH_UNLESS(basicSaberT);
}

void TrickManager::Start2() {
    logger().debug("TrickManager.Start2!");
    // Try to find a custom saber - will have same name as _saberT (LeftSaber or RightSaber) but be a child of it
    Il2CppObject* saberModelT = il2cpp_utils::RunMethod(_saberT, "Find", _saberName).value_or(nullptr);
    Il2CppObject* basicSaberT = FindBasicSaberTransform();

    if (!saberModelT) {
        logger().warning("Did not find custom saber! Thrown sabers will be BasicSaberModel(Clone)!");
        saberModelT = basicSaberT;
    } else if (Modloader::getMods().contains("Qosmetics")) {
        logger().debug("Not moving trail because Qosmetics is installed!");
    } else {
        logger().debug("Found '%s'!", to_utf8(csstrtostr(_saberName)).c_str());

        // TODO: remove the rest of this once CustomSabers correctly creates trails and removes/moves the vanilla ones
        // Now transfer the trail from basicSaber to saberModel (the custom saber)
        // Find the old trail script
        static auto* tTrail = CRASH_UNLESS(il2cpp_utils::GetSystemType("Xft", "XWeaponTrail"));
        auto* trailComponent = CRASH_UNLESS(il2cpp_utils::RunMethod(basicSaberT, "GetComponent", tTrail));
        if (!trailComponent) {
            logger().warning("trailComponent not found!");
        } else {
            // Create a new trail script on the custom saber
            auto* saberGO = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(saberModelT, "gameObject"));
            auto* newTrail = CRASH_UNLESS(il2cpp_utils::RunMethod(saberGO, "AddComponent", tTrail));

            // Relocate the children from the BasicSaberModel(Clone) transfrom to the custom saber transform
            // Most important children: TrailTop, TrailBottom, PointLight?
            std::vector<Il2CppObject*> children;
            int childCount = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<int>(basicSaberT, "childCount"));
            for (int i = 0; i < childCount; i++) {
                auto* childT = CRASH_UNLESS(il2cpp_utils::RunMethod(basicSaberT, "GetChild", i));
                children.push_back(childT);
            }
            for (auto* child : children) {
                CRASH_UNLESS(il2cpp_utils::RunMethod(child, "SetParent", saberModelT));
            }

            // Copy the necessary properties/fields over from old to new trail script
            Color trailColor = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Color>(trailComponent, "color"));
            CRASH_UNLESS(il2cpp_utils::SetPropertyValue(newTrail, "color", trailColor));
            auto* pointStart = CRASH_UNLESS(il2cpp_utils::GetFieldValue(trailComponent, "_pointStart"));
            CRASH_UNLESS(il2cpp_utils::SetFieldValue(newTrail, "_pointStart", pointStart));
            auto* pointEnd = CRASH_UNLESS(il2cpp_utils::GetFieldValue(trailComponent, "_pointEnd"));
            CRASH_UNLESS(il2cpp_utils::SetFieldValue(newTrail, "_pointEnd", pointEnd));
            auto* trailPrefab = CRASH_UNLESS(il2cpp_utils::GetFieldValue(trailComponent, "_trailRendererPrefab"));
            CRASH_UNLESS(il2cpp_utils::SetFieldValue(newTrail, "_trailRendererPrefab", trailPrefab));

            // Destroy the old trail script
            CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine", "Object", "Destroy", trailComponent));
        }
    }
    CRASH_UNLESS(saberModelT);

    auto* saberGO = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(saberModelT, "gameObject"));
    logger().debug("%sSaber gameObject: %p", _isLeftSaber ? "Left" : "Right", saberGO);
    _saberTrickModel = new SaberTrickModel(saberGO, _isLeftSaber ? 0 : 1);
}

void TrickManager::StaticClear() {
    _slowmoState = Inactive;
    AudioTimeSyncController = nullptr;
    _audioSource = nullptr;
    _gamePaused = false;
    
    cQuaternion = CRASH_UNLESS(il2cpp_utils::GetClassFromName("UnityEngine", "Quaternion"));
}

void TrickManager::Clear() {
    _throwState = Inactive;
    _spinState = Inactive;
    _saberTrickModel = nullptr;
    VRController = nullptr;
}

void TrickManager::Start() {
    if (!VRController) return;
    if (!AudioTimeSyncController) {
        auto* tATSC = CRASH_UNLESS(il2cpp_utils::GetSystemType("", "AudioTimeSyncController"));
        AudioTimeSyncController = CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine", "Object", "FindObjectOfType", tATSC));
        CRASH_UNLESS(AudioTimeSyncController);
    }
    if (!RigidbodySleep) {
        RigidbodySleep = (decltype(RigidbodySleep))CRASH_UNLESS(il2cpp_functions::resolve_icall("UnityEngine.Rigidbody::Sleep"));
        logger().debug("RigidbodySleep ptr offset: %lX", asOffset(RigidbodySleep));
    }

    // auto* rigidbody = CRASH_UNLESS(GetComponent(Saber, "UnityEngine", "Rigidbody"));
    _collider = CRASH_UNLESS(GetComponent(Saber, "UnityEngine", "BoxCollider"));
    _vrPlatformHelper = CRASH_UNLESS(il2cpp_utils::GetFieldValue(VRController, "_vrPlatformHelper"));

    _buttonMapping = ButtonMapping(_isLeftSaber);

    int velBufferSize = PluginConfig::Instance().VelocityBufferSize;
    _velocityBuffer = std::vector<Vector3>(velBufferSize);
    _angularVelocityBuffer = std::vector<Vector3>(velBufferSize);

    _saberT = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(Saber, "transform"));
    _saberName = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Il2CppString*>(Saber, "name"));
    logger().debug("saberName: %s", to_utf8(csstrtostr(_saberName)).c_str());
    _basicSaberName = il2cpp_utils::createcsstr("BasicSaberModel(Clone)");

    if (PluginConfig::Instance().EnableTrickCutting) {
        if (!VRController_get_transform) {
            VRController_get_transform = CRASH_UNLESS(il2cpp_utils::FindMethod("", "VRController", "get_transform"));
        }
        auto* fakeTransformName = CRASH_UNLESS(il2cpp_utils::createcsstr("FakeTrickSaberTransform"));
        auto* fakeTransformGO = CRASH_UNLESS(il2cpp_utils::New("UnityEngine", "GameObject", fakeTransformName));
        _fakeTransform = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(fakeTransformGO, "transform"));

        auto* saberParentT = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(_saberT, "parent"));
        CRASH_UNLESS(il2cpp_utils::RunMethod(_fakeTransform, "SetParent", saberParentT));
        auto fakePos = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Vector3>(_fakeTransform, "localPosition"));
        logger().debug("fakePos: {%f, %f, %f}", fakePos.x, fakePos.y, fakePos.z);

        // TODO: instead of patching this transform onto the VRController, add a clone VRController component to the object?
    }
    logger().debug("Leaving TrickManager.Start");
}

static float GetTimescale() {
    return CRASH_UNLESS(il2cpp_utils::GetFieldValue<float>(AudioTimeSyncController, "_timeScale"));
}

void SetTimescale(float timescale) {
    // Efficiency is top priority in FixedUpdate!
    if (AudioTimeSyncController) {
        CRASH_UNLESS(il2cpp_utils::SetFieldValue(AudioTimeSyncController, "_timeScale", timescale));
        auto songTime = CRASH_UNLESS(il2cpp_utils::GetFieldValue<float>(AudioTimeSyncController, "_songTime"));
        CRASH_UNLESS(il2cpp_utils::SetFieldValue(AudioTimeSyncController, "_startSongTime", songTime));
        auto timeSinceStart = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<float>(AudioTimeSyncController, "timeSinceStart"));
        auto songTimeOffset = CRASH_UNLESS(il2cpp_utils::GetFieldValue<float>(AudioTimeSyncController, "_songTimeOffset"));
        CRASH_UNLESS(il2cpp_utils::SetFieldValue(AudioTimeSyncController, "_audioStartTimeOffsetSinceStart",
            timeSinceStart - (songTime + songTimeOffset)));
        CRASH_UNLESS(il2cpp_utils::SetFieldValue(AudioTimeSyncController, "_fixingAudioSyncError", false));
        CRASH_UNLESS(il2cpp_utils::SetFieldValue(AudioTimeSyncController, "_playbackLoopIndex", 0));
        if (!CRASH_UNLESS(il2cpp_utils::GetPropertyValue<bool>(AudioTimeSyncController, "isAudioLoaded"))) return;
        static const MethodInfo* setPitch = CRASH_UNLESS(il2cpp_utils::FindMethodUnsafe("UnityEngine", "AudioSource", "SetPitch", 2));
        if (_audioSource) CRASH_UNLESS(il2cpp_utils::RunMethod(nullptr, setPitch, _audioSource, timescale));
    }
}

void ForceEndSlowmo() {
    if (_slowmoState != Inactive) {
        SetTimescale(_targetTimeScale);
        _slowmoState = Inactive;
        logger().debug("Slowmo ended. TimeScale: %f", _targetTimeScale);
    }
}

void TrickManager::StaticFixedUpdate() {
    // Efficiency is top priority in FixedUpdate!
    if (_gamePaused) return;
    if (_slowmoState == Started) {
        // IEnumerator ApplySlowmoSmooth
        if (_slowmoTimeScale > _targetTimeScale) {
            _slowmoTimeScale -= PluginConfig::Instance().SlowmoStepAmount;
            SetTimescale(_slowmoTimeScale);
        } else if (_slowmoTimeScale != _targetTimeScale) {
            logger().debug("Slowmo == Started; Forcing TimeScale from %f to %f", _slowmoTimeScale, _targetTimeScale);
            _slowmoTimeScale = _targetTimeScale;
            SetTimescale(_slowmoTimeScale);
        }
    } else if (_slowmoState == Ending) {
        // IEnumerator EndSlowmoSmooth
        if (_slowmoTimeScale < _targetTimeScale) {
            _slowmoTimeScale += PluginConfig::Instance().SlowmoStepAmount;
            SetTimescale(_slowmoTimeScale);
        } else if (_slowmoTimeScale != _targetTimeScale) {
            logger().debug("Slowmo == Ending; Forcing TimeScale from %f to %f", _slowmoTimeScale, _targetTimeScale);
            _slowmoTimeScale = _targetTimeScale;
            SetTimescale(_slowmoTimeScale);
        }
    }
}

void TrickManager::FixedUpdate() {
    // Efficiency is top priority in FixedUpdate!
    if (!_saberTrickModel) return;
}

void TrickManager::Update() {
    if (!VRController) return;
    if (!_saberTrickModel) {
        _timeSinceStart += getDeltaTime();
        if (PluginConfig::Instance().EnableTrickCutting || il2cpp_utils::RunMethod(_saberT, "Find", _saberName).value_or(nullptr) ||
                _timeSinceStart > 1) {
            Start2();
        } else {
            return;
        }
    }
    if (_gamePaused) return;
    // RET_V_UNLESS(il2cpp_utils::GetPropertyValue<bool>(VRController, "enabled").value_or(false));
    if (TrailFollowsSaberComponent) _saberTrickModel->Update();

    std::optional<Quaternion> oRot;
    if (_spinState == Ending) {  // not needed for Started because only Rotate and _currentRotation are used
        // Note: localRotation is the same as rotation for TrickCutting, since parent is VRGameCore
        auto tmp = il2cpp_utils::GetPropertyValue<Quaternion>(_saberTrickModel->SpinT, "localRotation");
        oRot.swap(tmp);
        // if (PluginConfig::Instance().EnableTrickCutting && oRot) {
        //     logger().debug("pre-manual VRController.Update rot: {%f, %f, %f, %f}", oRot->w, oRot->x, oRot->y, oRot->z);
        // }
    }

    if (PluginConfig::Instance().EnableTrickCutting && ((_spinState != Inactive) || (_throwState != Inactive))) {
        CRASH_UNLESS(il2cpp_utils::RunMethod(VRController, "Update"));  // sets position and pre-_currentRotation
    }

    // Note: iff TrickCutting, during throw, these properties are redirected to an unused object
    _controllerPosition = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Vector3>(VRController, "position"));
    _controllerRotation = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Quaternion>(VRController, "rotation"));

    auto dPos = Vector3_Subtract(_controllerPosition, _prevPos);
    auto velocity = Vector3_Divide(dPos, getDeltaTime());
    _angularVelocity = GetAngularVelocity(_prevRot, _controllerRotation);

    // float mag = Vector3_Magnitude(_angularVelocity);
    // if (mag) logger().debug("angularVelocity.x: %f, .y: %f, mag: %f", _angularVelocity.x, _angularVelocity.y, mag);
    AddProbe(velocity, _angularVelocity);
    _saberSpeed = Vector3_Magnitude(velocity);
    _prevPos = _controllerPosition;
    _prevRot = _controllerRotation;

    // TODO: move these to LateUpdate?
    if (_throwState == Ending) {
        Vector3 saberPos = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Vector3>(_saberTrickModel->Rigidbody, "position"));
        auto d = Vector3_Subtract(_controllerPosition, saberPos);
        float distance = Vector3_Magnitude(d);

        if (distance <= PluginConfig::Instance().ControllerSnapThreshold) {
            ThrowEnd();
        } else {
            float returnSpeed = fmax(distance, 1.0f) * PluginConfig::Instance().ReturnSpeed;
            logger().debug("distance: %f; return speed: %f", distance, returnSpeed);
            auto dirNorm = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Vector3>(d, "normalized"));
            auto newVel = Vector3_Multiply(dirNorm, returnSpeed);

            CRASH_UNLESS(il2cpp_utils::SetPropertyValue(_saberTrickModel->Rigidbody, "velocity", newVel));
        }
    }
    if (_spinState == Ending) {
        auto rot = CRASH_UNLESS(oRot);
        auto targetRot = SpinIsRelativeToVRController ? Quaternion_Identity : _controllerRotation;

        float angle = CRASH_UNLESS(il2cpp_utils::RunMethod<float>(cQuaternion, "Angle", rot, targetRot));
        // logger().debug("angle: %f (%f)", angle, 360.0f - angle);
        if (PluginConfig::Instance().CompleteRotationMode) {
            float minSpeed = 8.0f;
            float returnSpinSpeed = _finalSpinSpeed;
            if (abs(returnSpinSpeed) < minSpeed) {
                returnSpinSpeed = returnSpinSpeed < 0 ? -minSpeed : minSpeed;
            }
            float threshold = abs(returnSpinSpeed) + 0.1f;
            // TODO: cache returnSpinSpeed (and threshold?) in InPlaceRotationReturn
            if (angle <= threshold) {
                InPlaceRotationEnd();
            } else {
                _InPlaceRotate(returnSpinSpeed);
            }
        } else {  // LerpToOriginalRotation
            if (angle <= 5.0f) {
                InPlaceRotationEnd();
            } else {
                rot = CRASH_UNLESS(il2cpp_utils::RunMethod<Quaternion>(
                    cQuaternion, "Lerp", rot, targetRot, getDeltaTime() * 20.0f));
                CRASH_UNLESS(il2cpp_utils::SetPropertyValue(_saberTrickModel->SpinT, "localRotation", rot));
            }
        }
    }
    // TODO: no tricks while paused? https://github.com/ToniMacaroni/TrickSaber/blob/ea60dce35db100743e7ba72a1ffbd24d1472f1aa/TrickSaber/SaberTrickManager.cs#L66
    CheckButtons();
    // logger().debug("Leaving TrickSaber::Update");
}

ValueTuple TrickManager::GetTrackingPos() {
    XRNode controllerNode = CRASH_UNLESS(il2cpp_utils::RunMethod<XRNode>(VRController, "get_node"));
    int controllerNodeIdx = CRASH_UNLESS(il2cpp_utils::RunMethod<int>(VRController, "get_nodeIdx"));

    ValueTuple result;
    bool nodePose = CRASH_UNLESS(il2cpp_utils::RunMethod<bool>(_vrPlatformHelper, "GetNodePose", controllerNode,
        controllerNodeIdx, result.item1, result.item2));
    if (!nodePose) {
        logger().warning("Node pose missing for %s controller!", _isLeftSaber ? "Left": "Right");
        result.item1 = {-0.2f, 0.05f, 0.0f};
        result.item2 = {0.0f, 0.0f, 0.0f, 1.0f};
    }
    return result;
}

bool CheckHandlersDown(decltype(ButtonMapping::actionHandlers)::mapped_type& handlers, float& power) {
    power = 0;
    // logger().debug("handlers.size(): %lu", handlers.size());
    CRASH_UNLESS(handlers.size() > 0);
    if (handlers.size() == 0) return false;
    for (auto& handler : handlers) {
        float val;
        if (!handler->Activated(val)) {
            return false;
        }
        power += val;
    }
    power /= handlers.size();
    return true; 
}

bool CheckHandlersUp(decltype(ButtonMapping::actionHandlers)::mapped_type& handlers) {
    for (auto& handler : handlers) {
        if (handler->Deactivated()) return true;
    }
    return false;
}

void TrickManager::CheckButtons() {
    float power;
    if ((_throwState != Ending) && CheckHandlersDown(_buttonMapping.actionHandlers[TrickAction::Throw], power)) {
        ThrowStart();
    } else if ((_throwState == Started) && CheckHandlersUp(_buttonMapping.actionHandlers[TrickAction::Throw])) {
        ThrowReturn();
    } else if ((_spinState != Ending) && CheckHandlersDown(_buttonMapping.actionHandlers[TrickAction::Spin], power)) {
        InPlaceRotation(power);
    } else if ((_spinState == Started) && CheckHandlersUp(_buttonMapping.actionHandlers[TrickAction::Spin])) {
        InPlaceRotationReturn();
    }
}


void TrickManager::TrickStart() {
    if (PluginConfig::Instance().EnableTrickCutting) {
        // even on throws, we disable this to call Update manually and thus control ordering
        CRASH_UNLESS(il2cpp_utils::RunMethod(VRController, "set_enabled", false));
    }
    TrickManager::disableClashing = true;
}

void TrickManager::TrickEnd() {
    if (PluginConfig::Instance().EnableTrickCutting) {
        CRASH_UNLESS(il2cpp_utils::RunMethod(VRController, "set_enabled", true));
    }
    if ((other->_throwState == Inactive) && (other->_spinState == Inactive)) {
        TrickManager::disableClashing = false;
    }
}

void TrickManager::EndTricks() {
    ThrowReturn();
    InPlaceRotationReturn();
}

static void CheckAndLogAudioSync() {
    if (!AudioTimeSyncController) return;
    if (CRASH_UNLESS(il2cpp_utils::GetFieldValue<bool>(AudioTimeSyncController, "_fixingAudioSyncError")))
        logger().warning("AudioTimeSyncController is fixing audio time sync issue!");
    auto timeSinceStart = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<float>(AudioTimeSyncController, "timeSinceStart"));
    auto audioStartTimeOffsetSinceStart = CRASH_UNLESS(il2cpp_utils::GetFieldValue<float>(AudioTimeSyncController,
        "_audioStartTimeOffsetSinceStart"));
    if (timeSinceStart < audioStartTimeOffsetSinceStart) {
        logger().warning("timeSinceStart < audioStartTimeOffsetSinceStart! _songTime may progress while paused! %f, %f",
            timeSinceStart, audioStartTimeOffsetSinceStart);
    } else {
        logger().debug("timeSinceStart, audioStartTimeOffsetSinceStart: %f, %f", timeSinceStart, audioStartTimeOffsetSinceStart);
    }
    logger().debug("_songTime: %f", CRASH_UNLESS(il2cpp_utils::GetFieldValue<float>(AudioTimeSyncController, "_songTime")));
}

void TrickManager::StaticPause() {
    _gamePaused = true;
    logger().debug("Paused with TimeScale %f", _slowmoTimeScale);
    CheckAndLogAudioSync();
}

void TrickManager::PauseTricks() {
    if (!VRController) return;
    if (_saberTrickModel)
        CRASH_UNLESS(il2cpp_utils::RunMethod(_saberTrickModel->SaberGO, "SetActive", false));
}

void TrickManager::StaticResume() {
    _gamePaused = false;
    _slowmoTimeScale = GetTimescale();
    logger().debug("Resumed with TimeScale %f", _slowmoTimeScale);
    CheckAndLogAudioSync();
}

void TrickManager::ResumeTricks() {
    if (!VRController) return;
    if (_saberTrickModel)
        CRASH_UNLESS(il2cpp_utils::RunMethod(_saberTrickModel->SaberGO, "SetActive", true));
}

void TrickManager::ThrowStart() {
    if (_throwState == Inactive) {
        logger().debug("%s throw start!", _isLeftSaber ? "Left" : "Right");
        if (_spinState != Inactive) {
            InPlaceRotationEnd();
        }

        _saberTrickModel->PrepareForThrow();
        if (!PluginConfig::Instance().EnableTrickCutting) {
            ListGameObjects(Saber);
            logger().info("Now the saberTrickModel->SaberGO!");
            ListGameObjects(_saberTrickModel->SaberGO);
        } else {
            CRASH_UNLESS(il2cpp_utils::RunMethod(_collider, "set_enabled", false));
        }
        TrickStart();

        auto* rigidBody = _saberTrickModel->Rigidbody;
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(rigidBody, "isKinematic", false));

        Vector3 velo = GetAverageVelocity();
        float _velocityMultiplier = PluginConfig::Instance().ThrowVelocity;
        velo = Vector3_Multiply(velo, 3 * _velocityMultiplier);
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(rigidBody, "velocity", velo));

        _saberRotSpeed = _saberSpeed * _velocityMultiplier;
        // logger().debug("initial _saberRotSpeed: %f", _saberRotSpeed);
        if (_angularVelocity.x > 0) {
            _saberRotSpeed *= 150;
        } else {
            _saberRotSpeed *= -150;
        }

        auto* saberTransform = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(rigidBody, "transform"));
        logger().debug("velocity: %f", Vector3_Magnitude(velo));
        logger().debug("_saberRotSpeed: %f", _saberRotSpeed);
        auto torqRel = Vector3_Multiply(Vector3_Right, _saberRotSpeed);
        auto torqWorld = CRASH_UNLESS(il2cpp_utils::RunMethod<Vector3>(saberTransform, "TransformVector", torqRel));

        static auto AddTorque_Injected = (function_ptr_t<void, Il2CppObject*, Vector3&, int>)CRASH_UNLESS(
            il2cpp_functions::resolve_icall("UnityEngine.Rigidbody::AddTorque_Injected"));
        logger().debug("AddTorque_Injected ptr offset: %lX", asOffset(AddTorque_Injected));
        // 5 == ForceMode.Acceleration
        AddTorque_Injected(rigidBody, torqWorld, 5);

        if (PluginConfig::Instance().EnableTrickCutting) {
            fakeTransforms.emplace(VRController, _fakeTransform);
            if (!VRController_transform_is_hooked) {
                INSTALL_HOOK_OFFSETLESS(VRController_get_transform_hook, VRController_get_transform);
                VRController_transform_is_hooked = true;
            }
        }

        DisableBurnMarks(_isLeftSaber ? 0 : 1);

        _throwState = Started;

        if (PluginConfig::Instance().SlowmoDuringThrow) {
            _audioSource = CRASH_UNLESS(il2cpp_utils::GetFieldValue(AudioTimeSyncController, "_audioSource"));
            if (_slowmoState != Started) {
                // ApplySlowmoSmooth
                _slowmoTimeScale = GetTimescale();
                _originalTimeScale = (_slowmoState == Inactive) ? _slowmoTimeScale : _targetTimeScale;

                _targetTimeScale = _originalTimeScale - PluginConfig::Instance().SlowmoAmount;
                if (_targetTimeScale < 0.1f) _targetTimeScale = 0.1f;

                logger().debug("Starting slowmo; TimeScale from %f (%f original) to %f", _slowmoTimeScale, _originalTimeScale, _targetTimeScale);
                _slowmoState = Started;
            }
        }
    } else {
        logger().debug("%s throw continues", _isLeftSaber ? "Left" : "Right");
    }

    // ThrowUpdate();  // not needed as long as the velocity and torque do their job
}

void TrickManager::ThrowUpdate() {
    // auto* saberTransform = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(_saberTrickModel->SaberGO, "transform"));
    // // // For when AddTorque doesn't work
    // // CRASH_UNLESS(il2cpp_utils::RunMethod(saberTransform, "Rotate", Vector3_Right, _saberRotSpeed, RotateSpace));

    // auto rot = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Quaternion>(saberTransform, "rotation"));
    // auto angVel = GetAngularVelocity(_prevTrickRot, rot);
    // logger().debug("angVel.x: %f, .y: %f, mag: %f", angVel.x, angVel.y, Vector3_Magnitude(angVel));
    // _prevTrickRot = rot;
}

void TrickManager::ThrowReturn() {
    if (_throwState == Started) {
        logger().debug("%s throw return!", _isLeftSaber ? "Left" : "Right");

        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(_saberTrickModel->Rigidbody, "velocity", Vector3_Zero));
        _throwState = Ending;

        Vector3 saberPos = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Vector3>(_saberTrickModel->Rigidbody, "position"));
        _throwReturnDirection = Vector3_Subtract(_controllerPosition, saberPos);
        logger().debug("distance: %f", Vector3_Magnitude(_throwReturnDirection));

        if ((_slowmoState == Started) && (other->_throwState != Started)) {
            _slowmoTimeScale = GetTimescale();
            _targetTimeScale = _originalTimeScale;
            _audioSource = CRASH_UNLESS(il2cpp_utils::GetFieldValue(AudioTimeSyncController, "_audioSource"));
            logger().debug("Ending slowmo; TimeScale from %f to %f", _slowmoTimeScale, _targetTimeScale);
            _slowmoState = Ending;
        }
    }
}

void TrickManager::ThrowEnd() {
    logger().debug("%s throw end!", _isLeftSaber ? "Left" : "Right");
    CRASH_UNLESS(il2cpp_utils::SetPropertyValue(_saberTrickModel->Rigidbody, "isKinematic", true));  // restore
    _saberTrickModel->EndThrow();
    if (PluginConfig::Instance().EnableTrickCutting) {
        CRASH_UNLESS(il2cpp_utils::RunMethod(_collider, "set_enabled", true));
        fakeTransforms.erase(VRController);
    }
    if (other->_throwState == Inactive) {
        ForceEndSlowmo();
    }
    EnableBurnMarks(_isLeftSaber ? 0 : 1);
    _throwState = Inactive;
    TrickEnd();
}


void TrickManager::InPlaceRotationStart() {
    logger().debug("%s spin start!", _isLeftSaber ? "Left" : "Right");
    _saberTrickModel->PrepareForSpin();
    TrickStart();
    
    _currentRotation = 0.0f;

    if (PluginConfig::Instance().IsVelocityDependent) {
        auto angularVelocity = GetAverageAngularVelocity();
        _spinSpeed = abs(angularVelocity.x) + abs(angularVelocity.y);
        // logger().debug("_spinSpeed: %f", _spinSpeed);
        auto contRotInv = Quaternion_Inverse(_controllerRotation);
        angularVelocity = Quaternion_Multiply(contRotInv, angularVelocity);
        if (angularVelocity.x < 0) _spinSpeed *= -1;
    } else {
        float speed = 30;
        if (PluginConfig::Instance().SpinDirection == SpinDir::Backward) speed *= -1;
        _spinSpeed = speed;
    }
    _spinSpeed *= PluginConfig::Instance().SpinSpeed;
    _spinState = Started;
}

void TrickManager::InPlaceRotationReturn() {
    if (_spinState == Started) {
        logger().debug("%s spin return!", _isLeftSaber ? "Left" : "Right");
        _spinState = Ending;
        // where the PC mod would start a coroutine here, we'll wind the spin down starting in next TrickManager::Update
        // so just to maintain the movement (& restore the rotation that was reset by VRController.Update iff TrickCutting):
        _InPlaceRotate(_finalSpinSpeed);
    }
}

void TrickManager::InPlaceRotationEnd() {
    if (SpinIsRelativeToVRController) {  // otherwise, _saberTrickModel->SpinT should already be at _controllerRotation
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(_saberTrickModel->SpinT, "localRotation", Quaternion_Identity));
    }
    logger().debug("%s spin end!", _isLeftSaber ? "Left" : "Right");
    _saberTrickModel->EndSpin();
    _spinState = Inactive;
    TrickEnd();
}

void TrickManager::_InPlaceRotate(float amount) {
    if (SpinIsRelativeToVRController) {
        CRASH_UNLESS(il2cpp_utils::RunMethod(_saberTrickModel->SpinT, "Rotate", Vector3_Right, amount, RotateSpace));
    } else {
        _currentRotation += amount;
        CRASH_UNLESS(il2cpp_utils::RunMethod(_saberTrickModel->SpinT, "Rotate", Vector3_Right, _currentRotation, RotateSpace));
    }
}

void TrickManager::InPlaceRotation(float power) {
    if (_spinState == Inactive) {
        InPlaceRotationStart();
    } else {
        logger().debug("%s spin continues! power %f", _isLeftSaber ? "Left" : "Right", power);
    }

    if (PluginConfig::Instance().IsVelocityDependent) {
        _finalSpinSpeed = _spinSpeed;
    } else {
        _finalSpinSpeed = _spinSpeed * pow(power, 3);  // power is the degree to which the held buttons are pressed
    }
    _InPlaceRotate(_finalSpinSpeed);
}
