#include "../include/TrickManager.hpp"
// Define static fields
ButtonMapping ButtonMapping::LeftButtons = ButtonMapping(PrimaryIndexTrigger, PrimaryThumbstickLeft);
ButtonMapping ButtonMapping::RightButtons = ButtonMapping(SecondaryIndexTrigger, SecondaryThumbstickRight);

Il2CppClass *OVRInput = nullptr;
int minInt = -2147483648;
enum Space
{
    World,
    Self
};

enum XRNode
{
    // Token: 0x0400000B RID: 11
    LeftEye,
    // Token: 0x0400000C RID: 12
    RightEye,
    // Token: 0x0400000D RID: 13
    CenterEye,
    // Token: 0x0400000E RID: 14
    Head,
    // Token: 0x0400000F RID: 15
    LeftHand,
    // Token: 0x04000010 RID: 16
    RightHand,
    // Token: 0x04000011 RID: 17
    GameController,
    // Token: 0x04000012 RID: 18
    TrackingReference,
    // Token: 0x04000013 RID: 19
    HardwareTracker
};




Space spaceVar = Self;

Vector3 Vector3Zero = {0.0f, 0.0f, 0.0f};

void TrickManager::LogEverything()
{
    log(DEBUG, "_isThrowing %i", _isThrowing);
    log(DEBUG, "_isRotatingInPlace %i", _isRotatingInPlace);
    log(DEBUG, "RotationSpeed: %f", _saberSpeed);
}

float getDeltaTime()
{
    float result;
    if (!il2cpp_utils::RunMethod(&result, il2cpp_utils::GetClassFromName("UnityEngine", "Time"), "get_deltaTime"))
    {
        log(DEBUG, "Failed to get deltaTime");
    }
    return result;
}

Vector3 Vector3_Right = {1.0f, 0.0f, 0.0f};

Vector3 Vector3_Multiply(Vector3 &value1, float value2)
{
    Vector3 result;
    result.x = value1.x * value2;
    result.y = value1.y * value2;
    result.z = value1.z * value2;
    return result;
}

float Vector3_Distance(Vector3 &value1, Vector3 &value2)
{
    float num = value1.x - value2.x;
    float num2 = value1.y - value2.y;
    float num3 = value1.z - value2.z;
    return sqrt(num * num + num2 * num2 + num3 * num3);
}

Vector3 Vector3_Subtract(Vector3 &value1, Vector3 &value2)
{
    Vector3 result;
    result.x = value1.x - value2.x;
    result.y = value1.y - value2.y;
    result.z = value1.z - value2.z;
    return result;
}

Il2CppObject *GetComponent(Il2CppObject *object, Il2CppClass *type)
{
    Il2CppObject *componentGO;
    Il2CppObject *component;
    if (!il2cpp_utils::RunMethod(&componentGO, object, "get_gameObject"))
    {
        log(ERROR, "Failed to GetComponent!");
        return nullptr;
    }
    if (!il2cpp_utils::RunMethod(&component, componentGO, "GetComponent", il2cpp_utils::GetSystemType(type)))
    {
        log(ERROR, "Failed to GetComponent!");
        return nullptr;
    }

    return component;
}

void TrickManager::Start()
{
    OVRInput = il2cpp_utils::GetClassFromName("", "OVRInput");
    _rigidBody = GetComponent(Saber, il2cpp_utils::GetClassFromName("UnityEngine", "Rigidbody"));
    _collider = GetComponent(Saber, il2cpp_utils::GetClassFromName("UnityEngine", "BoxCollider"));
    _vrPlatformHelper = il2cpp_utils::GetFieldValue(Controller, "_vrPlatformHelper");
    if (_isLeftSaber)
    {
        _buttonMapping = _buttonMapping.LeftButtons;
    }
    else
    {
        _buttonMapping = _buttonMapping.RightButtons;
    }
}

void TrickManager::Update()
{

    ValueTuple trackingPos = GetTrackingPos();
    _controllerPosition = trackingPos.item1;
    _controllerRotation = trackingPos.item2;
    _velocity = Vector3_Subtract(_controllerPosition, _prevPos);
    _saberSpeed = Vector3_Distance(_controllerPosition, _prevPos);
    _prevPos = _controllerPosition;
    if (_getBack)
    {
        float deltaTime = getDeltaTime();
        float num = 8.0f * deltaTime;
        Il2CppObject *saberGO;
        Il2CppObject *saberTransform;
        Vector3 saberPos;
        // saberPos = this.Saber.transform.position;
        il2cpp_utils::RunMethod(&saberGO, Saber, "get_gameObject");
        il2cpp_utils::RunMethod(&saberTransform, saberGO, "get_transform");
        il2cpp_utils::RunMethod(&saberPos, saberTransform, "get_position");
        il2cpp_utils::RunMethod(&saberPos, il2cpp_utils::GetClassFromName("UnityEngine", "Vector3"), "Lerp", saberPos, _controllerPosition, num);
        il2cpp_utils::RunMethod(saberTransform, "set_position", saberPos);
        float num2 = Vector3_Distance(_controllerPosition, saberPos);
        if (num2 < 0.3f)
        {
            ThrowEnd();
        }
        else
        {
            il2cpp_utils::RunMethodUnsafe(saberTransform, "Rotate", Vector3_Right, _saberRotSpeed, &spaceVar);
        }
    }
    CheckButtons();
}

