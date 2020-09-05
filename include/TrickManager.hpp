#pragma once

#include <dlfcn.h>
#include <unordered_map>
#include <unordered_set>

#include "beatsaber-hook/shared/utils/typedefs.h"
#include "AllEnums.hpp"
#include "InputHandler.hpp"
#include "SaberTrickModel.hpp"

// Conventions:
// tX means the type (usually System.Type i.e. Il2CppReflectionType) of X
// cX means the Il2CppClass of X
// xT means the .transform of unity object X


const Vector3 Vector3_Zero = {0.0f, 0.0f, 0.0f};
const Vector3 Vector3_Right = {1.0f, 0.0f, 0.0f};
const Quaternion Quaternion_Identity = {0.0f, 0.0f, 0.0f, 1.0f};


struct ValueTuple {
    Vector3 item1;
    Quaternion item2;
};


struct ButtonMapping {
    public:
		bool left;
		// static ButtonMapping LeftButtons;
		// static ButtonMapping RightButtons;
		std::unordered_map<TrickAction, std::unordered_set<std::unique_ptr<InputHandler>>> actionHandlers;

		void Update();

        ButtonMapping() {};
        ButtonMapping(bool isLeft) {
			left = isLeft;
			Update();
        };
};


class TrickManager {
    public:
        void LogEverything();
        bool _isLeftSaber = false;
        Il2CppObject* Saber;         // ::Saber
        Il2CppObject* VRController;  // ::VRController
		TrickManager* other = nullptr;
		static void StaticClear();
		void Clear();
		void Start();
		void EndTricks();
		static void StaticPause();
		void PauseTricks();
		static void StaticResume();
    	void ResumeTricks();
		static void StaticFixedUpdate();
		void FixedUpdate();
        void Update();
	
	protected:
		TrickState _throwState;  // initialized in Start
		TrickState _spinState;

    private:
		void Start2();
        ValueTuple GetTrackingPos();
        void CheckButtons();
        void ThrowStart();
        void ThrowUpdate();
        void ThrowReturn();
        void ThrowEnd();
        void InPlaceRotationStart();
		void InPlaceRotation(float power);
		void _InPlaceRotate(float amount);
		void InPlaceRotationReturn();
        void InPlaceRotationEnd();
		void TrickStart();
		void TrickEnd();
		void AddProbe(const Vector3& vel, const Vector3& ang);
		Vector3 GetAverageVelocity();
		Vector3 GetAverageAngularVelocity();
        Il2CppObject* _vrPlatformHelper;  			// ::VRPlatformHelper
        ButtonMapping _buttonMapping;
        Il2CppObject* _collider = nullptr;    		// BoxCollider
        Vector3       _controllerPosition = Vector3_Zero;
        Quaternion    _controllerRotation = Quaternion_Identity;
        Vector3       _prevPos            = Vector3_Zero;
		Vector3       _angularVelocity    = Vector3_Zero;
        Quaternion    _prevRot            = Quaternion_Identity;
        float         _currentRotation;
        float         _saberSpeed         = 0.0f;
        float         _saberRotSpeed      = 0.0f;
		size_t _currentProbeIndex;
		std::vector<Vector3> _velocityBuffer;
		std::vector<Vector3> _angularVelocityBuffer;
		float _spinSpeed;
		float _finalSpinSpeed;
		SaberTrickModel* _saberTrickModel = nullptr;
		float _timeSinceStart = 0.0f;
		Il2CppObject* _originalSaberModelT = nullptr;
		Il2CppString* _saberName = nullptr;
		Il2CppString* _basicSaberName = nullptr;  // only exists up until Start2
		Il2CppObject* _saberT = nullptr;  // needed for effecient Start2 checking in Update
		// Vector3 _VRController_position_offset = Vector3_Zero;
		Vector3 _throwReturnDirection = Vector3_Zero;
		// float _prevThrowReturnDistance;
		Il2CppObject* _fakeTransform;  // will "replace" VRController's transform during trickCutting throws
};
