// *****************************************************************************************
// KXC354 - Computer Graphics & Animation - 2014
// 3D Engine Code  -  scene.cpp
//
// The scene:
//   * THE SOLAR SYSTEM is built and animated right here in the scene file (it is NOT a
//     composite): a textured sun + corona, 7 planets on visible orbit rings, a ringed planet,
//     a moon, an asteroid belt, an extrusion probe and a sweep comet.
//   * composite1 = THE COLLISION  (two rockets crash in an explosion of rocky debris)  - LEFT
//   * composite2 = THE SATELLITE  (a satellite orbits the Earth, firing a red laser at it) - RIGHT
//   * a cinematic camera tours the three.
// *****************************************************************************************



// -----------------------------------------------------------------------------------------
// includes
// -----------------------------------------------------------------------------------------
#include "prefix.h"
#include "constants.h"
#include "utility.h"
#include "light.h"
#include "camera.h"
#include "texture.h"
#include "scene.h"
#include "snd.h"
#include "skybox.h"
#include "fog.h"
#include "viewfrustum.h"

// #includes for geometry
#include "sweep.h"
#include "extrusion.h"
#include "cube.h"
#include "sphere.h"
#include "torus.h"
#include "terrain.h"
#include "billboard.h"
#include "plane.h"
#include "tsphere.h"
#include "cylinder.h"
#include "disc.h"

// my two composite objects
#include "composite1.h"
#include "composite2.h"



// -----------------------------------------------------------------------------------------
// engine globals
// -----------------------------------------------------------------------------------------
extern ProgramMode		gProgramMode;
extern cameraClass		gCamera;
extern object3d* gSky;
extern fog* gFog;
extern viewfrustum* gViewFrustum;
extern float			gCurrentFrameTime;

extern vector<object3d*> gShapeVector;
typedef vector<object3d*>::iterator shapeVectorIterator;



// -----------------------------------------------------------------------------------------
// the objects in the animation
// -----------------------------------------------------------------------------------------
composite1* rockets;			// composite1 - the rocket collision (LEFT)
composite2* satScene;			// composite2 - the satellite orbiting the Earth (RIGHT)

// background stars, spread inside a big cube
#define NUM_STARS 700
sphere* star[NUM_STARS];

// a few faint nebula clouds, far behind the action, for colour and depth
#define NUM_NEBULA 3
sphere* nebula[NUM_NEBULA];

// sound objects
snd* ambientSound;				// space ambient loop
snd* flareSound;				// solar flare one-shot
snd* crashSound;				// explosion sound when the rockets collide

// time tracking
float prevTime = 0.0f;
float sceneTime = 0.0f;
bool  gFlarePlayed = false;
float gAmbientLoopAt = 30.0f;
bool  gMusicStarted = false;

// the LIVE position of the Earth planet (planet[2]) - the satellite orbits it and the camera follows it
float gEarthX = 0.0f, gEarthY = 0.0f, gEarthZ = 0.0f;

// lights
light* ambient, * light0, * light1, * light2, * light3, * light4, * light5;

// helper: blend between two numbers
static float lerp(float a, float b, float t) { return a + (b - a) * t; }



// =========================================================================================
//   THE SOLAR SYSTEM  (built and animated here in the scene, NOT as a composite)
// =========================================================================================
#define SS_PLANETS 7
#define SS_CORONA 3
#define SS_ASTEROIDS 24
#define SS_COMET_TAIL 6

