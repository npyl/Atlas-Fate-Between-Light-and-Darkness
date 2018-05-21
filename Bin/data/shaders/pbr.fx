#include "common.fx"

#define PI 3.14159265359f


//--------------------------------------------------------------------------------------
// GBuffer Skinned generation pass. Vertex
//--------------------------------------------------------------------------------------
void VS_SKIN_GBuffer(
	float4 iPos : POSITION
	, float3 iN : NORMAL
	, float2 iUV : TEXCOORD
	, float4 iTangent : TANGENT
	, int4   iBones : BONES
	, float4 iWeights : WEIGHTS

	, out float4 oPos : SV_POSITION
	, out float3 oNormal : NORMAL0
	, out float4 oTangent : NORMAL1
	, out float2 oTex0 : TEXCOORD0
	, out float2 oTex1 : TEXCOORD1
	, out float3 oWorldPos : TEXCOORD2
)
{
	// Faking the verterx shader by now since we don't have tangents...
	float4x4 skin_mtx = getSkinMtx(iBones, iWeights);
	float4 skinned_Pos = mul(float4(iPos.xyz * BonesScale, 1), skin_mtx);

	oPos = mul(skinned_Pos, camera_view_proj); // Transform to viewproj, w_m inside skin_m
	oNormal = mul(iN, (float3x3)obj_world); // Rotate the normal
	oTangent.xyz = mul(iTangent.xyz, (float3x3)obj_world);
	oTangent.w = iTangent.w;

	oTex0 = iUV;
	oTex1 = iUV;
	oWorldPos = skinned_Pos.xyz;
}

//--------------------------------------------------------------------------------------
// GBuffer generation pass. Vertex
//--------------------------------------------------------------------------------------
void VS_GBuffer(
	in float4 iPos     : POSITION
	, in float3 iNormal : NORMAL0
	, in float2 iTex0 : TEXCOORD0
	, in float2 iTex1 : TEXCOORD1
	, in float4 iTangent : NORMAL1

	, out float4 oPos : SV_POSITION
	, out float3 oNormal : NORMAL0
	, out float4 oTangent : NORMAL1
	, out float2 oTex0 : TEXCOORD0
	, out float2 oTex1 : TEXCOORD1
	, out float3 oWorldPos : TEXCOORD2
)
{
	float4 world_pos = mul(iPos, obj_world);
	oPos = mul(world_pos, camera_view_proj);

	// Rotar la normal segun la transform del objeto
	oNormal = mul(iNormal, (float3x3)obj_world);
	oTangent.xyz = mul(iTangent.xyz, (float3x3)obj_world);
	oTangent.w = iTangent.w;

	// Las uv's se pasan directamente al ps
	oTex0 = iTex0;
	oTex1 = iTex1;
	oWorldPos = world_pos.xyz;
}

//--------------------------------------------------------------------------------------
// GBuffer generation pass. Pixel shader
//--------------------------------------------------------------------------------------
void PS_GBuffer(
	float4 Pos       : SV_POSITION
	, float3 iNormal : NORMAL0
	, float4 iTangent : NORMAL1
	, float2 iTex0 : TEXCOORD0
	, float2 iTex1 : TEXCOORD1
	, float3 iWorldPos : TEXCOORD2
	, out float4 o_albedo : SV_Target0
	, out float4 o_normal : SV_Target1
	, out float1 o_depth : SV_Target2
	, out float4 o_selfIllum : SV_Target3
)
{
  o_albedo = txAlbedo.Sample(samLinear, iTex0);
	o_albedo.a = txMetallic.Sample(samLinear, iTex0).r;
	o_selfIllum =  txEmissive.Sample(samLinear, iTex0) * self_intensity;
	o_selfIllum.xyz *= self_color;

	// Save roughness in the alpha coord of the N render target
	float roughness = txRoughness.Sample(samLinear, iTex0).r;
	float3 N = computeNormalMap(iNormal, iTangent, iTex0);
	o_normal = encodeNormal(N, roughness);

	// Si el material lo pide, sobreescribir los valores de la textura
	// por unos escalares uniformes. Only to playtesting...
	if (scalar_metallic >= 0.f)
		o_albedo.a = scalar_metallic;
	if (scalar_roughness >= 0.f)
		o_normal.a = scalar_roughness;

	// Compute the Z in linear space, and normalize it in the range 0...1
	// In the range z=0 to z=zFar of the camera (not zNear)
	float3 camera2wpos = iWorldPos - camera_pos;
	o_depth = dot(camera_front.xyz, camera2wpos) / camera_zfar;
}

