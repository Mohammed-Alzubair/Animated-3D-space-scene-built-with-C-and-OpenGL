// *****************************************************************************************
// composite2.cpp
// The Space Station: a complex modular satellite (long spine of modules, a rotating habitat
// ring, a Soyuz capsule, four gimballing solar arrays, scanning dishes, sweeping antennas) that
// ORBITS the solar system's Earth-like planet and fires a red laser onto it.  The Earth is a
// planet of the solar system (built in scene.cpp); its live position is handed in each frame so
// there is only ONE Earth and the station lives inside the solar system.  The laser only fires
// when the scene says so (when the camera has zoomed in on the Earth).
// *****************************************************************************************
#include "prefix.h"
#include "composite2.h"
#include "constants.h"
#include "utility.h"
#include "cube.h"
#include "cylinder.h"

static void metalFinish(object3d* o, float r, float g, float b, float shininess)
{
	o->setSpecularColour(r, g, b, 1.0f);
	o->setShininess(shininess);
}

static const float ORBIT_TILT = 22.0f;		// the station's orbit is inclined


// =========================================================================
// Constructor
// =========================================================================
composite2::composite2(float earthRadius, float orbitRadius)
{
	earthR = earthRadius;
	orbitR = orbitRadius;

	orbitAngle = 0; ringSpin = 0; panelAngle = 0; mainDishScan = 0;
	subDishAngle = 0; antennaSweep = 0; navBlink = 0; laserPulse = 0; laserRamp = 0;

	// --- the orbit pivot (its position is set to the Earth's centre each frame) ---
	satOrbit = new object3d();
	satOrbit->setPosition(0, 0, 0);
	satOrbit->setParent(this);

	// --- the central hub: a small node that is the structural PARENT of the whole station.
	//     It has NO rotation, so every part keeps its own orientation.  setScale sizes the station. ---
	satHub = new cylinder(16);
	satHub->setColour(0.80f, 0.82f, 0.86f);
	satHub->setEmissiveColour(0.10f, 0.10f, 0.12f, 1.0f);
	satHub->setDeformation(0.55f, 0.55f, 0.55f);
	satHub->setScale(2.20f);							// bigger station
	satHub->setPosition(orbitR, 0.0f, 0.0f);			// out at orbit radius; its -x side faces the Earth
	satHub->setParent(satOrbit);

	// --- the long backbone spine (a thin cylinder laid along z) ---
	spine = new cylinder(12);
	spine->setColour(0.70f, 0.72f, 0.76f);
	spine->setEmissiveColour(0.08f, 0.08f, 0.10f, 1.0f);
	spine->setDeformation(0.22f, 6.2f, 0.22f);
	spine->setRotation('x', 90.0f);						// lay it along z
	spine->setPosition(0.0f, 0.0f, 0.0f);
	spine->setParent(satHub);

	// --- four modules of varied sizes along the spine, each with a detail band ---
	float modZ[NUM_MODULES] = { -4.4f, -1.9f, 2.1f, 4.2f };
	float modR[NUM_MODULES] = { 0.95f, 0.78f, 0.85f, 0.62f };
	float modL[NUM_MODULES] = { 1.4f, 1.1f, 1.2f, 0.95f };
	for (int i = 0; i < NUM_MODULES; i++)
	{
		module[i] = new cylinder(16);
		module[i]->setColour(0.82f, 0.84f, 0.88f);
		module[i]->setEmissiveColour(0.10f, 0.10f, 0.12f, 1.0f);
		module[i]->setDeformation(modR[i], modL[i], modR[i]);
		module[i]->setRotation('x', 90.0f);				// along z
		module[i]->setPosition(0.0f, 0.0f, modZ[i]);
		module[i]->setParent(satHub);

		moduleBand[i] = new torus(20, 10, modR[i] + 0.04f, 0.10f);
		moduleBand[i]->setColour(0.45f, 0.46f, 0.50f);
		moduleBand[i]->setEmissiveColour(0.05f, 0.05f, 0.06f, 1.0f);
		moduleBand[i]->setRotation('x', 90.0f);			// wrap around the module (encircles z)
		moduleBand[i]->setPosition(0.0f, 0.0f, modZ[i]);
		moduleBand[i]->setParent(satHub);
	}

	// --- the big habitat ring around the spine (encircles z) + an inner ring ---
	// the ring sits a little along the spine (away from z=0) so the laser beam, which exits at
	// z=0 toward the Earth, stays clear of it
	ringMount = new cylinder(10);
	ringMount->setColour(0.6f, 0.61f, 0.65f);
	ringMount->setDeformation(0.10f, 1.4f, 0.10f);
	ringMount->setPosition(0.0f, 0.8f, 1.4f);
	ringMount->setParent(satHub);

	habitatRing = new torus(48, 18, 3.2f, 0.45f);
	habitatRing->setColour(0.84f, 0.86f, 0.90f);
	habitatRing->setEmissiveColour(0.10f, 0.10f, 0.12f, 1.0f);
	habitatRing->setRotation('x', 90.0f);				// hole along z -> encircles the spine
	habitatRing->setPosition(0.0f, 0.0f, 1.4f);
	habitatRing->setParent(satHub);

	habitatRingInner = new torus(48, 12, 2.85f, 0.18f);
	habitatRingInner->setColour(0.55f, 0.56f, 0.60f);
	habitatRingInner->setEmissiveColour(0.05f, 0.05f, 0.06f, 1.0f);
	habitatRingInner->setRotation('x', 90.0f);
	habitatRingInner->setPosition(0.0f, 0.0f, 1.4f);
	habitatRingInner->setParent(satHub);

	// the detail capsules ride around the ring (pivot rotates them in the x-y plane, about z)
	ringSpinPivot = new object3d();
	ringSpinPivot->setPosition(0.0f, 0.0f, 1.4f);
	ringSpinPivot->setParent(satHub);
	for (int i = 0; i < NUM_RING_DETAIL; i++)
	{
		float a = i * (360.0f / NUM_RING_DETAIL);
		ringDetail[i] = new sphere(10);
		ringDetail[i]->setColour(0.78f, 0.80f, 0.84f);
		ringDetail[i]->setEmissiveColour(0.10f, 0.10f, 0.12f, 1.0f);
		ringDetail[i]->setDeformation(0.34f, 0.34f, 0.50f);
		ringDetail[i]->setPosition(3.2f * cos_d(a), 3.2f * sin_d(a), 0.0f);
		ringDetail[i]->setParent(ringSpinPivot);
	}

	// --- docking node + Soyuz capsule at the far end of the spine ---
	dockNode = new sphere(14);
	dockNode->setColour(0.75f, 0.77f, 0.82f);
	dockNode->setEmissiveColour(0.09f, 0.09f, 0.11f, 1.0f);
	dockNode->setDeformation(0.7f, 0.7f, 0.7f);
	dockNode->setPosition(0.0f, 0.0f, 5.4f);
	dockNode->setParent(satHub);

	capsule = new sweep("aegisNose.txt", 18);
	capsule->setColour(0.72f, 0.74f, 0.80f);
	capsule->setEmissiveColour(0.08f, 0.08f, 0.10f, 1.0f);
	capsule->setScale(0.9f);
	capsule->setRotation('x', 90.0f);					// point the capsule along +z
	capsule->setPosition(0.0f, 0.0f, 6.4f);
	capsule->setParent(satHub);

	// --- the vertical truss + four solar arrays ---
	truss = new cylinder(10);
	truss->setColour(0.55f, 0.56f, 0.60f);
	truss->setEmissiveColour(0.05f, 0.05f, 0.06f, 1.0f);
	truss->setDeformation(0.13f, 3.0f, 0.13f);			// along y
	truss->setPosition(0.0f, 0.0f, 0.0f);
	truss->setParent(satHub);

	float panSide[NUM_PANELS] = { 1.0f, 1.0f, -1.0f, -1.0f };
	float panY[NUM_PANELS] = { 1.9f, -1.9f, 1.9f, -1.9f };
	for (int i = 0; i < NUM_PANELS; i++)
	{
		panelArm[i] = new cylinder(8);
		panelArm[i]->setColour(0.55f, 0.55f, 0.60f);
		panelArm[i]->setDeformation(0.05f, 0.9f, 0.05f);
		panelArm[i]->setRotation('z', 90.0f);			// along x
		panelArm[i]->setPosition(1.1f * panSide[i], panY[i], 0.0f);
		panelArm[i]->setParent(satHub);

		satPanel[i] = new cube();
		satPanel[i]->setColour(0.12f, 0.18f, 0.50f);
		satPanel[i]->setEmissiveColour(0.03f, 0.06f, 0.16f, 1.0f);
		satPanel[i]->setDeformation(1.3f, 0.04f, 0.8f);
		satPanel[i]->setPosition(2.7f * panSide[i], panY[i], 0.0f);
		satPanel[i]->setParent(satHub);
	}

	// --- the main comms dish (on the space-facing +x side) + feed ---
	mainDish = new tsphere(22);
	mainDish->setColour(0.92f, 0.93f, 0.96f);
	mainDish->setEmissiveColour(0.10f, 0.10f, 0.12f, 1.0f);
	mainDish->useSmoothShading();
	mainDish->setDeformation(1.5f, 0.18f, 1.5f);
	mainDish->setPosition(1.6f, 1.1f, -1.0f);
	mainDish->setRotation('z', -55.0f);					// tip it to face up-and-out
	mainDish->setParent(satHub);

	mainDishRim = new torus(36, 8, 1.0f, 0.05f);
	mainDishRim->setColour(0.78f, 0.79f, 0.84f);
	mainDishRim->setDeformation(1.5f, 1.5f, 1.5f);
	mainDishRim->setPosition(0.0f, 0.0f, 0.0f);
	mainDishRim->setParent(mainDish);

	mainFeedMast = new cylinder(8);
	mainFeedMast->setColour(0.70f, 0.70f, 0.74f);
	mainFeedMast->setDeformation(0.04f, 0.7f, 0.04f);
	mainFeedMast->setPosition(0.0f, 0.8f, 0.0f);
	mainFeedMast->setParent(mainDish);

	mainFeed = new sphere(10);
	mainFeed->setColour(0.85f, 0.86f, 0.90f);
	mainFeed->setEmissiveColour(0.10f, 0.10f, 0.12f, 1.0f);
	mainFeed->setDeformation(0.18f, 0.18f, 0.18f);
	mainFeed->setPosition(0.0f, 1.5f, 0.0f);
	mainFeed->setParent(mainDish);

	// --- two smaller sub-dishes near the end modules ---
	float subZ[NUM_SUB_DISHES] = { -4.2f, 3.8f };
	for (int i = 0; i < NUM_SUB_DISHES; i++)
	{
		subDish[i] = new tsphere(16);
		subDish[i]->setColour(0.90f, 0.91f, 0.95f);
		subDish[i]->setEmissiveColour(0.10f, 0.10f, 0.12f, 1.0f);
		subDish[i]->useSmoothShading();
		subDish[i]->setDeformation(0.7f, 0.12f, 0.7f);
		subDish[i]->setPosition(0.9f, 1.0f, subZ[i]);
		subDish[i]->setRotation('z', -50.0f);
		subDish[i]->setParent(satHub);
	}

	// --- two instrument booms (the required EXTRUSIONS), sticking out below ---
	boom[0] = new extrusion("wingProf.txt", "wingPath.txt");
	boom[0]->setColour(0.55f, 0.55f, 0.60f);
	boom[0]->setEmissiveColour(0.05f, 0.05f, 0.06f, 1.0f);
	boom[0]->setScale(0.7f);
	boom[0]->setRotation('z', -90.0f);					// point downward
	boom[0]->setPosition(0.0f, -1.0f, -2.2f);
	boom[0]->setParent(satHub);

	boom[1] = new extrusion("wingProf.txt", "wingPath.txt");
	boom[1]->setColour(0.55f, 0.55f, 0.60f);
	boom[1]->setEmissiveColour(0.05f, 0.05f, 0.06f, 1.0f);
	boom[1]->setScale(0.7f);
	boom[1]->setRotation('z', -90.0f);
	boom[1]->setPosition(0.0f, -1.0f, 2.2f);
	boom[1]->setParent(satHub);

	// --- three antenna whips with glowing tips (the tips follow each whip's sweep) ---
	float antZ[NUM_ANTENNAS] = { -1.9f, 1.0f, 3.6f };
	float antX[NUM_ANTENNAS] = { 0.4f, -0.4f, 0.3f };
	for (int i = 0; i < NUM_ANTENNAS; i++)
	{
		antenna[i] = new cylinder(8);
		antenna[i]->setColour(0.6f, 0.61f, 0.65f);
		antenna[i]->setDeformation(0.04f, 1.3f, 0.04f);	// thin whip along y
		antenna[i]->setPosition(antX[i], 1.0f, antZ[i]);
		antenna[i]->setParent(satHub);

		antennaTip[i] = new sphere(8);
		antennaTip[i]->setColour(1.0f, 0.85f, 0.4f);
		antennaTip[i]->setEmissiveColour(0.7f, 0.55f, 0.15f, 1.0f);
		antennaTip[i]->setDeformation(0.12f, 0.12f, 0.12f);
		antennaTip[i]->setPosition(0.0f, 1.35f, 0.0f);	// at the top of the whip
		antennaTip[i]->setParent(antenna[i]);
	}

	// --- three blinking navigation lights ---
	navRed = new sphere(6);
	navRed->setColour(1.0f, 0.15f, 0.10f);
	navRed->setEmissiveColour(0.85f, 0.05f, 0.02f, 1.0f);
	navRed->setDeformation(0.16f, 0.16f, 0.16f);
	navRed->setPosition(0.0f, 0.0f, -5.6f);
	navRed->setParent(satHub);

	navGreen = new sphere(6);
	navGreen->setColour(0.15f, 1.0f, 0.20f);
	navGreen->setEmissiveColour(0.05f, 0.85f, 0.10f, 1.0f);
	navGreen->setDeformation(0.16f, 0.16f, 0.16f);
	navGreen->setPosition(0.0f, 0.0f, 7.1f);
	navGreen->setParent(satHub);

	navWhite = new sphere(6);
	navWhite->setColour(1.0f, 1.0f, 1.0f);
	navWhite->setEmissiveColour(0.80f, 0.80f, 0.85f, 1.0f);
	navWhite->setDeformation(0.13f, 0.13f, 0.13f);
	navWhite->setPosition(-0.9f, 0.7f, 0.0f);
	navWhite->setParent(satHub);

	// --- the laser cannon (a SWEEP) on the Earth-facing (-x) side ---
	laserCannon = new sweep("aegisNose.txt", 18);
	laserCannon->setColour(0.55f, 0.20f, 0.18f);
	laserCannon->setEmissiveColour(0.55f, 0.08f, 0.05f, 1.0f);
	laserCannon->setScale(0.5f);
	laserCannon->setRotation('z', 90.0f);				// tip points toward the Earth (-x)
	laserCannon->setPosition(-0.8f, -0.4f, 0.0f);
	laserCannon->setParent(satHub);

	// --- the laser beam + impact (children of the orbit pivot, so they point at the Earth) ---
	float beamLen = orbitR - earthR;
	float beamMid = (orbitR + earthR) * 0.5f;
	laserBeam = new cylinder(10);
	laserBeam->setColour(1.0f, 0.12f, 0.10f, 0.70f);
	laserBeam->setEmissiveColour(1.0f, 0.10f, 0.06f, 1.0f);
	laserBeam->setDeformation(0.16f, beamLen * 0.5f, 0.16f);
	laserBeam->setRotation('z', 90.0f);
	laserBeam->setPosition(beamMid, 0.0f, 0.0f);
	laserBeam->setParent(satOrbit);

	laserImpact = new sphere(14);
	laserImpact->setColour(1.0f, 0.25f, 0.15f, 0.6f);
	laserImpact->setEmissiveColour(1.0f, 0.25f, 0.12f, 1.0f);
	laserImpact->useSmoothShading();
	laserImpact->setDeformation(0.9f, 0.9f, 0.9f);
	laserImpact->setPosition(earthR, 0.0f, 0.0f);
	laserImpact->setParent(satOrbit);


	// ---- surface finish ----
	metalFinish(satHub, 0.9f, 0.92f, 1.0f, 70.0f);
	metalFinish(spine, 0.85f, 0.87f, 0.95f, 60.0f);
	for (int i = 0; i < NUM_MODULES; i++) { metalFinish(module[i], 0.9f, 0.92f, 1.0f, 70.0f); metalFinish(moduleBand[i], 0.6f, 0.6f, 0.65f, 40.0f); }
	metalFinish(habitatRing, 0.9f, 0.92f, 1.0f, 75.0f);
	metalFinish(habitatRingInner, 0.6f, 0.6f, 0.65f, 50.0f);
	for (int i = 0; i < NUM_RING_DETAIL; i++) metalFinish(ringDetail[i], 0.85f, 0.87f, 0.95f, 60.0f);
	metalFinish(dockNode, 0.85f, 0.87f, 0.95f, 60.0f);
	metalFinish(capsule, 0.85f, 0.87f, 0.95f, 60.0f);
	metalFinish(truss, 0.6f, 0.6f, 0.65f, 45.0f);
	for (int i = 0; i < NUM_PANELS; i++) { metalFinish(panelArm[i], 0.6f, 0.6f, 0.65f, 45.0f); metalFinish(satPanel[i], 0.45f, 0.55f, 1.0f, 100.0f); }
	metalFinish(mainDish, 1.0f, 1.0f, 1.0f, 95.0f);
	metalFinish(mainDishRim, 0.85f, 0.85f, 0.9f, 70.0f);
	metalFinish(mainFeed, 0.85f, 0.87f, 0.95f, 70.0f);
	for (int i = 0; i < NUM_SUB_DISHES; i++) metalFinish(subDish[i], 1.0f, 1.0f, 1.0f, 90.0f);
	for (int i = 0; i < NUM_BOOMS; i++) metalFinish(boom[i], 0.6f, 0.6f, 0.65f, 45.0f);
	for (int i = 0; i < NUM_ANTENNAS; i++) metalFinish(antenna[i], 0.6f, 0.6f, 0.65f, 45.0f);
	metalFinish(laserCannon, 0.8f, 0.4f, 0.35f, 60.0f);

	// the laser starts OFF (it only fires when the camera zooms in on the Earth)
	laserBeam->hide();
	laserImpact->hide();

	satOrbit->setRotation('y', orbitAngle, 'z', ORBIT_TILT);
}


