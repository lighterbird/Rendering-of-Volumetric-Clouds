#shader vertex
#version 330 core

layout (location=0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;

out vec3 currentPosition;
out vec3 fragmentNormal;
out vec3 cubeMin;
out vec3 cubeMax;

uniform mat4 modelMatrix;
uniform mat4 cameraMatrix;
uniform vec3 cubeMinInitial;
uniform vec3 cubeMaxInitial;

void main()
{
    currentPosition = vec3(modelMatrix * vec4(vertexPosition, 1.0f));
    fragmentNormal = vec3(modelMatrix * vec4(vertexNormal, 0.0f));
    //fragmentNormal = vertexNormal;
    cubeMin = vec3(modelMatrix * vec4(cubeMinInitial, 1.0f));
    cubeMax = vec3(modelMatrix * vec4(cubeMaxInitial, 1.0f));
    gl_Position = cameraMatrix * vec4(currentPosition, 1.0f);
}


#shader fragment

#version 330 core

in vec3 currentPosition;
in vec3 fragmentNormal;
in vec3 cubeMin;
in vec3 cubeMax;

out vec4 color;

uniform vec3 lightPosition;
uniform vec4 lightColor;

uniform vec3 cameraPosition;


uniform vec3 surfaceColour;
uniform vec3 surfaceSpecularCoefficient;
uniform vec3 surfaceDiffuseCoefficient;

uniform float n_samples;
uniform float n_lightSamples;
uniform float maxDensity;
uniform float falloff;
uniform float cloudScale;
uniform vec3 cloudOffset;

uniform sampler3D worleyTexture;

vec3 IntersectionWithCube(vec3 currentPosition, vec3 viewRay)
{
    // Find intersection with cube
    viewRay = normalize(viewRay);
    bool intersected = false;
    vec3 intersectionPoint = vec3(-10000.0f, -10000.0f, -10000.0f);
    float constant, query_1, query_2;

    // int with x min
    if(intersected == false)
    {
        if(viewRay.x != 0.0f)
        {
            constant = (cubeMin.x - currentPosition.x) / viewRay.x;

            if(constant > 0.0f)
            {
                query_1 = currentPosition.y + (constant * viewRay.y);
                query_2 = currentPosition.z + (constant * viewRay.z);

                if((query_1 > cubeMin.y) && (query_1 < cubeMax.y) && (query_2 > cubeMin.z) && (query_2 < cubeMax.z)) 
                {
                    intersected = true;
                    intersectionPoint = vec3(cubeMin.x, query_1, query_2);
                }
            }
            
        }
        
    }

    // int with y min
    if(intersected == false)
    {
        if(viewRay.y != 0.0f)
        {
            constant = (cubeMin.y - currentPosition.y) / viewRay.y;
            
            if(constant > 0.0f)
            {
                query_1 = currentPosition.x + (constant * viewRay.x);
                query_2 = currentPosition.z + (constant * viewRay.z);

                if((query_1 > cubeMin.x) && (query_1 < cubeMax.x) && (query_2 > cubeMin.z) && (query_2 < cubeMax.z)) 
                {
                    intersected = true;
                    intersectionPoint = vec3(query_1, cubeMin.y, query_2);
                }
            }
            
        }
        
    }

    // int with z min
    if(intersected == false)
    {
        if(viewRay.z != 0.0f)
        {
            constant = (cubeMin.z - currentPosition.z) / viewRay.z;

            if(constant > 0.0f)
            {
                query_1 = currentPosition.x + (constant * viewRay.x);
                query_2 = currentPosition.y + (constant * viewRay.y);

                if((query_1 > cubeMin.x) && (query_1 < cubeMax.x) && (query_2 > cubeMin.y) && (query_2 < cubeMax.y)) 
                {
                    intersected = true;
                    intersectionPoint = vec3(query_1, query_2, cubeMin.z);
                }
            }
            
        }
        
    }

    // int with x max
    if(intersected == false)
    {
        if(viewRay.x != 0.0f)
        {
            constant = (cubeMax.x - currentPosition.x) / viewRay.x;
            
            if(constant > 0.0f)
            {
                query_1 = currentPosition.y + (constant * viewRay.y);
                query_2 = currentPosition.z + (constant * viewRay.z);

                if((query_1 > cubeMin.y) && (query_1 < cubeMax.y) && (query_2 > cubeMin.z) && (query_2 < cubeMax.z)) 
                {
                    intersected = true;
                    intersectionPoint = vec3(cubeMax.x, query_1, query_2);
                }
            }
            
        }
        
    }

    // int with y max
    if(intersected == false)
    {
        if(viewRay.y != 0.0f)
        {
            constant = (cubeMax.y - currentPosition.y) / viewRay.y;
            
            if(constant > 0.0f)
            {
                query_1 = currentPosition.x + (constant * viewRay.x);
                query_2 = currentPosition.z + (constant * viewRay.z);

                if((query_1 > cubeMin.x) && (query_1 < cubeMax.x) && (query_2 > cubeMin.z) && (query_2 < cubeMax.z)) 
                {
                    intersected = true;
                    intersectionPoint = vec3(query_1, cubeMax.y, query_2);
                }
            }
            
        }
        
    }

    // int with z max
    if(intersected == false)
    {
        if(viewRay.z != 0.0f)
        {
            constant = (cubeMax.z - currentPosition.z) / viewRay.z;

            if(constant > 0.0f)
            {
                query_1 = currentPosition.x + (constant * viewRay.x);
                query_2 = currentPosition.y + (constant * viewRay.y);

                if((query_1 > cubeMin.x) && (query_1 < cubeMax.x) && (query_2 > cubeMin.y) && (query_2 < cubeMax.y)) 
                {
            
                    intersected = true;
                    intersectionPoint = vec3(query_1, query_2, cubeMax.z);
                }
            }
           
        }
        
    }

    return intersectionPoint;
}
float DensityAtSamplePoint(vec3 samplePoint)
{
    //float density = 0.0f;
    vec4 texSample = texture(worleyTexture, (samplePoint + cloudOffset) * cloudScale);
    
    //if(texSample.a < 0.9) texSample.a = 0.0f;
    //if(texSample.b < 0.8) texSample.b = 0.0f;
    //if(texSample.g < 0.7) texSample.g = 0.0f;
    //if(texSample.r < 0.6) texSample.r = 0.0f;
    

    //float density = ((texSample.r) + (texSample.g) + (texSample.b) + (texSample.a)) / 4;
    float density = pow(texSample.r * texSample.g * texSample.b * texSample.a, 0.3f);

    if (density < 0.75f) density = 0.0f;
    //if (density > 0.95f) density = 1.0f;

    return density;
}
float LightIntensityAtSamplePoint(vec3 samplePoint, vec3 viewRay)
{
    vec3 lightRay = normalize(lightPosition - samplePoint);
    vec3 intersectionPoint = IntersectionWithCube(samplePoint, lightRay);
    //float lightFactor = abs(dot(lightRay, viewRay));

    if (length(intersectionPoint - samplePoint) > length(lightPosition - samplePoint))// Light is inside the cloud
    {
        intersectionPoint = lightPosition;
    }

    float totalDensity = 0.0f;
    float lengthofIntersection = length(intersectionPoint - samplePoint);
    float numLightSamples = n_lightSamples * lengthofIntersection;
    float dx = lengthofIntersection / numLightSamples;

    if (numLightSamples <= 1.0f)
    {
        return 1.0f;
    }
    /*for (int i = 0; i <= int(numLightSamples); i++)
    {
        float offset = i / numLightSamples;
        vec3 lightSamplePoint = ((1 - offset) * samplePoint) + ((offset) * intersectionPoint);
        totalDensity += DensityAtSamplePoint(lightSamplePoint) * dx;

    }*/
    

    //float densityFactor = exp(-1 * (totalDensity) / (lengthofIntersection)); // Normalized density  
    //float lightDist = length(lightPosition - samplePoint);
    //float atten = 1 / (lightDist + 0.1f);

    float max_maxDensity = (length(cubeMax - cubeMin) * n_lightSamples) - 1;
    for (int i = 1; i < int(numLightSamples); i++)
    {
        float offset = i / numLightSamples;
        vec3 lightSamplePoint = ((1 - offset) * samplePoint) + ((offset) * intersectionPoint);
        totalDensity += DensityAtSamplePoint(lightSamplePoint);

    }
    float densityFactor = exp(-1 * (totalDensity / max_maxDensity) * maxDensity); // Normalized density  
    return densityFactor;
   
}
vec4 VolumetricRenderCube()
{
    // Find view ray
    vec3 a0 = lightPosition;
    vec4 b0 = lightColor;
    vec3 c0 = surfaceSpecularCoefficient;
    vec3 d0 = surfaceDiffuseCoefficient;
    float e0 = falloff;

    vec3 viewRay = normalize(currentPosition - cameraPosition);

    float dotproduct = dot(normalize(fragmentNormal), viewRay);
    if (dotproduct > 0.0f) return vec4(abs(fragmentNormal), 0.0f);
    //else return vec4(vec3(0.0f, 0.0f, 0.0f), 0.0f);

    vec3 intersectionPoint = IntersectionWithCube(currentPosition, viewRay);


    if (intersectionPoint.x != -10000.0f)
    {
        // Sample n points on the ray
        float totalDensity = 0.0f; // To determine Opacity(alpha) of the point
        float totalLightIntensity = 0.0f; // To determine Colour of the point
        float lengthofIntersection = length(intersectionPoint - currentPosition);
        float numSamples = (n_samples * lengthofIntersection); // No. of parts the segment is divided into
        float dx = lengthofIntersection / (numSamples); // Length of each part
        // No. of sample points between the 2 intersection points on the line segment = numSamples - 1

        if (numSamples <= 1.0f)
        {
            return vec4(surfaceColour, 0.0f);
        }
        //for (int i = 1; i < int(numSamples); i++)
        //{
        //    float offset = i / numSamples;
        //    vec3 samplePoint = (1 - offset) * currentPosition + (offset)*intersectionPoint;

        //    float sampleDensity = DensityAtSamplePoint(samplePoint);
        //    totalDensity += (sampleDensity * dx);
        //    totalLightIntensity += (exp(-totalDensity / (i )) * LightIntensityAtSamplePoint(samplePoint, viewRay) * dx);


        //}
        //float densityFactor = 1 - exp(-1 * (totalDensity * maxDensity / (lengthofIntersection))); // Normalized density
        //float lightFactor = totalLightIntensity / (lengthofIntersection); // Normalized intensity

        //for (int i = 1; i <= int(numSamples); i++)
        //{
        //    float offset = i / numSamples;
        //    vec3 samplePoint = ((1.0f - offset) * currentPosition) + ((offset)*intersectionPoint);

        //    float sampleDensity = DensityAtSamplePoint(samplePoint);
        //    totalDensity += (sampleDensity * (1.0f - offset));
        //    totalLightIntensity += (exp(-totalDensity / i) * LightIntensityAtSamplePoint(samplePoint, viewRay) * dx);


        //}
        //float densityFactor = 1 - exp(-0.05 * (totalDensity * maxDensity)); // Normalized density
        //float lightFactor = totalLightIntensity / (lengthofIntersection); // Normalized intensity
        float max_maxDensity = (length(cubeMax - cubeMin) * n_samples) - 1; // Max Total points enclosed in segment
        for (int i = 1; i < int(numSamples); i++)
        {
            float offset = i / numSamples;
            vec3 samplePoint = (1 - offset) * currentPosition + (offset)*intersectionPoint;

            float sampleDensity = DensityAtSamplePoint(samplePoint);
            totalDensity += sampleDensity;
            totalLightIntensity += LightIntensityAtSamplePoint(samplePoint, viewRay);


        }
        //float densityFactor = (totalDensity / max_maxDensity) * maxDensity;
        float densityFactor = 1 - exp(-1 * (totalDensity / max_maxDensity) * maxDensity); // Normalized density
        float lightFactor = totalLightIntensity / (numSamples - 1); // Normalized intensity

        return vec4(lightFactor * vec3(lightColor) * surfaceColour, densityFactor);
    }
    return vec4(abs(fragmentNormal), 1.0f);
    
    
}

void main()
{
    color = VolumetricRenderCube();
}