static float SS_DIST[SS_PLANETS] = { 16.0f, 26.0f, 38.0f, 74.0f, 90.0f, 106.0f, 122.0f };	// planet[3]+ pushed out to clear the station orbiting planet[2]
static float SS_OSPEED[SS_PLANETS] = { 8.8f, 7.2f, 6.0f, 5.1f, 4.2f, 3.6f, 3.1f };
static float SS_SSPEED[SS_PLANETS] = { 2.0f, 4.5f, 9.0f, 8.0f, 18.0f, 14.0f, 7.0f };
static float SS_SIZE[SS_PLANETS] = { 1.9f, 2.7f, 9.0f, 2.4f, 7.6f, 5.7f, 4.3f };	// planet[2] (Earth) is the big host planet
static float SS_TILT[SS_PLANETS] = { 2.0f, 177.0f, 23.5f, 25.0f, 3.1f, 26.7f, 98.0f };
static float SS_R[SS_PLANETS] = { 0.72f, 0.96f, 0.18f, 0.85f, 0.88f, 0.92f, 0.38f };
static float SS_G[SS_PLANETS] = { 0.55f, 0.88f, 0.45f, 0.30f, 0.72f, 0.82f, 0.55f };
static float SS_B[SS_PLANETS] = { 0.38f, 0.52f, 0.92f, 0.14f, 0.45f, 0.52f, 0.88f };
static const int SS_RINGED = 5, SS_MOONP = 2;
static float SS_CORONA_SIZE[SS_CORONA] = { 10.0f, 12.2f, 15.0f };
static float SS_CORONA_ALPHA[SS_CORONA] = { 0.22f, 0.13f, 0.07f };
static const float SS_BELT = 98.0f;
static const float SS_MOON_ORBIT = 13.0f;	// the Earth's moon orbits this far out (clear of the big Earth)
static float SS_START[SS_PLANETS] = { 0, 52, 137, 204, 290, 48, 175 };

static tsphere* ssSun;
static sphere* ssCorona[SS_CORONA];
static tsphere* ssPlanet[SS_PLANETS];
static torus* ssRing[SS_PLANETS];
static torus* ssSaturn;
static extrusion* ssProbe;
static sphere* ssMoon;
static sphere* ssAst[SS_ASTEROIDS];
static sweep* ssComet;
static sphere* ssTail[SS_COMET_TAIL];
static texture* ssSunTex;
static texture* ssPlanetTex[SS_PLANETS];

static float ssOrbit[SS_PLANETS], ssSpin[SS_PLANETS];
static float ssMoonAngle, ssSunGlow, ssCometAngle, ssAstAngle;