// =========================================================================
// Destructor
// =========================================================================
composite2::~composite2()
{
	delete satOrbit; delete satHub; delete spine;
	for (int i = 0; i < NUM_MODULES; i++) { delete module[i]; delete moduleBand[i]; }
	delete ringMount; delete habitatRing; delete habitatRingInner; delete ringSpinPivot;
	for (int i = 0; i < NUM_RING_DETAIL; i++) delete ringDetail[i];
	delete dockNode; delete capsule;
	delete truss;
	for (int i = 0; i < NUM_PANELS; i++) { delete panelArm[i]; delete satPanel[i]; }
	delete mainDish; delete mainDishRim; delete mainFeedMast; delete mainFeed;
	for (int i = 0; i < NUM_SUB_DISHES; i++) delete subDish[i];
	for (int i = 0; i < NUM_BOOMS; i++) delete boom[i];
	for (int i = 0; i < NUM_ANTENNAS; i++) { delete antenna[i]; delete antennaTip[i]; }
	delete navRed; delete navGreen; delete navWhite;
	delete laserCannon; delete laserBeam; delete laserImpact;
}


// =========================================================================
// hide / show
// =========================================================================
void composite2::hide(void)
{
	invisible = true;
	satHub->hide(); spine->hide();
	for (int i = 0; i < NUM_MODULES; i++) { module[i]->hide(); moduleBand[i]->hide(); }
	ringMount->hide(); habitatRing->hide(); habitatRingInner->hide();
	for (int i = 0; i < NUM_RING_DETAIL; i++) ringDetail[i]->hide();
	dockNode->hide(); capsule->hide(); truss->hide();
	for (int i = 0; i < NUM_PANELS; i++) { panelArm[i]->hide(); satPanel[i]->hide(); }
	mainDish->hide(); mainDishRim->hide(); mainFeedMast->hide(); mainFeed->hide();
	for (int i = 0; i < NUM_SUB_DISHES; i++) subDish[i]->hide();
	for (int i = 0; i < NUM_BOOMS; i++) boom[i]->hide();
	for (int i = 0; i < NUM_ANTENNAS; i++) { antenna[i]->hide(); antennaTip[i]->hide(); }
	navRed->hide(); navGreen->hide(); navWhite->hide();
	laserCannon->hide(); laserBeam->hide(); laserImpact->hide();
}

