//
//  Precomputed Atmospheric Scattering
//  Copyright (c) 2008 INRIA
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//  1. Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//  3. Neither the name of the copyright holders nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//  THE POSSIBILITY OF SUCH DAMAGE.
//
//  Author: Eric Bruneton
//
#version 450 core

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    //vec4 cameraPos;
    vec3  earthPos;
    float sunIntensity;
    vec4  sunDir;
    vec3  betaR;
    float mieG;
} ubo;

layout(set = 0,binding = 1) uniform sampler2D uTransmittance;
//layout(set = 0,binding = 2) uniform sampler2D uIrradiance;
layout(set = 0,binding = 2) uniform sampler3D uInscatter;

layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec4 outColor;

#define M_PI 3.141592
#define Rg 6360000.0
#define Rt 6420000.0
#define RL 6421000.0
#define RES_R 32.0
#define RES_MU 128.0
#define RES_MU_S 32.0
#define RES_NU 8.0

vec3 hdr(vec3 L)
{
    L   = L * 0.4;
    L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r);
    L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g);
    L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b);
    return L;
}

vec4 Texture4D(sampler3D table, float r, float mu, float muS, float nu)
{
    float H   = sqrt(Rt * Rt - Rg * Rg);
    float rho = sqrt(r * r - Rg * Rg);

    float rmu   = r * mu;
    float delta = rmu * rmu - r * r + Rg * Rg;
    vec4  cst   = rmu < 0.0 && delta > 0.0 ? vec4(1.0, 0.0, 0.0, 0.5 - 0.5 / RES_MU) : vec4(-1.0, H * H, H, 0.5 + 0.5 / RES_MU);
    float uR    = 0.5 / RES_R + rho / H * (1.0 - 1.0 / RES_R);
    float uMu   = cst.w + (rmu * cst.x + sqrt(delta + cst.y)) / (rho + cst.z) * (0.5 - 1.0 / float(RES_MU));
    // paper formula
    //float uMuS = 0.5 / RES_MU_S + max((1.0 - exp(-3.0 * muS - 0.6)) / (1.0 - exp(-3.6)), 0.0) * (1.0 - 1.0 / RES_MU_S);
    // better formula
    float uMuS = 0.5 / RES_MU_S + (atan(max(muS, -0.1975) * tan(1.26 * 1.1)) / 1.1 + (1.0 - 0.26)) * 0.5 * (1.0 - 1.0 / RES_MU_S);

    float lep = (nu + 1.0) / 2.0 * (RES_NU - 1.0);
    float uNu = floor(lep);
    lep       = lep - uNu;

    return texture(table, vec3((uNu + uMuS) / RES_NU, uMu, uR)) * (1.0 - lep) + texture(table, vec3((uNu + uMuS + 1.0) / RES_NU, uMu, uR)) * lep;
}

vec3 GetMie(vec4 rayMie)
{
    // approximated single Mie scattering (cf. approximate Cm in paragraph "Angular precision")
    // rayMie.rgb=C*, rayMie.w=Cm,r
    return rayMie.rgb * rayMie.w / max(rayMie.r, 1e-4) * (ubo.betaR.r / ubo.betaR);
}

float PhaseFunctionR(float mu)
{
    // Rayleigh phase function
    return (3.0 / (16.0 * M_PI)) * (1.0 + mu * mu);
}

float PhaseFunctionM(float mu)
{
    // Mie phase function
    return 1.5 * 1.0 / (4.0 * M_PI) *
	 (1.0 - ubo.mieG * ubo.mieG) * pow(1.0 + (ubo.mieG * ubo.mieG) - 2.0 * ubo.mieG * mu, -3.0 / 2.0) * (1.0 + mu * mu) / (2.0 + ubo.mieG * ubo.mieG);
}