static void buildSolarSystem()
{
	ssMoonAngle = 0; ssSunGlow = 0; ssCometAngle = 0; ssAstAngle = 0;
	for (int i = 0; i < SS_PLANETS; i++) { ssOrbit[i] = SS_START[i]; ssSpin[i] = 0; }

	ssSunTex = new texture("sunSurface.bmp");
	ssPlanetTex[0] = new texture("planet0_tex.bmp");
	ssPlanetTex[1] = new texture("planet1_tex.bmp");
	ssPlanetTex[2] = new texture("planet2_tex.bmp");
	ssPlanetTex[3] = new texture("planet3_tex.bmp");
	ssPlanetTex[4] = new texture("planet4_tex.bmp");
	ssPlanetTex[5] = new texture("planet5_tex.bmp");
	ssPlanetTex[6] = new texture("planet6_tex.bmp");

	// the sun (textured, self-lit)
	ssSun = new tsphere(28);
	ssSun->setColour(1.0f, 0.80f, 0.28f);
	ssSun->setEmissiveColour(1.0f, 0.78f, 0.32f, 1.0f);
	ssSun->useSmoothShading();
	ssSun->setTexture(ssSunTex);
	ssSun->setDeformation(8.5f, 8.5f, 8.5f);

	// the corona (soft transparent shells)
	for (int i = 0; i < SS_CORONA; i++)
	{
		ssCorona[i] = new sphere(24);
		ssCorona[i]->setColour(1.0f, 0.62f, 0.18f, SS_CORONA_ALPHA[i]);
		ssCorona[i]->setEmissiveColour(1.0f, 0.55f, 0.15f, 1.0f);
		ssCorona[i]->useSmoothShading();
		ssCorona[i]->setDeformation(SS_CORONA_SIZE[i], SS_CORONA_SIZE[i], SS_CORONA_SIZE[i]);
		ssCorona[i]->setPosition(0, 0, 0);
	}

	// a faint visible orbit ring on each planet's path (the torus lies flat in x-z)
	for (int i = 0; i < SS_PLANETS; i++)
	{
		ssRing[i] = new torus(64, 6, SS_DIST[i], 0.05f);
		ssRing[i]->setColour(0.40f, 0.52f, 0.75f, 0.22f);
		ssRing[i]->setEmissiveColour(0.12f, 0.16f, 0.24f, 1.0f);
		ssRing[i]->setPosition(0, 0, 0);
	}

	// the 7 planets (low emissive, so the sun lights their day side)
	for (int i = 0; i < SS_PLANETS; i++)
	{
		ssPlanet[i] = new tsphere(20);
		ssPlanet[i]->setColour(SS_R[i], SS_G[i], SS_B[i]);
		ssPlanet[i]->setEmissiveColour(SS_R[i] * 0.10f, SS_G[i] * 0.10f, SS_B[i] * 0.10f, 1.0f);
		ssPlanet[i]->setSpecularColour(0.30f, 0.32f, 0.38f, 1.0f);
		ssPlanet[i]->setShininess(16.0f);
		ssPlanet[i]->useSmoothShading();
		ssPlanet[i]->setTexture(ssPlanetTex[i]);
		ssPlanet[i]->setDeformation(SS_SIZE[i], SS_SIZE[i], SS_SIZE[i]);
		ssPlanet[i]->setPosition(SS_DIST[i] * cos_d(ssOrbit[i]), 0, SS_DIST[i] * sin_d(ssOrbit[i]));
		ssPlanet[i]->setRotation('y', 0.0f, 'z', SS_TILT[i]);
	}

	// the transparent ring around the ringed planet
	ssSaturn = new torus(56, 10, SS_SIZE[SS_RINGED] * 1.85f, SS_SIZE[SS_RINGED] * 0.35f);
	ssSaturn->setColour(0.85f, 0.78f, 0.52f, 0.72f);
	ssSaturn->setSpecularColour(0.60f, 0.55f, 0.40f, 1.0f);
	ssSaturn->setShininess(28.0f);
	ssSaturn->setPosition(SS_DIST[SS_RINGED] * cos_d(ssOrbit[SS_RINGED]), 0, SS_DIST[SS_RINGED] * sin_d(ssOrbit[SS_RINGED]));
	ssSaturn->setRotation('x', 27);

	// a small extrusion probe
	ssProbe = new extrusion("orbitRingProf.txt", "orbitRingPath.txt");
	ssProbe->setColour(0.70f, 0.75f, 0.85f, 0.85f);
	ssProbe->setSpecularColour(0.75f, 0.80f, 0.90f, 1.0f);
	ssProbe->setShininess(55.0f);
	ssProbe->setScale(0.9f);
	ssProbe->setPosition(SS_DIST[3] * cos_d(ssOrbit[3]), 1.2f, SS_DIST[3] * sin_d(ssOrbit[3]));

	// the moon
	ssMoon = new sphere(12);
	ssMoon->setColour(0.62f, 0.60f, 0.55f);
	ssMoon->setEmissiveColour(0.06f, 0.06f, 0.06f, 1.0f);
	ssMoon->setSpecularColour(0.25f, 0.25f, 0.25f, 1.0f);
	ssMoon->setShininess(8.0f);
	ssMoon->setDeformation(2.5f, 2.5f, 2.5f);
	ssMoon->setPosition(SS_DIST[SS_MOONP] + SS_MOON_ORBIT, 0.4f, 0.0f);

	// the asteroid belt
	for (int i = 0; i < SS_ASTEROIDS; i++)
	{
		ssAst[i] = new sphere(6);
		ssAst[i]->setColour(0.46f, 0.42f, 0.38f);
		ssAst[i]->setEmissiveColour(0.06f, 0.05f, 0.04f, 1.0f);
		ssAst[i]->setSpecularColour(0.18f, 0.16f, 0.13f, 1.0f);
		ssAst[i]->setShininess(6.0f);
		float s = 0.10f + 0.10f * fabsf(sinf(i * 12.9898f));
		ssAst[i]->setDeformation(s, s, s);
	}

	// the comet (a real, visible sweep)
	ssComet = new sweep("aegisNose.txt", 24);
	ssComet->setColour(0.80f, 0.92f, 1.0f);
	ssComet->setEmissiveColour(0.55f, 0.78f, 1.0f, 1.0f);
	ssComet->setSpecularColour(0.85f, 0.92f, 1.0f, 1.0f);
	ssComet->setShininess(70.0f);
	ssComet->useSmoothShading();
	ssComet->setScale(1.8f);
	ssComet->setPosition(104.0f, 0.0f, 0.0f);

	// the comet tail (transparent, streams away from the sun)
	for (int i = 0; i < SS_COMET_TAIL; i++)
	{
		ssTail[i] = new sphere(10);
		ssTail[i]->setColour(0.62f, 0.82f, 1.0f, 0.80f - 0.11f * i);
		ssTail[i]->setEmissiveColour(0.45f - 0.05f * i, 0.68f - 0.07f * i, 1.0f - 0.10f * i, 1.0f);
		ssTail[i]->setDeformation(1.0f - 0.10f * i, 1.0f - 0.10f * i, 1.0f - 0.10f * i);
		ssTail[i]->setPosition(104.0f, 0.0f, 0.0f);
	}
}


