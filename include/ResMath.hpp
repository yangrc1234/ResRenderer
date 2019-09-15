#pragma once
namespace ResRenderer
{
	struct Vector2
	{
		float x;
		float y;
	};

	struct Vector3
	{
		float x;
		float y;
		float z;
	};

	struct Color
	{
		inline Color(float r, float g, float b, float a)
		{
			this->r = r;
			this->g = g;
			this->b = b;
			this->a = a;
		}
		float r, g, b, a;
	};

	struct Vector4
	{
		inline Vector4(const Color& color) {
			x = color.r;
			y = color.g;
			z = color.b;
			w = color.a;
		}
		float x;
		float y;
		float z;
		float w;
		
	};
}