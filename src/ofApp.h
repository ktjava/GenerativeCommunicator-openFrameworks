#pragma once

#include "ofMain.h"
#include "ofxOsc.h"

#define DRAW_FPS 60
#define TIME 3
#define ROW 10
#define COLUMN 10
#define CURSOR_QUAD_SIDE 2
#define HOST "localhost"
#define PORT 20000

class Particle{

	public:
		Particle(float pos_x, float pos_y, float vel_x, float vel_y);
		void addRadius(float r);
		void setColor(ofColor c);
		void resetAcceleration();
		void addAcceleration(float acc_x, float acc_y);
		void addFriction();
		void update(float volAve);
		void draw();

		ofVec2f position,
		velocity,
		acceleration;
		float add_radius,
		friction,
		volumeAve;
		unsigned int erapsedTime;
};

class ofApp : public ofBaseApp{

	public:
		ofApp();

		void setup();
		void update();
		void draw();
		void exit(ofEventArgs &args);

		void audioIn(ofSoundBuffer &buffer);
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mouseScrolled(int x, int y, float scrollX, float scrollY);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

		bool paint, cursor_draw, senderSetup;
		int clickX, clickY, lastX, lastY, mouseVelocityX, mouseVelocityY;
		double ***calc_mesh;
		float wheelValue, hueValue, volumeIn, volumeAve;
		double spread_mul;
		ofSoundStream soundStream;
		ofxOscReceiver oscReceiver;
		ofxOscSender oscSender;
		deque <Particle> particles;
};