static void resetSolarSystem()
{
	ssMoonAngle = 0; ssSunGlow = 0; ssCometAngle = 0; ssAstAngle = 0;
	for (int i = 0; i < SS_PLANETS; i++) { ssOrbit[i] = SS_START[i]; ssSpin[i] = 0; }
}


static void animateSolarSystem(float dt)
{
	// 1) spin the sun and pulse its glow + corona
	ssSunGlow += dt * 0.52f;
	ssSun->setRotation('y', ssSunGlow * 12.0f);
	float glow = 0.92f + 0.08f * sinf(ssSunGlow * 2.0f);
	ssSun->setEmissiveColour(glow, 0.78f * glow, 0.32f * glow, 1.0f);
	for (int i = 0; i < SS_CORONA; i++)
	{
		float breathe = 1.0f + 0.04f * sinf(ssSunGlow * 1.5f + i);
		ssCorona[i]->setDeformation(SS_CORONA_SIZE[i] * breathe, SS_CORONA_SIZE[i] * breathe, SS_CORONA_SIZE[i] * breathe);
		ssCorona[i]->setColour(1.0f, 0.62f, 0.18f, SS_CORONA_ALPHA[i] * (0.85f + 0.15f * sinf(ssSunGlow * 2.0f)));
	}

	// 2) move the planets, moon and asteroid belt around the sun
	for (int i = 0; i < SS_PLANETS; i++)
	{
		ssOrbit[i] += SS_OSPEED[i] * dt;
		if (ssOrbit[i] >= 360) ssOrbit[i] -= 360;
		ssPlanet[i]->setPosition(SS_DIST[i] * cos_d(ssOrbit[i]), 0, SS_DIST[i] * sin_d(ssOrbit[i]));
	}
	ssSaturn->setPosition(SS_DIST[SS_RINGED] * cos_d(ssOrbit[SS_RINGED]), 0, SS_DIST[SS_RINGED] * sin_d(ssOrbit[SS_RINGED]));
	ssMoonAngle += 25.0f * dt;
	if (ssMoonAngle >= 360) ssMoonAngle -= 360;
	float ex = SS_DIST[SS_MOONP] * cos_d(ssOrbit[SS_MOONP]);
	float ez = SS_DIST[SS_MOONP] * sin_d(ssOrbit[SS_MOONP]);
	ssMoon->setPosition(ex + SS_MOON_ORBIT * cos_d(ssMoonAngle), 0.4f, ez + SS_MOON_ORBIT * sin_d(ssMoonAngle));
	ssAstAngle += 4.0f * dt;
	if (ssAstAngle >= 360) ssAstAngle -= 360;
	for (int i = 0; i < SS_ASTEROIDS; i++)
	{
		float a = i * (360.0f / SS_ASTEROIDS) + ssAstAngle;
		float r = SS_BELT + 2.2f * sinf(i * 7.31f);
		float y = 1.3f * sinf(i * 3.17f);
		ssAst[i]->setPosition(r * cos_d(a), y, r * sin_d(a));
	}

	// 3) spin each planet on its tilted axis (and tumble the probe)
	for (int i = 0; i < SS_PLANETS; i++)
	{
		ssSpin[i] += SS_SSPEED[i] * dt;
		if (ssSpin[i] >= 360) ssSpin[i] -= 360;
		ssPlanet[i]->setRotation('y', ssSpin[i], 'z', SS_TILT[i]);
	}
	ssProbe->setRotation('y', ssSpin[0] * 1.2f, 'x', ssSpin[0] * 0.6f);
	ssProbe->setPosition(SS_DIST[3] * cos_d(ssOrbit[3] + 40), 1.2f, SS_DIST[3] * sin_d(ssOrbit[3] + 40));

	// 4) sweep the comet past the sun; tail always streams away from the sun
	ssCometAngle += 22.0f * dt;
	if (ssCometAngle >= 360) ssCometAngle -= 360;
	float x = 104.0f * cos_d(ssCometAngle);
	float z = 66.0f * sin_d(ssCometAngle);
	float y = 10.0f * sin_d(ssCometAngle);
	ssComet->setPosition(x, y, z);
	ssComet->setRotation('y', ssCometAngle * 2.2f, 'x', ssCometAngle * 1.1f);
	float cg = 0.80f + 0.20f * sinf(ssCometAngle * 0.20f);
	ssComet->setEmissiveColour(0.55f * cg, 0.78f * cg, 1.0f * cg, 1.0f);
	float len = sqrtf(x * x + y * y + z * z);
	if (len < 0.001f) len = 0.001f;
	float ox = x / len, oy = y / len, oz = z / len;
	for (int i = 0; i < SS_COMET_TAIL; i++)
	{
		float d = 0.9f * (i + 1);
		ssTail[i]->setPosition(x + ox * d, y + oy * d, z + oz * d);
	}
}



