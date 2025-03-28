#version 450 core

#define NUM_VERTICES 6

layout(location = 0) in vec3 aPosition;
layout(location = 1) in float aIndex;

// Transform buffer
layout(location = 2) in vec4 aMRow0;
layout(location = 3) in vec4 aMRow1;
layout(location = 4) in vec4 aMRow2;

layout(location = 0) out vec2 o_texPos;
layout(location = 1) out vec4 o_color;
layout(location = 2) out flat uint o_type;

layout (std140, binding = 0) uniform Camera
{
	mat4 uViewProjectionMatrix;
	mat4 uInverseViewProjectionMatrix;
	mat4 uProjectionMatrix;
	mat4 uViewMatrix;
};

//---------------------------------------------------

struct Particle
{
	vec2 pos;
	float height;
	float angle;
	float tiltAngle;
	float angleVel;
	float opacity;
	float temp;
};

layout(std430, binding = 20) readonly buffer Particles
{
    Particle particlesData[];
} particles;

//---------------------------------------------------

layout(std140, binding = 19) uniform Params
{
	float u_time;

	uint u_numStars;

	float u_starSize;
	float u_dustSize;
	float u_h2Size;

	float u_h2DistCheck;
};

//---------------------------------------------------

float ease_in_circ(float x)
{
	return x >= 1.0 ? 1.0 : 1.0 - sqrt(1.0 - x * x);
}

//------------------------------------------------------------

vec2 calc_pos(Particle particle)
{
	float angle = particle.angle + particle.angleVel * u_time;
	
	float cosAngle = cos(angle);
	float sinAngle = sin(angle);
	float cosTilt = cos(particle.tiltAngle);
	float sinTilt = sin(particle.tiltAngle);

	vec2 pos = particle.pos;

	return vec2(pos.x * cosAngle * cosTilt - pos.y * sinAngle * sinTilt,
	            pos.x * cosAngle * sinTilt + pos.y * sinAngle * cosTilt);
}

