#version 330

layout(location = 0)in vec3 inPos;
layout(location = 1)in vec2 texCoord;
layout(location = 2)in vec3 obj;
layout(location = 3)in vec3 normal;
layout(location = 4)in mat4 projection_matrix;
out vec4 outColor;

const float pi = 3.14159265359;
const float EarthRadius = 6360000.0;
const float AtomosphereRadius = 6420000.0;
float Hr = 7994;
const float Hm= 1200;
const vec3 betaSR= vec3(5.8e-6,13.5e-6,33.1e-6);
const vec3 betaSM = vec3(1.0e-5,1.0e-5,1.0e-5);
const vec3 betaER = betaSR;              //Absorption ignored	
const vec3 betaEM = betaSM * 1.1;
const float g = 0.76;

uniform vec3 sunDir;
uniform vec3 cam;
uniform mat4 depthBiasVP;

//vec3 sunDir = vec3(0,-1,0);
uniform sampler2D rayleighDensity;
uniform sampler2D mieDensity;
uniform sampler2D shadowMap;
uniform sampler2D texture_diffuse;
uniform sampler2D texture_normalmap;

bool intersection(vec3 p1, vec3 p2, inout vec3 t1, inout vec3 t2, vec3 cEarth, float atmRadius_2) 
{
	vec3 rayD = normalize(p2 - p1);
	vec3 oc = p1 - cEarth;

	float b = 2.0f * dot(rayD, oc);
	float c = dot(oc, oc) - atmRadius_2;
	float disc = b*b - 4.0f*c;

	t1 = p1;
	t2 = p2;

	if(disc < 0.0f) return false;
	
	float d0 = (-b - sqrt(disc)) / 2.0f;
	float d1 = (-b + sqrt(disc)) / 2.0f;

	if (d0 > d1) {
		float aux = d0;
		d0 = d1;
		d1 = aux;
	}
	
	if( d1 < 0.0f ) return false;

	t1 = max(d0, 0.0f) * rayD + p1;
	t2 = (d1 > distance(p1, p2)) ? p2 : d1 * rayD + p1;	

	return true;
}

float RayleighPhase(float cosTheta)
{
	float result;
	result = 3/(16*pi);
	result = result *cosTheta;
	return result;
}

float MiePhase(float cos1, float cos2)
{
	float result;
	result = 3/(8*pi);
	result = result * (1-pow(g,2.0)) * (1+pow(cos2,2.0));
	result = result / (2+pow(g,2.0));
	result = result / pow((1+pow(g,2.0)-2*g*cos1),1.5);
	return result;
}

float shadowDistance(vec3 p, float cosPhi) {
	vec4 shadowMapCoord = depthBiasVP * vec4(p, 1);
	if( shadowMapCoord.x > 1.0f || shadowMapCoord.x < 0.0f || 
		shadowMapCoord.y > 1.0f || shadowMapCoord.y < 0.0f ||
		shadowMapCoord.z > 1.0f || shadowMapCoord.z < 0.0f || cosPhi < -0.24f) {
		return cosPhi < -0.24f ? 1.0f : 0.0f;
		//return 1.0f;
	}
	float bias = 0.0003;

	return ((shadowMapCoord.z - bias) - texture( shadowMap, shadowMapCoord.xy ).z);
}

float shadowDistanceBlur(vec3 p, float cosPhi) {
	vec4 shadowMapCoord = depthBiasVP * vec4(p, 1);
	if( shadowMapCoord.x > 1.0f || shadowMapCoord.x < 0.0f || 
		shadowMapCoord.y > 1.0f || shadowMapCoord.y < 0.0f ||
		shadowMapCoord.z > 1.0f || shadowMapCoord.z < 0.0f || cosPhi < -0.24f) {
		return 1.0f;//cosPhi < -0.24f ? 1.0f : 0.0f;
		//return 1.0f;
	}
	float bias = 0.0003;
	
	mat3 gaussian = mat3(2.25f/25, 3.0f/25, 2.25f/25,
						 3.0f/25,  4.0f/25, 3.0f/25,
						 2.25f/25, 3.0f/25, 2.25f/25);
	/*/
	mat3 gaussian = mat3( 0.0f,  1.0f, 0.0f,
						  1.0f, -4.0f, 1.0f,
						  0.0f,  1.0f, 0.0f);
	//*/					 
	float diff = 0.0f;  
	float currShadow = 0.0f;
	vec2 pp = vec2(0.0f, 0.0f);
	for(int i = -1; i < 2; i+=1) {
		for(int j = -1; j < 2; j+=1) {
			pp = vec2(shadowMapCoord.x + bias * i, shadowMapCoord.y + bias * j);
			currShadow = ((shadowMapCoord.z - bias) - texture( shadowMap, pp ).z);// * gaussian[i+1][j+1];
			diff += currShadow * gaussian[i+1][j+1];
		}
	}

	return diff > 0.0f? exp(-diff * 750) : 1.0f;
}