void composite2::show(void)
{
	invisible = false;
	orbitAngle = 0; ringSpin = 0; panelAngle = 0; mainDishScan = 0;
	subDishAngle = 0; antennaSweep = 0; navBlink = 0; laserPulse = 0; laserRamp = 0;

	satHub->show(); spine->show();
	for (int i = 0; i < NUM_MODULES; i++) { module[i]->show(); moduleBand[i]->show(); }
	ringMount->show(); habitatRing->show(); habitatRingInner->show();
	for (int i = 0; i < NUM_RING_DETAIL; i++) ringDetail[i]->show();
	dockNode->show(); capsule->show(); truss->show();
	for (int i = 0; i < NUM_PANELS; i++) { panelArm[i]->show(); satPanel[i]->show(); }
	mainDish->show(); mainDishRim->show(); mainFeedMast->show(); mainFeed->show();
	for (int i = 0; i < NUM_SUB_DISHES; i++) subDish[i]->show();
	for (int i = 0; i < NUM_BOOMS; i++) boom[i]->show();
	for (int i = 0; i < NUM_ANTENNAS; i++) { antenna[i]->show(); antennaTip[i]->show(); }
	navRed->show(); navGreen->show(); navWhite->show();
	laserCannon->show();

	// the laser starts OFF - it is revealed when the scene starts firing it
	laserBeam->hide(); laserImpact->hide();
}