vec3 Transmittance(float r, float mu)
{
    // transmittance(=transparency) of atmosphere for infinite ray (r,mu)
    // (mu=cos(view zenith angle)), intersections with ground ignored
    float uR, uMu;
    uR  = sqrt((r - Rg) / (Rt - Rg));
    uMu = atan((mu + 0.15) / (1.0 + 0.15) * tan(1.5)) / 1.5;

    return texture(uTransmittance, vec2(uMu, uR)).rgb;
}

vec3 TransmittanceWithShadow(float r, float mu)
{
    // transmittance(=transparency) of atmosphere for infinite ray (r,mu)
    // (mu=cos(view zenith angle)), or zero if ray intersects ground

    return mu < -sqrt(1.0 - (Rg / r) * (Rg / r)) ? vec3(0, 0, 0) : Transmittance(r, mu);
}

/*vec3 Irradiance(float r, float muS)
{
    float uR   = (r - Rg) / (Rt - Rg);
    float uMuS = (muS + 0.2) / (1.0 + 0.2);

    return texture(uIrradiance, vec2(uMuS, uR)).rgb;
}

vec3 SkyIrradiance(float r, float muS)
{
    return Irradiance(r, muS) * ubo.sunIntensity;
}

vec3 SkyIrradiance(vec3 worldPos)
{
    vec3  worldV = normalize(worldPos + ubo.earthPos); // vertical vector
    float r      = length(worldPos + ubo.earthPos);
    float muS    = dot(worldV, ubo.sunDir);

    return Irradiance(r, muS) * ubo.sunIntensity;
}*/

vec3 SunRadiance(vec3 worldPos)
{
    vec3  worldV = normalize(worldPos + ubo.earthPos); // vertical vector
    float r      = length(worldPos + ubo.earthPos);
    float muS    = dot(worldV, ubo.sunDir.xyz);

    return TransmittanceWithShadow(r, muS) * ubo.sunIntensity;
}

vec3 SkyRadiance(vec3 camera, vec3 viewdir, out vec3 extinction)
{
    // scattered sunlight between two points
    // camera=observer
    // viewdir=unit vector towards observed point
    // sundir=unit vector towards the sun
    // return scattered light

    camera += ubo.earthPos;

    vec3  result = vec3(0, 0, 0);
    float r      = length(camera);
    float rMu    = dot(camera, viewdir);
    float mu     = rMu / r;
    float r0     = r;
    float mu0    = mu;

    float deltaSq = sqrt(rMu * rMu - r * r + Rt * Rt);
    float din     = max(-rMu - deltaSq, 0.0);
    if (din > 0.0)
    {
        camera += din * viewdir;
        rMu += din;
        mu = rMu / Rt;
        r  = Rt;
    }

    float nu  = dot(viewdir, ubo.sunDir.xyz);
    float muS = dot(camera, ubo.sunDir.xyz) / r;

    vec4 inScatter = Texture4D(uInscatter, r, rMu / r, muS, nu);
    extinction     = TransmittanceWithShadow(r,mu);
    //Transmittance(r, mu);

    if (r <= Rt)
    {
        vec3  inScatterM = GetMie(inScatter);
        float phase      = PhaseFunctionR(nu);
        float phaseM     = PhaseFunctionM(nu);
        result           = inScatter.rgb * phase + inScatterM * phaseM;
    }
    else
    {
        result     = vec3(0, 0, 0);
        extinction = vec3(1, 1, 1);
    }

    return result * ubo.sunIntensity;
}