ValueTuple TrickManager::GetTrackingPos()
{
    ValueTuple result;
    ValueTuple result2;
    Il2CppObject *OVRPose;
    bool NodePose;
    Vector3*      RefVector;
    Quaternion*   RefQuaternion;

    XRNode controllerNode;
    int controllerNodeIdx;

    il2cpp_utils::RunMethod(&controllerNode, Controller, "get_node");
    il2cpp_utils::RunMethod(&controllerNodeIdx, Controller, "get_nodeIdx");

    il2cpp_utils::RunMethodUnsafe(&NodePose, _vrPlatformHelper, "GetNodePose", controllerNode, controllerNodeIdx, &RefVector, &RefQuaternion); // IF IT DOESN*T WORK REPLACE THIS WITH INVOKE!
    if(!NodePose)
    {
        log(DEBUG, "Think!?");
        result2.item1 = {-0.2f, 0.05f, 0.0f};
        result2.item2 = {0.0f, 0.0f, 0.0f, 1.0f};
        return result2;
    }

    return result;
}

void TrickManager::CheckButtons()
{
    bool ThrowButtonPressed;
    bool ThrowButtonNotPressed;
    bool RotateButtonPressed;
    bool RotateButtonNotPressed;

    if (OVRInput == nullptr)
    {
        OVRInput = il2cpp_utils::GetClassFromName("", "OVRInput");
    }
    il2cpp_utils::RunMethodUnsafe(&ThrowButtonPressed, OVRInput, "Get", &_buttonMapping.ThrowButton, minInt);
    if (ThrowButtonPressed && !_getBack)
    {
        ThrowStart();
    }
    else
    {
        il2cpp_utils::RunMethodUnsafe(&ThrowButtonNotPressed, OVRInput, "GetUp", &_buttonMapping.ThrowButton, minInt);

        if (ThrowButtonNotPressed && !_getBack)
        {
            ThrowReturn();
        }
        else
        {
            il2cpp_utils::RunMethodUnsafe(&RotateButtonPressed, OVRInput, "Get", &_buttonMapping.RotateButton, minInt);
            if (RotateButtonPressed && !_isThrowing && !_getBack)
            {
                InPlaceRotation();
            }
            else
            {
                il2cpp_utils::RunMethodUnsafe(&RotateButtonNotPressed, OVRInput, "GetUp", &_buttonMapping.RotateButton, minInt);
                if (RotateButtonNotPressed && _isRotatingInPlace)
                {
                    InPlaceRotationEnd();
                }
            }
        }
    }
}

void TrickManager::ThrowStart()
{
    if (!_isThrowing)
    {

        il2cpp_utils::RunMethod(Controller, "set_enabled", false);
        il2cpp_utils::RunMethod(_rigidBody, "set_isKinematic", false);
        Vector3 velo = Vector3_Multiply(_velocity, 220.0F);
        il2cpp_utils::RunMethod(_rigidBody, "set_velocity", velo);
        il2cpp_utils::RunMethod(_collider, "set_enabled", false);
        il2cpp_utils::RunMethod(_rigidBody, "set_isKinematic", false);
        _saberRotSpeed = _saberSpeed * 400.0f;
        _isThrowing = true;
    }
    ThrowUpdate();
}

void TrickManager::ThrowUpdate()
{
    Il2CppObject *saberGO;
    Il2CppObject *saberTransform;
    Vector3 saberPos;
    // saberPos = this.Saber.transform.position;
    il2cpp_utils::RunMethod(&saberGO, Saber, "get_gameObject");
    il2cpp_utils::RunMethod(&saberTransform, saberGO, "get_transform");
    il2cpp_utils::RunMethod(&saberPos, saberTransform, "get_position");
    il2cpp_utils::RunMethodUnsafe(saberTransform, "Rotate", Vector3_Right, _saberRotSpeed, &spaceVar);
}

void TrickManager::ThrowReturn()
{
    if (_isThrowing)
    {

        il2cpp_utils::RunMethod(_rigidBody, "set_isKinematic", true);
        il2cpp_utils::RunMethod(_rigidBody, "set_velocity", Vector3Zero);
        _getBack = true;
        _isThrowing = false;
    }
}

void TrickManager::ThrowEnd()
{
    _getBack = false;
    il2cpp_utils::RunMethod(_collider, "set_enabled", true);
    il2cpp_utils::RunMethod(Controller, "set_enabled", true);
}

void TrickManager::InPlaceRotationStart()
{
    _currentRotation = 0.0f;
    il2cpp_utils::RunMethod(Controller, "set_enabled", false);
    _isRotatingInPlace = true;
}

void TrickManager::InPlaceRotationEnd()
{
    _isRotatingInPlace = false;
    il2cpp_utils::RunMethod(Controller, "set_enabled", true);
}

void TrickManager::InPlaceRotation()
{
    Il2CppObject *saberGO;
    Il2CppObject *saberTransform;

    if (!_isRotatingInPlace)
    {
        InPlaceRotationStart();
    }
    il2cpp_utils::RunMethod(&saberGO, Saber, "get_gameObject");
    il2cpp_utils::RunMethod(&saberTransform, saberGO, "get_transform");
    il2cpp_utils::RunMethod(saberTransform, "set_rotation", _controllerRotation);
    il2cpp_utils::RunMethod(saberTransform, "set_position", _controllerPosition);
    _currentRotation -= 18.0F;
    il2cpp_utils::RunMethodUnsafe(saberTransform, "Rotate", Vector3_Right, _currentRotation, &spaceVar);
}
