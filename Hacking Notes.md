##Objects
###JumpPole/CObjSpringPole
Recognized vftable functions:
* 01030CE0 CObjSpringPole__animation
* 01030D00 CObjSpringPole__render
* 01030FB0 CObjSpringPole__onSonic
* 01031760 CObjSpringPole__registerXMLParams

CObjSpringPole__onSonic gets current Sonic velocity after collision and issues two message that make him jump: MsgSpringImpulse and MsgHitEventCollision. THe wrapper for MsgSpringImpulse (004581E0) receives the following args:
```
SendMsgSpringImpulse(__m128 *added_impulse, __m128 *player_position, __m128 *new_velocity, *a4, *a5, *a6, *a7, *a8);
```
Each coordinate is represented by a vector 3 of floats.

Y spring impulse is calculated from the gradient between MaxVelocity and MinVelocity depending on Sonic's distance from pole's edge. AddMinVelocity and AddMaxVelocity can be specified in object's XML params and added to the base velocity value. Sonic Generations Demo 1 (and possibly Sonic Unleashed) used 20 as base MinVelocity and 30 as MaxVelocity, but later versions use 30 for both.
Specifying lesser (including negative numbers) value for AddMinVelocity than AddMaxVelocity restores JumpPole behavior from Sonic Unleashed.