void PS_GBuffer_Parallax(
  float4 Pos       : SV_POSITION
  , float3 iNormal : NORMAL0
  , float4 iTangent : NORMAL1
  , float2 iTex0 : TEXCOORD0
  , float2 iTex1 : TEXCOORD1
  , float3 iWorldPos : TEXCOORD2
  , float3 oCameraOut : TEXCOORD3
  , out float4 o_albedo : SV_Target0
  , out float4 o_normal : SV_Target1
  , out float1 o_depth : SV_Target2
  , out float4 o_selfIllum : SV_Target3
)
{
  // Store in the Alpha channel of the albedo texture, the 'metallic' amount of
  // the material
  float3x3 TBN = computeTBN(iNormal, iTangent);
  TBN = transpose(TBN);
  float3 view_dir = normalize(mul(camera_pos, TBN) - mul(iWorldPos, TBN));
  iTex0 = parallaxMappingB(iTex0, view_dir);

  o_albedo = txAlbedo.Sample(samLinear, iTex0);
  o_albedo.a = txMetallic.Sample(samLinear, iTex0).r;
  o_selfIllum = txEmissive.Sample(samLinear, iTex0) * self_intensity;
  o_selfIllum.xyz *= self_color;

  // Save roughness in the alpha coord of the N render target
  float roughness = txRoughness.Sample(samLinear, iTex0).r;
  float3 N = computeNormalMap(iNormal, iTangent, iTex0);
  o_normal = encodeNormal(N, roughness);

  // Si el material lo pide, sobreescribir los valores de la textura
  // por unos escalares uniformes. Only to playtesting...
  if (scalar_metallic >= 0.f)
    o_albedo.a = scalar_metallic;
  if (scalar_roughness >= 0.f)
    o_normal.a = scalar_roughness;

  // Compute the Z in linear space, and normalize it in the range 0...1
  // In the range z=0 to z=zFar of the camera (not zNear)
  float3 camera2wpos = iWorldPos - camera_pos;
  o_depth = dot(camera_front.xyz, camera2wpos) / camera_zfar;
}

//--------------------------------------------------------------------------------------
void decodeGBuffer(
	in float2 iPosition          // Screen coords
	, out float3 wPos
	, out float3 N
	, out float3 real_albedo
	, out float3 real_specular_color
	, out float  roughness
	, out float3 reflected_dir
	, out float3 view_dir
) {

	int3 ss_load_coords = uint3(iPosition.xy, 0);

	// Recover world position coords
	float  zlinear = txGBufferLinearDepth.Load(ss_load_coords).x;
	wPos = getWorldCoords(iPosition.xy, zlinear);

	// Recuperar la normal en ese pixel. Sabiendo que se
	// guardó en el rango 0..1 pero las normales se mueven
	// en el rango -1..1
	float4 N_rt = txGBufferNormals.Load(ss_load_coords);
	N = decodeNormal(N_rt.xyz);
	N = normalize(N);

	// Get other inputs from the GBuffer
  float4 albedo = txGBufferAlbedos.Load(ss_load_coords);
	// In the alpha of the albedo, we stored the metallic value
	// and in the alpha of the normal, we stored the roughness
	float  metallic = albedo.a;
	roughness = N_rt.a;

	// Apply gamma correction to albedo to bring it back to linear.
  albedo.rgb = pow(abs(albedo.rgb), 2.2f);// *projectColor(wPos).xyz;

	// Lerp with metallic value to find the good diffuse and specular.
	// If metallic = 0, albedo is the albedo, if metallic = 1, the
	// used albedo is almost black
  real_albedo = albedo.rgb * (1. - metallic);

	// 0.03 default specular value for dielectric.
  real_specular_color = lerp(0.03f, albedo.rgb, metallic);

	// Eye to object
	float3 incident_dir = normalize(wPos - camera_pos.xyz);
	reflected_dir = normalize(reflect(incident_dir, N));
	view_dir = -incident_dir;
}

