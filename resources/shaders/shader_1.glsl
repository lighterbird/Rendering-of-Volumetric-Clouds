#shader vertex
#version 330 core

layout (location=0) in vec3 vertexPosition;
layout (location=1) in vec3 vertexNormalCoord;

out vec3 currentPosition;
out vec3 fragmentNormal;
out vec3 cubeMin;
out vec3 cubeMax;

uniform vec3 cubeMinInitial;
uniform vec3 cubeMaxInitial;
uniform mat4 modelMatrix;
uniform mat4 cloudModelMatrix;
uniform mat4 cameraMatrix;

void main()
{
    currentPosition = vec3(modelMatrix * vec4(vertexPosition, 1.0f));
    gl_Position = cameraMatrix * vec4(currentPosition, 1.0f);

    cubeMin = vec3(cloudModelMatrix * vec4(cubeMinInitial, 1.0f));
    cubeMax = vec3(cloudModelMatrix * vec4(cubeMaxInitial, 1.0f));

    fragmentNormal = vertexNormalCoord;
}


#shader fragment

#version 330 core

in vec3 currentPosition;
in vec3 fragmentNormal;

out vec4 color;

uniform vec3 lightPosition;
uniform vec4 lightColor;

uniform vec3 cameraPosition;

uniform vec3 surfaceColour;
uniform vec3 surfaceSpecularCoefficient;
uniform vec3 surfaceDiffuseCoefficient;

in vec3 cubeMin;
in vec3 cubeMax;

uniform float shadow_samples;
uniform float cloudScale;
uniform float maxDensity;
uniform vec3 cloudOffset;

uniform sampler3D worleyTexture;

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
vec3 IntersectionWithCube(vec3 currentPosition, vec3 viewRay)
{
    // Find intersection with cube
    viewRay = normalize(viewRay);
    bool intersected = false;
    vec3 intersectionPoint = vec3(-10000.0f, -10000.0f, -10000.0f);
    float constant, query_1, query_2;


    // int with z min
    if (intersected == false)
    {
        if (viewRay.z != 0.0f)
        {
            constant = (cubeMin.z - currentPosition.z) / viewRay.z;

            if (constant != 0.0f)
            {
                query_1 = currentPosition.x + (constant * viewRay.x);
                query_2 = currentPosition.y + (constant * viewRay.y);

                if ((query_1 > cubeMin.x) && (query_1 < cubeMax.x) && (query_2 > cubeMin.y) && (query_2 < cubeMax.y))
                {
                    intersected = true;
                    intersectionPoint = vec3(query_1, query_2, cubeMin.z);
                }
            }

        }

    }
    // int with x min
    if (intersected == false)
    {
        if (viewRay.x != 0.0f)
        {
            constant = (cubeMin.x - currentPosition.x) / viewRay.x;

            if (constant != 0.0f)
            {
                query_1 = currentPosition.y + (constant * viewRay.y);
                query_2 = currentPosition.z + (constant * viewRay.z);

                if ((query_1 > cubeMin.y) && (query_1 < cubeMax.y) && (query_2 > cubeMin.z) && (query_2 < cubeMax.z))
                {
                    intersected = true;
                    intersectionPoint = vec3(cubeMin.x, query_1, query_2);
                }
            }

        }

    }

    // int with y min
    if (intersected == false)
    {
        if (viewRay.y != 0.0f)
        {
            constant = (cubeMin.y - currentPosition.y) / viewRay.y;

            if (constant != 0.0f)
            {
                query_1 = currentPosition.x + (constant * viewRay.x);
                query_2 = currentPosition.z + (constant * viewRay.z);

                if ((query_1 > cubeMin.x) && (query_1 < cubeMax.x) && (query_2 > cubeMin.z) && (query_2 < cubeMax.z))
                {
                    intersected = true;
                    intersectionPoint = vec3(query_1, cubeMin.y, query_2);
                }
            }

        }

    }

    

    // int with x max
    if (intersected == false)
    {
        if (viewRay.x != 0.0f)
        {
            constant = (cubeMax.x - currentPosition.x) / viewRay.x;

            if (constant != 0.0f)
            {
                query_1 = currentPosition.y + (constant * viewRay.y);
                query_2 = currentPosition.z + (constant * viewRay.z);

                if ((query_1 > cubeMin.y) && (query_1 < cubeMax.y) && (query_2 > cubeMin.z) && (query_2 < cubeMax.z))
                {
                    intersected = true;
                    intersectionPoint = vec3(cubeMax.x, query_1, query_2);
                }
            }

        }

    }

    // int with y max
    if (intersected == false)
    {
        if (viewRay.y != 0.0f)
        {
            constant = (cubeMax.y - currentPosition.y) / viewRay.y;

            if (constant != 0.0f)
            {
                query_1 = currentPosition.x + (constant * viewRay.x);
                query_2 = currentPosition.z + (constant * viewRay.z);

                if ((query_1 > cubeMin.x) && (query_1 < cubeMax.x) && (query_2 > cubeMin.z) && (query_2 < cubeMax.z))
                {
                    intersected = true;
                    intersectionPoint = vec3(query_1, cubeMax.y, query_2);
                }
            }

        }

    }

    // int with z max
    if (intersected == false)
    {
        if (viewRay.z != 0.0f)
        {
            constant = (cubeMax.z - currentPosition.z) / viewRay.z;

            if (constant != 0.0f)
            {
                query_1 = currentPosition.x + (constant * viewRay.x);
                query_2 = currentPosition.y + (constant * viewRay.y);

                if ((query_1 > cubeMin.x) && (query_1 < cubeMax.x) && (query_2 > cubeMin.y) && (query_2 < cubeMax.y))
                {

                    intersected = true;
                    intersectionPoint = vec3(query_1, query_2, cubeMax.z);
                }
            }

        }

    }

    return intersectionPoint;
}