void main()
{
	vec4 transfVec = vec4(256.0f * 256.0f * 256.0f, 256.0f * 256.0f, 256.0f, 1.0f) * 256.0f;
	float N_STEPS = 30.0f;
	vec3 c_Earth = vec3(0,-EarthRadius,0);

	mat4 projTrans = transpose(projection_matrix);
	vec3 view = normalize(projTrans[2].xyz);
	vec3 normSunDir = normalize(sunDir);

	vec3 computedCam;
	vec3 obj2;

	bool q = intersection(cam, obj, computedCam, obj2, c_Earth,pow(AtomosphereRadius,2.0));
	vec3 distanceVec = (obj2 - computedCam);
	float distanceToPoint = length(distanceVec);
	float ReducedSteps = clamp(distanceToPoint/(AtomosphereRadius/10), 1.0f, 1.0f) * N_STEPS;
	vec3 delta_P = distanceVec / ReducedSteps;

	float diferential_s = length(delta_P);
	
	vec2 density_PC = vec2(0.0f, 0.0f);
	vec3 rayLeigh_In = vec3(0.0f, 0.0f, 0.0f);
	vec3 mie_In = vec3(0.0f, 0.0f, 0.0f);
	vec3 test = vec3 (0,0,0);
	if (q)
	{
		for (float s = 0.5f; s < ReducedSteps; s += 1.0f)
		{

			vec3 point = computedCam + delta_P * s;
			float h = max(length(point - c_Earth) - EarthRadius, 0.0f);
			vec3 normalEarth = normalize(point - c_Earth);
			vec2 partDenRM = 1.0 * exp(-h / vec2(Hr, Hm));
			float cosPhi = dot(normalEarth, -normSunDir);
				
			vec2 densityCoord = vec2((h / (EarthRadius-AtomosphereRadius)), (cosPhi + 1.0f) / 2.0f);
			vec4 vDAP_Ray = texture(rayleighDensity, densityCoord);
			vec4 vDAP_Mie = texture(mieDensity, densityCoord);
			float fDAP_Ray = dot(transfVec, vDAP_Ray.abgr);
			float fDAP_Mie = dot(transfVec, vDAP_Mie.abgr);
				
			vec2 density_AP = vec2(fDAP_Ray, fDAP_Mie);
				
	
			density_PC += partDenRM * diferential_s;
	
			vec2 density_APC = density_AP + density_PC;
	
			vec3 extinction_RM = exp(-( density_APC.x * betaER + density_APC.y * betaEM ));
	
			vec3 difLR = partDenRM.x * betaSR * extinction_RM * diferential_s;
			vec3 difLM = partDenRM.y * betaSM * extinction_RM * diferential_s;
	
			// Calcular visibilidad de P
			float visi = shadowDistance(point, cosPhi) > 0.0f? 0.0f : 1.0f;
	
			rayLeigh_In += difLR*visi;
			mie_In += difLM*visi;
		}
	}

	float cosTheta = dot(normalize(-distanceVec), normSunDir);

	float cos2ThetaP1 = 1.0f + clamp((cosTheta * abs(cosTheta)), 0.0f, 1.0f);
	float PhaseR = RayleighPhase(cos2ThetaP1);
	float PhaseM = MiePhase(cosTheta,cos2ThetaP1);
	
	vec3 inScattering = (rayLeigh_In * PhaseR + mie_In * PhaseM)*20;

	vec3 extintion = exp(-(density_PC.x * betaER + density_PC.y * betaEM));
	vec3 texelColor = texture(texture_diffuse, texCoord.st).rgb;
	vec3  normalmap_Color;
	if (distance(obj, c_Earth) < AtomosphereRadius) 
	{
		vec3 Kambi = vec3(0.3f, 0.3f, 0.3f);
		vec3 Kdiff = vec3(0.8f, 0.8f, 0.8f);
		vec3 Kspec = vec3(0.05f, 0.05f, 0.05f);

		float Kshadow = shadowDistanceBlur(obj, dot(normalize(obj - c_Earth), -normSunDir));	
		//vec3 vertex_normal = normalize(texture(texture_normalmap, texCoord.st).gbr * 2.0f - 1.0f);
		vec3 vertex_normal = normal;
		float diffuseFactor = min(clamp(dot(-normSunDir, vertex_normal), 0.0f, 1.0f), Kshadow);
		float specularFactor = pow(clamp(dot(normalize(obj2-computedCam), reflect(-normSunDir, normal)), 0,1 ), 5.0f);
		vec3 texelColor = texture(texture_diffuse, texCoord.st).rgb;
		normalmap_Color = Kambi*texelColor  + Kdiff*texelColor*diffuseFactor +  Kspec * pow(specularFactor, 5.0f);
	}
	
	vec3 L0_Ext = normalmap_Color * extintion;
	float pupil = 1.0f - dot(normalize(-view), normSunDir)/25;
	outColor =vec4(pupil - exp(-1.0 *(L0_Ext+ inScattering) ), 1.0f);
}