// -------------------------------------------------
// Gloss = 1 - rough*rough
float3 Specular_F_Roughness(float3 specularColor, float gloss, float3 h, float3 v)
{
	// Sclick using roughness to attenuate fresnel.
	return (specularColor + (max(gloss, specularColor) - specularColor) * pow((1 - saturate(dot(v, h))), 5));
}

float NormalDistribution_GGX(float a, float NdH)
{
	// Isotropic ggx.
	float a2 = a * a;
	float NdH2 = NdH * NdH;

	float denominator = NdH2 * (a2 - 1.0f) + 1.0f;
	denominator *= denominator;
	denominator *= PI;

	return a2 / denominator;
}

float Geometric_Smith_Schlick_GGX(float a, float NdV, float NdL)
{
	// Smith schlick-GGX.
	float k = a * 0.5f;
	float GV = NdV / (NdV * (1 - k) + k);
	float GL = NdL / (NdL * (1 - k) + k);

	return GV * GL;
}

float Specular_D(float a, float NdH)
{
	return NormalDistribution_GGX(a, NdH);
}

float Specular_G(float a, float NdV, float NdL, float NdH, float VdH, float LdV)
{
	return Geometric_Smith_Schlick_GGX(a, NdV, NdL);
}

float3 Fresnel_Schlick(float3 specularColor, float3 h, float3 v)
{
	return (specularColor + (1.0f - specularColor) * pow((1.0f - saturate(dot(v, h))), 5));
}

float3 Specular_F(float3 specularColor, float3 h, float3 v)
{
	return Fresnel_Schlick(specularColor, h, v);
}

float3 Specular(float3 specularColor, float3 h, float3 v, float3 l, float a, float NdL, float NdV, float NdH, float VdH, float LdV)
{
	return ((Specular_D(a, NdH) * Specular_G(a, NdV, NdL, NdH, VdH, LdV)) * Specular_F(specularColor, v, h)) / (4.0f * NdL * NdV + 0.0001f);
}

//--------------------------------------------------------------------------------------
// Ambient pass, to compute the ambient light of each pixel
float4 PS_ambient(in float4 iPosition : SV_Position, in float2 iUV : TEXCOORD0) : SV_Target
{
	// Declare some float3 to store the values from the GBuffer
	float  roughness;
	float3 wPos, N, albedo, specular_color, reflected_dir, view_dir;
	decodeGBuffer(iPosition.xy, wPos, N, albedo, specular_color, roughness, reflected_dir, view_dir);

	// if roughness = 0 -> I want to use the miplevel 0, the all-detailed image
	// if roughness = 1 -> I will use the most blurred image, the 8-th mipmap, If image was 256x256 => 1x1
	float mipIndex = roughness * roughness * 32.0f;
	float3 env = txEnvironmentMap.SampleLevel(samLinear, reflected_dir, mipIndex).xyz;
	env = pow(abs(env), 2.2f);	// Convert the color to linear also.

	// The irrandiance, is read using the N direction.
	// Here we are sampling using the cubemap-miplevel 4, and the already blurred txIrradiance texture
	// and mixing it in base to the scalar_irradiance_vs_mipmaps which comes from the ImGui.
	// Remove the interpolation in the final version!!!
	float3 irradiance_mipmaps = txEnvironmentMap.SampleLevel(samLinear, N, 6).xyz;
	float3 irradiance_texture = txIrradianceMap.Sample(samLinear, N).xyz;
	float3 irradiance = irradiance_texture * scalar_irradiance_vs_mipmaps + irradiance_mipmaps * (1. - scalar_irradiance_vs_mipmaps);

	// How much the environment we see
	float3 env_fresnel = Specular_F_Roughness(specular_color, 1. - roughness * roughness, N, view_dir);

	float g_ReflectionIntensity = 1.0;
	float g_AmbientLightIntensity = 1.0;

	float ao = txAO.Sample(samLinear, iUV).x;
	float4 self_illum = txSelfIllum.Load(uint3(iPosition.xy,0)); // temp 

  // Compute global fog on ambient.
  float3 pixel_depth = camera_pos.xyz - wPos;
  float distancet = length(pixel_depth);
  float visibility = exp(distancet *distancet * -global_fog_density * global_fog_density * 1.442695);
  visibility = clamp(visibility, 0, 1);

  float4 final_color = float4(env_fresnel * env * g_ReflectionIntensity + albedo.xyz * irradiance * g_AmbientLightIntensity, 1.0f);
  final_color = final_color * global_ambient_adjustment * ao;
  return lerp(float4(env, 1), final_color, visibility) + float4(self_illum.xyz, 1) * global_ambient_adjustment * global_self_intensity;
}

