// *****************************************************************************************
// composite1.cpp
// The Collision: two classic rockets (white body, red nose cone, fins, glowing flame) roll
// about themselves AND fly toward each other, then crash in a single explosion that scatters a
// field of rocky debris (once).
// *****************************************************************************************
#include "prefix.h"
#include "composite1.h"
#include "constants.h"
#include "utility.h"

// a small helper that keeps a number between 0 and 1
static float clamp01(float v)
{
	if (v < 0) return 0;
	if (v > 1) return 1;
	return v;
}

// give a part a realistic surface finish: a specular highlight colour + shininess (0-128)
static void metalFinish(object3d* o, float r, float g, float b, float shininess)
{
	o->setSpecularColour(r, g, b, 1.0f);
	o->setShininess(shininess);
}


// =========================================================================
// Constructor - build the two rockets and the (hidden) explosion + debris
// =========================================================================
composite1::composite1(float x, float y, float z)
{
	zoneX = x; zoneY = y; zoneZ = z;

	crashTimer = 0;
	hasCrashed = false;
	justCrashed = false;
	explosionFinished = false;
	rocketSpin = 0;
	flameFlicker = 0;
	launchClock = 0;
	for (int i = 0; i < NUM_DEBRIS; i++) debrisDistance[i] = 0;

	rocketAx = zoneX + 16;
	rocketBx = zoneX - 16;

	textureA = new texture("hullA.bmp");
	textureB = new texture("hullB.bmp");


	// ===================== ROCKET A =====================
	rocketA = new sweep("shipBody.txt", 18);
	rocketA->setColour(0.93f, 0.94f, 0.97f);			// white body
	rocketA->setEmissiveColour(0.10f, 0.10f, 0.12f, 1.0f);
	rocketA->setTexture(textureA);
	rocketA->setPosition(rocketAx, zoneY, zoneZ);
	rocketA->setRotation('z', 90);						// nose points left, toward the centre
	rocketA->setScale(2.6f);
	rocketA->setParent(this);

	noseA = new sweep("aegisNose.txt", 18);
	noseA->setColour(0.88f, 0.16f, 0.12f);				// red nose cone
	noseA->setEmissiveColour(0.20f, 0.02f, 0.02f, 1.0f);
	noseA->setScale(0.62f);
	noseA->setPosition(0.0f, 1.85f, 0.0f);				// on top of the body
	noseA->setParent(rocketA);

	ringA = new torus(20, 10, 0.62f, 0.10f);			// a trim ring around the body
	ringA->setColour(0.80f, 0.20f, 0.16f);
	ringA->setPosition(0.0f, 0.35f, 0.0f);
	ringA->setParent(rocketA);

	for (int i = 0; i < NUM_FINS; i++)					// three fins around the base
	{
		finA[i] = new extrusion("wingProf.txt", "wingPath.txt");
		finA[i]->setColour(0.85f, 0.22f, 0.18f);		// red fins
		finA[i]->setScale(0.5f);
		finA[i]->setRotation('y', i * 120.0f);
		finA[i]->setPosition(0.0f, -1.15f, 0.0f);
		finA[i]->setParent(rocketA);
	}

	flameA = new sphere(12);							// glowing exhaust flame (transparent)
	flameA->setColour(1.0f, 0.6f, 0.15f, 0.8f);
	flameA->setEmissiveColour(1.0f, 0.5f, 0.1f, 1.0f);
	flameA->useSmoothShading();
	flameA->setDeformation(0.45f, 1.1f, 0.45f);
	flameA->setPosition(0.0f, -2.1f, 0.0f);				// out the tail
	flameA->setParent(rocketA);


	// ===================== ROCKET B =====================
	rocketB = new sweep("shipBody.txt", 18);
	rocketB->setColour(0.93f, 0.94f, 0.97f);			// white body
	rocketB->setEmissiveColour(0.10f, 0.10f, 0.12f, 1.0f);
	rocketB->setTexture(textureB);
	rocketB->setPosition(rocketBx, zoneY, zoneZ);
	rocketB->setRotation('z', -90);						// nose points right, toward the centre
	rocketB->setScale(2.6f);
	rocketB->setParent(this);

	noseB = new sweep("aegisNose.txt", 18);
	noseB->setColour(0.88f, 0.16f, 0.12f);
	noseB->setEmissiveColour(0.20f, 0.02f, 0.02f, 1.0f);
	noseB->setScale(0.62f);
	noseB->setPosition(0.0f, 1.85f, 0.0f);
	noseB->setParent(rocketB);

	ringB = new torus(20, 10, 0.62f, 0.10f);
	ringB->setColour(0.80f, 0.20f, 0.16f);
	ringB->setPosition(0.0f, 0.35f, 0.0f);
	ringB->setParent(rocketB);

	for (int i = 0; i < NUM_FINS; i++)
	{
		finB[i] = new extrusion("wingProf.txt", "wingPath.txt");
		finB[i]->setColour(0.85f, 0.22f, 0.18f);
		finB[i]->setScale(0.5f);
		finB[i]->setRotation('y', i * 120.0f);
		finB[i]->setPosition(0.0f, -1.15f, 0.0f);
		finB[i]->setParent(rocketB);
	}

	flameB = new sphere(12);
	flameB->setColour(1.0f, 0.6f, 0.15f, 0.8f);
	flameB->setEmissiveColour(1.0f, 0.5f, 0.1f, 1.0f);
	flameB->useSmoothShading();
	flameB->setDeformation(0.45f, 1.1f, 0.45f);
	flameB->setPosition(0.0f, -2.1f, 0.0f);
	flameB->setParent(rocketB);


	// ===================== THE EXPLOSION (hidden until the crash) =====================
	explosion = new sphere(16);
	explosion->setColour(1.0f, 0.5f, 0.1f, 0.8f);
	explosion->setEmissiveColour(1.0f, 0.4f, 0.05f, 1.0f);
	explosion->setScale(0.001f);
	explosion->setPosition(zoneX, zoneY, zoneZ);
	explosion->setParent(this);
	explosion->hide();

	for (int i = 0; i < NUM_DEBRIS; i++)
	{
		debris[i] = new sphere(6);
		debris[i]->setColour(0.60f, 0.42f, 0.25f);
		debris[i]->setEmissiveColour(0.25f, 0.10f, 0.03f, 1.0f);
		float s = 0.45f + 0.65f * fabsf(sinf(i * 7.31f));
		debris[i]->setDeformation(s, s * (0.6f + 0.4f * fabsf(cosf(i * 3.1f))), s * (0.7f + 0.3f * fabsf(sinf(i * 5.2f))));
		debris[i]->setPosition(zoneX, zoneY, zoneZ);
		debris[i]->setParent(this);
		debris[i]->hide();
	}


	// ---- surface finish ----
	metalFinish(rocketA, 0.9f, 0.92f, 1.0f, 70.0f);
	metalFinish(rocketB, 0.9f, 0.92f, 1.0f, 70.0f);
	metalFinish(noseA, 0.9f, 0.5f, 0.45f, 50.0f);
	metalFinish(noseB, 0.9f, 0.5f, 0.45f, 50.0f);
	for (int i = 0; i < NUM_FINS; i++) { metalFinish(finA[i], 0.9f, 0.5f, 0.45f, 40.0f); metalFinish(finB[i], 0.9f, 0.5f, 0.45f, 40.0f); }
}


