#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};

		void Initialize(float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);

			origin = _origin;
		}

		void CalculateViewMatrix()
		{
			//TODO W1
			//ONB => invViewMatrix
			right = Vector3::Cross(Vector3::UnitY, forward);
			right.Normalize();
			up = Vector3::Cross(forward, right);
			up.Normalize();
			invViewMatrix = { right, up, forward, origin };

			//Inverse(ONB) => ViewMatrix
			viewMatrix = invViewMatrix.Inverse();

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixlookatlh
		}

		void CalculateProjectionMatrix()
		{
			//TODO W2

			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...) [not implemented yet]
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//Camera Update Logic
			
			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);
			const float moveSpeed{ 5.f };

			if (pKeyboardState[SDL_SCANCODE_W])
			{
				origin += forward * moveSpeed * deltaTime;
			}
			else if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin -= forward * moveSpeed * deltaTime;
			}
			if (pKeyboardState[SDL_SCANCODE_A])
			{
				origin -= right * moveSpeed * deltaTime;
			}
			else if (pKeyboardState[SDL_SCANCODE_D])
			{
				origin += right * moveSpeed * deltaTime;
			}

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);
			const float rotationSpeed{ 0.5f };


			if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
				{
					origin.y += mouseY * deltaTime;
				}
				else
				{
					origin += forward * float(mouseY) * deltaTime;
					totalYaw = mouseX * rotationSpeed * deltaTime;
					Matrix yawRotation{ Matrix::CreateRotationY(totalYaw) };
					forward = yawRotation.TransformVector(forward);
				}
			}
			else if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				totalPitch = -mouseY * rotationSpeed * deltaTime;
				totalYaw = mouseX * rotationSpeed * deltaTime;

				Matrix finalRotation{ Matrix::CreateRotationY(totalYaw) * Matrix::CreateRotationX(totalPitch) };
				forward = finalRotation.TransformVector(forward);
			}

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