// =========================================================================
// ANIMATION 1 (master): orbit the station + drive every internal motion
// =========================================================================
void composite2::animateStation(float dt)
{
	orbitAngle = orbitAngle + 13.0f * dt;
	if (orbitAngle >= 360) orbitAngle = orbitAngle - 360;
	satOrbit->setRotation('y', orbitAngle, 'z', ORBIT_TILT);	// carry the station around the Earth, -x facing it

	animateHabitatRing(dt);
	animateSolarPanels(dt);
	animateDishes(dt);
	animateAntennas(dt);
	animateNavLights(dt);
}


// =========================================================================
// ANIMATION 2: the habitat ring's detail capsules rotate around the ring
// =========================================================================
void composite2::animateHabitatRing(float dt)
{
	ringSpin = ringSpin + 30.0f * dt;
	if (ringSpin >= 360) ringSpin = ringSpin - 360;
	ringSpinPivot->setRotation('z', ringSpin);
}


// =========================================================================
// ANIMATION 3: the four solar arrays gimbal to track the sun
// =========================================================================
void composite2::animateSolarPanels(float dt)
{
	panelAngle = panelAngle + 12.0f * dt;
	if (panelAngle >= 360) panelAngle = panelAngle - 360;
	for (int i = 0; i < NUM_PANELS; i++)
		satPanel[i]->setRotation('y', panelAngle);
}