// =========================================================================
// Destructor
// =========================================================================
composite1::~composite1()
{
	delete rocketA; delete noseA; delete ringA; delete flameA;
	delete rocketB; delete noseB; delete ringB; delete flameB;
	for (int i = 0; i < NUM_FINS; i++) { delete finA[i]; delete finB[i]; }
	delete explosion;
	for (int i = 0; i < NUM_DEBRIS; i++) delete debris[i];
	delete textureA; delete textureB;
}


// =========================================================================
// hide - hide every part
// =========================================================================
void composite1::hide(void)
{
	invisible = true;
	rocketA->hide(); noseA->hide(); ringA->hide(); flameA->hide();
	rocketB->hide(); noseB->hide(); ringB->hide(); flameB->hide();
	for (int i = 0; i < NUM_FINS; i++) { finA[i]->hide(); finB[i]->hide(); }
	explosion->hide();
	for (int i = 0; i < NUM_DEBRIS; i++) debris[i]->hide();
}


// =========================================================================
// show - reset the sequence to its START, then show the two rockets
// =========================================================================
void composite1::show(void)
{
	invisible = false;

	hasCrashed = false; explosionFinished = false; justCrashed = false;
	launchClock = 0; crashTimer = 0; rocketSpin = 0; flameFlicker = 0;
	rocketAx = zoneX + 16; rocketBx = zoneX - 16;
	for (int i = 0; i < NUM_DEBRIS; i++) debrisDistance[i] = 0;

	rocketA->setScale(2.6f); rocketA->setPosition(rocketAx, zoneY, zoneZ); rocketA->setRotation('z', 90);
	rocketB->setScale(2.6f); rocketB->setPosition(rocketBx, zoneY, zoneZ); rocketB->setRotation('z', -90);
	rocketA->show(); noseA->show(); ringA->show(); flameA->show();
	rocketB->show(); noseB->show(); ringB->show(); flameB->show();
	for (int i = 0; i < NUM_FINS; i++) { finA[i]->show(); finB[i]->show(); }

	explosion->setScale(0.001f); explosion->hide();
	for (int i = 0; i < NUM_DEBRIS; i++) debris[i]->hide();
}