//--------------------------------------------------------------------------------------
// The geometry that approximates the light volume uses this shader
void VS_pass(
	in float4 iPos : POSITION
	, in float3 iNormal : NORMAL0
	, in float2 iTex0 : TEXCOORD0
	, in float2 iTex1 : TEXCOORD0
	, in float4 iTangent : NORMAL1
	, out float4 oPos : SV_POSITION
) {
	float4 world_pos = mul(iPos, obj_world);
	oPos = mul(world_pos, camera_view_proj);
}

// --------------------------------------------------------
float3 Diffuse(float3 pAlbedo)
{
	return pAlbedo / PI;
}

// --------------------------------------------------------
float4 shade(float4 iPosition, out float3 light_dir, bool use_shadows)
{
	// Decode GBuffer information
	float3 wPos, N, albedo, specular_color, reflected_dir, view_dir;
	float  roughness;
	decodeGBuffer(iPosition.xy, wPos, N, albedo, specular_color, roughness, reflected_dir, view_dir);
	N = normalize(N);

	// From wPos to Light
	float3 light_dir_full = light_pos.xyz - wPos;
	float  distance_to_light = length(light_dir_full);
	light_dir = light_dir_full / distance_to_light;

	float  NdL = saturate(dot(N, light_dir));
	float  NdV = saturate(dot(N, view_dir));
	float3 h = normalize(light_dir + view_dir); // half vector

	float  NdH = saturate(dot(N, h));
	float  VdH = saturate(dot(view_dir, h));
	float  LdV = saturate(dot(light_dir, view_dir));
	float  a = max(0.001f, roughness * roughness);
	float3 cDiff = Diffuse(albedo);
	float3 cSpec = Specular(specular_color, h, view_dir, light_dir, a, NdL, NdV, NdH, VdH, LdV);

	float  att = (1. - smoothstep(0.90, 0.98, distance_to_light / light_radius)); // Att, point light
	//att *= 1 / distance_to_light;

	// Spotlight attenuation
	float shadow_factor = use_shadows ? computeShadowFactor(wPos) : 1.; // shadow factor
  
	//return projectColor(wPos);
  float3 final_color = light_color.xyz * NdL * (cDiff * (1.0f - cSpec) + cSpec) * light_intensity * att * shadow_factor;
	return float4(final_color, 1);
}

float4 PS_point_lights(in float4 iPosition : SV_Position) : SV_Target
{
	float3 out_lightdir;
	return shade(iPosition, out_lightdir, false);
}

float4 PS_dir_lights(in float4 iPosition : SV_Position) : SV_Target
{ 
	float3 out_lightdir;
	return shade(iPosition, out_lightdir, true);
}

float4 PS_spot_lights(in float4 iPosition : SV_Position) : SV_Target
{ 
	float3 out_lightdir;
	float4 light_color = shade(iPosition, out_lightdir, true);

	float theta = dot(out_lightdir, -light_direction.xyz);
	float att_spot = clamp((theta - light_outer_cut) / (light_inner_cut - light_outer_cut), 0, 1);
	float clamp_spot = theta > light_angle ? 1.0 * att_spot : 0.0; // spot factor 
	
	return light_color * clamp_spot;
}

// ----------------------------------------
void VS_skybox(in float4 iPosition : POSITION, in float4 iColor : COLOR0, out float4 oPosition : SV_Position)
{
	oPosition = float4(iPosition.x * 2 - 1., 1 - iPosition.y * 2, 1, 1);
}

// --------------------------------------------------------
float4 PS_skybox(in float4 iPosition : SV_Position) : SV_Target
{
	float3 view_dir = mul(float4(iPosition.xy, 1, 1), camera_screen_to_world).xyz;
	float4 skybox_color = txEnvironmentMap.Sample(samLinear, view_dir);
  skybox_color = pow(skybox_color, float4(2.2,2.2,2.2, 2.2));
  return float4(skybox_color.xyz, 1);// *global_ambient_adjustment;
}