// =========================================================================
// ANIMATION 4: the main dish scans, and the two sub-dishes scan faster
// =========================================================================
void composite2::animateDishes(float dt)
{
	mainDishScan = mainDishScan + 35.0f * dt;
	if (mainDishScan >= 360) mainDishScan = mainDishScan - 360;
	mainDish->setRotation('y', mainDishScan, 'z', -55.0f);		// scan, keeping its tilt

	subDishAngle = subDishAngle + 70.0f * dt;
	if (subDishAngle >= 360) subDishAngle = subDishAngle - 360;
	for (int i = 0; i < NUM_SUB_DISHES; i++)
		subDish[i]->setRotation('y', (i == 0) ? subDishAngle : -subDishAngle, 'z', -50.0f);
}


// =========================================================================
// ANIMATION 5: the three antenna whips sweep gently (their glowing tips follow)
// =========================================================================
void composite2::animateAntennas(float dt)
{
	antennaSweep = antennaSweep + 1.6f * dt;
	for (int i = 0; i < NUM_ANTENNAS; i++)
	{
		float sweep = 22.0f * sinf(antennaSweep + i * 1.7f);
		antenna[i]->setRotation('z', sweep);
	}
}


// =========================================================================
// ANIMATION 6: blink the three navigation lights
// =========================================================================
void composite2::animateNavLights(float dt)
{
	navBlink = navBlink + dt * 3.0f;
	float red = (sinf(navBlink) > 0) ? 1.0f : 0.15f;
	float green = (sinf(navBlink + 2) > 0) ? 1.0f : 0.15f;
	float white = (sinf(navBlink * 2.5f) > 0.3f) ? 1.0f : 0.10f;
	navRed->setEmissiveColour(0.85f * red, 0.05f * red, 0.02f * red, 1.0f);
	navGreen->setEmissiveColour(0.05f * green, 0.85f * green, 0.10f * green, 1.0f);
	navWhite->setEmissiveColour(0.80f * white, 0.80f * white, 0.85f * white, 1.0f);
}