// =========================================================================
// ANIMATION 1: the two rockets spin about themselves AND fly together (flames flicker)
// =========================================================================
void composite1::animateRockets(float dt)
{
	rocketAx = rocketAx - 3.5f * dt;
	rocketBx = rocketBx + 3.2f * dt;
	rocketA->setPosition(rocketAx, zoneY, zoneZ);
	rocketB->setPosition(rocketBx, zoneY, zoneZ);

	rocketSpin = rocketSpin + 220.0f * dt;
	if (rocketSpin >= 360) rocketSpin = rocketSpin - 360;
	rocketA->setRotation('y', rocketSpin, 'z', 90);
	rocketB->setRotation('y', -rocketSpin, 'z', -90);

	// flicker the exhaust flames
	flameFlicker = flameFlicker + dt * 14.0f;
	float f = 0.8f + 0.2f * sinf(flameFlicker);
	flameA->setDeformation(0.45f * f, 1.1f * (0.85f + 0.3f * f), 0.45f * f);
	flameB->setDeformation(0.45f * f, 1.1f * (0.85f + 0.3f * f), 0.45f * f);
	flameA->setEmissiveColour(1.0f, 0.5f * f, 0.1f, 1.0f);
	flameB->setEmissiveColour(1.0f, 0.5f * f, 0.1f, 1.0f);
}


// =========================================================================
// ANIMATION 2: grow the explosion (only plays once)
// =========================================================================
void composite1::animateExplosion(float dt)
{
	float size = 0.5f + clamp01(crashTimer / 4.5f) * 9.0f;
	explosion->setScale(size);

	float fade = clamp01(1.0f - (crashTimer - 1.0f) / 9.0f);
	if (crashTimer < 1.0f) fade = crashTimer;
	explosion->setColour(1.0f, 0.5f, 0.1f, 0.8f * fade);
}


// =========================================================================
// ANIMATION 3: fly the rocky debris outward in a real burst (and tumble it)
// =========================================================================
void composite1::animateDebris(float dt)
{
	for (int i = 0; i < NUM_DEBRIS; i++)
	{
		float speed = 0.8f + 0.9f * fabsf(sinf(i * 12.9898f));
		float dx = cosf(i * 2.3998f);
		float dy = sinf(i * 2.3998f) * cosf(i * 1.117f);
		float dz = sinf(i * 2.3998f) * sinf(i * 1.117f);

		debrisDistance[i] = debrisDistance[i] + speed * dt;
		float d = debrisDistance[i];
		debris[i]->setPosition(zoneX + dx * d, zoneY + dy * d, zoneZ + dz * d);
		debris[i]->setRotation('x', d * 60.0f, 'y', d * 40.0f);

		float fade = clamp01(1.0f - crashTimer / 16.0f);
		debris[i]->setColour(0.80f, 0.55f, 0.30f, fade);
	}
}


// =========================================================================
// MASTER METHOD: rockets fly in and collide -> explosion + debris (once) -> debris drifts
// =========================================================================
void composite1::runSequence(float dt)
{
	justCrashed = false;

	if (!hasCrashed)
	{
		launchClock = launchClock + dt;
		if (launchClock > 6.0f)
			animateRockets(dt);

		if (launchClock > 6.0f && rocketAx - rocketBx < 10.0f)
		{
			hasCrashed = true;
			justCrashed = true;
			crashTimer = 0;
			for (int i = 0; i < NUM_DEBRIS; i++) debrisDistance[i] = 0;

			// the rockets are destroyed - hide them
			rocketA->hide(); noseA->hide(); ringA->hide(); flameA->hide();
			rocketB->hide(); noseB->hide(); ringB->hide(); flameB->hide();
			for (int i = 0; i < NUM_FINS; i++) { finA[i]->hide(); finB[i]->hide(); }

			explosion->show();
			for (int i = 0; i < NUM_DEBRIS; i++) debris[i]->show();
		}
	}
	else
	{
		crashTimer = crashTimer + dt;
		if (!explosionFinished)
		{
			animateExplosion(dt);
			if (crashTimer > 10.0f) explosionFinished = true;
		}
		animateDebris(dt);
	}
}