// -----------------------------------------------------------------------------------------
// camera keyframes  (time, cameraX,Y,Z, targetX,Y,Z)
//   rockets crash (LEFT, -65) -> solar system (CENTRE, 0) -> satellite + Earth (RIGHT, +75)
// -----------------------------------------------------------------------------------------
// (after t = 38 the camera switches to a live tracking shot of the Earth + satellite - see below)
struct CameraKey { float t, px, py, pz, tx, ty, tz; };
CameraKey cameraKey[] = {
	{  0.0f,  -30.0f,130.0f, 440.0f,  -30.0f,  5.0f, 0.0f },	// WIDE establishing shot (the whole big scene)
	{  5.0f, -115.0f, 66.0f, 150.0f, -165.0f, 26.0f, 0.0f },	// fly toward the rockets (far left)
	{ 10.0f, -170.0f, 42.0f,  62.0f, -164.0f, 26.0f, 0.0f },	// close as the rockets collide
	{ 18.0f, -160.0f, 40.0f,  58.0f, -165.0f, 25.0f, 0.0f },	// hold on the explosion + debris
	{ 26.0f,    0.0f, 84.0f, 250.0f,    0.0f,  0.0f, 0.0f },	// glide back to the big SOLAR SYSTEM (centre)
	{ 33.0f,   34.0f, 56.0f, 150.0f,   10.0f,  0.0f, 0.0f },	// drift over the solar system
	{ 38.0f,   82.0f, 46.0f, 124.0f,   38.0f,  0.0f, 2.0f },	// approach the Earth (hands off to the tracking shot)
	{ 44.0f,   82.0f, 46.0f, 124.0f,   38.0f,  0.0f, 2.0f },	// (tracking takes over at t >= 38)
};
const int NUM_CAMERA_KEYS = 8;



