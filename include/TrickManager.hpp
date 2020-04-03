#include <dlfcn.h>
#include "../extern/beatsaber-hook/shared/utils/utils.h"


enum Button {
    // Token: 0x04000A69 RID: 2665
		None = 0,
		// Token: 0x04000A6A RID: 2666
		One = 1,
		// Token: 0x04000A6B RID: 2667
		Two = 2,
		// Token: 0x04000A6C RID: 2668
		Three = 4,
		// Token: 0x04000A6D RID: 2669
		Four = 8,
		// Token: 0x04000A6E RID: 2670
		Start = 256,
		// Token: 0x04000A6F RID: 2671
		Back = 512,
		// Token: 0x04000A70 RID: 2672
		PrimaryShoulder = 4096,
		// Token: 0x04000A71 RID: 2673
		PrimaryIndexTrigger = 8192,
		// Token: 0x04000A72 RID: 2674
		PrimaryHandTrigger = 16384,
		// Token: 0x04000A73 RID: 2675
		PrimaryThumbstick = 32768,
		// Token: 0x04000A74 RID: 2676
		PrimaryThumbstickUp = 65536,
		// Token: 0x04000A75 RID: 2677
		PrimaryThumbstickDown = 131072,
		// Token: 0x04000A76 RID: 2678
		PrimaryThumbstickLeft = 262144,
		// Token: 0x04000A77 RID: 2679
		PrimaryThumbstickRight = 524288,
		// Token: 0x04000A78 RID: 2680
		PrimaryTouchpad = 1024,
		// Token: 0x04000A79 RID: 2681
		SecondaryShoulder = 1048576,
		// Token: 0x04000A7A RID: 2682
		SecondaryIndexTrigger = 2097152,
		// Token: 0x04000A7B RID: 2683
		SecondaryHandTrigger = 4194304,
		// Token: 0x04000A7C RID: 2684
		SecondaryThumbstick = 8388608,
		// Token: 0x04000A7D RID: 2685
		SecondaryThumbstickUp = 16777216,
		// Token: 0x04000A7E RID: 2686
		SecondaryThumbstickDown = 33554432,
		// Token: 0x04000A7F RID: 2687
		SecondaryThumbstickLeft = 67108864,
		// Token: 0x04000A80 RID: 2688
		SecondaryThumbstickRight = 134217728,
		// Token: 0x04000A81 RID: 2689
		SecondaryTouchpad = 2048,
		// Token: 0x04000A82 RID: 2690
		DpadUp = 16,
		// Token: 0x04000A83 RID: 2691
		DpadDown = 32,
		// Token: 0x04000A84 RID: 2692
		DpadLeft = 64,
		// Token: 0x04000A85 RID: 2693
		DpadRight = 128,
		// Token: 0x04000A86 RID: 2694
		Up = 268435456,
		// Token: 0x04000A87 RID: 2695
		Down = 536870912,
		// Token: 0x04000A88 RID: 2696
		Left = 1073741824,
		// Token: 0x04000A89 RID: 2697
		Right = -2147483648,
		// Token: 0x04000A8A RID: 2698
		Any = -1
};


struct ValueTuple {
    Vector3 item1;
    Quaternion item2;
};


struct ButtonMapping {
    public: 
        Button ThrowButton; // OVRInput.Button
        Button RotateButton; // /OVRInput.Button
        ButtonMapping(){

        };
        ButtonMapping(Button throwButton, Button rotateButton){
         ThrowButton = throwButton;
         RotateButton = rotateButton;
        };
    
        static ButtonMapping LeftButtons;
        static ButtonMapping RightButtons; 

};

class TrickManager {
    
    public:
        void LogEverything();
        bool _isLeftSaber = false;
        Il2CppObject* Saber;            // Saber
        Il2CppObject* Controller; // VRController
        void Update();
        void Start(); 

    private:
        Vector3 Vector3_zero = {0.0f, 0.0f, 0.0f};
        ValueTuple GetTrackingPos();
        void CheckButtons();
        void ThrowStart();
        void ThrowUpdate();
        void ThrowReturn();
        void ThrowEnd();
        void InPlaceRotationStart();
        void InPlaceRotationEnd();
        void InPlaceRotation();
        Il2CppObject* _vrPlatformHelper; // VRPlatformHelper
        ButtonMapping _buttonMapping;
        bool          _isThrowing = false;
        bool          _isRotatingInPlace = false;
        bool          _getBack = false;
        Il2CppObject* _rigidBody = nullptr;        // Rigidbody
        Il2CppObject* _collider = nullptr;    // BoxCollider
        Vector3       _controllerPosition = Vector3_zero; 
        Quaternion    _controllerRotation = {0.0f, 0.0f, 0.0f, 1.0f}; // same as Quaternion.identity
        Vector3       _velocity           = Vector3_zero; 
        Vector3       _prevPos            = Vector3_zero;
        float         _currentRotation;   
        float         _saberSpeed         = 0.0f;
        float         _saberRotSpeed      = 0.0f;

};