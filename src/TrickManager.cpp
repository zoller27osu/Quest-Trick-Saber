#include "../include/TrickManager.hpp"
// Define static fields
// According to Oculus documentation, left is always Primary and Right is always secondary UNLESS referred to individually.
// https://developer.oculus.com/reference/unity/v14/class_o_v_r_input
ButtonMapping ButtonMapping::LeftButtons = ButtonMapping(PrimaryIndexTrigger, PrimaryThumbstickLeft);
ButtonMapping ButtonMapping::RightButtons = ButtonMapping(SecondaryIndexTrigger, SecondaryThumbstickRight);
Space RotateSpace = Self;

Il2CppClass *OVRInput = nullptr;
Controller ControllerMask;


void TrickManager::LogEverything() {
    log(DEBUG, "_isThrowing %i", _isThrowing);
    log(DEBUG, "_isRotatingInPlace %i", _isRotatingInPlace);
    log(DEBUG, "RotationSpeed: %f", _saberSpeed);
}

float getDeltaTime() {
    float result;
    RET_0_UNLESS(il2cpp_utils::RunMethod(&result, "UnityEngine", "Time", "get_deltaTime"));
    return result;
}


Vector3 Vector3_Zero = {0.0f, 0.0f, 0.0f};
Vector3 Vector3_Right = {1.0f, 0.0f, 0.0f};

Vector3 Vector3_Multiply(Vector3 &vec, float scalar) {
    Vector3 result;
    result.x = vec.x * scalar;
    result.y = vec.y * scalar;
    result.z = vec.z * scalar;
    return result;
}

