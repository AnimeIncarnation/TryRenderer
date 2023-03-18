#pragma once
#ifndef _LIGHT_H_
#define _LIGHT_H_
#include "../DXMath/MathHelper.h"


class Light
{
public:
    enum class LightType
    {
        PARALLEL,
        POINT,
        AREA
    };


    Light(LightType type = LightType::POINT) :type(type), strength(0.5f, 0.5f, 0.5f)
    {
        switch (type)
        {
        case LightType::AREA:
            direction = { 1.f,1.f,1.f };
            spotPower = 64.f;
        case LightType::POINT:
            position = { -0.2f, -0.2f, -0.2f };
            fallOffStart = 1.f;
            fallOffEnd = 10.f;
            break;
        case LightType::PARALLEL:
            direction = { 1.f,1.f,1.f };
        }
    }
    void Update()
    {
        if (type == LightType::AREA);
        else if (type == LightType::POINT)
        {
            float offset = 0.01;
            
            if (!changePoint)
                position += {offset, offset, offset};
            else
                position -= {offset, offset, offset};
            if (position.x >= 1)
                changePoint = !changePoint;
        }
        else if (type == LightType::PARALLEL)
        {
            float offset2 = 0.01;
            
            if (!changeParallel)
                direction.x += offset2;
            else
                direction.x -= offset2;
            if (direction.x >= 1)
                changeParallel = !changeParallel;
        }
    }
    Math::XMFLOAT3 GetDirection() const { return direction; }
    Math::XMFLOAT3 GetPosition() const { return position; }
    Math::XMFLOAT3 GetStrength() const { return strength; }

private:
    LightType type;
    Math::XMFLOAT3 strength;    //共有
    Math::XMFLOAT3 direction;   //方/聚
    Math::XMFLOAT3 position;    //点/聚
    float fallOffStart;         //点/聚
    float fallOffEnd;           //点/聚
    float spotPower;            //聚
    bool changePoint = false;
    bool changeParallel = false;
};

#endif