// -----------------------------------------------------------------------------------------
// constructScene  (called once)
// -----------------------------------------------------------------------------------------
void constructScene()
{
	// lights
	ambient = new light(GL_LIGHT_MODEL_AMBIENT);
	light0 = new light(GL_LIGHT0);
	light1 = new light(GL_LIGHT1);
	light2 = new light(GL_LIGHT2);
	light3 = new light(GL_LIGHT3);
	light4 = new light(GL_LIGHT4);
	light5 = new light(GL_LIGHT5);

	// the solar system (built here in the scene, NOT a composite)
	buildSolarSystem();

	// the two composites
	rockets = new composite1(-165.0f, 26.0f, 0.0f);			// the collision, well clear of the (big) solar system
	satScene = new composite2(SS_SIZE[SS_MOONP], 22.0f);	// the space station orbits the Earth planet (radius, orbit radius - wider, for the big station)

	// background stars, spread inside a big cube
	for (int i = 0; i < NUM_STARS; i++)
	{
		star[i] = new sphere(6);
		star[i]->useSmoothShading();
		float hx = sinf(i * 12.9898f + 78.233f) * 43758.5453f; hx = hx - floorf(hx);
		float hy = sinf(i * 39.3468f + 11.135f) * 24634.6345f; hy = hy - floorf(hy);
		float hz = sinf(i * 93.9898f + 47.987f) * 31415.9265f; hz = hz - floorf(hz);
		float hs = sinf(i * 7.1234f + 19.191f) * 12911.1234f; hs = hs - floorf(hs);
		star[i]->setScale(0.035f + 0.16f * hs * hs * hs);
		float r = 0.80f + 0.20f * hx, g = 0.85f + 0.12f * hx, b = 1.00f - 0.10f * hx;
		star[i]->setColour(r, g, b);
		star[i]->setEmissiveColour(r * 0.9f, g * 0.9f, b * 0.9f, 1.0f);
		star[i]->setPosition(hx * 460 - 230, hy * 460 - 230, hz * 460 - 230);
	}

	// faint nebula clouds far behind everything
	float nebX[NUM_NEBULA] = { -130.0f, 150.0f,  30.0f };
	float nebY[NUM_NEBULA] = { 64.0f,  56.0f, -84.0f };
	float nebZ[NUM_NEBULA] = { -210.0f,-230.0f,-195.0f };
	float nebR[NUM_NEBULA] = { 0.40f,  0.55f,  0.30f };
	float nebG[NUM_NEBULA] = { 0.22f,  0.30f,  0.45f };
	float nebB[NUM_NEBULA] = { 0.70f,  0.55f,  0.75f };
	for (int i = 0; i < NUM_NEBULA; i++)
	{
		nebula[i] = new sphere(20);
		nebula[i]->useSmoothShading();
		nebula[i]->setColour(nebR[i], nebG[i], nebB[i], 0.06f);
		nebula[i]->setEmissiveColour(nebR[i] * 0.5f, nebG[i] * 0.5f, nebB[i] * 0.5f, 1.0f);
		nebula[i]->setScale(60.0f + 16.0f * i);
		nebula[i]->setPosition(nebX[i], nebY[i], nebZ[i]);
	}

	// sounds
	ambientSound = new snd("space_ambient.wav");
	flareSound = new snd("solar_flare.wav");
	crashSound = new snd("crash.wav");
}



