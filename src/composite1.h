// *****************************************************************************************
// composite1.h
// The Collision: two classic rockets (white body, red nose cone, fins and a glowing flame) fly
// toward each other, roll about their own axes, and crash in a single explosion that scatters a
// field of rocky debris.  The explosion happens exactly once.
//
// Drawing note: every primitive registers itself with the engine's global draw list in its own
// constructor, so the engine draws and depth-sorts each child automatically.  This composite
// only builds, parents, animates, and hides/shows its children.
// *****************************************************************************************
#ifndef _COMPOSITE1_H
#define _COMPOSITE1_H

#include "object3d.h"
#include "sweep.h"
#include "extrusion.h"
#include "sphere.h"
#include "tsphere.h"
#include "torus.h"
#include "texture.h"

#define NUM_DEBRIS 16
#define NUM_FINS 3

class composite1 : public object3d
{
private:
	// --- ROCKET A (one of the two that collide) ---
	sweep* rocketA;					// the white body (a sweep)
	sweep* noseA;						// the red nose cone (a sweep)
	extrusion* finA[NUM_FINS];				// three fins at the base (extrusions)
	torus* ringA;						// a trim ring around the body
	sphere* flameA;						// a glowing exhaust flame (transparent)

	// --- ROCKET B (the other one) ---
	sweep* rocketB;
	sweep* noseB;
	extrusion* finB[NUM_FINS];
	torus* ringB;
	sphere* flameB;

	// --- the explosion + debris (hidden until the crash) ---
	sphere* explosion;					// the big ball of fire
	sphere* debris[NUM_DEBRIS];			// rocky bits flying out

	// --- textures ---
	texture* textureA, * textureB;

	// --- where the crash happens ---
	float	zoneX, zoneY, zoneZ;

	// --- numbers that control the animation ---
	float	rocketAx, rocketBx;			// x position of each rocket
	float	rocketSpin;					// how far the rockets have rolled about their own axis
	float	flameFlicker;				// flickers the exhaust flames
	float	launchClock;				// counts up at the start; the rockets wait, then fly in to crash on-camera
	float	crashTimer;					// how long since the crash started
	float	debrisDistance[NUM_DEBRIS];	// how far each bit has flown
	bool	hasCrashed;					// true once the rockets have hit
	bool	explosionFinished;			// true once the explosion is over (so it never repeats)

public:
	bool	justCrashed;				// becomes true for ONE frame when the crash happens

	composite1(float x, float y, float z);
	virtual ~composite1();

	// hide / show are overridden so the whole scene can be hidden or shown at once
	// (show() also resets the sequence to its start, so the animation can be replayed)
	virtual void hide(void);
	virtual void show(void);

	// --- the animation methods (the only way the children are animated) ---
	void animateRockets(float dt);			// fly the two rockets toward each other (and flicker their flames)
	void animateExplosion(float dt);		// grow the explosion
	void animateDebris(float dt);			// fly the bits outward
	void runSequence(float dt);				// the master method that runs the whole sequence
};

#endif