// =========================================================================
// ANIMATION 7: power up + flicker the red laser
// =========================================================================
void composite2::animateLaser(float dt)
{
	laserPulse = laserPulse + dt * 9.0f;
	float glow = (0.70f + 0.30f * sinf(laserPulse)) * laserRamp;

	float bw = 0.05f + 0.13f * laserRamp;
	float beamLen = orbitR - earthR;
	laserBeam->setDeformation(bw, beamLen * 0.5f, bw);
	laserBeam->setColour(1.0f, 0.12f, 0.10f, (0.45f + 0.35f * laserRamp));
	laserBeam->setEmissiveColour(glow + 0.15f * laserRamp, 0.10f * glow, 0.06f * glow, 1.0f);

	float s = laserRamp * (0.7f + 0.6f * (0.5f + 0.5f * sinf(laserPulse)));
	if (s < 0.05f) s = 0.05f;
	laserImpact->setDeformation(s, s, s);
	laserImpact->setColour(1.0f, 0.25f, 0.15f, 0.30f + 0.45f * laserRamp);
	laserImpact->setEmissiveColour(glow + 0.15f * laserRamp, 0.25f * glow, 0.12f * glow, 1.0f);
}


// =========================================================================
// MASTER METHOD: place the pivot at the Earth's LIVE position, run the station, fire the laser
// only when the scene asks (when the camera has zoomed in on the Earth)
// =========================================================================
void composite2::runSequence(float dt, float earthX, float earthY, float earthZ, bool fireLaser)
{
	satOrbit->setPosition(earthX, earthY, earthZ);

	animateStation(dt);

	if (fireLaser)
	{
		laserRamp = laserRamp + dt * 1.5f;
		if (laserRamp > 1.0f) laserRamp = 1.0f;
		laserBeam->show();
		laserImpact->show();
		animateLaser(dt);
	}
	else
	{
		laserRamp = 0.0f;
		laserBeam->hide();
		laserImpact->hide();
	}
}