vec3 color_from_temp(float temp)
{
	const float minTemp = 1000.0;
	const float maxTemp = 10000.0;
	const int numColors = 200;

	const vec3 colors[200] = {
		vec3(1       , 0.000000, 0.000000),
		vec3(1       , 0.000672, 0.000000),
		vec3(1       , 0.011348, 0.000000),
		vec3(1       , 0.022136, 0.000000),
		vec3(1       , 0.033018, 0.000000),
		vec3(1       , 0.043977, 0.000000),
		vec3(1       , 0.054999, 0.000000),
		vec3(1       , 0.066070, 0.000000),
		vec3(1       , 0.077177, 0.000000),
		vec3(1       , 0.088301, 0.000000),
		vec3(1       , 0.099455, 0.000000),
		vec3(1       , 0.110607, 0.000000),
		vec3(1       , 0.121756, 0.000000),
		vec3(1       , 0.132894, 0.000000),
		vec3(1       , 0.144013, 0.000000),
		vec3(1       , 0.155107, 0.000000),
		vec3(1       , 0.166171, 0.000000),
		vec3(1       , 0.177198, 0.000000),
		vec3(1       , 0.188184, 0.000000),
		vec3(1       , 0.199125, 0.000000),
		vec3(1       , 0.210015, 0.002490),
		vec3(1       , 0.220853, 0.005844),
		vec3(1       , 0.231633, 0.009450),
		vec3(1       , 0.242353, 0.013308),
		vec3(1       , 0.253010, 0.017416),
		vec3(1       , 0.263601, 0.021773),
		vec3(1       , 0.274125, 0.026376),
		vec3(1       , 0.284579, 0.031222),
		vec3(1       , 0.294962, 0.036309),
		vec3(1       , 0.305271, 0.041633),
		vec3(1       , 0.315505, 0.047190),
		vec3(1       , 0.325662, 0.052977),
		vec3(1       , 0.335742, 0.058988),
		vec3(1       , 0.345744, 0.065221),
		vec3(1       , 0.355666, 0.071671),
		vec3(1       , 0.365508, 0.078332),
		vec3(1       , 0.375268, 0.085200),
		vec3(1       , 0.384948, 0.092271),
		vec3(1       , 0.394544, 0.099539),
		vec3(1       , 0.404059, 0.106999),
		vec3(1       , 0.413490, 0.114646),
		vec3(1       , 0.422838, 0.122476),
		vec3(1       , 0.432103, 0.130482),
		vec3(1       , 0.441284, 0.138661),
		vec3(1       , 0.450381, 0.147005),
		vec3(1       , 0.459395, 0.155512),
		vec3(1       , 0.468325, 0.164175),
		vec3(1       , 0.477172, 0.172989),
		vec3(1       , 0.485935, 0.181949),
		vec3(1       , 0.494614, 0.191050),
		vec3(1       , 0.503211, 0.200288),
		vec3(1       , 0.511724, 0.209657),
		vec3(1       , 0.520155, 0.219152),
		vec3(1       , 0.528504, 0.228769),
		vec3(1       , 0.536771, 0.238502),
		vec3(1       , 0.544955, 0.248347),
		vec3(1       , 0.553059, 0.258300),
		vec3(1       , 0.561082, 0.268356),
		vec3(1       , 0.569024, 0.278510),
		vec3(1       , 0.576886, 0.288758),
		vec3(1       , 0.584668, 0.299095),
		vec3(1       , 0.592372, 0.309518),
		vec3(1       , 0.599996, 0.320022),
		vec3(1       , 0.607543, 0.330603),
		vec3(1       , 0.615012, 0.341257),
		vec3(1       , 0.622403, 0.351980),
		vec3(1       , 0.629719, 0.362768),
		vec3(1       , 0.636958, 0.373617),
		vec3(1       , 0.644122, 0.384524),
		vec3(1       , 0.651210, 0.395486),
		vec3(1       , 0.658225, 0.406497),
		vec3(1       , 0.665166, 0.417556),
		vec3(1       , 0.672034, 0.428659),
		vec3(1       , 0.678829, 0.439802),
		vec3(1       , 0.685552, 0.450982),
		vec3(1       , 0.692204, 0.462196),
		vec3(1       , 0.698786, 0.473441),
		vec3(1       , 0.705297, 0.484714),
		vec3(1       , 0.711739, 0.496013),
		vec3(1       , 0.718112, 0.507333),
		vec3(1       , 0.724417, 0.518673),
		vec3(1       , 0.730654, 0.530030),
		vec3(1       , 0.736825, 0.541402),
		vec3(1       , 0.742929, 0.552785),
		vec3(1       , 0.748968, 0.564177),
		vec3(1       , 0.754942, 0.575576),
		vec3(1       , 0.760851, 0.586979),
		vec3(1       , 0.766696, 0.598385),
		vec3(1       , 0.772479, 0.609791),
		vec3(1       , 0.778199, 0.621195),
		vec3(1       , 0.783858, 0.632595),
		vec3(1       , 0.789455, 0.643989),
		vec3(1       , 0.794991, 0.655375),
		vec3(1       , 0.800468, 0.666751),
		vec3(1       , 0.805886, 0.678116),
		vec3(1       , 0.811245, 0.689467),
		vec3(1       , 0.816546, 0.700803),
		vec3(1       , 0.821790, 0.712122),
		vec3(1       , 0.826976, 0.723423),
		vec3(1       , 0.832107, 0.734704),
		vec3(1       , 0.837183, 0.745964),
		vec3(1       , 0.842203, 0.757201),
		vec3(1       , 0.847169, 0.768414),
		vec3(1       , 0.852082, 0.779601),
		vec3(1       , 0.856941, 0.790762),
		vec3(1       , 0.861748, 0.801895),
		vec3(1       , 0.866503, 0.812999),
		vec3(1       , 0.871207, 0.824073),
		vec3(1       , 0.875860, 0.835115),
		vec3(1       , 0.880463, 0.846125),
		vec3(1       , 0.885017, 0.857102),
		vec3(1       , 0.889521, 0.868044),
		vec3(1       , 0.893977, 0.878951),
		vec3(1       , 0.898386, 0.889822),
		vec3(1       , 0.902747, 0.900657),
		vec3(1       , 0.907061, 0.911453),
		vec3(1       , 0.911330, 0.922211),
		vec3(1       , 0.915552, 0.932929),
		vec3(1       , 0.919730, 0.943608),
		vec3(1       , 0.923863, 0.954246),
		vec3(1       , 0.927952, 0.964842),
		vec3(1       , 0.931998, 0.975397),
		vec3(1       , 0.936001, 0.985909),
		vec3(1       , 0.939961, 0.996379),
		vec3(0.993241, 0.937500, 1       ),
		vec3(0.983104, 0.931743, 1       ),
		vec3(0.973213, 0.926103, 1       ),
		vec3(0.963562, 0.920576, 1       ),
		vec3(0.954141, 0.915159, 1       ),
		vec3(0.944943, 0.909849, 1       ),
		vec3(0.935961, 0.904643, 1       ),
		vec3(0.927189, 0.899538, 1       ),
		vec3(0.918618, 0.894531, 1       ),
		vec3(0.910244, 0.889620, 1       ),
		vec3(0.902059, 0.884801, 1       ),
		vec3(0.894058, 0.880074, 1       ),
		vec3(0.886236, 0.875434, 1       ),
		vec3(0.878586, 0.870880, 1       ),
		vec3(0.871103, 0.866410, 1       ),
		vec3(0.863783, 0.862021, 1       ),
		vec3(0.856621, 0.857712, 1       ),
		vec3(0.849611, 0.853479, 1       ),
		vec3(0.842750, 0.849322, 1       ),
		vec3(0.836033, 0.845239, 1       ),
		vec3(0.829456, 0.841227, 1       ),
		vec3(0.823014, 0.837285, 1       ),
		vec3(0.816705, 0.833410, 1       ),
		vec3(0.810524, 0.829602, 1       ),
		vec3(0.804468, 0.825859, 1       ),
		vec3(0.798532, 0.822180, 1       ),
		vec3(0.792715, 0.818562, 1       ),
		vec3(0.787012, 0.815004, 1       ),
		vec3(0.781421, 0.811505, 1       ),
		vec3(0.775939, 0.808063, 1       ),
		vec3(0.770561, 0.804678, 1       ),
		vec3(0.765287, 0.801348, 1       ),
		vec3(0.760112, 0.798071, 1       ),
		vec3(0.755035, 0.794846, 1       ),
		vec3(0.750053, 0.791672, 1       ),
		vec3(0.745164, 0.788549, 1       ),
		vec3(0.740364, 0.785474, 1       ),
		vec3(0.735652, 0.782448, 1       ),
		vec3(0.731026, 0.779468, 1       ),
		vec3(0.726482, 0.776534, 1       ),
		vec3(0.722021, 0.773644, 1       ),
		vec3(0.717638, 0.770798, 1       ),
		vec3(0.713333, 0.767996, 1       ),
		vec3(0.709103, 0.765235, 1       ),
		vec3(0.704947, 0.762515, 1       ),
		vec3(0.700862, 0.759835, 1       ),
		vec3(0.696848, 0.757195, 1       ),
		vec3(0.692902, 0.754593, 1       ),
		vec3(0.689023, 0.752029, 1       ),
		vec3(0.685208, 0.749502, 1       ),
		vec3(0.681458, 0.747011, 1       ),
		vec3(0.677770, 0.744555, 1       ),
		vec3(0.674143, 0.742134, 1       ),
		vec3(0.670574, 0.739747, 1       ),
		vec3(0.667064, 0.737394, 1       ),
		vec3(0.663611, 0.735073, 1       ),
		vec3(0.660213, 0.732785, 1       ),
		vec3(0.656869, 0.730528, 1       ),
		vec3(0.653579, 0.728301, 1       ),
		vec3(0.650340, 0.726105, 1       ),
		vec3(0.647151, 0.723939, 1       ),
		vec3(0.644013, 0.721801, 1       ),
		vec3(0.640922, 0.719692, 1       ),
		vec3(0.637879, 0.717611, 1       ),
		vec3(0.634883, 0.715558, 1       ),
		vec3(0.631932, 0.713531, 1       ),
		vec3(0.629025, 0.711531, 1       ),
		vec3(0.626162, 0.709557, 1       ),
		vec3(0.623342, 0.707609, 1       ),
		vec3(0.620563, 0.705685, 1       ),
		vec3(0.617825, 0.703786, 1       ),
		vec3(0.615127, 0.701911, 1       ),
		vec3(0.612469, 0.700060, 1       ),
		vec3(0.609848, 0.698231, 1       ),
		vec3(0.607266, 0.696426, 1       ),
		vec3(0.604720, 0.694643, 1       )
	};

	int idx = int((temp - minTemp) / (maxTemp - minTemp) * numColors);
	idx = min(idx, numColors - 1);
	idx = max(idx, 0);

	return colors[idx];
}