// -----------------------------------------------------------------------------------------
// resetScene  (called whenever the animation restarts)
// -----------------------------------------------------------------------------------------
void resetScene()
{
	gCamera.setPosition(0, 55, 165);
	gCamera.setTarget(0, 5, 0);

	prevTime = 0.0f;
	sceneTime = 0.0f;
	gFlarePlayed = false;
	gAmbientLoopAt = 30.0f;
	gMusicStarted = false;

	// a gentle ambient so nothing is ever pure black (the moving Earth still reads from any angle)
	ambient->setColour(0.40, 0.40, 0.44, 1.0);

	// light0 - the SUN at the centre (lights the whole solar system)
	light0->turnOn();
	light0->setPosition(0, 0, 0);
	light0->setColour(1.0, 1.0, 0.95, 1.0);
	light0->setSpecularColour(1.0, 1.0, 0.9, 1.0);
	light0->makePositional();
	light0->setLinearAttenuation(0.00003);

	// light1 - warm fill near the sun
	light1->turnOn();
	light1->setPosition(0.4, 0.4, 0.0);
	light1->setColour(1.0, 0.95, 0.75, 1.0);
	light1->setSpecularColour(0.5, 0.4, 0.16, 1.0);
	light1->makePositional();
	light1->setLinearAttenuation(0.001);

	// light2 - cool work light over the ROCKET collision (far left)
	light2->turnOn();
	light2->setPosition(-160, 34, 22);
	light2->setColour(0.85, 0.9, 1.0, 1.0);
	light2->setSpecularColour(0.8, 0.85, 1.0, 1.0);
	light2->makePositional();
	light2->setLinearAttenuation(0.0035);

	// light3 - a broad camera-side fill so the planets + Earth read from the outside
	light3->turnOn();
	light3->setPosition(50, 60, 150);
	light3->setColour(0.9, 0.92, 1.0, 1.0);
	light3->setSpecularColour(0.9, 0.95, 1.0, 1.0);
	light3->makePositional();
	light3->setLinearAttenuation(0.0011);

	// light4 - a second cool fill from the other side, so the moving Earth is lit from any angle
	light4->turnOn();
	light4->setPosition(-40, 44, 90);
	light4->setColour(0.7, 0.74, 0.9, 1.0);
	light4->setSpecularColour(0.6, 0.65, 0.85, 1.0);
	light4->makePositional();
	light4->setLinearAttenuation(0.002);

	// light5 - cool rim light from behind to separate things from space
	light5->turnOn();
	light5->setPosition(0, 26, -160);
	light5->setColour(0.6, 0.64, 0.8, 1.0);
	light5->setSpecularColour(0.5, 0.55, 0.7, 1.0);
	light5->makePositional();
	light5->setLinearAttenuation(0.0004);

	// reset everything to its starting state
	resetSolarSystem();
	rockets->show();
	satScene->show();

	snd::stopAllSounds();
}