vec4 PointLight()
{	
	// Attributes needed for Light Equation
	vec3 lightVector = lightPosition - currentPosition;
	vec3 lightDirection = normalize(lightVector);
	vec3 normal = normalize(fragmentNormal);
	vec3 viewDirection = normalize(cameraPosition - currentPosition);
	vec3 reflectionDirection = reflect(-lightDirection, normal);

	// Attenuation
	float dist = length(lightVector);
	//float atten = 1.0f / (dist*dist + 0.5 * dist + 0.001f);

	// ambient lighting
	vec3 ambientIntensity = vec3(0.0f, 0.0f, 0.0f);

	// diffuse lighting
	vec3 diffuseIntensity = surfaceDiffuseCoefficient * max(dot(normal, lightDirection), 0.0f);

	// specular lighting
	vec3 specularIntensity = surfaceSpecularCoefficient * pow(max(dot(viewDirection, reflectionDirection), 0.0f), 10);

	// Final Colour
	vec4 totalLightIntensity = vec4(diffuseIntensity + specularIntensity + ambientIntensity, 1.0f);
	vec4 finalcolor = lightColor * vec4(surfaceColour, 1.0f) * totalLightIntensity;
	return vec4(finalcolor.x,finalcolor.y,finalcolor.z, 1.0f);
}
vec4 CloudShadow()
{
    
    vec3 viewRay = normalize(lightPosition - currentPosition);
    if (dot(normalize(currentPosition - cameraPosition), fragmentNormal) > 0.0f)
    {
        return vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    vec3 inPoint = IntersectionWithCube(currentPosition, viewRay);
    
    if (inPoint.x == -10000.0f) // No intersection with cloud
    {
        return vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (length(inPoint - currentPosition) > length(lightPosition - currentPosition))
    {
        return vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    // Intersected
    vec3 outPoint = IntersectionWithCube(inPoint, viewRay);

    if (outPoint.x == -10000.0f && outPoint != inPoint) // No intersection with cloud
    {
        return vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    float totalDensity = 0.0f; 
    float lengthofIntersection = length(outPoint - inPoint);
    float numSamples = shadow_samples * lengthofIntersection;
    float dx = lengthofIntersection / (numSamples + 1);

    if (lengthofIntersection < 1.0f)
    {
        return vec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    //for (int i = 1; i < int(numSamples); i++)
    //{
    //    float offset = i / numSamples;
    //    vec3 samplePoint = (1 - offset) * inPoint + (offset)*outPoint;

    //    float sampleDensity = DensityAtSamplePoint(samplePoint);
    //    totalDensity += (sampleDensity * dx);


    //}
    //float lightComingThrough = exp(-1 * (totalDensity * (maxDensity/5.0f) / (lengthofIntersection))); // Normalized density

    float max_maxDensity = (length(cubeMax - cubeMin) * shadow_samples) - 1; // Max Total points enclosed in segment
    for (int i = 1; i < int(numSamples); i++)
    {
        float offset = i / numSamples;
        vec3 samplePoint = (1 - offset) * inPoint + (offset) * outPoint;

        float sampleDensity = DensityAtSamplePoint(samplePoint);
        totalDensity += sampleDensity;


    }
    float lightComingThrough = exp(-0.5f * (totalDensity / max_maxDensity) * maxDensity); // Normalized density

    return vec4(vec3(lightComingThrough, lightComingThrough, lightComingThrough), 1.0f);
}


void main()
{
	color = PointLight() * CloudShadow();
}