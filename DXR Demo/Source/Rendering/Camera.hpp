#pragma once
#include <array>
#include <DirectXMath.h>

class Camera
{
public:
	Camera operator=(const Camera&) = delete;

	void Initialize(float AspectRatio) noexcept;
	void Update() noexcept;

	void SetPosition(const DirectX::XMVECTOR NewPosition) noexcept;
	void SetPosition(const std::array<float, 3> NewPosition) noexcept;

	void ResetPitch() noexcept;
	void ResetYaw() noexcept;

	void ResetCamera() noexcept;

	const DirectX::XMMATRIX& GetView() const noexcept;
	const DirectX::XMMATRIX& GetProjection() const noexcept;
	const DirectX::XMMATRIX  GetViewProjection() noexcept;

	const DirectX::XMVECTOR& GetPosition() const noexcept;
	const DirectX::XMFLOAT4 GetPositionFloat() const noexcept;
	const DirectX::XMVECTOR& GetTarget() const noexcept;
	const DirectX::XMVECTOR& GetUp() const noexcept;

	//void DrawGUI();

	// Required when window is resizing
	// thus Render Targets change their aspect ratio
	void OnAspectRatioChange(float NewAspectRatio) noexcept;

private:
	DirectX::XMMATRIX m_View					{ DirectX::XMMATRIX() };
	DirectX::XMMATRIX m_Projection				{ DirectX::XMMATRIX() };
	DirectX::XMMATRIX m_ViewProjection			{ DirectX::XMMATRIX() };

	DirectX::XMVECTOR m_Position				{ DirectX::XMVECTOR() };
	DirectX::XMVECTOR m_Target					{ DirectX::XMVECTOR() };
	DirectX::XMVECTOR m_Up						{ DirectX::XMVECTOR() };

	DirectX::XMMATRIX m_RotationX				{ DirectX::XMMATRIX() };
	DirectX::XMMATRIX m_RotationY				{ DirectX::XMMATRIX() };
	DirectX::XMMATRIX m_RotationMatrix			{ DirectX::XMMATRIX() };

	DirectX::XMVECTOR m_Forward					{ DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f) };
	DirectX::XMVECTOR m_Right					{ DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f) };
	DirectX::XMVECTOR m_Upward					{ DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };

	DirectX::XMVECTOR const m_DefaultPosition	{ DirectX::XMVectorSet(0.0f, 1.5f, -8.0f, 0.0f) };
	DirectX::XMVECTOR const m_DefaultTarget		{ DirectX::XMVectorSet(0.0f, 5.0f, 0.0f, 0.0f) };
	DirectX::XMVECTOR const m_DefaultUp			{ DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };

	DirectX::XMVECTOR const m_DefaultForward	{ DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f) };
	DirectX::XMVECTOR const m_DefaultRight		{ DirectX::XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f) };
	DirectX::XMVECTOR const m_DefaultUpward		{ DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) };

	float m_zNear{ 0.1f };
	float m_zFar{ 50000.0f };

public:
	// For calling camera movement from keyboard inputs
	float MoveForwardBack{ 0.0f };
	float MoveRightLeft{ 0.0f };
	float MoveUpDown{ 0.0f };

	float m_Pitch{ 0.0f };
	float m_Yaw{ 0.0f };

	float GetCameraSpeed() const noexcept { return m_CameraSpeed; };
	void SetCameraSpeed(float NewSpeed) noexcept { m_CameraSpeed = NewSpeed; }

	float GetZNear() const noexcept { return m_zNear; }
	void SetZNear(float NewZ) noexcept { m_zNear = NewZ; }
	float GetZFar() const noexcept { return m_zFar; }
	void SetZFar(float NewZ) noexcept { m_zFar = NewZ; }

	// For GUI usage
	std::array<float, 3> m_CameraSlider;

	inline static float m_CameraSpeed{ 5.0f };

};