vec3 InScattering(vec3 camera, vec3 _point, out vec3 extinction, float shaftWidth)
{
    // single scattered sunlight between two points
    // camera=observer
    // point=point on the ground
    // sundir=unit vector towards the sun
    // return scattered light and extinction coefficient

    vec3 result = vec3(0, 0, 0);
    extinction  = vec3(1, 1, 1);

    vec3  viewdir = _point - camera;
    float d       = length(viewdir);
    viewdir       = viewdir / d;
    float r       = length(camera);

    if (r < 0.9 * Rg)
    {
        camera.y += Rg;
        _point.y += Rg;
        r = length(camera);
    }
    float rMu = dot(camera, viewdir);
    float mu  = rMu / r;
    float r0  = r;
    float mu0 = mu;
    _point -= viewdir * clamp(shaftWidth, 0.0, d);

    float deltaSq = sqrt(rMu * rMu - r * r + Rt * Rt);
    float din     = max(-rMu - deltaSq, 0.0);

    if (din > 0.0 && din < d)
    {
        camera += din * viewdir;
        rMu += din;
        mu = rMu / Rt;
        r  = Rt;
        d -= din;
    }

    if (r <= Rt)
    {
        float nu  = dot(viewdir, ubo.sunDir.xyz);
        float muS = dot(camera, ubo.sunDir.xyz) / r;

        vec4 inScatter;

        if (r < Rg + 600.0)
        {
            // avoids imprecision problems in aerial perspective near ground
            float f = (Rg + 600.0) / r;
            r       = r * f;
            rMu     = rMu * f;
            _point  = _point * f;
        }

        float r1   = length(_point);
        float rMu1 = dot(_point, viewdir);
        float mu1  = rMu1 / r1;
        float muS1 = dot(_point, ubo.sunDir.xyz) / r1;

        if (mu > 0.0)
            extinction = min(Transmittance(r, mu) / Transmittance(r1, mu1), 1.0);
        else
            extinction = min(Transmittance(r1, -mu1) / Transmittance(r, -mu), 1.0);

        const float EPS = 0.004;
        float       lim = -sqrt(1.0 - (Rg / r) * (Rg / r));

        if (abs(mu - lim) < EPS)
        {
            float a = ((mu - lim) + EPS) / (2.0 * EPS);

            mu  = lim - EPS;
            r1  = sqrt(r * r + d * d + 2.0 * r * d * mu);
            mu1 = (r * mu + d) / r1;

            vec4 inScatter0 = Texture4D(uInscatter, r, mu, muS, nu);
            vec4 inScatter1 = Texture4D(uInscatter, r1, mu1, muS1, nu);
            vec4 inScatterA = max(inScatter0 - inScatter1 * extinction.rgbr, 0.0);

            mu  = lim + EPS;
            r1  = sqrt(r * r + d * d + 2.0 * r * d * mu);
            mu1 = (r * mu + d) / r1;

            inScatter0      = Texture4D(uInscatter, r, mu, muS, nu);
            inScatter1      = Texture4D(uInscatter, r1, mu1, muS1, nu);
            vec4 inScatterB = max(inScatter0 - inScatter1 * extinction.rgbr, 0.0);

            inScatter = mix(inScatterA, inScatterB, a);
        }
        else
        {
            vec4 inScatter0 = Texture4D(uInscatter, r, mu, muS, nu);
            vec4 inScatter1 = Texture4D(uInscatter, r1, mu1, muS1, nu);
            inScatter       = max(inScatter0 - inScatter1 * extinction.rgbr, 0.0);
        }

        // avoids imprecision problems in Mie scattering when sun is below horizon
        inScatter.w *= smoothstep(0.00, 0.02, muS);

        vec3  inScatterM = GetMie(inScatter);
        float phase      = PhaseFunctionR(nu);
        float phaseM     = PhaseFunctionM(nu);
        result           = inScatter.rgb * phase + inScatterM * phaseM;
    }
    return result * ubo.sunIntensity;
}

void main()
{
    vec3 dir = normalize(inWorldPos);
    float sun = step(cos(M_PI / 360.0), dot(dir, ubo.sunDir.xyz));
    vec3 sunColor = vec3(sun, sun, sun) * ubo.sunIntensity;
    
    const vec3 cameraPos = vec3(0,555,0);

    vec3 extinction;
    vec3 inscatter = SkyRadiance(cameraPos, dir, extinction);
    vec3 col       = sunColor * extinction + inscatter;
    
    outColor = vec4(col, 1.0);
}