float Vector3_Distance(Vector3 &a, Vector3 &b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

Vector3 Vector3_Subtract(Vector3 &a, Vector3 &b) {
    Vector3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

Il2CppObject *GetComponent(Il2CppObject *object, Il2CppClass *type) {
    Il2CppObject *component;
    // Blame Sc2ad if this simplification doesn't work
    RET_0_UNLESS(il2cpp_utils::RunMethod(&component, object, "GetComponent", il2cpp_utils::GetSystemType(type)));
    return component;
}


void TrickManager::Start() {
    OVRInput = il2cpp_utils::GetClassFromName("", "OVRInput");
    CRASH_UNLESS(il2cpp_utils::GetFieldValue(&ControllerMask, il2cpp_utils::GetClassFromName("", "OVRInput/Controller"), "All"));

    _rigidBody = GetComponent(Saber, il2cpp_utils::GetClassFromName("UnityEngine", "Rigidbody"));
    _collider  = GetComponent(Saber, il2cpp_utils::GetClassFromName("UnityEngine", "BoxCollider"));
    _vrPlatformHelper = CRASH_UNLESS(il2cpp_utils::GetFieldValue(VRController, "_vrPlatformHelper"));

    if (_isLeftSaber) {
        _buttonMapping = ButtonMapping::LeftButtons;
    } else {
        _buttonMapping = ButtonMapping::RightButtons;
    }
}

void TrickManager::Update() {
    if (!_vrPlatformHelper) return;
    ValueTuple trackingPos = GetTrackingPos();
    _controllerPosition = trackingPos.item1;
    _controllerRotation = trackingPos.item2;
    _velocity = Vector3_Subtract(_controllerPosition, _prevPos);
    _saberSpeed = Vector3_Distance(_controllerPosition, _prevPos);
    _prevPos = _controllerPosition;
    if (_getBack) {
        float deltaTime = getDeltaTime();
        float num = 8.0f * deltaTime;
        Il2CppObject *saberGO;
        Il2CppObject *saberTransform;
        Vector3 saberPos;
        // saberPos = this.Saber.transform.position;
        CRASH_UNLESS(il2cpp_utils::RunMethod(&saberGO, Saber, "get_gameObject"));
        CRASH_UNLESS(il2cpp_utils::RunMethod(&saberTransform, saberGO, "get_transform"));
        CRASH_UNLESS(il2cpp_utils::RunMethod(&saberPos, saberTransform, "get_position"));
        CRASH_UNLESS(il2cpp_utils::RunMethod(&saberPos, "UnityEngine", "Vector3", "Lerp", saberPos, _controllerPosition, num));
        CRASH_UNLESS(il2cpp_utils::RunMethod(saberTransform, "set_position", saberPos));
        float num2 = Vector3_Distance(_controllerPosition, saberPos);
        if (num2 < 0.3f) {
            ThrowEnd();
        } else {
            CRASH_UNLESS(il2cpp_utils::RunMethod(saberTransform, "Rotate", Vector3_Right, _saberRotSpeed, RotateSpace));
        }
    }
    CheckButtons();
}

ValueTuple TrickManager::GetTrackingPos() {
    ValueTuple result;
    bool nodePose;

    XRNode controllerNode;
    int controllerNodeIdx;

    CRASH_UNLESS(il2cpp_utils::RunMethod(&controllerNode, VRController, "get_node"));
    CRASH_UNLESS(il2cpp_utils::RunMethod(&controllerNodeIdx, VRController, "get_nodeIdx"));

    CRASH_UNLESS(il2cpp_utils::RunMethod(&nodePose, _vrPlatformHelper, "GetNodePose", controllerNode, controllerNodeIdx,
        result.item1, result.item2));
    if (!nodePose) {
        result.item1 = {-0.2f, 0.05f, 0.0f};
        result.item2 = {0.0f, 0.0f, 0.0f, 1.0f};
    }
    return result;
}

void TrickManager::CheckButtons() {
    bool ThrowButtonPressed;
    bool ThrowButtonNotPressed;
    bool RotateButtonPressed;
    bool RotateButtonNotPressed;

    if (!OVRInput) {
        OVRInput = il2cpp_utils::GetClassFromName("", "OVRInput");
    }

    il2cpp_utils::RunMethod(&ThrowButtonPressed, OVRInput, "Get", _buttonMapping.ThrowButton, ControllerMask);
    if (ThrowButtonPressed && !_getBack) {
        ThrowStart();
    } else {

        il2cpp_utils::RunMethod(&ThrowButtonNotPressed, OVRInput, "GetUp", _buttonMapping.ThrowButton, ControllerMask);
        if (ThrowButtonNotPressed && !_getBack) {
            ThrowReturn();
        } else {

            il2cpp_utils::RunMethod(&RotateButtonPressed, OVRInput, "Get", _buttonMapping.RotateButton, ControllerMask);
            if (RotateButtonPressed && !_isThrowing && !_getBack) {
                InPlaceRotation();
            } else {

                il2cpp_utils::RunMethod(&RotateButtonNotPressed, OVRInput, "GetUp", _buttonMapping.RotateButton, ControllerMask);
                if (RotateButtonNotPressed && _isRotatingInPlace) {
                    InPlaceRotationEnd();
                }
            }
        }
    }
}


void TrickManager::ThrowStart() {
    log(DEBUG, "%s throw start!", _isLeftSaber ? "Left" : "Right");
    if (!_isThrowing) {
        CRASH_UNLESS(il2cpp_utils::RunMethod(VRController, "set_enabled", false));
        CRASH_UNLESS(il2cpp_utils::RunMethod(_rigidBody, "set_isKinematic", false));
        Vector3 velo = Vector3_Multiply(_velocity, 220.0F);
        CRASH_UNLESS(il2cpp_utils::RunMethod(_rigidBody, "set_velocity", velo));
        CRASH_UNLESS(il2cpp_utils::RunMethod(_collider, "set_enabled", false));
        _saberRotSpeed = _saberSpeed * 400.0f;
        _isThrowing = true;
    }

    ThrowUpdate();
}

void TrickManager::ThrowUpdate() {
    Il2CppObject *saberGO;
    Il2CppObject *saberTransform;

    // saberPos = this.Saber.transform.position;
    CRASH_UNLESS(il2cpp_utils::RunMethod(&saberGO, Saber, "get_gameObject"));
    CRASH_UNLESS(il2cpp_utils::RunMethod(&saberTransform, saberGO, "get_transform"));
    CRASH_UNLESS(il2cpp_utils::RunMethod(saberTransform, "Rotate", Vector3_Right, _saberRotSpeed, RotateSpace));
}

void TrickManager::ThrowReturn() {
    if (_isThrowing) {
        CRASH_UNLESS(il2cpp_utils::RunMethod(_rigidBody, "set_isKinematic", true));
        CRASH_UNLESS(il2cpp_utils::RunMethod(_rigidBody, "set_velocity", Vector3_Zero));
        _getBack = true;
        _isThrowing = false;
    }
}

void TrickManager::ThrowEnd() {
    _getBack = false;
    CRASH_UNLESS(il2cpp_utils::RunMethod(_collider, "set_enabled", true));
    CRASH_UNLESS(il2cpp_utils::RunMethod(VRController, "set_enabled", true));
}


void TrickManager::InPlaceRotationStart() {
    log(DEBUG, "%s rotate start!", _isLeftSaber ? "Left" : "Right");
    _currentRotation = 0.0f;
    CRASH_UNLESS(il2cpp_utils::RunMethod(VRController, "set_enabled", false));
    _isRotatingInPlace = true;
}

void TrickManager::InPlaceRotationEnd() {
    _isRotatingInPlace = false;
    CRASH_UNLESS(il2cpp_utils::RunMethod(VRController, "set_enabled", true));
}

void TrickManager::InPlaceRotation() {
    Il2CppObject *saberGO;
    Il2CppObject *saberTransform;

    if (!_isRotatingInPlace) {
        InPlaceRotationStart();
    }

    CRASH_UNLESS(il2cpp_utils::RunMethod(&saberGO, Saber, "get_gameObject"));
    CRASH_UNLESS(il2cpp_utils::RunMethod(&saberTransform, saberGO, "get_transform"));
    CRASH_UNLESS(il2cpp_utils::RunMethod(saberTransform, "set_rotation", _controllerRotation));
    CRASH_UNLESS(il2cpp_utils::RunMethod(saberTransform, "set_position", _controllerPosition));
    _currentRotation -= 18.0F;
    CRASH_UNLESS(il2cpp_utils::RunMethod(saberTransform, "Rotate", Vector3_Right, _currentRotation, RotateSpace));
}