// -----------------------------------------------------------------------------------------
// animateForNextFrame
// -----------------------------------------------------------------------------------------
void animateForNextFrame(float time, long frame)
{
	float dt = time - prevTime;
	if (dt > 0.1f) dt = 0.1f;
	prevTime = time;
	sceneTime = sceneTime + dt;

	// start the music on the very first animation frame
	if (!gMusicStarted)
	{
		ambientSound->startSound();
		gMusicStarted = true;
	}

	// twinkle the stars
	for (int i = 0; i < NUM_STARS; i++)
	{
		float hx = sinf(i * 12.9898f + 78.233f) * 43758.5453f; hx = hx - floorf(hx);
		float r = 0.80f + 0.20f * hx, g = 0.85f + 0.12f * hx, b = 1.00f - 0.10f * hx;
		float spd = 1.1f + 0.9f * (sinf(i * 2.3994f) * 0.5f + 0.5f);
		float tw = 0.55f + 0.45f * sinf(time * spd + i * 0.61803f * 6.2831f);
		if (tw < 0.18f) tw = 0.18f;
		star[i]->setEmissiveColour(r * tw, g * tw, b * tw, 1.0f);
	}

	// animate the solar system (centre)
	animateSolarSystem(dt);

	// the Earth planet (planet[2]) is orbiting the sun - read its live world position
	gEarthX = SS_DIST[SS_MOONP] * cos_d(ssOrbit[SS_MOONP]);
	gEarthY = 0.0f;
	gEarthZ = SS_DIST[SS_MOONP] * sin_d(ssOrbit[SS_MOONP]);

	// animate composite1 (the rocket collision, left)
	rockets->runSequence(dt);
	if (rockets->justCrashed)
		crashSound->restartSound();

	// animate composite2 (the satellite, which orbits the Earth planet INSIDE the solar system)
	// the laser only fires once the camera has zoomed in on the Earth (t >= 38)
	satScene->runSequence(dt, gEarthX, gEarthY, gEarthZ, (time >= 38.0f));

	// keep the ambient music going (the track is ~30s and the engine does not loop)
	if (time >= gAmbientLoopAt)
	{
		ambientSound->restartSound();
		gAmbientLoopAt += 30.0f;
	}

	// a one-shot solar flare while the camera is near the sun
	if (!gFlarePlayed && time > 26.0f)
	{
		flareSound->startSound();
		gFlarePlayed = true;
	}

	// move the camera CINEMATICALLY between the keyframes, then track the Earth + satellite
	{
		int k = 0;
		for (int i = 0; i < NUM_CAMERA_KEYS - 1; i++)
			if (time >= cameraKey[i].t && time < cameraKey[i + 1].t) { k = i; break; }
		if (time >= cameraKey[NUM_CAMERA_KEYS - 1].t) k = NUM_CAMERA_KEYS - 2;

		float span = cameraKey[k + 1].t - cameraKey[k].t;
		float blend = (span > 0) ? (time - cameraKey[k].t) / span : 1.0f;
		if (blend < 0) blend = 0;
		if (blend > 1) blend = 1;
		float e = blend * blend * (3.0f - 2.0f * blend);		// smoothstep ease

		float cpx = lerp(cameraKey[k].px, cameraKey[k + 1].px, e);
		float cpy = lerp(cameraKey[k].py, cameraKey[k + 1].py, e);
		float cpz = lerp(cameraKey[k].pz, cameraKey[k + 1].pz, e);
		float ctx = lerp(cameraKey[k].tx, cameraKey[k + 1].tx, e);
		float cty = lerp(cameraKey[k].ty, cameraKey[k + 1].ty, e);
		float ctz = lerp(cameraKey[k].tz, cameraKey[k + 1].tz, e);

		// FINALE: from t=38 on, ease into a LIVE tracking shot that follows the Earth + satellite
		// (the Earth is orbiting the sun, so the camera arcs around its moving position)
		if (time >= 38.0f)
		{
			float u = (time - 38.0f) / 6.0f;
			if (u > 1.0f) u = 1.0f;
			float fe = u * u * (3.0f - 2.0f * u);
			float ca = (time - 38.0f) * 7.0f;					// a slow arc around the Earth
			float fpx = gEarthX + 52.0f * cos_d(ca);
			float fpy = gEarthY + 26.0f;
			float fpz = gEarthZ + 52.0f * sin_d(ca);
			cpx = lerp(cpx, fpx, fe); cpy = lerp(cpy, fpy, fe); cpz = lerp(cpz, fpz, fe);
			ctx = lerp(ctx, gEarthX, fe); cty = lerp(cty, gEarthY, fe); ctz = lerp(ctz, gEarthZ, fe);
		}

		// a tiny, slow drift so the shot feels alive
		float driftX = 0.9f * sinf(time * 0.21f);
		float driftY = 0.6f * sinf(time * 0.17f + 1.3f);

		gCamera.setPosition(cpx + driftX, cpy + driftY, cpz);
		gCamera.setTarget(ctx, cty, ctz);
	}

	// eventually stop - and stop the music with it
	if (time >= 56)
	{
		snd::stopAllSounds();
		gProgramMode = kpmFinished;
	}
}