//----------------------------------------------------------------------------//

void main()
{
	vec3 a_pos    = aPosition;
	vec2 a_texPos = aPosition.xz + vec2(0.5);
	uint particleIndex = uint(aIndex);

	Particle particle = particles.particlesData[particleIndex];
	uint type = particleIndex > u_numStars ? 1 : 0;
	if(type == 0 && particleIndex % 150 == 0)
		type = 2;

	float scale;
	if(type == 0)
		scale = u_starSize;
	else if(type == 1)
		scale = u_dustSize;
	else
	{
		Particle distTest = particle;
		distTest.pos.x += u_h2DistCheck;

		vec2 test = calc_pos(distTest);
		vec2 test2 = calc_pos(particle);
		float dist = distance(test, test2);
		dist = ease_in_circ(dist / u_h2DistCheck);

		scale = u_h2Size * (1.0 - dist);
	}

	vec3 camRight = vec3(uViewMatrix[0][0], uViewMatrix[1][0], uViewMatrix[2][0]);
	vec3 camUp    = vec3(uViewMatrix[0][1], uViewMatrix[1][1], uViewMatrix[2][1]);
	vec2 pos = calc_pos(particle);
	vec3 worldspacePos = vec3(pos.x, particle.height, pos.y) + ((camRight * a_pos.x) + (camUp * a_pos.z)) * scale;

	vec3 color = color_from_temp(particle.temp);

	o_texPos = a_texPos;
	o_color = vec4(color, particle.opacity);
	o_type = type;
	
	mat4 transform = mat4(
		vec4(aMRow0.x, aMRow1.x, aMRow2.x, 0.0),
		vec4(aMRow0.y, aMRow1.y, aMRow2.y, 0.0),
		vec4(aMRow0.z, aMRow1.z, aMRow2.z, 0.0),
		vec4(aMRow0.w, aMRow1.w, aMRow2.w, 1.0)
	);

	gl_Position = uViewProjectionMatrix  * transform * vec4(worldspacePos / 100, 1.0);
	
	// This is for debug
	//gl_Position = uViewProjectionMatrix  * transform * vec4(aPosition, 1.0);

	float vecCol = aIndex / 10.0;
	o_color = vec4(vecCol, vecCol, vecCol, 0.5);
}