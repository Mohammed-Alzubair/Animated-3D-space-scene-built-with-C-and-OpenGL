// *****************************************************************************************
// composite2.h
// The Space Station: a complex modular satellite that ORBITS the Earth-like planet of the solar
// system and fires a red laser down onto it.
//
// Structure (parented to a single hub):
//   * a long SPINE backbone with FOUR cylindrical modules of varied sizes (with detail bands)
//   * a big HABITAT RING around the spine, with SIX detail capsules that ROTATE around it
//   * a docking node and a SOYUZ capsule (sweep) at the far end
//   * a vertical TRUSS carrying FOUR solar arrays that gimbal to track the sun
//   * a main COMMS DISH that scans, plus TWO smaller SUB-DISHES that scan faster
//   * TWO instrument BOOMS (extrusions - keeps the required extrusion)
//   * THREE antenna whips that sweep gently, each with a glowing tip
//   * THREE blinking navigation lights
//   * a LASER CANNON (sweep - keeps the required sweep) on the Earth-facing side
//
// Motions (each runs independently):
//   1. orbit around Earth                        - animateStation
//   2. the habitat ring's detail capsules spin   - animateHabitatRing
//   3. the four solar arrays gimbal              - animateSolarPanels
//   4. main dish + two sub-dishes scan           - animateDishes
//   5. three antenna whips sweep                 - animateAntennas
//   6. three nav lights blink                    - animateNavLights
//   7. red laser powers up + flickers (only when the camera zooms in on the Earth)
//
// The Earth's live position is passed in every frame, so there is only ONE Earth and the station
// truly lives INSIDE the solar system.
//
// Drawing note: every primitive registers itself with the engine's global draw list in its own
// constructor, so the engine draws and depth-sorts each child automatically.
// *****************************************************************************************
#ifndef _COMPOSITE2_H
#define _COMPOSITE2_H

#include "object3d.h"
#include "sweep.h"
#include "extrusion.h"
#include "sphere.h"
#include "tsphere.h"
#include "torus.h"
#include "cube.h"
#include "cylinder.h"
#include "texture.h"

#define NUM_MODULES 4
#define NUM_RING_DETAIL 6
#define NUM_PANELS 4
#define NUM_SUB_DISHES 2
#define NUM_BOOMS 2
#define NUM_ANTENNAS 3

class composite2 : public object3d
{
private:
	// --- the orbit pivot (carries the whole station around the Earth) ---
	object3d* satOrbit;

	// --- the central structure ---
	cylinder* satHub;								// hub: structural parent for the whole station
	cylinder* spine;								// long horizontal backbone (lies along z)
	cylinder* module[NUM_MODULES];				// four modules of varying sizes along the spine
	torus* moduleBand[NUM_MODULES];			// a detail band around each module

	// --- the habitat ring (with detail capsules that orbit around it) ---
	cylinder* ringMount;							// short connector between spine and ring
	torus* habitatRing;						// the big habitat ring
	torus* habitatRingInner;					// inner detail ring
	object3d* ringSpinPivot;						// transform-only pivot - rotates the capsules around the ring
	sphere* ringDetail[NUM_RING_DETAIL];		// six small capsules around the ring's circumference

	// --- the docking node + Soyuz capsule at the far end ---
	sphere* dockNode;							// docking sphere at the end of the spine
	sweep* capsule;							// Soyuz-style capsule (a SWEEP)

	// --- the solar truss and four solar arrays ---
	cylinder* truss;								// vertical truss (along +y from the hub)
	cylinder* panelArm[NUM_PANELS];				// short arm connecting each panel to the truss
	cube* satPanel[NUM_PANELS];				// four flat solar arrays

	// --- the dishes ---
	tsphere* mainDish;							// the big main comms dish
	torus* mainDishRim;						// rim around the main dish
	cylinder* mainFeedMast;						// central support holding the feed above the dish
	sphere* mainFeed;							// feed horn at the focus
	tsphere* subDish[NUM_SUB_DISHES];			// two smaller sub-dishes

	// --- two instrument booms (the required EXTRUSIONS) ---
	extrusion* boom[NUM_BOOMS];

	// --- three antenna whips (each tip parented to the antenna so it follows the sweep) ---
	cylinder* antenna[NUM_ANTENNAS];
	sphere* antennaTip[NUM_ANTENNAS];

	// --- three blinking navigation lights ---
	sphere* navRed, * navGreen, * navWhite;

	// --- the laser cannon (the required SWEEP) and its beam ---
	sweep* laserCannon;						// the laser-emitter horn on the Earth-facing side
	cylinder* laserBeam;							// the red beam (transparent, glowing)
	sphere* laserImpact;						// the glow where the beam meets the Earth

	// --- the radii (the Earth's position is dynamic and passed in each frame) ---
	float	earthR;									// the host planet's radius (so the laser lands on its surface)
	float	orbitR;									// the radius of the station's orbit around the planet

	// --- numbers that control the animation ---
	float	orbitAngle;								// where the station is on its orbit
	float	ringSpin;								// how far the ring detail capsules have rotated
	float	panelAngle;								// gimbals the solar arrays
	float	mainDishScan;							// scans the main dish
	float	subDishAngle;							// scans the sub-dishes
	float	antennaSweep;							// sweeps the antenna whips
	float	navBlink;								// blinks the nav lights
	float	laserPulse;								// flickers the laser
	float	laserRamp;								// 0..1 - the laser powers up smoothly when it starts firing

public:
	composite2(float earthRadius, float orbitRadius);
	virtual ~composite2();

	// hide / show are overridden so the whole station can be hidden or shown at once
	virtual void hide(void);
	virtual void show(void);

	// --- the animation methods (the only way the children are animated) ---
	void animateStation(float dt);			// orbit the station + drive all internal animations
	void animateHabitatRing(float dt);		// spin the ring's detail capsules
	void animateSolarPanels(float dt);		// gimbal the solar arrays
	void animateDishes(float dt);			// scan the main dish and both sub-dishes
	void animateAntennas(float dt);			// sweep the antenna whips
	void animateNavLights(float dt);		// blink the nav lights
	void animateLaser(float dt);			// power up + flicker the laser

	// the master method - the scene passes in the Earth's LIVE position and whether to fire the laser
	void runSequence(float dt, float earthX, float earthY, float earthZ, bool fireLaser);
};

#endif
